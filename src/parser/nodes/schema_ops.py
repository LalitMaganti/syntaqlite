# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Schema operations (DROP/ALTER) and transaction control AST node definitions."""

from python.syntaqlite.ast_codegen.defs import Node, Enum, inline, index
from python.syntaqlite.ast_codegen.fmt_dsl import (
    seq, kw, span, child,
    if_set, if_flag, if_span, switch, enum_display,
)

ENUMS = [
    # Type of object being dropped
    Enum("DropObjectType",
        "TABLE",      # 0
        "INDEX",      # 1
        "VIEW",       # 2
        "TRIGGER",    # 3
    ),

    # ALTER TABLE sub-operation type
    Enum("AlterOp",
        "RENAME_TABLE",   # 0 - RENAME TO new_name
        "RENAME_COLUMN",  # 1 - RENAME [COLUMN] old TO new
        "DROP_COLUMN",    # 2 - DROP [COLUMN] name
        "ADD_COLUMN",     # 3 - ADD [COLUMN] name
    ),

    # Transaction type
    Enum("TransactionType",
        "DEFERRED",    # 0
        "IMMEDIATE",   # 1
        "EXCLUSIVE",   # 2
    ),

    # Transaction control statement kind
    Enum("TransactionOp",
        "BEGIN",       # 0
        "COMMIT",      # 1
        "ROLLBACK",    # 2
    ),

    # Savepoint operation kind
    Enum("SavepointOp",
        "SAVEPOINT",   # 0 - SAVEPOINT name
        "RELEASE",     # 1 - RELEASE [SAVEPOINT] name
        "ROLLBACK_TO", # 2 - ROLLBACK TO [SAVEPOINT] name
    ),
]

NODES = [
    # Qualified name: [schema.]name -- used by DROP and ALTER
    Node("QualifiedName",
        object_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        fmt=seq(
            if_span("schema", seq(span("schema"), kw("."))),
            span("object_name"),
        ),
    ),

    # DROP TABLE|INDEX|VIEW|TRIGGER [IF EXISTS] [schema.]name
    Node("DropStmt",
        object_type=inline("DropObjectType"),
        if_exists=inline("Bool"),
        target=index("Expr"),   # QualifiedName node
        fmt=seq(
            kw("DROP "),
            enum_display("object_type", {
                "TABLE": "TABLE",
                "INDEX": "INDEX",
                "VIEW": "VIEW",
                "TRIGGER": "TRIGGER",
            }),
            if_flag("if_exists", kw(" IF EXISTS")),
            kw(" "),
            child("target"),
        ),
    ),

    # ALTER TABLE [schema.]table <operation>
    Node("AlterTableStmt",
        op=inline("AlterOp"),
        target=index("Expr"),                     # QualifiedName - the table
        new_name=inline("SyntaqliteSourceSpan"),  # RENAME TO / RENAME COLUMN new name
        old_name=inline("SyntaqliteSourceSpan"),  # RENAME COLUMN old name / DROP COLUMN name / ADD COLUMN name
        fmt=seq(
            kw("ALTER TABLE "),
            if_set("target", seq(child("target"), kw(" "))),
            switch("op", {
                "RENAME_TABLE": seq(kw("RENAME TO "), span("new_name")),
                "RENAME_COLUMN": seq(kw("RENAME COLUMN "), span("old_name"), kw(" TO "), span("new_name")),
                "DROP_COLUMN": seq(kw("DROP COLUMN "), span("old_name")),
                "ADD_COLUMN": seq(kw("ADD COLUMN "), span("old_name")),
            }),
        ),
    ),

    # BEGIN [DEFERRED|IMMEDIATE|EXCLUSIVE] [TRANSACTION]
    # COMMIT|END [TRANSACTION]
    # ROLLBACK [TRANSACTION]
    Node("TransactionStmt",
        op=inline("TransactionOp"),
        trans_type=inline("TransactionType"),
        fmt=switch("op", {
            "BEGIN": seq(
                kw("BEGIN"),
                enum_display("trans_type", {
                    "IMMEDIATE": " IMMEDIATE",
                    "EXCLUSIVE": " EXCLUSIVE",
                }),
            ),
            "COMMIT": kw("COMMIT"),
            "ROLLBACK": kw("ROLLBACK"),
        }),
    ),

    # SAVEPOINT name / RELEASE [SAVEPOINT] name / ROLLBACK TO [SAVEPOINT] name
    Node("SavepointStmt",
        op=inline("SavepointOp"),
        savepoint_name=inline("SyntaqliteSourceSpan"),
        fmt=switch("op", {
            "SAVEPOINT": seq(kw("SAVEPOINT "), span("savepoint_name")),
            "RELEASE": seq(kw("RELEASE SAVEPOINT "), span("savepoint_name")),
            "ROLLBACK_TO": seq(kw("ROLLBACK TO SAVEPOINT "), span("savepoint_name")),
        }),
    ),
]
