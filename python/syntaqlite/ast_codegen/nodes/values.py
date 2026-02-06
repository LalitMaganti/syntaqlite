# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""VALUES clause AST node definitions."""

from ..defs import Node, List, inline, index

ENUMS = []

NODES = [
    # List of value rows (each child is an ExprList)
    List("ValuesRowList", child_type="ExprList"),

    # VALUES clause: VALUES (row1), (row2), ...
    Node("ValuesClause",
        rows=index("ValuesRowList"),
    ),
]
