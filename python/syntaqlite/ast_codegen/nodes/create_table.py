# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TABLE statement AST node definitions.

Covers: CREATE TABLE, column definitions, column constraints,
table constraints, foreign keys, generated columns, table options.
"""

from ..defs import Node, List, Enum, inline, index

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

NODES = [
    # Foreign key clause: REFERENCES table [(columns)] [ON DELETE action] [ON UPDATE action] [deferrable]
    Node("ForeignKeyClause",
        ref_table=inline("SyntaqliteSourceSpan"),
        ref_columns=index("ExprList"),          # nullable - referenced column list
        on_delete=inline("ForeignKeyAction"),    # NO_ACTION if not specified
        on_update=inline("ForeignKeyAction"),    # NO_ACTION if not specified
        is_deferred=inline("u8"),                # 0=not deferrable/immediate, 1=deferred
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
        constraint_name=inline("SyntaqliteSourceSpan"),  # CONSTRAINT name (optional)
        onconf=inline("u8"),                    # conflict resolution action (ConflictAction cast)
        sort_order=inline("u8"),                # for PRIMARY KEY: ASC(0)/DESC(1)
        is_autoincrement=inline("u8"),          # for PRIMARY KEY
        collation_name=inline("SyntaqliteSourceSpan"),   # for COLLATE
        generated_storage=inline("GeneratedColumnStorage"),  # for GENERATED
        default_expr=index("Expr"),             # for DEFAULT
        check_expr=index("Expr"),               # for CHECK
        generated_expr=index("Expr"),           # for GENERATED
        fk_clause=index("Expr"),                # for REFERENCES -> ForeignKeyClause node
    ),

    # Column constraint list
    List("ColumnConstraintList", child_type="ColumnConstraint"),

    # Column definition: name type [constraints]
    Node("ColumnDef",
        column_name=inline("SyntaqliteSourceSpan"),
        type_name=inline("SyntaqliteSourceSpan"),   # type token (may be empty)
        constraints=index("Expr"),                   # ColumnConstraintList (nullable)
    ),

    # Column definition list
    List("ColumnDefList", child_type="ColumnDef"),

    # Table constraint
    # kind determines which fields are meaningful:
    #   PRIMARY_KEY: columns, onconf, is_autoincrement
    #   UNIQUE: columns, onconf
    #   CHECK: check_expr, onconf
    #   FOREIGN_KEY: columns (local), fk_clause
    Node("TableConstraint",
        kind=inline("TableConstraintKind"),
        constraint_name=inline("SyntaqliteSourceSpan"),
        onconf=inline("u8"),
        is_autoincrement=inline("u8"),
        columns=index("Expr"),              # OrderByList (for PK/UNIQUE) or ExprList (for FK local columns)
        check_expr=index("Expr"),           # for CHECK
        fk_clause=index("Expr"),            # for FOREIGN KEY -> ForeignKeyClause node
    ),

    # Table constraint list
    List("TableConstraintList", child_type="TableConstraint"),

    # CREATE TABLE statement
    # CREATE [TEMP] TABLE [IF NOT EXISTS] [schema.]name (columns [, constraints]) [options]
    # CREATE [TEMP] TABLE [IF NOT EXISTS] [schema.]name AS select
    Node("CreateTableStmt",
        table_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        is_temp=inline("u8"),
        if_not_exists=inline("u8"),
        table_options=inline("u8"),          # bit flags: 1=WITHOUT ROWID, 2=STRICT
        columns=index("Expr"),               # ColumnDefList (nullable for AS SELECT)
        table_constraints=index("Expr"),     # TableConstraintList (nullable)
        as_select=index("Stmt"),             # for CREATE TABLE ... AS SELECT (nullable)
    ),
]
