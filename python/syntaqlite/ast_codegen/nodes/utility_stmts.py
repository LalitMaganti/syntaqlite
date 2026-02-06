# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Utility statement (PRAGMA/ANALYZE/ATTACH/DETACH/VACUUM/REINDEX/EXPLAIN/CREATE INDEX/VIEW) AST node definitions."""

from ..defs import Node, List, Enum, inline, index

ENUMS = [
    # EXPLAIN mode: 1 = EXPLAIN, 2 = EXPLAIN QUERY PLAN
    Enum("ExplainMode",
        "EXPLAIN",         # 0
        "QUERY_PLAN",      # 1
    ),
]

NODES = [
    # PRAGMA statement: PRAGMA [schema.]name [= value | (value)]
    # pragma_form: 0 = bare (no value), 1 = EQ form, 2 = function call form
    # value: source span of the value (nmnum or minus_num), null if bare
    Node("PragmaStmt",
        pragma_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        value=inline("SyntaqliteSourceSpan"),
        pragma_form=inline("u8"),
    ),

    # ANALYZE / REINDEX statement
    # is_reindex: 0 = ANALYZE, 1 = REINDEX
    Node("AnalyzeStmt",
        target_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        is_reindex=inline("u8"),
    ),

    # ATTACH statement: ATTACH [DATABASE] expr AS expr [KEY expr]
    Node("AttachStmt",
        filename=index("Expr"),
        db_name=index("Expr"),
        key=index("Expr"),
    ),

    # DETACH statement: DETACH [DATABASE] expr
    Node("DetachStmt",
        db_name=index("Expr"),
    ),

    # VACUUM statement: VACUUM [schema] [INTO expr]
    Node("VacuumStmt",
        schema=inline("SyntaqliteSourceSpan"),
        into_expr=index("Expr"),
    ),

    # EXPLAIN [QUERY PLAN] statement - wraps any cmd
    Node("ExplainStmt",
        explain_mode=inline("ExplainMode"),
        stmt=index("Stmt"),
    ),

    # CREATE INDEX statement
    # CREATE [UNIQUE] INDEX [IF NOT EXISTS] [schema.]name ON table (columns) [WHERE expr]
    Node("CreateIndexStmt",
        index_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        table_name=inline("SyntaqliteSourceSpan"),
        is_unique=inline("u8"),
        if_not_exists=inline("u8"),
        columns=index("OrderByList"),
        where=index("Expr"),
    ),

    # CREATE VIEW statement
    # CREATE [TEMP] VIEW [IF NOT EXISTS] [schema.]name [(columns)] AS select
    Node("CreateViewStmt",
        view_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        is_temp=inline("u8"),
        if_not_exists=inline("u8"),
        column_names=index("ExprList"),
        select=index("Stmt"),
    ),
]
