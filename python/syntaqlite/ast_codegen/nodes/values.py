# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""VALUES clause AST node definitions."""

from ..defs import Node, List, inline, index
from ..fmt_dsl import seq, kw, child, line, hardline, nest, for_each_child

ENUMS = []

NODES = [
    # List of value rows (each child is an ExprList)
    # Each row formatted as (expr, expr, ...) with comma+line separator
    List("ValuesRowList", child_type="ExprList",
        fmt=for_each_child(
            seq(kw("("), child("_item"), kw(")")),
            separator=seq(kw(","), line()),
        ),
    ),

    # VALUES clause: VALUES (row1), (row2), ...
    Node("ValuesClause",
        rows=index("ValuesRowList"),
        fmt=seq(kw("VALUES"), nest(seq(hardline(), child("rows")))),
    ),
]
