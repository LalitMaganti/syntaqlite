# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Function call AST node definitions."""

from ..defs import Node, List, inline, index

ENUMS = []

NODES = [
    # Function call: name(args)
    # name is stored as a source span (the function identifier)
    # args is an ExprList (or SYNTAQLITE_NULL_NODE for no args)
    # flags: bit 0 = DISTINCT, bit 1 = star (COUNT(*))
    Node("FunctionCall",
        func_name=inline("SyntaqliteSourceSpan"),
        flags=inline("u8"),
        args=index("ExprList"),
    ),
]
