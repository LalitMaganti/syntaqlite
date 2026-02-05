# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Conditional expression AST node definitions: IS, BETWEEN, LIKE, CASE."""

from ..defs import Node, List, Enum, inline, index

ENUMS = [
    # IS expression variants
    Enum("IsOp",
        "IS",              # 0 - expr IS expr
        "IS_NOT",          # 1 - expr IS NOT expr
        "ISNULL",          # 2 - expr ISNULL
        "NOTNULL",         # 3 - expr NOTNULL / expr NOT NULL
        "IS_NOT_DISTINCT",   # 4 - expr IS DISTINCT FROM expr
        "IS_DISTINCT",       # 5 - expr IS NOT DISTINCT FROM expr
    ),
]

NODES = [
    # IS/ISNULL/NOTNULL expression
    Node("IsExpr",
        op=inline("IsOp"),
        left=index("Expr"),
        right=index("Expr"),       # nullable - only used for IS/IS NOT
    ),

    # BETWEEN expression: expr [NOT] BETWEEN expr AND expr
    Node("BetweenExpr",
        negated=inline("u8"),      # 0 = BETWEEN, 1 = NOT BETWEEN
        operand=index("Expr"),
        low=index("Expr"),
        high=index("Expr"),
    ),

    # LIKE/GLOB/MATCH expression: expr [NOT] LIKE expr [ESCAPE expr]
    Node("LikeExpr",
        negated=inline("u8"),      # 0 = LIKE, 1 = NOT LIKE
        operand=index("Expr"),
        pattern=index("Expr"),
        escape=index("Expr"),      # nullable - only with ESCAPE clause
    ),

    # CASE expression: CASE [operand] WHEN expr THEN expr ... [ELSE expr] END
    Node("CaseExpr",
        operand=index("Expr"),     # nullable - simple CASE has no operand
        else_expr=index("Expr"),   # nullable - no ELSE clause
        whens=index("CaseWhenList"),
    ),

    # Single WHEN ... THEN ... pair
    Node("CaseWhen",
        when_expr=index("Expr"),
        then_expr=index("Expr"),
    ),

    # List of WHEN clauses
    List("CaseWhenList", child_type="CaseWhen"),
]
