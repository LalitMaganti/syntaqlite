# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Building blocks for declaring AST node types.

Definitions here are used by codegen.py to generate C structs,
builder functions, and related infrastructure.
"""

from __future__ import annotations

from dataclasses import dataclass


def pascal_to_snake(name: str) -> str:
    """Convert PascalCase to snake_case."""
    result = []
    for i, c in enumerate(name):
        if c.isupper() and i > 0:
            result.append("_")
        result.append(c.lower())
    return "".join(result)


@dataclass(frozen=True)
class InlineField:
    """Field stored directly in the struct (scalars, enums, source spans)."""

    type_name: str


@dataclass(frozen=True)
class IndexField:
    """Field stored as a node ID (for cyclic/recursive references)."""

    type_name: str


FieldType = InlineField | IndexField


@dataclass(frozen=True)
class NodeDef:
    """Definition of a fixed-structure AST node."""

    name: str
    fields: dict[str, FieldType] = None

    def __post_init__(self):
        if self.fields is None:
            object.__setattr__(self, "fields", {})


@dataclass(frozen=True)
class ListDef:
    """Definition of a variable-length list node."""

    name: str
    child_type: str


@dataclass(frozen=True)
class EnumDef:
    """Definition of an enum type for AST fields."""

    name: str
    values: tuple[str, ...]


# Factory functions for a cleaner declarative API in nodes.py.
# inline/index convey storage semantics; Node/List/Enum handle
# **kwargs → dict and *args → tuple conversions.


def inline(type_name: str) -> InlineField:
    """Inline field: stored directly in the struct."""
    return InlineField(type_name=type_name)


def index(type_name: str) -> IndexField:
    """Index field: stored as a node ID (use for cyclic references)."""
    return IndexField(type_name=type_name)


def Node(name: str, **fields: FieldType) -> NodeDef:
    """Define a fixed-structure AST node with named fields."""
    return NodeDef(name=name, fields=fields)


def List(name: str, child_type: str) -> ListDef:
    """Define a variable-length list node."""
    return ListDef(name=name, child_type=child_type)


def Enum(name: str, *values: str) -> EnumDef:
    """Define an enum type for AST fields."""
    return EnumDef(name=name, values=values)


AnyNodeDef = NodeDef | ListDef
