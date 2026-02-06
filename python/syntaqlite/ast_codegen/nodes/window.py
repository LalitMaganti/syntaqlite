# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Window function AST node definitions.

Covers: WINDOW clause, window definitions, OVER clause,
FILTER clause, frame specifications (ROWS/RANGE/GROUPS),
frame bounds, frame exclusion.
"""

from ..defs import Node, List, Enum, inline, index
from ..fmt_dsl import (
    seq, kw, span, child,
    if_set, if_span, if_enum, switch,
)

ENUMS = [
    Enum("FrameType",
        "NONE",        # 0 - no explicit frame (default)
        "RANGE",       # 1
        "ROWS",        # 2
        "GROUPS",      # 3
    ),

    Enum("FrameBoundType",
        "UNBOUNDED_PRECEDING",  # 0
        "EXPR_PRECEDING",       # 1
        "CURRENT_ROW",          # 2
        "EXPR_FOLLOWING",       # 3
        "UNBOUNDED_FOLLOWING",  # 4
    ),

    Enum("FrameExclude",
        "NONE",          # 0 - no EXCLUDE clause
        "NO_OTHERS",     # 1 - EXCLUDE NO OTHERS
        "CURRENT_ROW",   # 2 - EXCLUDE CURRENT ROW
        "GROUP",         # 3 - EXCLUDE GROUP
        "TIES",          # 4 - EXCLUDE TIES
    ),
]

NODES = [
    # Frame bound: UNBOUNDED PRECEDING, expr PRECEDING, CURRENT ROW, etc.
    Node("FrameBound",
        bound_type=inline("FrameBoundType"),
        expr=index("Expr"),             # nullable - only for EXPR_PRECEDING/FOLLOWING
        fmt=switch("bound_type", {
            "UNBOUNDED_PRECEDING": kw("UNBOUNDED PRECEDING"),
            "EXPR_PRECEDING": seq(child("expr"), kw(" PRECEDING")),
            "CURRENT_ROW": kw("CURRENT ROW"),
            "EXPR_FOLLOWING": seq(child("expr"), kw(" FOLLOWING")),
            "UNBOUNDED_FOLLOWING": kw("UNBOUNDED FOLLOWING"),
        }),
    ),

    # Frame specification: ROWS/RANGE/GROUPS BETWEEN ... AND ... EXCLUDE ...
    Node("FrameSpec",
        frame_type=inline("FrameType"),
        exclude=inline("FrameExclude"),
        start_bound=index("FrameBound"),
        end_bound=index("FrameBound"),
        fmt=seq(
            switch("frame_type", {
                "RANGE": kw("RANGE"),
                "ROWS": kw("ROWS"),
                "GROUPS": kw("GROUPS"),
            }),
            kw(" BETWEEN "), child("start_bound"),
            kw(" AND "), child("end_bound"),
            switch("exclude", {
                "NO_OTHERS": kw(" EXCLUDE NO OTHERS"),
                "CURRENT_ROW": kw(" EXCLUDE CURRENT ROW"),
                "GROUP": kw(" EXCLUDE GROUP"),
                "TIES": kw(" EXCLUDE TIES"),
            }),
        ),
    ),

    # Window definition: base_name PARTITION BY ... ORDER BY ... frame_spec
    Node("WindowDef",
        base_window_name=inline("SynqSourceSpan"),  # optional inherited window name
        partition_by=index("ExprList"),                    # nullable
        orderby=index("OrderByList"),                      # nullable
        frame=index("FrameSpec"),                          # nullable
        fmt=if_span("base_window_name",
            # Named window reference: just the name
            span("base_window_name"),
            # Inline window spec: (...)
            seq(
                kw("("),
                if_set("partition_by", seq(kw("PARTITION BY "), child("partition_by"))),
                if_set("orderby", seq(
                    if_set("partition_by", kw(" ")),
                    kw("ORDER BY "), child("orderby"),
                )),
                if_set("frame", seq(
                    if_set("partition_by", kw(" "), if_set("orderby", kw(" "))),
                    child("frame"),
                )),
                kw(")"),
            )),
    ),

    # List of window definitions (for WINDOW clause)
    List("WindowDefList", child_type="WindowDef"),

    # Named window definition: name AS (window_spec)
    Node("NamedWindowDef",
        window_name=inline("SynqSourceSpan"),
        window_def=index("WindowDef"),
        fmt=seq(span("window_name"), kw(" AS "), child("window_def")),
    ),

    # List of named window definitions
    List("NamedWindowDefList", child_type="NamedWindowDef"),

    # FilterOver: temporary node combining optional FILTER and OVER
    # Used as intermediate result by filter_over nonterminal
    Node("FilterOver",
        filter_expr=index("Expr"),       # FILTER (WHERE expr), nullable
        over_def=index("WindowDef"),     # OVER (...), nullable
        over_name=inline("SynqSourceSpan"),  # OVER nm (named window ref)
        fmt=seq(
            if_set("filter_expr", seq(kw("FILTER (WHERE "), child("filter_expr"), kw(")"))),
            if_set("over_def", seq(kw(" OVER "), child("over_def"))),
            if_span("over_name", seq(kw(" OVER "), span("over_name"))),
        ),
    ),
]
