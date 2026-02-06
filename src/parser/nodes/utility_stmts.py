# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Utility statement (PRAGMA/ANALYZE/ATTACH/DETACH/VACUUM/REINDEX/EXPLAIN/CREATE INDEX/VIEW) AST node definitions."""

from python.syntaqlite.ast_codegen.defs import Node, Enum, inline, index
from python.syntaqlite.ast_codegen.fmt_dsl import (
    seq, kw, span, child, group, hardline,
    if_set, if_flag, if_span, if_enum, clause, switch,
)

ENUMS = [
    # EXPLAIN mode
    Enum("ExplainMode",
        "EXPLAIN",         # 0
        "QUERY_PLAN",      # 1
    ),

    # PRAGMA value form
    Enum("PragmaForm",
        "BARE",            # 0 - no value
        "EQ",              # 1 - = value
        "CALL",            # 2 - (value)
    ),

    # ANALYZE vs REINDEX (they share a node)
    Enum("AnalyzeKind",
        "ANALYZE",         # 0
        "REINDEX",         # 1
    ),
]

NODES = [
    # PRAGMA statement: PRAGMA [schema.]name [= value | (value)]
    Node("PragmaStmt",
        pragma_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        value=inline("SyntaqliteSourceSpan"),
        pragma_form=inline("PragmaForm"),
        fmt=seq(
            kw("PRAGMA "),
            if_span("schema", seq(span("schema"), kw("."))),
            span("pragma_name"),
            switch("pragma_form", {
                "EQ": seq(kw(" = "), span("value")),
                "CALL": seq(kw("("), span("value"), kw(")")),
            }),
        ),
    ),

    # ANALYZE / REINDEX statement
    Node("AnalyzeStmt",
        target_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        kind=inline("AnalyzeKind"),
        fmt=seq(
            if_enum("kind", "REINDEX", kw("REINDEX"), kw("ANALYZE")),
            if_span("schema", seq(kw(" "), span("schema"), kw("."), span("target_name")),
                if_span("target_name", seq(kw(" "), span("target_name")))),
        ),
    ),

    # ATTACH statement: ATTACH [DATABASE] expr AS expr [KEY expr]
    Node("AttachStmt",
        filename=index("Expr"),
        db_name=index("Expr"),
        key=index("Expr"),
        fmt=seq(
            kw("ATTACH "),
            child("filename"),
            kw(" AS "),
            child("db_name"),
            if_set("key", seq(kw(" KEY "), child("key"))),
        ),
    ),

    # DETACH statement: DETACH [DATABASE] expr
    Node("DetachStmt",
        db_name=index("Expr"),
        fmt=seq(kw("DETACH "), child("db_name")),
    ),

    # VACUUM statement: VACUUM [schema] [INTO expr]
    Node("VacuumStmt",
        schema=inline("SyntaqliteSourceSpan"),
        into_expr=index("Expr"),
        fmt=seq(
            kw("VACUUM"),
            if_span("schema", seq(kw(" "), span("schema"))),
            if_set("into_expr", seq(kw(" INTO "), child("into_expr"))),
        ),
    ),

    # EXPLAIN [QUERY PLAN] statement - wraps any cmd
    Node("ExplainStmt",
        explain_mode=inline("ExplainMode"),
        stmt=index("Stmt"),
        fmt=seq(
            if_enum("explain_mode", "QUERY_PLAN",
                kw("EXPLAIN QUERY PLAN"),
                kw("EXPLAIN")),
            hardline(),
            child("stmt"),
        ),
    ),

    # CREATE INDEX statement
    # CREATE [UNIQUE] INDEX [IF NOT EXISTS] [schema.]name ON table (columns) [WHERE expr]
    Node("CreateIndexStmt",
        index_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        table_name=inline("SyntaqliteSourceSpan"),
        is_unique=inline("Bool"),
        if_not_exists=inline("Bool"),
        columns=index("OrderByList"),
        where=index("Expr"),
        fmt=group(seq(
            kw("CREATE"),
            if_flag("is_unique", kw(" UNIQUE")),
            kw(" INDEX"),
            if_flag("if_not_exists", kw(" IF NOT EXISTS")),
            kw(" "),
            if_span("schema", seq(span("schema"), kw("."))),
            span("index_name"),
            kw(" ON "),
            span("table_name"),
            kw(" ("),
            child("columns"),
            kw(")"),
            if_set("where", clause("WHERE", child("where"))),
        )),
    ),

    # CREATE VIEW statement
    # CREATE [TEMP] VIEW [IF NOT EXISTS] [schema.]name [(columns)] AS select
    Node("CreateViewStmt",
        view_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        is_temp=inline("Bool"),
        if_not_exists=inline("Bool"),
        column_names=index("ExprList"),
        select=index("Stmt"),
        fmt=seq(
            kw("CREATE"),
            if_flag("is_temp", kw(" TEMP")),
            kw(" VIEW"),
            if_flag("if_not_exists", kw(" IF NOT EXISTS")),
            kw(" "),
            if_span("schema", seq(span("schema"), kw("."))),
            span("view_name"),
            if_set("column_names", group(seq(kw("("), child("column_names"), kw(")")))),
            kw(" AS"),
            hardline(),
            child("select"),
        ),
    ),
]
