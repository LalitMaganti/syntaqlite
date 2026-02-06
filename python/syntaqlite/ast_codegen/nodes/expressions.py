# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Expression AST node definitions: binary, unary, and literal."""

from ..defs import Node, List, inline, index
from ..fmt_dsl import seq, kw, span, child, line, group, enum_display, switch

ENUMS = []

NODES = [
    # Binary expression: left OP right
    Node("BinaryExpr",
        op=inline("BinaryOp"),
        left=index("Expr"),
        right=index("Expr"),
        fmt=switch("op", {
            # AND/OR: no group — breaks propagate from parent (WHERE clause body)
            "AND": seq(child("left"), line(), kw("AND "), child("right")),
            "OR": seq(child("left"), line(), kw("OR "), child("right")),
        }, default=group(seq(
            # All other ops: own group — stays flat independently
            child("left"),
            line(),
            enum_display("op", {
                "PLUS": "+", "MINUS": "-", "STAR": "*", "SLASH": "/",
                "REM": "%", "LT": "<", "GT": ">", "LE": "<=", "GE": ">=",
                "EQ": "=", "NE": "!=", "AND": "AND", "OR": "OR",
                "BITAND": "&", "BITOR": "|", "LSHIFT": "<<", "RSHIFT": ">>",
                "CONCAT": "||", "PTR": "->",
            }),
            kw(" "),
            child("right"),
        ))),
    ),

    # Unary expression: OP operand
    Node("UnaryExpr",
        op=inline("UnaryOp"),
        operand=index("Expr"),
        fmt=seq(
            enum_display("op", {
                "MINUS": "-", "PLUS": "+", "BITNOT": "~", "NOT": "NOT ",
            }),
            child("operand"),
        ),
    ),

    # Literal value (integer, float, string, blob, null)
    Node("Literal",
        literal_type=inline("LiteralType"),
        source=inline("SynqSourceSpan"),
        fmt=span("source"),
    ),

    # List of expressions (used by GROUP BY, function args, etc.)
    List("ExprList", child_type="Expr"),
]
