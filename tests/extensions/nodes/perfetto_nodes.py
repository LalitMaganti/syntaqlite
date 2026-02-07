# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""PerfettoSQL extension AST node definitions.

These nodes represent PerfettoSQL-specific statements. They use the same
Node/inline/index DSL as src/parser/nodes/*.py but live out-of-tree.
"""

from python.syntaqlite.ast_codegen.defs import Node, inline, index

ENUMS = []

NODES = [
    # CREATE [OR REPLACE] PERFETTO FUNCTION name(...) RETURNS type AS select
    # CREATE [OR REPLACE] PERFETTO FUNCTION name(...) RETURNS type DELEGATES TO name
    Node("PerfettoFunctionStmt",
        func_name=inline("SyntaqliteSourceSpan"),
        body=index("Stmt"),
        is_replace=inline("SyntaqliteBool"),
    ),

    # CREATE [OR REPLACE] PERFETTO TABLE name [USING impl] [(schema)] AS select
    Node("PerfettoTableStmt",
        table_name=inline("SyntaqliteSourceSpan"),
        body=index("Stmt"),
        is_replace=inline("SyntaqliteBool"),
    ),

    # CREATE [OR REPLACE] PERFETTO VIEW name [(schema)] AS select
    Node("PerfettoViewStmt",
        view_name=inline("SyntaqliteSourceSpan"),
        body=index("Stmt"),
        is_replace=inline("SyntaqliteBool"),
    ),

    # CREATE [OR REPLACE] PERFETTO INDEX name ON table (columns)
    Node("PerfettoIndexStmt",
        index_name=inline("SyntaqliteSourceSpan"),
        table_name=inline("SyntaqliteSourceSpan"),
        is_replace=inline("SyntaqliteBool"),
    ),

    # CREATE [OR REPLACE] PERFETTO MACRO name(...) RETURNS type AS body
    Node("PerfettoMacroStmt",
        macro_name=inline("SyntaqliteSourceSpan"),
        is_replace=inline("SyntaqliteBool"),
    ),

    # INCLUDE PERFETTO MODULE module.path
    Node("PerfettoIncludeModuleStmt",
        module_name=inline("SyntaqliteSourceSpan"),
    ),

    # DROP PERFETTO INDEX name ON table
    Node("PerfettoDropIndexStmt",
        index_name=inline("SyntaqliteSourceSpan"),
        table_name=inline("SyntaqliteSourceSpan"),
    ),
]
