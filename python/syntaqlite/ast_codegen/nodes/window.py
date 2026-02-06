# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Window function AST node definitions.

Covers: WINDOW clause, window definitions, OVER clause,
FILTER clause, frame specifications (ROWS/RANGE/GROUPS),
frame bounds, frame exclusion.
"""

from ..defs import Node, List, Enum, inline, index

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
    ),

    # Frame specification: ROWS/RANGE/GROUPS BETWEEN ... AND ... EXCLUDE ...
    Node("FrameSpec",
        frame_type=inline("FrameType"),
        exclude=inline("FrameExclude"),
        start_bound=index("FrameBound"),
        end_bound=index("FrameBound"),
    ),

    # Window definition: base_name PARTITION BY ... ORDER BY ... frame_spec
    Node("WindowDef",
        base_window_name=inline("SyntaqliteSourceSpan"),  # optional inherited window name
        partition_by=index("ExprList"),                    # nullable
        orderby=index("OrderByList"),                      # nullable
        frame=index("FrameSpec"),                          # nullable
    ),

    # List of window definitions (for WINDOW clause)
    List("WindowDefList", child_type="WindowDef"),

    # Named window definition: name AS (window_spec)
    Node("NamedWindowDef",
        window_name=inline("SyntaqliteSourceSpan"),
        window_def=index("WindowDef"),
    ),

    # List of named window definitions
    List("NamedWindowDefList", child_type="NamedWindowDef"),

    # FilterOver: temporary node combining optional FILTER and OVER
    # Used as intermediate result by filter_over nonterminal
    Node("FilterOver",
        filter_expr=index("Expr"),       # FILTER (WHERE expr), nullable
        over_def=index("WindowDef"),     # OVER (...), nullable
        over_name=inline("SyntaqliteSourceSpan"),  # OVER nm (named window ref)
    ),
]
