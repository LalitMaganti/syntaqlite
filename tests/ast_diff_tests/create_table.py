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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: null
  as_select: null
""",
        )

    def test_multiple_columns(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b TEXT, c REAL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[3]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "TEXT"
      constraints: null
    ColumnDef
      column_name: "c"
      type_name: "REAL"
      constraints: null
  table_constraints: null
  as_select: null
""",
        )

    def test_no_type(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a, b, c)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[3]
    ColumnDef
      column_name: "a"
      type_name: null
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: null
      constraints: null
    ColumnDef
      column_name: "c"
      type_name: null
      constraints: null
  table_constraints: null
  as_select: null
""",
        )

    def test_compound_type(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a VARCHAR(255), b DECIMAL(10, 2))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "VARCHAR(255)"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "DECIMAL(10, 2)"
      constraints: null
  table_constraints: null
  as_select: null
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
  is_temp: TRUE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: null
  as_select: null
""",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE TABLE IF NOT EXISTS t(a INT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: TRUE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: null
  as_select: null
""",
        )

    def test_schema_prefix(self):
        return AstTestBlueprint(
            sql="CREATE TABLE main.t(a INT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: "main"
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: WITHOUT_ROWID
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_strict(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT) STRICT",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: STRICT
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: null
  as_select: null
""",
        )

    def test_without_rowid_strict(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY) WITHOUT ROWID, STRICT",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: WITHOUT_ROWID STRICT
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: null
  table_constraints: null
  as_select: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
        expr: null
    from_clause: TableRef
      table_name: "t1"
      schema: null
      alias: null
    where: null
    groupby: null
    having: null
    orderby: null
    limit_clause: null
    window_clause: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: Literal
            literal_type: INTEGER
            source: "42"
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_default_string(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT DEFAULT 'hello')",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: Literal
            literal_type: STRING
            source: "'hello'"
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_default_negative(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT -1)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: UnaryExpr
            op: MINUS
            operand: Literal
              literal_type: INTEGER
              source: "1"
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_default_expr(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT (1 + 2))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: BinaryExpr
            op: PLUS
            left: Literal
              literal_type: INTEGER
              source: "1"
            right: Literal
              literal_type: INTEGER
              source: "2"
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_default_true(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT DEFAULT TRUE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: DEFAULT
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: Literal
            literal_type: STRING
            source: "TRUE"
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_primary_key_autoincrement(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INTEGER PRIMARY KEY AUTOINCREMENT)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INTEGER"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: TRUE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_primary_key_desc(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT PRIMARY KEY DESC)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: PRIMARY_KEY
          constraint_name: null
          onconf: DEFAULT
          sort_order: DESC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_not_null(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT NOT NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: NOT_NULL
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_unique(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a TEXT UNIQUE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: UNIQUE
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: CHECK
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: BinaryExpr
            op: GT
            left: ColumnRef
              column: "a"
              table: null
              schema: null
            right: Literal
              literal_type: INTEGER
              source: "0"
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: REFERENCES
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: ForeignKeyClause
            ref_table: "other"
            ref_columns: ExprList[1]
              ColumnRef
                column: "id"
                table: null
                schema: null
            on_delete: NO_ACTION
            on_update: NO_ACTION
            is_deferred: FALSE
  table_constraints: null
  as_select: null
""",
        )

    def test_references_on_delete_cascade(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id) ON DELETE CASCADE)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: REFERENCES
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: ForeignKeyClause
            ref_table: "other"
            ref_columns: ExprList[1]
              ColumnRef
                column: "id"
                table: null
                schema: null
            on_delete: CASCADE
            on_update: NO_ACTION
            is_deferred: FALSE
  table_constraints: null
  as_select: null
""",
        )

    def test_references_on_update_set_null(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT REFERENCES other(id) ON UPDATE SET NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: REFERENCES
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: ForeignKeyClause
            ref_table: "other"
            ref_columns: ExprList[1]
              ColumnRef
                column: "id"
                table: null
                schema: null
            on_delete: NO_ACTION
            on_update: SET_NULL
            is_deferred: FALSE
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "TEXT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: COLLATE
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: "NOCASE"
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: GENERATED
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: STORED
          default_expr: null
          check_expr: null
          generated_expr: BinaryExpr
            op: STAR
            left: ColumnRef
              column: "a"
              table: null
              schema: null
            right: Literal
              literal_type: INTEGER
              source: "2"
          fk_clause: null
  table_constraints: null
  as_select: null
""",
        )

    def test_generated_virtual(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, b INT AS (a + 1))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: GENERATED
          constraint_name: null
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: BinaryExpr
            op: PLUS
            left: ColumnRef
              column: "a"
              table: null
              schema: null
            right: Literal
              literal_type: INTEGER
              source: "1"
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: ColumnConstraintList[1]
        ColumnConstraint
          kind: NOT_NULL
          constraint_name: "nn"
          onconf: DEFAULT
          sort_order: ASC
          is_autoincrement: FALSE
          collation_name: null
          generated_storage: VIRTUAL
          default_expr: null
          check_expr: null
          generated_expr: null
          fk_clause: null
  table_constraints: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: PRIMARY_KEY
      constraint_name: null
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: OrderByList[2]
        OrderingTerm
          expr: ColumnRef
            column: "a"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
        OrderingTerm
          expr: ColumnRef
            column: "b"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
      check_expr: null
      fk_clause: null
  as_select: null
""",
        )

    def test_named_table_pk(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, CONSTRAINT pk PRIMARY KEY(a))",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: PRIMARY_KEY
      constraint_name: "pk"
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: OrderByList[1]
        OrderingTerm
          expr: ColumnRef
            column: "a"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
      check_expr: null
      fk_clause: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: UNIQUE
      constraint_name: null
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: OrderByList[2]
        OrderingTerm
          expr: ColumnRef
            column: "a"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
        OrderingTerm
          expr: ColumnRef
            column: "b"
            table: null
            schema: null
          sort_order: ASC
          nulls_order: NONE
      check_expr: null
      fk_clause: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[2]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
    ColumnDef
      column_name: "b"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: CHECK
      constraint_name: null
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: null
      check_expr: BinaryExpr
        op: GT
        left: ColumnRef
          column: "a"
          table: null
          schema: null
        right: ColumnRef
          column: "b"
          table: null
          schema: null
      fk_clause: null
  as_select: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: FOREIGN_KEY
      constraint_name: null
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: ExprList[1]
        ColumnRef
          column: "a"
          table: null
          schema: null
      check_expr: null
      fk_clause: ForeignKeyClause
        ref_table: "other"
        ref_columns: ExprList[1]
          ColumnRef
            column: "id"
            table: null
            schema: null
        on_delete: NO_ACTION
        on_update: NO_ACTION
        is_deferred: FALSE
  as_select: null
""",
        )

    def test_table_fk_with_actions(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id) ON DELETE CASCADE ON UPDATE SET NULL)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: FOREIGN_KEY
      constraint_name: null
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: ExprList[1]
        ColumnRef
          column: "a"
          table: null
          schema: null
      check_expr: null
      fk_clause: ForeignKeyClause
        ref_table: "other"
        ref_columns: ExprList[1]
          ColumnRef
            column: "id"
            table: null
            schema: null
        on_delete: CASCADE
        on_update: SET_NULL
        is_deferred: FALSE
  as_select: null
""",
        )

    def test_table_fk_deferred(self):
        return AstTestBlueprint(
            sql="CREATE TABLE t(a INT, FOREIGN KEY(a) REFERENCES other(id) DEFERRABLE INITIALLY DEFERRED)",
            out="""\
CreateTableStmt
  table_name: "t"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  flags: (none)
  columns: ColumnDefList[1]
    ColumnDef
      column_name: "a"
      type_name: "INT"
      constraints: null
  table_constraints: TableConstraintList[1]
    TableConstraint
      kind: FOREIGN_KEY
      constraint_name: null
      onconf: DEFAULT
      is_autoincrement: FALSE
      columns: ExprList[1]
        ColumnRef
          column: "a"
          table: null
          schema: null
      check_expr: null
      fk_clause: ForeignKeyClause
        ref_table: "other"
        ref_columns: ExprList[1]
          ColumnRef
            column: "id"
            table: null
            schema: null
        on_delete: NO_ACTION
        on_update: NO_ACTION
        is_deferred: TRUE
  as_select: null
""",
        )
