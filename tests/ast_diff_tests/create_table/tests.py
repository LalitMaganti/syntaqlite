# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TABLE AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class CreateTableBasic(TestSuite):
    """Basic CREATE TABLE tests."""

    def test_simple_one_column(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
""",
        )

    def test_multiple_columns(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b TEXT, c REAL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[3]
    ColumnDef
      column_name: "a"
      type_name: "INT"
    ColumnDef
      column_name: "b"
      type_name: "TEXT"
    ColumnDef
      column_name: "c"
      type_name: "REAL"
""",
        )

    def test_no_type(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a, b, c)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[3]
    ColumnDef
      column_name: "a"
      type_name: null
    ColumnDef
      column_name: "b"
      type_name: null
    ColumnDef
      column_name: "c"
      type_name: null
""",
        )

    def test_compound_type(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a VARCHAR(255), b DECIMAL(10, 2))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "VARCHAR(255)"
    ColumnDef
      column_name: "b"
      type_name: "DECIMAL(10, 2)"
""",
        )


class CreateTableModifiers(TestSuite):
    """CREATE TABLE with TEMP, IF NOT EXISTS, schema prefix."""

    def test_temp(self):
        return AstTestBlueprint(
            sql="CREATE TEMP TABLE t(a INT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 1
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
""",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE TABLE IF NOT EXISTS t(a INT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 1
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
""",
        )

    def test_schema_prefix(self):
        return AstTestBlueprint(
            sql="CREATE TABLE main.t(a INT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: "main"
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
""",
        )


class CreateTableOptions(TestSuite):
    """Table options: WITHOUT ROWID, STRICT."""

    def test_without_rowid(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY) WITHOUT ROWID",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 1
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )

    def test_strict(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT) STRICT",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 2
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
""",
        )

    def test_without_rowid_strict(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY) WITHOUT ROWID, STRICT",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 3
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )


class CreateTableAsSelect(TestSuite):
    """CREATE TABLE AS SELECT."""

    def test_as_select(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t2 AS SELECT * FROM t1",
            out="""\
CreateTableStmt
  table_name: "t2"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 1
        alias: null
    TableRef
      table_name: "t1"
      schema: null
      alias: null
""",
        )


class ColumnConstraintDefault(TestSuite):
    """Column DEFAULT constraint tests."""

    def test_default_integer(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT 42)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          Literal
            literal_type: INTEGER
            source: "42"
""",
        )

    def test_default_string(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT DEFAULT 'hello')",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          Literal
            literal_type: STRING
            source: "'hello'"
""",
        )

    def test_default_negative(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT -1)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          UnaryExpr
            op: MINUS
            Literal
              literal_type: INTEGER
              source: "1"
""",
        )

    def test_default_expr(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT (1 + 2))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          BinaryExpr
            op: PLUS
            Literal
              literal_type: INTEGER
              source: "1"
            Literal
              literal_type: INTEGER
              source: "2"
""",
        )

    def test_default_true(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT TRUE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          Literal
            literal_type: STRING
            source: "TRUE"
""",
        )


class ColumnConstraintKeys(TestSuite):
    """PRIMARY KEY, UNIQUE, NOT NULL column constraints."""

    def test_primary_key(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )

    def test_primary_key_autoincrement(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INTEGER PRIMARY KEY AUTOINCREMENT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INTEGER"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 1
          collation_name: null
          generated_storage: VIRTUAL
""",
        )

    def test_primary_key_desc(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY DESC)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: 0
          sort_order: 1
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )

    def test_not_null(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT NOT NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: NOT_NULL
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )

    def test_unique(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT UNIQUE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: UNIQUE
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )


class ColumnConstraintCheck(TestSuite):
    """CHECK column constraint."""

    def test_check(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT CHECK(a > 0))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: CHECK
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          BinaryExpr
            op: GT
            ColumnRef
              column: "a"
              table: null
              schema: null
            Literal
              literal_type: INTEGER
              source: "0"
""",
        )


class ColumnConstraintReferences(TestSuite):
    """REFERENCES (foreign key) column constraint."""

    def test_references_simple(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: REFERENCES
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          ForeignKeyClause
            ref_table: "other"
            ExprList[1]
              ColumnRef
                column: "id"
                table: null
                schema: null
            on_delete: NO_ACTION
            on_update: NO_ACTION
            is_deferred: 0
""",
        )

    def test_references_on_delete_cascade(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id) ON DELETE CASCADE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: REFERENCES
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          ForeignKeyClause
            ref_table: "other"
            ExprList[1]
              ColumnRef
                column: "id"
                table: null
                schema: null
            on_delete: CASCADE
            on_update: NO_ACTION
            is_deferred: 0
""",
        )

    def test_references_on_update_set_null(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id) ON UPDATE SET NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: REFERENCES
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          ForeignKeyClause
            ref_table: "other"
            ExprList[1]
              ColumnRef
                column: "id"
                table: null
                schema: null
            on_delete: NO_ACTION
            on_update: SET_NULL
            is_deferred: 0
""",
        )


class ColumnConstraintCollate(TestSuite):
    """COLLATE column constraint."""

    def test_collate(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT COLLATE NOCASE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: COLLATE
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: "NOCASE"
          generated_storage: VIRTUAL
""",
        )


class ColumnConstraintGenerated(TestSuite):
    """Generated column constraints."""

    def test_generated_always_stored(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT AS (a * 2) STORED)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
    ColumnDef
      column_name: "b"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: GENERATED
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: STORED
          BinaryExpr
            op: STAR
            ColumnRef
              column: "a"
              table: null
              schema: null
            Literal
              literal_type: INTEGER
              source: "2"
""",
        )

    def test_generated_virtual(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT AS (a + 1))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
    ColumnDef
      column_name: "b"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: GENERATED
          constraint_name: null
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
          BinaryExpr
            op: PLUS
            ColumnRef
              column: "a"
              table: null
              schema: null
            Literal
              literal_type: INTEGER
              source: "1"
""",
        )


class ColumnConstraintName(TestSuite):
    """Named column constraints."""

    def test_named_not_null(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT CONSTRAINT nn NOT NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      ColumnConstraintList[1]
        ColumnConstraint
          kind: NOT_NULL
          constraint_name: "nn"
          onconf: 0
          sort_order: 0
          is_autoincrement: 0
          collation_name: null
          generated_storage: VIRTUAL
""",
        )


class TableConstraintPrimaryKey(TestSuite):
    """Table-level PRIMARY KEY constraint."""

    def test_table_pk(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT, PRIMARY KEY(a, b))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
    ColumnDef
      column_name: "b"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: PRIMARY_KEY
      constraint_name: null
      onconf: 0
      is_autoincrement: 0
      OrderByList[2]
        OrderingTerm
          ColumnRef
            column: "a"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
        OrderingTerm
          ColumnRef
            column: "b"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
""",
        )

    def test_named_table_pk(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, CONSTRAINT pk PRIMARY KEY(a))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: PRIMARY_KEY
      constraint_name: "pk"
      onconf: 0
      is_autoincrement: 0
      OrderByList[1]
        OrderingTerm
          ColumnRef
            column: "a"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
""",
        )


class TableConstraintUnique(TestSuite):
    """Table-level UNIQUE constraint."""

    def test_table_unique(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT, UNIQUE(a, b))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
    ColumnDef
      column_name: "b"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: UNIQUE
      constraint_name: null
      onconf: 0
      is_autoincrement: 0
      OrderByList[2]
        OrderingTerm
          ColumnRef
            column: "a"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
        OrderingTerm
          ColumnRef
            column: "b"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
""",
        )


class TableConstraintCheck(TestSuite):
    """Table-level CHECK constraint."""

    def test_table_check(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT, CHECK(a > b))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
    ColumnDef
      column_name: "b"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: CHECK
      constraint_name: null
      onconf: 0
      is_autoincrement: 0
      BinaryExpr
        op: GT
        ColumnRef
          column: "a"
          table: null
          schema: null
        ColumnRef
          column: "b"
          table: null
          schema: null
""",
        )


class TableConstraintForeignKey(TestSuite):
    """Table-level FOREIGN KEY constraint."""

    def test_table_fk(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: FOREIGN_KEY
      constraint_name: null
      onconf: 0
      is_autoincrement: 0
      ExprList[1]
        ColumnRef
          column: "a"
          table: null
          schema: null
      ForeignKeyClause
        ref_table: "other"
        ExprList[1]
          ColumnRef
            column: "id"
            table: null
            schema: null
        on_delete: NO_ACTION
        on_update: NO_ACTION
        is_deferred: 0
""",
        )

    def test_table_fk_with_actions(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id) ON DELETE CASCADE ON UPDATE SET NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: FOREIGN_KEY
      constraint_name: null
      onconf: 0
      is_autoincrement: 0
      ExprList[1]
        ColumnRef
          column: "a"
          table: null
          schema: null
      ForeignKeyClause
        ref_table: "other"
        ExprList[1]
          ColumnRef
            column: "id"
            table: null
            schema: null
        on_delete: CASCADE
        on_update: SET_NULL
        is_deferred: 0
""",
        )

    def test_table_fk_deferred(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id) DEFERRABLE INITIALLY DEFERRED)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: 0
  if_not_exists: 0
  table_options: 0
  ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
  TableConstraintList[1]
    TableConstraint
      kind: FOREIGN_KEY
      constraint_name: null
      onconf: 0
      is_autoincrement: 0
      ExprList[1]
        ColumnRef
          column: "a"
          table: null
          schema: null
      ForeignKeyClause
        ref_table: "other"
        ExprList[1]
          ColumnRef
            column: "id"
            table: null
            schema: null
        on_delete: NO_ACTION
        on_update: NO_ACTION
        is_deferred: 1
""",
        )
