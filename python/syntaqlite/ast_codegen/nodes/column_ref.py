# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Column reference AST node definitions."""

from ..defs import Node, inline, index

ENUMS = []

NODES = [
    # Column reference: name, optionally qualified with table and/or schema
    # For "col": column=source span, table=no_span, schema=no_span
    # For "tbl.col": column=source span, table=source span, schema=no_span
    # For "schema.tbl.col": all three populated
    Node("ColumnRef",
        column=inline("SyntaqliteSourceSpan"),
        table=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
    ),
]
