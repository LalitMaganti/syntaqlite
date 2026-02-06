# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Function call AST node definitions."""

from ..defs import Node, List, Flags, inline, index

ENUMS = []

FLAGS = [
    Flags("FunctionCallFlags", DISTINCT=0x01, STAR=0x02),
]

NODES = [
    # Function call: name(args)
    # name is stored as a source span (the function identifier)
    # args is an ExprList (or SYNTAQLITE_NULL_NODE for no args)
    Node("FunctionCall",
        func_name=inline("SyntaqliteSourceSpan"),
        flags=inline("FunctionCallFlags"),
        args=index("ExprList"),
        filter_clause=index("Expr"),        # FILTER (WHERE expr), or null
        over_clause=index("WindowDef"),     # OVER (...) or OVER name, or null
    ),
]
