# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Miscellaneous expression AST node definitions: Variable, CollateExpr."""

from ..defs import Node, inline, index

ENUMS = []

NODES = [
    # Bind parameter: ?, ?1, :name, @name, $name
    Node("Variable",
        source=inline("SyntaqliteSourceSpan"),
    ),

    # COLLATE expression: expr COLLATE collation_name
    Node("CollateExpr",
        expr=index("Expr"),
        collation=inline("SyntaqliteSourceSpan"),
    ),
]
