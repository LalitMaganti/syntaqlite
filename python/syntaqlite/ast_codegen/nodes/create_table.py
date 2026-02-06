# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TABLE statement AST node definitions.

Covers: CREATE TABLE, column definitions, column constraints,
table constraints, foreign keys, generated columns, table options.
"""

from ..defs import Node, List, Enum, Flags, inline, index
from ..fmt_dsl import (
    seq, kw, span, child, hardline, group, nest, line, softline,
    if_set, if_flag, if_enum, if_span, switch, for_each_child,
)

ENUMS = [
    # Foreign key action for ON DELETE / ON UPDATE
    Enum("ForeignKeyAction",
        "NO_ACTION",    # 0
        "SET_NULL",     # 1
        "SET_DEFAULT",  # 2
        "CASCADE",      # 3
        "RESTRICT",     # 4
    ),

    # Generated column storage type
    Enum("GeneratedColumnStorage",
        "VIRTUAL",  # 0
        "STORED",   # 1
    ),

    # Column constraint kind
    Enum("ColumnConstraintKind",
        "DEFAULT",      # 0
        "NOT_NULL",     # 1
        "PRIMARY_KEY",  # 2
        "UNIQUE",       # 3
        "CHECK",        # 4
        "REFERENCES",   # 5
        "COLLATE",      # 6
        "GENERATED",    # 7
        "NULL",         # 8
    ),

    # Table constraint kind
    Enum("TableConstraintKind",
        "PRIMARY_KEY",  # 0
        "UNIQUE",       # 1
        "CHECK",        # 2
        "FOREIGN_KEY",  # 3
    ),
]

FLAGS = [
    Flags("CreateTableStmtFlags", WITHOUT_ROWID=0x01, STRICT=0x02),
]

NODES = [
    # Foreign key clause: REFERENCES table [(columns)] [ON DELETE action] [ON UPDATE action] [deferrable]
    Node("ForeignKeyClause",
        ref_table=inline("SynqSourceSpan"),
        ref_columns=index("ExprList"),          # nullable - referenced column list
        on_delete=inline("ForeignKeyAction"),    # NO_ACTION if not specified
        on_update=inline("ForeignKeyAction"),    # NO_ACTION if not specified
        is_deferred=inline("Bool"),               # FALSE=not deferrable/immediate, TRUE=deferred
        fmt=seq(
            if_span("ref_table", seq(kw("REFERENCES "), span("ref_table"))),
            if_set("ref_columns", seq(kw("("), child("ref_columns"), kw(")"))),
            switch("on_delete", {
                "SET_NULL": kw(" ON DELETE SET NULL"),
                "SET_DEFAULT": kw(" ON DELETE SET DEFAULT"),
                "CASCADE": kw(" ON DELETE CASCADE"),
                "RESTRICT": kw(" ON DELETE RESTRICT"),
            }),
            switch("on_update", {
                "SET_NULL": kw(" ON UPDATE SET NULL"),
                "SET_DEFAULT": kw(" ON UPDATE SET DEFAULT"),
                "CASCADE": kw(" ON UPDATE CASCADE"),
                "RESTRICT": kw(" ON UPDATE RESTRICT"),
            }),
            if_flag("is_deferred", kw("DEFERRABLE INITIALLY DEFERRED")),
        ),
    ),

    # Column constraint
    # kind determines which fields are meaningful:
    #   DEFAULT: default_expr is set
    #   NOT_NULL/UNIQUE: onconf has conflict resolution
    #   PRIMARY_KEY: onconf, sort_order, is_autoincrement
    #   CHECK: check_expr is set
    #   REFERENCES: fk_clause is set
    #   COLLATE: collation_name is set
    #   GENERATED: generated_expr, generated_storage
    #   NULL: onconf (conflict resolution, usually ignored)
    Node("ColumnConstraint",
        kind=inline("ColumnConstraintKind"),
        constraint_name=inline("SynqSourceSpan"),  # CONSTRAINT name (optional)
        onconf=inline("ConflictAction"),        # conflict resolution action
        sort_order=inline("SortOrder"),         # for PRIMARY KEY: ASC/DESC
        is_autoincrement=inline("Bool"),        # for PRIMARY KEY
        collation_name=inline("SynqSourceSpan"),   # for COLLATE
        generated_storage=inline("GeneratedColumnStorage"),  # for GENERATED
        default_expr=index("Expr"),             # for DEFAULT
        check_expr=index("Expr"),               # for CHECK
        generated_expr=index("Expr"),           # for GENERATED
        fk_clause=index("Expr"),                # for REFERENCES -> ForeignKeyClause node
        fmt=seq(
            if_span("constraint_name", seq(kw("CONSTRAINT "), span("constraint_name"), kw(" "))),
            switch("kind", {
                "PRIMARY_KEY": seq(
                    kw("PRIMARY KEY"),
                    if_enum("sort_order", "DESC", kw(" DESC")),
                    if_flag("is_autoincrement", kw(" AUTOINCREMENT")),
                    switch("onconf", {
                        "ROLLBACK": kw(" ON CONFLICT ROLLBACK"),
                        "ABORT": kw(" ON CONFLICT ABORT"),
                        "FAIL": kw(" ON CONFLICT FAIL"),
                        "IGNORE": kw(" ON CONFLICT IGNORE"),
                        "REPLACE": kw(" ON CONFLICT REPLACE"),
                    }),
                ),
                "NOT_NULL": seq(
                    kw("NOT NULL"),
                    switch("onconf", {
                        "ROLLBACK": kw(" ON CONFLICT ROLLBACK"),
                        "ABORT": kw(" ON CONFLICT ABORT"),
                        "FAIL": kw(" ON CONFLICT FAIL"),
                        "IGNORE": kw(" ON CONFLICT IGNORE"),
                        "REPLACE": kw(" ON CONFLICT REPLACE"),
                    }),
                ),
                "UNIQUE": seq(
                    kw("UNIQUE"),
                    switch("onconf", {
                        "ROLLBACK": kw(" ON CONFLICT ROLLBACK"),
                        "ABORT": kw(" ON CONFLICT ABORT"),
                        "FAIL": kw(" ON CONFLICT FAIL"),
                        "IGNORE": kw(" ON CONFLICT IGNORE"),
                        "REPLACE": kw(" ON CONFLICT REPLACE"),
                    }),
                ),
                "CHECK": seq(kw("CHECK("), child("check_expr"), kw(")")),
                "DEFAULT": seq(kw("DEFAULT "), child("default_expr")),
                "COLLATE": seq(kw("COLLATE "), span("collation_name")),
                "REFERENCES": child("fk_clause"),
                "GENERATED": seq(
                    kw("AS ("), child("generated_expr"), kw(")"),
                    if_enum("generated_storage", "STORED", kw(" STORED")),
                ),
                "NULL": kw("NULL"),
            }),
        ),
    ),

    # Column constraint list
    List("ColumnConstraintList", child_type="ColumnConstraint",
        fmt=for_each_child(child("_item"), separator=kw(" ")),
    ),

    # Column definition: name type [constraints]
    Node("ColumnDef",
        column_name=inline("SynqSourceSpan"),
        type_name=inline("SynqSourceSpan"),   # type token (may be empty)
        constraints=index("Expr"),                   # ColumnConstraintList (nullable)
        fmt=seq(
            span("column_name"),
            if_span("type_name", seq(kw(" "), span("type_name"))),
            if_set("constraints", seq(kw(" "), child("constraints"))),
        ),
    ),

    # Column definition list
    List("ColumnDefList", child_type="ColumnDef",
        fmt=for_each_child(child("_item"), separator=seq(kw(","), line())),
    ),

    # Table constraint
    # kind determines which fields are meaningful:
    #   PRIMARY_KEY: columns, onconf, is_autoincrement
    #   UNIQUE: columns, onconf
    #   CHECK: check_expr, onconf
    #   FOREIGN_KEY: columns (local), fk_clause
    Node("TableConstraint",
        kind=inline("TableConstraintKind"),
        constraint_name=inline("SynqSourceSpan"),
        onconf=inline("ConflictAction"),
        is_autoincrement=inline("Bool"),
        columns=index("Expr"),              # OrderByList (for PK/UNIQUE) or ExprList (for FK local columns)
        check_expr=index("Expr"),           # for CHECK
        fk_clause=index("Expr"),            # for FOREIGN KEY -> ForeignKeyClause node
        fmt=seq(
            if_span("constraint_name", seq(kw("CONSTRAINT "), span("constraint_name"), kw(" "))),
            switch("kind", {
                "PRIMARY_KEY": seq(
                    kw("PRIMARY KEY("), child("columns"), kw(")"),
                    switch("onconf", {
                        "ROLLBACK": kw(" ON CONFLICT ROLLBACK"),
                        "ABORT": kw(" ON CONFLICT ABORT"),
                        "FAIL": kw(" ON CONFLICT FAIL"),
                        "IGNORE": kw(" ON CONFLICT IGNORE"),
                        "REPLACE": kw(" ON CONFLICT REPLACE"),
                    }),
                ),
                "UNIQUE": seq(
                    kw("UNIQUE("), child("columns"), kw(")"),
                    switch("onconf", {
                        "ROLLBACK": kw(" ON CONFLICT ROLLBACK"),
                        "ABORT": kw(" ON CONFLICT ABORT"),
                        "FAIL": kw(" ON CONFLICT FAIL"),
                        "IGNORE": kw(" ON CONFLICT IGNORE"),
                        "REPLACE": kw(" ON CONFLICT REPLACE"),
                    }),
                ),
                "CHECK": seq(kw("CHECK("), child("check_expr"), kw(")")),
                "FOREIGN_KEY": seq(
                    kw("FOREIGN KEY("), child("columns"), kw(") "),
                    child("fk_clause"),
                ),
            }),
        ),
    ),

    # Table constraint list
    List("TableConstraintList", child_type="TableConstraint",
        fmt=for_each_child(child("_item"), separator=seq(kw(","), line())),
    ),

    # CREATE TABLE statement
    # CREATE [TEMP] TABLE [IF NOT EXISTS] [schema.]name (columns [, constraints]) [options]
    # CREATE [TEMP] TABLE [IF NOT EXISTS] [schema.]name AS select
    Node("CreateTableStmt",
        table_name=inline("SynqSourceSpan"),
        schema=inline("SynqSourceSpan"),
        is_temp=inline("Bool"),
        if_not_exists=inline("Bool"),
        flags=inline("CreateTableStmtFlags"),   # WITHOUT_ROWID | STRICT
        columns=index("Expr"),               # ColumnDefList (nullable for AS SELECT)
        table_constraints=index("Expr"),     # TableConstraintList (nullable)
        as_select=index("Stmt"),             # for CREATE TABLE ... AS SELECT (nullable)
        fmt=group(seq(
            kw("CREATE"),
            if_flag("is_temp", kw(" TEMP")),
            kw(" TABLE"),
            if_flag("if_not_exists", kw(" IF NOT EXISTS")),
            kw(" "),
            if_span("schema", seq(span("schema"), kw("."))),
            span("table_name"),
            if_set("columns", seq(
                kw("("),
                group(nest(seq(softline(), child("columns"),
                    if_set("table_constraints", seq(kw(","), line(), child("table_constraints"))),
                ))),
                softline(),
                kw(")"),
            )),
            if_set("as_select", seq(kw(" AS"), hardline(), child("as_select"))),
            if_flag("flags.without_rowid", kw(" WITHOUT ROWID")),
            if_flag("flags.strict", kw(" STRICT")),
        )),
    ),
]
