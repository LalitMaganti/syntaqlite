# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Helper functions for defining AST nodes.

This module provides the building blocks for declaring AST node types
in a declarative way. The definitions are then used by codegen.py to
generate C structs, builder functions, and related infrastructure.

Example usage:
    from syntaqlite.ast_codegen.defs import Node, List, inline, index

    nodes = [
        Node("BinaryExpr",
            op=inline("u8"),
            left=index("Expr"),
            right=index("Expr"),
        ),
        List("ExprList", child_type="Expr"),
    ]
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Literal as TypingLiteral


@dataclass(frozen=True)
class FieldType:
    """Base class for field type specifications."""

    type_name: str
    storage: TypingLiteral["inline", "index"]


@dataclass(frozen=True)
class InlineField(FieldType):
    """Field stored directly in the struct (scalars, non-cyclic data)."""

    storage: TypingLiteral["inline"] = "inline"


@dataclass(frozen=True)
class IndexField(FieldType):
    """Field stored as node ID (cyclic/recursive references)."""

    storage: TypingLiteral["index"] = "index"


def inline(type_name: str) -> InlineField:
    """Field stored directly in the struct (scalars, non-cyclic data).

    Use for:
    - Scalars (u8, u16, u32)
    - Flags and enum values
    - Source text offsets/lengths
    - Embedded sub-structs that don't create cycles

    Args:
        type_name: The C type name (u8, u16, u32, or custom type).

    Returns:
        An InlineField specification.
    """
    return InlineField(type_name=type_name)


def index(type_name: str) -> IndexField:
    """Field stored as node ID (cyclic/recursive references).

    Use when the field type can contain the current type (directly or
    indirectly), creating a cycle. For example, Expr contains BinaryExpr,
    and BinaryExpr contains Expr, so BinaryExpr.left must be an index.

    Args:
        type_name: The AST node type name this index points to.

    Returns:
        An IndexField specification.
    """
    return IndexField(type_name=type_name)


@dataclass(frozen=True)
class NodeDef:
    """Definition of a fixed-structure AST node."""

    kind: TypingLiteral["node"] = "node"
    name: str = ""
    fields: dict[str, FieldType] = None

    def __post_init__(self):
        if self.fields is None:
            object.__setattr__(self, "fields", {})


@dataclass(frozen=True)
class ListDef:
    """Definition of a variable-length list node."""

    kind: TypingLiteral["list"] = "list"
    name: str = ""
    child_type: str = ""


def Node(name: str, **fields: FieldType) -> NodeDef:
    """Define a fixed-structure AST node.

    Each node has a tag byte followed by its fields. Fields can be either
    inline (stored directly) or index (stored as node ID).

    Args:
        name: The node type name (e.g., "BinaryExpr").
        **fields: Field definitions as keyword arguments.

    Returns:
        A NodeDef specification.

    Example:
        Node("BinaryExpr",
            op=inline("u8"),
            left=index("Expr"),
            right=index("Expr"),
        )
    """
    return NodeDef(name=name, fields=fields)


def List(name: str, child_type: str) -> ListDef:
    """Define a variable-length list node.

    List nodes have a header (tag + count) followed by a flexible array
    of child node indices.

    Args:
        name: The list type name (e.g., "ExprList").
        child_type: The type of elements in the list (e.g., "Expr").

    Returns:
        A ListDef specification.

    Example:
        List("ExprList", child_type="Expr")
    """
    return ListDef(name=name, child_type=child_type)


@dataclass(frozen=True)
class EnumDef:
    """Definition of an enum type for AST fields."""

    kind: TypingLiteral["enum"] = "enum"
    name: str = ""
    values: tuple[str, ...] = ()


def Enum(name: str, *values: str) -> EnumDef:
    """Define an enum type for AST fields.

    Args:
        name: The enum type name (e.g., "LiteralType").
        *values: Enum value names in order (e.g., "INTEGER", "FLOAT", ...).

    Returns:
        An EnumDef specification.

    Example:
        Enum("LiteralType", "INTEGER", "FLOAT", "STRING", "BLOB", "NULL")
    """
    return EnumDef(name=name, values=values)


# Type alias for any node definition
AnyNodeDef = NodeDef | ListDef

# Type alias for any definition (nodes, lists, enums)
AnyDef = NodeDef | ListDef | EnumDef
