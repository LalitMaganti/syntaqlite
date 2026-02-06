# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""FROM clause table source AST node definitions: table refs, joins, subqueries."""

from ..defs import Node, Enum, inline, index

ENUMS = [
    # Join types for FROM clause
    Enum("JoinType",
        "COMMA",          # 0 - implicit cross join (comma-separated)
        "INNER",          # 1 - [INNER] JOIN
        "LEFT",           # 2 - LEFT [OUTER] JOIN
        "RIGHT",          # 3 - RIGHT [OUTER] JOIN
        "FULL",           # 4 - FULL [OUTER] JOIN
        "CROSS",          # 5 - CROSS JOIN
        "NATURAL_INNER",  # 6 - NATURAL [INNER] JOIN
        "NATURAL_LEFT",   # 7 - NATURAL LEFT [OUTER] JOIN
        "NATURAL_RIGHT",  # 8 - NATURAL RIGHT [OUTER] JOIN
        "NATURAL_FULL",   # 9 - NATURAL FULL [OUTER] JOIN
    ),
]

NODES = [
    # Simple table reference: [schema.]table [AS alias]
    Node("TableRef",
        table_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        alias=inline("SyntaqliteSourceSpan"),
    ),

    # Subquery as table source: (SELECT ...) [AS alias]
    Node("SubqueryTableSource",
        select=index("Stmt"),
        alias=inline("SyntaqliteSourceSpan"),
    ),

    # Join clause: left_source JOIN right_source [ON expr | USING (columns)]
    Node("JoinClause",
        join_type=inline("JoinType"),
        left=index("Expr"),              # left table source
        right=index("Expr"),             # right table source
        on_expr=index("Expr"),           # nullable - ON condition
        using_columns=index("ExprList"), # nullable - USING column list
    ),

    # Grammar plumbing: carries (accumulated source, pending join_type) through stl_prefix
    # This is an internal node used during parsing; not visible in final AST output.
    Node("JoinPrefix",
        source=index("Expr"),            # accumulated table source tree
        join_type=inline("JoinType"),    # join operator for next table
    ),
]
