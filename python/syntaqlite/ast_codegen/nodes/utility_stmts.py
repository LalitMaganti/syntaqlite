# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Utility statement (PRAGMA/ANALYZE/ATTACH/DETACH/VACUUM/REINDEX/EXPLAIN/CREATE INDEX/VIEW) AST node definitions."""

from ..defs import Node, List, Enum, inline, index

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
    ),

    # ANALYZE / REINDEX statement
    Node("AnalyzeStmt",
        target_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        kind=inline("AnalyzeKind"),
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
        is_unique=inline("Bool"),
        if_not_exists=inline("Bool"),
        columns=index("OrderByList"),
        where=index("Expr"),
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
    ),
]
