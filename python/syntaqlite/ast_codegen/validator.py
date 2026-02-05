# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Validate AST node definitions.

This module validates that index fields in AST node definitions
reference valid node types.
"""

from __future__ import annotations

from .defs import AnyNodeDef, NodeDef, ListDef, IndexField


# Abstract types that multiple concrete nodes implement
ABSTRACT_TYPES = {
    "Expr",  # BinaryExpr, UnaryExpr, Literal, etc.
    "Stmt",  # SelectStmt, InsertStmt, etc.
    "TableSource",  # TableRef, JoinSource, SubquerySource
    "InsertSource",  # Values or Select
}


def validate_node_references(node_defs: list[AnyNodeDef]) -> list[str]:
    """Validate that index fields reference valid node types.

    Args:
        node_defs: List of node definitions.

    Returns:
        List of error messages.
    """
    errors: list[str] = []

    # Collect all defined node names
    defined_nodes: set[str] = {node.name for node in node_defs}
    valid_types = defined_nodes | ABSTRACT_TYPES

    for node in node_defs:
        if isinstance(node, NodeDef):
            for field_name, field_type in node.fields.items():
                if isinstance(field_type, IndexField):
                    if field_type.type_name not in valid_types:
                        errors.append(
                            f"Node '{node.name}' field '{field_name}' references "
                            f"undefined type '{field_type.type_name}'"
                        )
        elif isinstance(node, ListDef):
            if node.child_type not in valid_types:
                errors.append(
                    f"List '{node.name}' has undefined child type '{node.child_type}'"
                )

    return errors
