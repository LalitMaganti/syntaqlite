# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Common Table Expression (CTE / WITH clause) AST node definitions."""

from ..defs import Node, List, Enum, inline, index

ENUMS = [
    Enum("Materialized",
        "DEFAULT",           # 0 - AS (no hint)
        "MATERIALIZED",      # 1 - AS MATERIALIZED
        "NOT_MATERIALIZED",  # 2 - AS NOT MATERIALIZED
    ),
]

NODES = [
    # Single CTE definition: name [(columns)] AS [MATERIALIZED|NOT MATERIALIZED] (select)
    Node("CteDefinition",
        cte_name=inline("SyntaqliteSourceSpan"),
        materialized=inline("Materialized"),
        columns=index("ExprList"),     # nullable - column name list
        select=index("Stmt"),          # the CTE subquery
    ),

    # List of CTE definitions
    List("CteList", child_type="CteDefinition"),

    # WITH clause: WITH [RECURSIVE] cte1, cte2, ... select
    Node("WithClause",
        recursive=inline("Bool"),      # FALSE = non-recursive, TRUE = RECURSIVE
        ctes=index("CteList"),
        select=index("Stmt"),          # the main select
    ),
]
