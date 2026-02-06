# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Aggregate function call with ORDER BY AST node definition."""

from ..defs import Node, Flags, inline, index
from ..fmt_dsl import seq, kw, span, child, group, nest, softline, if_set, if_flag

ENUMS = []

FLAGS = [
    Flags("AggregateFunctionCallFlags", DISTINCT=0x01),
]

NODES = [
    # Aggregate function call with ORDER BY: func(args ORDER BY sortlist)
    Node("AggregateFunctionCall",
        func_name=inline("SynqSourceSpan"),
        flags=inline("AggregateFunctionCallFlags"),
        args=index("ExprList"),
        orderby=index("OrderByList"),
        filter_clause=index("Expr"),
        over_clause=index("WindowDef"),
        fmt=seq(
            span("func_name"),
            group(seq(
                kw("("),
                if_flag("flags.distinct", kw("DISTINCT ")),
                if_set("args",
                    nest(seq(softline(), child("args")))),
                if_set("orderby", seq(kw(" ORDER BY "), child("orderby"))),
                softline(),
                kw(")"))),
            if_set("filter_clause", seq(kw(" FILTER (WHERE "), child("filter_clause"), kw(")"))),
            if_set("over_clause", seq(kw(" OVER "), child("over_clause"))),
        ),
    ),
]
