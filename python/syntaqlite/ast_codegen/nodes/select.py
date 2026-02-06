# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST node definitions."""

from ..defs import Node, List, Enum, Flags, inline, index
from ..fmt_dsl import (
    seq, kw, span, child, line, group, nest, clause,
    if_set, if_flag, if_enum, if_span,
)

ENUMS = [
    Enum("SortOrder",
        "ASC",        # 0
        "DESC",       # 1
    ),

    Enum("NullsOrder",
        "NONE",       # 0 - no explicit NULLS keyword
        "FIRST",      # 1
        "LAST",       # 2
    ),
]

FLAGS = [
    Flags("ResultColumnFlags", STAR=0x01),
    Flags("SelectStmtFlags", DISTINCT=0x01),
]

NODES = [
    # Result column: expr [AS alias] or *
    Node("ResultColumn",
        flags=inline("ResultColumnFlags"),
        alias=inline("SynqSourceSpan"),
        expr=index("Expr"),
        fmt=seq(
            if_flag("flags.star",
                # star column: either bare * or table.*
                if_set("expr",
                    seq(child("expr"), kw(".*")),
                    kw("*")),
                # normal column: expr [AS alias]
                child("expr")),
            if_span("alias", seq(kw(" AS "), span("alias"))),
        ),
    ),

    # List of result columns
    List("ResultColumnList", child_type="ResultColumn"),

    # SELECT statement
    Node("SelectStmt",
        flags=inline("SelectStmtFlags"),
        columns=index("ResultColumnList"),
        from_clause=index("Expr"),
        where=index("Expr"),
        groupby=index("ExprList"),
        having=index("Expr"),
        orderby=index("OrderByList"),
        limit_clause=index("LimitClause"),
        window_clause=index("NamedWindowDefList"),
        fmt=group(seq(
            if_flag("flags.distinct", kw("SELECT DISTINCT"), kw("SELECT")),
            if_set("columns",
                group(nest(seq(line(), child("columns"))))),
            if_set("from_clause", clause("FROM", child("from_clause"))),
            if_set("where", clause("WHERE", child("where"))),
            if_set("groupby", clause("GROUP BY", child("groupby"))),
            if_set("having", clause("HAVING", child("having"))),
            if_set("orderby", clause("ORDER BY", child("orderby"))),
            if_set("limit_clause", clause("LIMIT", child("limit_clause"))),
            if_set("window_clause", clause("WINDOW", child("window_clause"))),
        )),
    ),

    # Ordering term: expr [ASC|DESC] [NULLS FIRST|LAST]
    Node("OrderingTerm",
        expr=index("Expr"),
        sort_order=inline("SortOrder"),
        nulls_order=inline("NullsOrder"),
        fmt=seq(
            child("expr"),
            if_enum("sort_order", "DESC", kw(" DESC")),
            if_enum("nulls_order", "FIRST", kw(" NULLS FIRST")),
            if_enum("nulls_order", "LAST", kw(" NULLS LAST")),
        ),
    ),

    # List of ordering terms (ORDER BY clause)
    List("OrderByList", child_type="OrderingTerm"),

    # LIMIT clause with optional OFFSET
    Node("LimitClause",
        limit=index("Expr"),
        offset=index("Expr"),
        fmt=seq(
            child("limit"),
            if_set("offset", seq(kw(" OFFSET "), child("offset"))),
        ),
    ),
]
