# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""FROM clause table source AST node definitions: table refs, joins, subqueries."""

from ..defs import Node, Enum, inline, index
from ..fmt_dsl import (
    seq, kw, span, child, hardline,
    if_set, if_span, if_enum, switch, enum_display,
)

ENUMS = [
    Enum("JoinType",
        "COMMA",          # 0
        "INNER",          # 1
        "LEFT",           # 2
        "RIGHT",          # 3
        "FULL",           # 4
        "CROSS",          # 5
        "NATURAL_INNER",  # 6
        "NATURAL_LEFT",   # 7
        "NATURAL_RIGHT",  # 8
        "NATURAL_FULL",   # 9
    ),
]

NODES = [
    # Simple table reference: [schema.]table [AS alias]
    Node("TableRef",
        table_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        alias=inline("SyntaqliteSourceSpan"),
        fmt=seq(
            if_span("schema", seq(span("schema"), kw("."))),
            span("table_name"),
            if_span("alias", seq(kw(" AS "), span("alias"))),
        ),
    ),

    # Subquery as table source: (SELECT ...) [AS alias]
    Node("SubqueryTableSource",
        select=index("Stmt"),
        alias=inline("SyntaqliteSourceSpan"),
        fmt=seq(
            kw("("), child("select"), kw(")"),
            if_span("alias", seq(kw(" AS "), span("alias"))),
        ),
    ),

    # Join clause: left_source JOIN right_source [ON expr | USING (columns)]
    Node("JoinClause",
        join_type=inline("JoinType"),
        left=index("Expr"),
        right=index("Expr"),
        on_expr=index("Expr"),
        using_columns=index("ExprList"),
        fmt=switch("join_type", {
            "COMMA": seq(child("left"), kw(", "), child("right"),
                         if_set("on_expr", seq(hardline(), kw("ON "), child("on_expr"))),
                         if_set("using_columns", seq(kw(" USING ("), child("using_columns"), kw(")")))),
        }, default=seq(
            child("left"), hardline(),
            enum_display("join_type", {
                "INNER": "JOIN", "LEFT": "LEFT JOIN", "RIGHT": "RIGHT JOIN",
                "FULL": "FULL JOIN", "CROSS": "CROSS JOIN",
                "NATURAL_INNER": "NATURAL JOIN", "NATURAL_LEFT": "NATURAL LEFT JOIN",
                "NATURAL_RIGHT": "NATURAL RIGHT JOIN", "NATURAL_FULL": "NATURAL FULL JOIN",
                "COMMA": ",",
            }),
            kw(" "), child("right"),
            if_set("on_expr", seq(hardline(), kw("ON "), child("on_expr"))),
            if_set("using_columns", seq(kw(" USING ("), child("using_columns"), kw(")"))),
        )),
    ),

    # Grammar plumbing: carries accumulated source through stl_prefix
    Node("JoinPrefix",
        source=index("Expr"),
        join_type=inline("JoinType"),
        fmt=child("source"),
    ),
]
