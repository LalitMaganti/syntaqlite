# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CAST expression AST node definition."""

from ..defs import Node, inline, index

ENUMS = []

NODES = [
    # CAST expression: CAST(expr AS type_name)
    Node("CastExpr",
        expr=index("Expr"),
        type_name=inline("SyntaqliteSourceSpan"),
    ),
]
