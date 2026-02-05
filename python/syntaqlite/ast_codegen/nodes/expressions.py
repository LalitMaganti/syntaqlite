# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Expression AST node definitions: binary, unary, and literal."""

from ..defs import Node, inline, index

ENUMS = []

NODES = [
    # Binary expression: left OP right
    Node("BinaryExpr",
        op=inline("BinaryOp"),
        left=index("Expr"),
        right=index("Expr"),
    ),

    # Unary expression: OP operand
    Node("UnaryExpr",
        op=inline("UnaryOp"),
        operand=index("Expr"),
    ),

    # Literal value (integer, float, string, blob, null)
    Node("Literal",
        literal_type=inline("LiteralType"),
        source=inline("SyntaqliteSourceSpan"),
    ),
]
