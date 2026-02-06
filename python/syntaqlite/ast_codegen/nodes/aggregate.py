# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Aggregate function call with ORDER BY AST node definition."""

from ..defs import Node, Flags, inline, index

ENUMS = []

FLAGS = [
    Flags("AggregateFunctionCall", DISTINCT=0x01),
]

NODES = [
    # Aggregate function call with ORDER BY: func(args ORDER BY sortlist)
    # Separate from FunctionCall to preserve ABI compatibility.
    Node("AggregateFunctionCall",
        func_name=inline("SyntaqliteSourceSpan"),
        flags=inline("u8"),
        args=index("ExprList"),
        orderby=index("OrderByList"),
        filter_clause=index("Expr"),        # FILTER (WHERE expr), or null
        over_clause=index("WindowDef"),     # OVER (...) or OVER name, or null
    ),
]
