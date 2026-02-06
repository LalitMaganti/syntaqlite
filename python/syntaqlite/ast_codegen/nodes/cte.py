# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Common Table Expression (CTE / WITH clause) AST node definitions."""

from ..defs import Node, List, inline, index

ENUMS = []

NODES = [
    # Single CTE definition: name [(columns)] AS [MATERIALIZED|NOT MATERIALIZED] (select)
    # materialized: 0 = default (AS), 1 = AS MATERIALIZED, 2 = AS NOT MATERIALIZED
    Node("CteDefinition",
        cte_name=inline("SyntaqliteSourceSpan"),
        materialized=inline("u8"),
        columns=index("ExprList"),     # nullable - column name list
        select=index("Stmt"),          # the CTE subquery
    ),

    # List of CTE definitions
    List("CteList", child_type="CteDefinition"),

    # WITH clause: WITH [RECURSIVE] cte1, cte2, ... select
    Node("WithClause",
        recursive=inline("u8"),        # 0 = non-recursive, 1 = RECURSIVE
        ctes=index("CteList"),
        select=index("Stmt"),          # the main select
    ),
]
