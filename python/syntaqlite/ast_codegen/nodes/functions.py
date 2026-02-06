# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Function call AST node definitions."""

from ..defs import Node, List, Flags, inline, index
from ..fmt_dsl import (
    seq, kw, span, child, softline, group, nest,
    if_set, if_flag,
)

ENUMS = []

FLAGS = [
    Flags("FunctionCallFlags", DISTINCT=0x01, STAR=0x02),
]

NODES = [
    # Function call: name(args)
    Node("FunctionCall",
        func_name=inline("SynqSourceSpan"),
        flags=inline("FunctionCallFlags"),
        args=index("ExprList"),
        filter_clause=index("Expr"),
        over_clause=index("WindowDef"),
        fmt=seq(
            span("func_name"),
            group(seq(
                kw("("),
                if_flag("flags.distinct", kw("DISTINCT ")),
                if_flag("flags.star",
                    kw("*"),
                    if_set("args",
                        nest(seq(softline(), child("args"))))),
                softline(),
                kw(")"))),
            if_set("filter_clause", seq(kw(" FILTER (WHERE "), child("filter_clause"), kw(")"))),
            if_set("over_clause", seq(kw(" OVER "), child("over_clause"))),
        ),
    ),
]
