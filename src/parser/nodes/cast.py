# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CAST expression AST node definition."""

from python.syntaqlite.ast_codegen.defs import Node, inline, index
from python.syntaqlite.ast_codegen.fmt_dsl import seq, kw, span, child

ENUMS = []

NODES = [
    # CAST expression: CAST(expr AS type_name)
    Node("CastExpr",
        expr=index("Expr"),
        type_name=inline("SyntaqliteSourceSpan"),
        fmt=seq(kw("CAST("), child("expr"), kw(" AS "), span("type_name"), kw(")")),
    ),
]
