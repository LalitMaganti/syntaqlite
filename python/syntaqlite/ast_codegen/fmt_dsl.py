# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Fmt DSL: declarative formatting descriptions for AST nodes.

These types describe how to format each node type. The fmt_codegen module
compiles them into C code that builds a Wadler-Lindig document tree.

Usage in node definition files:
    from ..fmt_dsl import seq, kw, span, child, line, group, ...

    Node("Literal",
        source=inline("SynqSourceSpan"),
        fmt=span("source"),
    )
"""

from __future__ import annotations

from dataclasses import dataclass


# ============ Primitives ============

@dataclass(frozen=True)
class FmtKw:
    """Emit a static keyword string."""
    text: str

@dataclass(frozen=True)
class FmtSpan:
    """Emit text from a source span field."""
    field: str

@dataclass(frozen=True)
class FmtChild:
    """Recursively format a child node (index field).

    Use field="_item" inside for_each_child templates to refer to the
    current iteration child.
    """
    field: str

@dataclass(frozen=True)
class FmtLine:
    """synq_doc_line: space when flat, newline+indent when broken."""
    pass

@dataclass(frozen=True)
class FmtSoftline:
    """synq_doc_softline: empty when flat, newline+indent when broken."""
    pass

@dataclass(frozen=True)
class FmtHardline:
    """synq_doc_hardline: always newline+indent."""
    pass


# ============ Combinators ============

@dataclass(frozen=True)
class FmtSeq:
    """Concatenate multiple doc nodes."""
    items: tuple[FmtDoc, ...]

@dataclass(frozen=True)
class FmtGroup:
    """synq_doc_group: try flat, break if doesn't fit."""
    child: FmtDoc

@dataclass(frozen=True)
class FmtNest:
    """synq_doc_nest: increase indent for child by options.indent_width."""
    child: FmtDoc


# ============ Conditionals ============

@dataclass(frozen=True)
class FmtIfSet:
    """Conditional on an index field being non-null (not SYNQ_NULL_NODE)."""
    field: str
    then: FmtDoc
    else_: FmtDoc | None = None

@dataclass(frozen=True)
class FmtIfFlag:
    """Conditional on a field being truthy. Works for bitflags (flags.name)
    and Bool enum fields (since TRUE=1, FALSE=0)."""
    field: str
    then: FmtDoc
    else_: FmtDoc | None = None

@dataclass(frozen=True)
class FmtIfEnum:
    """Conditional on an enum field having a specific value."""
    field: str
    value: str
    then: FmtDoc
    else_: FmtDoc | None = None

@dataclass(frozen=True)
class FmtIfSpan:
    """Conditional on a source span having non-zero length."""
    field: str
    then: FmtDoc
    else_: FmtDoc | None = None


# ============ Patterns ============

@dataclass(frozen=True)
class FmtClause:
    """Emit: line + keyword + nest(indent, line + body).

    Standard pattern for SQL clauses like FROM, WHERE, GROUP BY, etc.
    """
    keyword: str
    body: FmtDoc

@dataclass(frozen=True)
class FmtEnumDisplay:
    """Map an enum field value to a display string.

    The mapping dict maps enum value names (e.g. "PLUS") to display
    strings (e.g. "+"). Generates a switch statement in C.
    """
    field: str
    mapping: dict[str, str]

@dataclass(frozen=True)
class FmtSwitch:
    """Multi-branch on an enum field. Each case maps to a FmtDoc.

    More general than FmtEnumDisplay: each branch can be an arbitrary
    FmtDoc tree, not just a keyword string.
    """
    field: str
    cases: dict[str, FmtDoc]
    default: FmtDoc | None = None


# ============ List-specific ============

@dataclass(frozen=True)
class FmtForEachChild:
    """For ListDef nodes: iterate children with per-item formatting.

    The template is a FmtDoc where child("_item") refers to the current
    child being iterated. An optional separator is emitted between items.

    Examples:
        # Pure concatenation (e.g. CaseWhenList)
        for_each_child(child("_item"))

        # Space-separated (e.g. ColumnConstraintList)
        for_each_child(child("_item"), separator=kw(" "))

        # Each item nested with hardline + semicolon (e.g. TriggerCmdList)
        for_each_child(nest(seq(hardline(), child("_item"), kw(";")))))
    """
    template: FmtDoc
    separator: FmtDoc | None = None


# ============ Union Type ============

FmtDoc = (
    FmtKw | FmtSpan | FmtChild | FmtLine | FmtSoftline | FmtHardline |
    FmtSeq | FmtGroup | FmtNest | FmtIfSet | FmtIfFlag | FmtIfEnum |
    FmtIfSpan | FmtClause | FmtEnumDisplay | FmtSwitch | FmtForEachChild
)


# ============ Factory Functions ============

def kw(text: str) -> FmtKw:
    return FmtKw(text=text)

def span(field: str) -> FmtSpan:
    return FmtSpan(field=field)

def child(field: str) -> FmtChild:
    return FmtChild(field=field)

def line() -> FmtLine:
    return FmtLine()

def softline() -> FmtSoftline:
    return FmtSoftline()

def hardline() -> FmtHardline:
    return FmtHardline()

def seq(*items: FmtDoc) -> FmtSeq:
    return FmtSeq(items=items)

def group(doc: FmtDoc) -> FmtGroup:
    return FmtGroup(child=doc)

def nest(doc: FmtDoc) -> FmtNest:
    return FmtNest(child=doc)

def if_set(field: str, then: FmtDoc, else_: FmtDoc | None = None) -> FmtIfSet:
    return FmtIfSet(field=field, then=then, else_=else_)

def if_flag(field: str, then: FmtDoc, else_: FmtDoc | None = None) -> FmtIfFlag:
    return FmtIfFlag(field=field, then=then, else_=else_)

def if_enum(field: str, value: str, then: FmtDoc, else_: FmtDoc | None = None) -> FmtIfEnum:
    return FmtIfEnum(field=field, value=value, then=then, else_=else_)

def if_span(field: str, then: FmtDoc, else_: FmtDoc | None = None) -> FmtIfSpan:
    return FmtIfSpan(field=field, then=then, else_=else_)

def clause(keyword: str, body: FmtDoc) -> FmtClause:
    return FmtClause(keyword=keyword, body=body)

def enum_display(field: str, mapping: dict[str, str]) -> FmtEnumDisplay:
    return FmtEnumDisplay(field=field, mapping=mapping)

def switch(field: str, cases: dict[str, FmtDoc], default: FmtDoc | None = None) -> FmtSwitch:
    return FmtSwitch(field=field, cases=cases, default=default)

def for_each_child(template: FmtDoc, separator: FmtDoc | None = None) -> FmtForEachChild:
    return FmtForEachChild(template=template, separator=separator)
