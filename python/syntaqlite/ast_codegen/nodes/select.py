# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST node definitions."""

from ..defs import Node, List, inline, index

ENUMS = []

NODES = [
    # Result column: expr [AS alias] or *
    Node("ResultColumn",
        flags=inline("u8"),                # Is star, has alias, etc.
        alias=inline("SyntaqliteSourceSpan"),  # Alias (empty if none)
        expr=index("Expr"),                # Expression (nullable for table.*)
    ),

    # List of result columns
    List("ResultColumnList", child_type="ResultColumn"),

    # SELECT statement (minimal version)
    Node("SelectStmt",
        flags=inline("u8"),          # DISTINCT, ALL
        columns=index("ResultColumnList"), # Result columns
    ),
]
