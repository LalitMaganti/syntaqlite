# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Common Table Expression (CTE / WITH clause) AST node definitions."""

from ..defs import Node, List, Enum, inline, index
from ..fmt_dsl import seq, kw, span, child, hardline, if_set, if_flag, if_enum

ENUMS = [
    Enum("Materialized",
        "DEFAULT",           # 0
        "MATERIALIZED",      # 1
        "NOT_MATERIALIZED",  # 2
    ),
]

NODES = [
    # Single CTE definition: name [(columns)] AS [MATERIALIZED|NOT MATERIALIZED] (select)
    Node("CteDefinition",
        cte_name=inline("SynqSourceSpan"),
        materialized=inline("Materialized"),
        columns=index("ExprList"),
        select=index("Stmt"),
        fmt=seq(
            span("cte_name"),
            if_set("columns", seq(kw("("), child("columns"), kw(")"))),
            kw(" AS "),
            if_enum("materialized", "MATERIALIZED", kw("MATERIALIZED ")),
            if_enum("materialized", "NOT_MATERIALIZED", kw("NOT MATERIALIZED ")),
            kw("("), child("select"), kw(")"),
        ),
    ),

    # List of CTE definitions
    List("CteList", child_type="CteDefinition"),

    # WITH clause: WITH [RECURSIVE] cte1, cte2, ... select
    Node("WithClause",
        recursive=inline("Bool"),
        ctes=index("CteList"),
        select=index("Stmt"),
        fmt=seq(
            if_flag("recursive", kw("WITH RECURSIVE "), kw("WITH ")),
            if_set("ctes", child("ctes")),
            hardline(),
            child("select"),
        ),
    ),
]
