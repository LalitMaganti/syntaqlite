# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Conditional expression AST node definitions: IS, BETWEEN, LIKE, CASE."""

from python.syntaqlite.ast_codegen.defs import Node, List, Enum, inline, index
from python.syntaqlite.ast_codegen.fmt_dsl import (
    seq, kw, child, if_set, if_flag, switch, for_each_child,
)

ENUMS = [
    Enum("IsOp",
        "IS",              # 0
        "IS_NOT",          # 1
        "ISNULL",          # 2
        "NOTNULL",         # 3
        "IS_NOT_DISTINCT",   # 4
        "IS_DISTINCT",       # 5
    ),
]

NODES = [
    # IS/ISNULL/NOTNULL expression
    Node("IsExpr",
        op=inline("IsOp"),
        left=index("Expr"),
        right=index("Expr"),
        fmt=switch("op", {
            "ISNULL": seq(child("left"), kw(" ISNULL")),
            "NOTNULL": seq(child("left"), kw(" NOTNULL")),
            "IS": seq(child("left"), kw(" IS "), child("right")),
            "IS_NOT": seq(child("left"), kw(" IS NOT "), child("right")),
            "IS_NOT_DISTINCT": seq(child("left"), kw(" IS NOT DISTINCT FROM "), child("right")),
            "IS_DISTINCT": seq(child("left"), kw(" IS DISTINCT FROM "), child("right")),
        }),
    ),

    # BETWEEN expression: expr [NOT] BETWEEN expr AND expr
    Node("BetweenExpr",
        negated=inline("Bool"),
        operand=index("Expr"),
        low=index("Expr"),
        high=index("Expr"),
        fmt=seq(
            child("operand"),
            if_flag("negated", kw(" NOT BETWEEN "), kw(" BETWEEN ")),
            child("low"),
            kw(" AND "),
            child("high"),
        ),
    ),

    # LIKE/GLOB/MATCH expression: expr [NOT] LIKE expr [ESCAPE expr]
    Node("LikeExpr",
        negated=inline("Bool"),
        operand=index("Expr"),
        pattern=index("Expr"),
        escape=index("Expr"),
        fmt=seq(
            child("operand"),
            if_flag("negated", kw(" NOT LIKE "), kw(" LIKE ")),
            child("pattern"),
            if_set("escape", seq(kw(" ESCAPE "), child("escape"))),
        ),
    ),

    # CASE expression: CASE [operand] WHEN expr THEN expr ... [ELSE expr] END
    Node("CaseExpr",
        operand=index("Expr"),
        else_expr=index("Expr"),
        whens=index("CaseWhenList"),
        fmt=seq(
            kw("CASE"),
            if_set("operand", seq(kw(" "), child("operand"))),
            if_set("whens", child("whens")),
            if_set("else_expr", seq(kw(" ELSE "), child("else_expr"))),
            kw(" END"),
        ),
    ),

    # Single WHEN ... THEN ... pair
    Node("CaseWhen",
        when_expr=index("Expr"),
        then_expr=index("Expr"),
        fmt=seq(kw(" WHEN "), child("when_expr"), kw(" THEN "), child("then_expr")),
    ),

    # List of WHEN clauses - concatenated (no commas)
    List("CaseWhenList", child_type="CaseWhen",
        fmt=for_each_child(child("_item")),
    ),
]
