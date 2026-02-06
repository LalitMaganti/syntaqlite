# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Column reference AST node definitions."""

from python.syntaqlite.ast_codegen.defs import Node, inline
from python.syntaqlite.ast_codegen.fmt_dsl import seq, kw, span, if_span

ENUMS = []

NODES = [
    # Column reference: name, optionally qualified with table and/or schema
    Node("ColumnRef",
        column=inline("SyntaqliteSourceSpan"),
        table=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        fmt=seq(
            if_span("schema", seq(span("schema"), kw("."))),
            if_span("table", seq(span("table"), kw("."))),
            span("column"),
        ),
    ),
]
