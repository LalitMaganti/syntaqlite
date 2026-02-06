# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""DML (INSERT/UPDATE/DELETE) AST node definitions."""

from python.syntaqlite.ast_codegen.defs import Node, List, Enum, inline, index
from python.syntaqlite.ast_codegen.fmt_dsl import (
    seq, kw, span, child, hardline, group, if_set, if_span, if_enum, clause, switch,
)

ENUMS = [
    Enum("ConflictAction",
        "DEFAULT",    # 0
        "ROLLBACK",   # 1
        "ABORT",      # 2
        "FAIL",       # 3
        "IGNORE",     # 4
        "REPLACE",    # 5
    ),
]

NODES = [
    # DELETE FROM [schema.]table [WHERE expr]
    Node("DeleteStmt",
        table=index("Expr"),
        where=index("Expr"),
        fmt=group(seq(
            kw("DELETE"),
            if_set("table", clause("FROM", child("table"))),
            if_set("where", clause("WHERE", child("where"))),
        )),
    ),

    # SET clause assignment: column = expr (or (col1, col2) = expr)
    Node("SetClause",
        column=inline("SyntaqliteSourceSpan"),
        columns=index("ExprList"),
        value=index("Expr"),
        fmt=seq(
            if_span("column",
                span("column"),
                if_set("columns", seq(kw("("), child("columns"), kw(")")))),
            kw(" = "),
            child("value"),
        ),
    ),

    # List of SET clause assignments
    List("SetClauseList", child_type="SetClause"),

    # UPDATE [OR conflict] [schema.]table SET ... [FROM ...] [WHERE expr]
    Node("UpdateStmt",
        conflict_action=inline("ConflictAction"),
        table=index("Expr"),
        setlist=index("SetClauseList"),
        from_clause=index("Expr"),
        where=index("Expr"),
        fmt=group(seq(
            kw("UPDATE"),
            switch("conflict_action", {
                "ROLLBACK": kw(" OR ROLLBACK"),
                "ABORT": kw(" OR ABORT"),
                "FAIL": kw(" OR FAIL"),
                "IGNORE": kw(" OR IGNORE"),
                "REPLACE": kw(" OR REPLACE"),
            }),
            kw(" "), child("table"),
            if_set("setlist", clause("SET", child("setlist"))),
            if_set("from_clause", clause("FROM", child("from_clause"))),
            if_set("where", clause("WHERE", child("where"))),
        )),
    ),

    # INSERT [OR conflict] INTO [schema.]table [(columns)] source
    Node("InsertStmt",
        conflict_action=inline("ConflictAction"),
        table=index("Expr"),
        columns=index("ExprList"),
        source=index("Stmt"),
        fmt=seq(
            if_enum("conflict_action", "REPLACE",
                kw("REPLACE"),
                seq(
                    kw("INSERT"),
                    switch("conflict_action", {
                        "ROLLBACK": kw(" OR ROLLBACK"),
                        "ABORT": kw(" OR ABORT"),
                        "FAIL": kw(" OR FAIL"),
                        "IGNORE": kw(" OR IGNORE"),
                    }),
                )),
            kw(" INTO "), child("table"),
            if_set("columns", seq(kw(" ("), child("columns"), kw(")"))),
            if_set("source", seq(hardline(), child("source"))),
        ),
    ),
]
