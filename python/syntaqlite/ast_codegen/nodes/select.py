# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST node definitions."""

from ..defs import Node, List, Enum, inline, index

ENUMS = [
    Enum("SortOrder",
        "ASC",        # 0
        "DESC",       # 1
    ),

    Enum("NullsOrder",
        "NONE",       # 0 - no explicit NULLS keyword
        "FIRST",      # 1
        "LAST",       # 2
    ),
]

NODES = [
    # Result column: expr [AS alias] or *
    Node("ResultColumn",
        flags=inline("u8"),                # Is star, has alias, etc.
        alias=inline("SyntaqliteSourceSpan"),  # Alias (empty if none)
        expr=index("Expr"),                # Expression (nullable for table.*)
    ),

    # List of result columns
    List("ResultColumnList", child_type="ResultColumn"),

    # SELECT statement
    Node("SelectStmt",
        flags=inline("u8"),                    # DISTINCT, ALL
        columns=index("ResultColumnList"),     # Result columns
        from_clause=index("Expr"),             # FROM clause (TableRef, JoinClause, etc.)
        where=index("Expr"),                   # WHERE clause expression
        groupby=index("ExprList"),             # GROUP BY expressions
        having=index("Expr"),                  # HAVING clause expression
        orderby=index("OrderByList"),          # ORDER BY clause
        limit_clause=index("LimitClause"),     # LIMIT/OFFSET clause
        window_clause=index("NamedWindowDefList"),  # WINDOW clause (nullable)
    ),

    # Ordering term: expr [ASC|DESC] [NULLS FIRST|LAST]
    Node("OrderingTerm",
        expr=index("Expr"),
        sort_order=inline("SortOrder"),
        nulls_order=inline("NullsOrder"),
    ),

    # List of ordering terms (ORDER BY clause)
    List("OrderByList", child_type="OrderingTerm"),

    # LIMIT clause with optional OFFSET
    Node("LimitClause",
        limit=index("Expr"),
        offset=index("Expr"),
    ),
]
