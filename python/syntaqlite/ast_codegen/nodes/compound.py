# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Compound SELECT and subquery AST node definitions."""

from ..defs import Node, Enum, inline, index

ENUMS = [
    Enum("CompoundOp",
        "UNION",        # 0
        "UNION_ALL",    # 1
        "INTERSECT",    # 2
        "EXCEPT",       # 3
    ),
]

NODES = [
    # Compound SELECT: left UNION/INTERSECT/EXCEPT right
    Node("CompoundSelect",
        op=inline("CompoundOp"),
        left=index("Stmt"),
        right=index("Stmt"),
    ),

    # Scalar subquery: (SELECT ...)
    Node("SubqueryExpr",
        select=index("Stmt"),
    ),

    # EXISTS (SELECT ...)
    Node("ExistsExpr",
        select=index("Stmt"),
    ),

    # expr [NOT] IN (exprlist) or expr [NOT] IN (SELECT ...)
    Node("InExpr",
        negated=inline("Bool"),
        operand=index("Expr"),
        source=index("Expr"),  # ExprList for (1,2,3) or SubqueryExpr for (SELECT...)
    ),
]
