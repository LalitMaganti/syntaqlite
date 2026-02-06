# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Miscellaneous expression AST node definitions: Variable, CollateExpr."""

from ..defs import Node, inline, index
from ..fmt_dsl import seq, kw, span, child

ENUMS = []

NODES = [
    # Bind parameter: ?, ?1, :name, @name, $name
    Node("Variable",
        source=inline("SynqSourceSpan"),
        fmt=span("source"),
    ),

    # COLLATE expression: expr COLLATE collation_name
    Node("CollateExpr",
        expr=index("Expr"),
        collation=inline("SynqSourceSpan"),
        fmt=seq(child("expr"), kw(" COLLATE "), span("collation")),
    ),
]
