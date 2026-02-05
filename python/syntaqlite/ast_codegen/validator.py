# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Validate AST generation consistency.

This module ensures:
1. Every upstream grammar rule has a corresponding action in ast_grammar.y
2. No orphan rules exist in ast_grammar.y
3. Builder function calls reference defined node types
"""

from __future__ import annotations

import re

from .defs import AnyNodeDef, NodeDef, ListDef


def _pascal_to_snake(name: str) -> str:
    """Convert PascalCase to snake_case."""
    result = []
    for i, c in enumerate(name):
        if c.isupper() and i > 0:
            result.append("_")
        result.append(c.lower())
    return "".join(result)


def _snake_to_pascal(name: str) -> str:
    """Convert snake_case to PascalCase."""
    return "".join(word.capitalize() for word in name.split("_"))


def _infer_node_from_builder(builder_name: str) -> str | None:
    """Infer node type name from builder function name.

    Examples:
        ast_binary_expr -> BinaryExpr
        ast_select_stmt -> SelectStmt
        ast_expr_list -> ExprList
        ast_alloc -> None (generic allocator)

    Args:
        builder_name: Builder function name (e.g., "ast_binary_expr").

    Returns:
        Node type name or None if not inferable.
    """
    if not builder_name.startswith("ast_"):
        return None

    # Remove ast_ prefix
    snake_name = builder_name[4:]

    # Skip generic functions
    if snake_name in ("alloc",):
        return None

    # Handle list append functions
    if snake_name.endswith("_append"):
        snake_name = snake_name[:-7]  # Remove _append
    elif snake_name.endswith("_empty"):
        snake_name = snake_name[:-6]  # Remove _empty

    # Convert to PascalCase
    return _snake_to_pascal(snake_name)


def validate_rule_coverage(
    upstream_rules: dict[str, object],
    our_rules: dict[str, object],
) -> list[str]:
    """Validate that all upstream rules have corresponding actions.

    Args:
        upstream_rules: Rules from SQLite's parse.y (signature -> any).
        our_rules: Rules from our ast_grammar.y (signature -> any).

    Returns:
        List of error messages.
    """
    errors: list[str] = []

    # Every upstream rule must have a corresponding action
    for sig in upstream_rules:
        if sig not in our_rules:
            errors.append(f"Missing action for upstream rule: {sig}")

    return errors


def validate_no_orphan_rules(
    upstream_rules: dict[str, object],
    our_rules: dict[str, object],
) -> list[str]:
    """Validate that no orphan rules exist in our grammar.

    Orphan rules are rules in ast_grammar.y that don't exist in upstream.
    These might be leftover from grammar changes and should be removed.

    Args:
        upstream_rules: Rules from SQLite's parse.y.
        our_rules: Rules from our ast_grammar.y.

    Returns:
        List of error messages.
    """
    errors: list[str] = []

    for sig in our_rules:
        if sig not in upstream_rules:
            errors.append(f"Orphan rule (not in upstream): {sig}")

    return errors


def validate_builder_calls(
    builder_calls: set[str],
    node_defs: list[AnyNodeDef],
) -> list[str]:
    """Validate that builder calls reference defined nodes.

    Args:
        builder_calls: Set of builder function names called in actions.
        node_defs: List of node definitions.

    Returns:
        List of error messages.
    """
    errors: list[str] = []

    # Build set of expected builder function names
    expected_builders: set[str] = {"ast_alloc"}  # Generic allocator

    for node in node_defs:
        snake_name = _pascal_to_snake(node.name)
        base_name = f"ast_{snake_name}"
        expected_builders.add(base_name)

        if isinstance(node, ListDef):
            expected_builders.add(f"{base_name}_empty")
            expected_builders.add(f"{base_name}_append")

    # Check each builder call
    for call in builder_calls:
        if call not in expected_builders:
            # Try to infer what node it should reference
            inferred = _infer_node_from_builder(call)
            if inferred:
                errors.append(
                    f"Unknown builder '{call}' (expected node '{inferred}' not defined)"
                )
            else:
                errors.append(f"Unknown builder function: {call}")

    return errors


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

    # Also accept abstract type names that aren't nodes themselves
    # These are union types that multiple concrete nodes implement
    abstract_types = {
        "Expr",  # BinaryExpr, UnaryExpr, Literal, etc.
        "Stmt",  # SelectStmt, InsertStmt, etc.
        "TableSource",  # TableRef, JoinSource, SubquerySource
        "InsertSource",  # Values or Select
    }

    valid_types = defined_nodes | abstract_types

    for node in node_defs:
        if isinstance(node, NodeDef):
            for field_name, field_type in node.fields.items():
                from .defs import IndexField

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


def validate_all(
    upstream_rules: dict[str, object] | None,
    our_rules: dict[str, object] | None,
    builder_calls: set[str] | None,
    node_defs: list[AnyNodeDef],
) -> list[str]:
    """Run all validations.

    Args:
        upstream_rules: Rules from SQLite's parse.y (optional).
        our_rules: Rules from our ast_grammar.y (optional).
        builder_calls: Set of builder function names (optional).
        node_defs: List of node definitions.

    Returns:
        List of error messages (empty if valid).
    """
    errors: list[str] = []

    # Always validate node references
    errors.extend(validate_node_references(node_defs))

    # Validate rule coverage if we have both rule sets
    if upstream_rules is not None and our_rules is not None:
        errors.extend(validate_rule_coverage(upstream_rules, our_rules))
        errors.extend(validate_no_orphan_rules(upstream_rules, our_rules))

    # Validate builder calls if provided
    if builder_calls is not None:
        errors.extend(validate_builder_calls(builder_calls, node_defs))

    return errors
