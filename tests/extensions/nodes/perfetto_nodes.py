# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""PerfettoSQL extension AST node definitions.

These nodes represent PerfettoSQL-specific statements. They use the same
Node/inline/index DSL as src/parser/nodes/*.py but live out-of-tree.
"""

from python.syntaqlite.ast_codegen.defs import Node, inline, index
from python.syntaqlite.ast_codegen.fmt_dsl import seq, kw, span, child, if_flag, if_set, nest, line

ENUMS = []

NODES = [
    # CREATE [OR REPLACE] PERFETTO FUNCTION name(...) RETURNS type AS select
    # CREATE [OR REPLACE] PERFETTO FUNCTION name(...) RETURNS type DELEGATES TO name
    Node("PerfettoFunctionStmt",
        func_name=inline("SyntaqliteSourceSpan"),
        body=index("Stmt"),
        is_replace=inline("SyntaqliteBool"),
        fmt=seq(
            kw("CREATE "),
            if_flag("is_replace", kw("OR REPLACE ")),
            kw("PERFETTO FUNCTION "),
            span("func_name"),
            if_set("body", nest(seq(line(), child("body")))),
        ),
    ),

    # CREATE [OR REPLACE] PERFETTO TABLE name [USING impl] [(schema)] AS select
    Node("PerfettoTableStmt",
        table_name=inline("SyntaqliteSourceSpan"),
        body=index("Stmt"),
        is_replace=inline("SyntaqliteBool"),
        fmt=seq(
            kw("CREATE "),
            if_flag("is_replace", kw("OR REPLACE ")),
            kw("PERFETTO TABLE "),
            span("table_name"),
            if_set("body", nest(seq(line(), child("body")))),
        ),
    ),

    # CREATE [OR REPLACE] PERFETTO VIEW name [(schema)] AS select
    Node("PerfettoViewStmt",
        view_name=inline("SyntaqliteSourceSpan"),
        body=index("Stmt"),
        is_replace=inline("SyntaqliteBool"),
        fmt=seq(
            kw("CREATE "),
            if_flag("is_replace", kw("OR REPLACE ")),
            kw("PERFETTO VIEW "),
            span("view_name"),
            if_set("body", nest(seq(line(), child("body")))),
        ),
    ),

    # CREATE [OR REPLACE] PERFETTO INDEX name ON table (columns)
    Node("PerfettoIndexStmt",
        index_name=inline("SyntaqliteSourceSpan"),
        table_name=inline("SyntaqliteSourceSpan"),
        is_replace=inline("SyntaqliteBool"),
        fmt=seq(
            kw("CREATE "),
            if_flag("is_replace", kw("OR REPLACE ")),
            kw("PERFETTO INDEX "),
            span("index_name"),
            kw(" ON "),
            span("table_name"),
        ),
    ),

    # CREATE [OR REPLACE] PERFETTO MACRO name(...) RETURNS type AS body
    Node("PerfettoMacroStmt",
        macro_name=inline("SyntaqliteSourceSpan"),
        is_replace=inline("SyntaqliteBool"),
        fmt=seq(
            kw("CREATE "),
            if_flag("is_replace", kw("OR REPLACE ")),
            kw("PERFETTO MACRO "),
            span("macro_name"),
        ),
    ),

    # INCLUDE PERFETTO MODULE module.path
    Node("PerfettoIncludeModuleStmt",
        module_name=inline("SyntaqliteSourceSpan"),
        fmt=seq(
            kw("INCLUDE PERFETTO MODULE "),
            span("module_name"),
        ),
    ),

    # DROP PERFETTO INDEX name ON table
    Node("PerfettoDropIndexStmt",
        index_name=inline("SyntaqliteSourceSpan"),
        table_name=inline("SyntaqliteSourceSpan"),
        fmt=seq(
            kw("DROP PERFETTO INDEX "),
            span("index_name"),
            kw(" ON "),
            span("table_name"),
        ),
    ),
]
