# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TABLE AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CreateTableBasic(TestSuite):
    """Basic CREATE TABLE tests."""

    def test_simple_one_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_multiple_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, b TEXT, c REAL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [3 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "TEXT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "c"
                    type_name: "REAL"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_no_type(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a, b, c)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [3 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: (none)
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: (none)
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "c"
                    type_name: (none)
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_compound_type(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a VARCHAR(255), b DECIMAL(10, 2))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "VARCHAR(255)"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "DECIMAL(10, 2)"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class CreateTableModifiers(TestSuite):
    """CREATE TABLE with TEMP, IF NOT EXISTS, schema prefix."""

    def test_temp(self):
        return DiffTestBlueprint(
            sql="CREATE TEMP TABLE t(a INT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: TRUE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_if_not_exists(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE IF NOT EXISTS t(a INT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: TRUE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_schema_prefix(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE main.t(a INT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: "main"
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class CreateTableOptions(TestSuite):
    """Table options: WITHOUT ROWID, STRICT."""

    def test_without_rowid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY) WITHOUT ROWID",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: WITHOUT_ROWID
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_strict(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT) STRICT",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: STRICT
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_without_rowid_strict(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY) WITHOUT ROWID, STRICT",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: WITHOUT_ROWID STRICT
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class CreateTableAsSelect(TestSuite):
    """CREATE TABLE AS SELECT."""

    def test_as_select(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t2 AS SELECT * FROM t1",
            out="""\
            CreateTableStmt
              table_name: "t2"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns: (none)
              table_constraints: (none)
              as_select:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: STAR
                        alias: (none)
                        expr: (none)
                  from_clause:
                    TableRef
                      table_name: "t1"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )


class ColumnConstraintDefault(TestSuite):
    """Column DEFAULT constraint tests."""

    def test_default_integer(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT 42)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: DEFAULT
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr:
                            Literal
                              literal_type: INTEGER
                              source: "42"
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_default_string(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a TEXT DEFAULT 'hello')",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "TEXT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: DEFAULT
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr:
                            Literal
                              literal_type: STRING
                              source: "'hello'"
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_default_negative(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT -1)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: DEFAULT
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr:
                            UnaryExpr
                              op: MINUS
                              operand:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_default_expr(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT (1 + 2))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: DEFAULT
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr:
                            BinaryExpr
                              op: PLUS
                              left:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                              right:
                                Literal
                                  literal_type: INTEGER
                                  source: "2"
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_default_true(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT TRUE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: DEFAULT
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr:
                            ColumnRef
                              column: "TRUE"
                              table: (none)
                              schema: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintKeys(TestSuite):
    """PRIMARY KEY, UNIQUE, NOT NULL column constraints."""

    def test_primary_key(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_primary_key_autoincrement(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INTEGER PRIMARY KEY AUTOINCREMENT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INTEGER"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: TRUE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_primary_key_desc(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY DESC)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: DESC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_not_null(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a TEXT NOT NULL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "TEXT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: NOT_NULL
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_unique(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a TEXT UNIQUE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "TEXT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: UNIQUE
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintCheck(TestSuite):
    """CHECK column constraint."""

    def test_check(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT CHECK(a > 0))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: CHECK
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr:
                            BinaryExpr
                              op: GT
                              left:
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                              right:
                                Literal
                                  literal_type: INTEGER
                                  source: "0"
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintReferences(TestSuite):
    """REFERENCES (foreign key) column constraint."""

    def test_references_simple(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: UNSET
                              on_update: UNSET
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_references_on_delete_cascade(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id) ON DELETE CASCADE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: CASCADE
                              on_update: UNSET
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_references_on_update_set_null(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id) ON UPDATE SET NULL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: UNSET
                              on_update: SET_NULL
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintCollate(TestSuite):
    """COLLATE column constraint."""

    def test_collate(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a TEXT COLLATE NOCASE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "TEXT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: COLLATE
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: "NOCASE"
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintGenerated(TestSuite):
    """Generated column constraints."""

    def test_generated_always_stored(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT AS (a * 2) STORED)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: GENERATED
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: STORED
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr:
                            BinaryExpr
                              op: STAR
                              left:
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                              right:
                                Literal
                                  literal_type: INTEGER
                                  source: "2"
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_generated_virtual(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT AS (a + 1))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: GENERATED
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr:
                            BinaryExpr
                              op: PLUS
                              left:
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                              right:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintName(TestSuite):
    """Named column constraints."""

    def test_named_not_null(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT CONSTRAINT nn NOT NULL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: NOT_NULL
                          constraint_name: "nn"
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class TableConstraintPrimaryKey(TestSuite):
    """Table-level PRIMARY KEY constraint."""

    def test_table_pk(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT, PRIMARY KEY(a, b))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: PRIMARY_KEY
                    constraint_name: (none)
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns:
                      OrderByList [2 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "b"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    fk_columns: (none)
                    check_expr: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )

    def test_named_table_pk(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, CONSTRAINT pk PRIMARY KEY(a))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: PRIMARY_KEY
                    constraint_name: "pk"
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    fk_columns: (none)
                    check_expr: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )


class TableConstraintUnique(TestSuite):
    """Table-level UNIQUE constraint."""

    def test_table_unique(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT, UNIQUE(a, b))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: UNIQUE
                    constraint_name: (none)
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns:
                      OrderByList [2 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "b"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    fk_columns: (none)
                    check_expr: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )


class TableConstraintCheck(TestSuite):
    """Table-level CHECK constraint."""

    def test_table_check(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT, CHECK(a > b))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: CHECK
                    constraint_name: (none)
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns: (none)
                    fk_columns: (none)
                    check_expr:
                      BinaryExpr
                        op: GT
                        left:
                          ColumnRef
                            column: "a"
                            table: (none)
                            schema: (none)
                        right:
                          ColumnRef
                            column: "b"
                            table: (none)
                            schema: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )


class TableConstraintForeignKey(TestSuite):
    """Table-level FOREIGN KEY constraint."""

    def test_table_fk(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: FOREIGN_KEY
                    constraint_name: (none)
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns: (none)
                    fk_columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "a"
                          table: (none)
                          schema: (none)
                    check_expr: (none)
                    fk_clause:
                      ForeignKeyClause
                        ref_table: "other"
                        ref_columns:
                          ExprList [1 items]
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                        match_name: (none)
                        on_delete: UNSET
                        on_update: UNSET
                        on_insert: UNSET
                        deferrable: UNSET
                        initial_defer: UNSET
              as_select: (none)
""",
        )

    def test_table_fk_with_actions(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id) ON DELETE CASCADE ON UPDATE SET NULL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: FOREIGN_KEY
                    constraint_name: (none)
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns: (none)
                    fk_columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "a"
                          table: (none)
                          schema: (none)
                    check_expr: (none)
                    fk_clause:
                      ForeignKeyClause
                        ref_table: "other"
                        ref_columns:
                          ExprList [1 items]
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                        match_name: (none)
                        on_delete: CASCADE
                        on_update: SET_NULL
                        on_insert: UNSET
                        deferrable: UNSET
                        initial_defer: UNSET
              as_select: (none)
""",
        )

    def test_table_fk_deferred(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id) DEFERRABLE INITIALLY DEFERRED)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: FOREIGN_KEY
                    constraint_name: (none)
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns: (none)
                    fk_columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "a"
                          table: (none)
                          schema: (none)
                    check_expr: (none)
                    fk_clause:
                      ForeignKeyClause
                        ref_table: "other"
                        ref_columns:
                          ExprList [1 items]
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                        match_name: (none)
                        on_delete: UNSET
                        on_update: UNSET
                        on_insert: UNSET
                        deferrable: DEFERRABLE
                        initial_defer: DEFERRED
              as_select: (none)
""",
        )


class ForeignKeyActionSetDefaultRestrict(TestSuite):
    """Foreign key SET DEFAULT and RESTRICT actions."""

    def test_fk_on_delete_set_default(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT REFERENCES other(id) ON DELETE SET DEFAULT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: SET_DEFAULT
                              on_update: UNSET
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_fk_on_delete_restrict(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT REFERENCES other(id) ON DELETE RESTRICT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: RESTRICT
                              on_update: UNSET
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_fk_on_update_set_default(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT REFERENCES other(id) ON UPDATE SET DEFAULT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: UNSET
                              on_update: SET_DEFAULT
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_fk_on_update_restrict(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT REFERENCES other(id) ON UPDATE RESTRICT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: REFERENCES
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause:
                            ForeignKeyClause
                              ref_table: "other"
                              ref_columns:
                                ExprList [1 items]
                                  ColumnRef
                                    column: "id"
                                    table: (none)
                                    schema: (none)
                              match_name: (none)
                              on_delete: UNSET
                              on_update: RESTRICT
                              on_insert: UNSET
                              deferrable: UNSET
                              initial_defer: UNSET
              table_constraints: (none)
              as_select: (none)
""",
        )


class OnConflictClause(TestSuite):
    """ON CONFLICT clause on column and table constraints."""

    def test_on_conflict_column_pk(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT PRIMARY KEY ON CONFLICT ROLLBACK)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: ROLLBACK
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_on_conflict_column_not_null(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT NOT NULL ON CONFLICT ABORT)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: NOT_NULL
                          constraint_name: (none)
                          onconf: ABORT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_on_conflict_column_unique(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT UNIQUE ON CONFLICT FAIL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: UNIQUE
                          constraint_name: (none)
                          onconf: FAIL
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_on_conflict_table_pk(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, PRIMARY KEY (a) ON CONFLICT IGNORE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: PRIMARY_KEY
                    constraint_name: (none)
                    onconf: IGNORE
                    is_autoincrement: FALSE
                    pk_columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    fk_columns: (none)
                    check_expr: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )

    def test_on_conflict_table_unique(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, UNIQUE (a) ON CONFLICT REPLACE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: UNIQUE
                    constraint_name: (none)
                    onconf: REPLACE
                    is_autoincrement: FALSE
                    pk_columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    fk_columns: (none)
                    check_expr: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )


class MultipleColumnConstraints(TestSuite):
    """Multiple constraints on a single column."""

    def test_not_null_default_unique(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT NOT NULL DEFAULT 0 UNIQUE)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [3 items]
                        ColumnConstraint
                          kind: NOT_NULL
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
                        ColumnConstraint
                          kind: DEFAULT
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr:
                            Literal
                              literal_type: INTEGER
                              source: "0"
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
                        ColumnConstraint
                          kind: UNIQUE
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class NamedTableConstraints(TestSuite):
    """Named table-level constraints."""

    def test_named_unique(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, CONSTRAINT uq UNIQUE (a))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: UNIQUE
                    constraint_name: "uq"
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    fk_columns: (none)
                    check_expr: (none)
                    fk_clause: (none)
              as_select: (none)
""",
        )

    def test_named_check(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, CONSTRAINT chk CHECK (a > 0))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: CHECK
                    constraint_name: "chk"
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns: (none)
                    fk_columns: (none)
                    check_expr:
                      BinaryExpr
                        op: GT
                        left:
                          ColumnRef
                            column: "a"
                            table: (none)
                            schema: (none)
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "0"
                    fk_clause: (none)
              as_select: (none)
""",
        )

    def test_named_fk(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, CONSTRAINT fk FOREIGN KEY (a) REFERENCES b(id))",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
              table_constraints:
                TableConstraintList [1 items]
                  TableConstraint
                    kind: FOREIGN_KEY
                    constraint_name: "fk"
                    onconf: DEFAULT
                    is_autoincrement: FALSE
                    pk_columns: (none)
                    fk_columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "a"
                          table: (none)
                          schema: (none)
                    check_expr: (none)
                    fk_clause:
                      ForeignKeyClause
                        ref_table: "b"
                        ref_columns:
                          ExprList [1 items]
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                        match_name: (none)
                        on_delete: UNSET
                        on_update: UNSET
                        on_insert: UNSET
                        deferrable: UNSET
                        initial_defer: UNSET
              as_select: (none)
""",
        )


class GeneratedColumn(TestSuite):
    """GENERATED ALWAYS AS column constraint."""

    def test_generated_always_as_stored(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b GENERATED ALWAYS AS (a * 2) STORED)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "b"
                    type_name: "GENERATED ALWAYS"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: GENERATED
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: STORED
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr:
                            BinaryExpr
                              op: STAR
                              left:
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                              right:
                                Literal
                                  literal_type: INTEGER
                                  source: "2"
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )


class ColumnConstraintNull(TestSuite):
    """Column constraint NULL kind."""

    def test_column_null(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT NULL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [1 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "a"
                    type_name: "INT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: NULL
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )
