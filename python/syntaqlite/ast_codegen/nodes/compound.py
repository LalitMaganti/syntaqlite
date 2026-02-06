# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Compound SELECT and subquery AST node definitions."""

from ..defs import Node, Enum, inline, index
from ..fmt_dsl import seq, kw, child, hardline, if_flag, if_set, enum_display

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
        fmt=seq(
            child("left"),
            hardline(),
            enum_display("op", {
                "UNION": "UNION",
                "UNION_ALL": "UNION ALL",
                "INTERSECT": "INTERSECT",
                "EXCEPT": "EXCEPT",
            }),
            hardline(),
            child("right"),
        ),
    ),

    # Scalar subquery: (SELECT ...)
    Node("SubqueryExpr",
        select=index("Stmt"),
        fmt=seq(kw("("), child("select"), kw(")")),
    ),

    # EXISTS (SELECT ...)
    Node("ExistsExpr",
        select=index("Stmt"),
        fmt=seq(kw("EXISTS ("), child("select"), kw(")")),
    ),

    # expr [NOT] IN (exprlist) or expr [NOT] IN (SELECT ...)
    Node("InExpr",
        negated=inline("Bool"),
        operand=index("Expr"),
        source=index("Expr"),
        fmt=seq(
            child("operand"),
            if_flag("negated", kw(" NOT IN "), kw(" IN ")),
            kw("("),
            child("source"),
            kw(")"),
        ),
    ),
]
