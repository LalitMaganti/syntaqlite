# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Utility statement AST tests (PRAGMA, ANALYZE, ATTACH, DETACH, VACUUM, REINDEX, EXPLAIN, CREATE INDEX, CREATE VIEW)."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class PragmaStmts(TestSuite):
    """PRAGMA statement tests."""

    def test_pragma_bare(self):
        return AstTestBlueprint(
            sql="PRAGMA journal_mode",
            out="""\
PragmaStmt
  pragma_name: "journal_mode"
  schema: null
  value: null
  pragma_form: BARE
""",
        )

    def test_pragma_with_schema(self):
        return AstTestBlueprint(
            sql="PRAGMA main.journal_mode",
            out="""\
PragmaStmt
  pragma_name: "journal_mode"
  schema: "main"
  value: null
  pragma_form: BARE
""",
        )

    def test_pragma_eq_value(self):
        return AstTestBlueprint(
            sql="PRAGMA journal_mode = wal",
            out="""\
PragmaStmt
  pragma_name: "journal_mode"
  schema: null
  value: "wal"
  pragma_form: EQ
""",
        )

    def test_pragma_function_form(self):
        return AstTestBlueprint(
            sql="PRAGMA table_info(t)",
            out="""\
PragmaStmt
  pragma_name: "table_info"
  schema: null
  value: "t"
  pragma_form: CALL
""",
        )

    def test_pragma_negative_value(self):
        return AstTestBlueprint(
            sql="PRAGMA cache_size = -2000",
            out="""\
PragmaStmt
  pragma_name: "cache_size"
  schema: null
  value: "-2000"
  pragma_form: EQ
""",
        )


class AnalyzeReindexStmts(TestSuite):
    """ANALYZE and REINDEX statement tests."""

    def test_analyze_bare(self):
        return AstTestBlueprint(
            sql="ANALYZE",
            out="""\
AnalyzeStmt
  target_name: null
  schema: null
  kind: ANALYZE
""",
        )

    def test_analyze_table(self):
        return AstTestBlueprint(
            sql="ANALYZE t",
            out="""\
AnalyzeStmt
  target_name: "t"
  schema: null
  kind: ANALYZE
""",
        )

    def test_analyze_with_schema(self):
        return AstTestBlueprint(
            sql="ANALYZE main.t",
            out="""\
AnalyzeStmt
  target_name: "t"
  schema: "main"
  kind: ANALYZE
""",
        )

    def test_reindex_bare(self):
        return AstTestBlueprint(
            sql="REINDEX",
            out="""\
AnalyzeStmt
  target_name: null
  schema: null
  kind: REINDEX
""",
        )

    def test_reindex_table(self):
        return AstTestBlueprint(
            sql="REINDEX t",
            out="""\
AnalyzeStmt
  target_name: "t"
  schema: null
  kind: REINDEX
""",
        )


class AttachDetachStmts(TestSuite):
    """ATTACH and DETACH statement tests."""

    def test_attach(self):
        return AstTestBlueprint(
            sql="ATTACH 'file.db' AS db2",
            out="""\
AttachStmt
  filename: Literal
    literal_type: STRING
    source: "'file.db'"
  db_name: ColumnRef
    column: "db2"
    table: null
    schema: null
  key: null
""",
        )

    def test_attach_database(self):
        return AstTestBlueprint(
            sql="ATTACH DATABASE 'file.db' AS db2",
            out="""\
AttachStmt
  filename: Literal
    literal_type: STRING
    source: "'file.db'"
  db_name: ColumnRef
    column: "db2"
    table: null
    schema: null
  key: null
""",
        )

    def test_detach(self):
        return AstTestBlueprint(
            sql="DETACH db2",
            out="""\
DetachStmt
  db_name: ColumnRef
    column: "db2"
    table: null
    schema: null
""",
        )

    def test_detach_database(self):
        return AstTestBlueprint(
            sql="DETACH DATABASE db2",
            out="""\
DetachStmt
  db_name: ColumnRef
    column: "db2"
    table: null
    schema: null
""",
        )


class VacuumStmts(TestSuite):
    """VACUUM statement tests."""

    def test_vacuum_bare(self):
        return AstTestBlueprint(
            sql="VACUUM",
            out="""\
VacuumStmt
  schema: null
  into_expr: null
""",
        )

    def test_vacuum_into(self):
        return AstTestBlueprint(
            sql="VACUUM INTO 'backup.db'",
            out="""\
VacuumStmt
  schema: null
  into_expr: Literal
    literal_type: STRING
    source: "'backup.db'"
""",
        )

    def test_vacuum_schema(self):
        return AstTestBlueprint(
            sql="VACUUM main",
            out="""\
VacuumStmt
  schema: "main"
  into_expr: null
""",
        )


class ExplainStmts(TestSuite):
    """EXPLAIN statement tests."""

    def test_explain(self):
        return AstTestBlueprint(
            sql="EXPLAIN SELECT 1",
            out="""\
ExplainStmt
  explain_mode: EXPLAIN
  stmt: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
          literal_type: INTEGER
          source: "1"
    from_clause: null
    where: null
    groupby: null
    having: null
    orderby: null
    limit_clause: null
    window_clause: null
""",
        )

    def test_explain_query_plan(self):
        return AstTestBlueprint(
            sql="EXPLAIN QUERY PLAN SELECT * FROM t",
            out="""\
ExplainStmt
  explain_mode: QUERY_PLAN
  stmt: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
        expr: null
    from_clause: TableRef
      table_name: "t"
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


class CreateIndexStmts(TestSuite):
    """CREATE INDEX statement tests."""

    def test_create_index(self):
        return AstTestBlueprint(
            sql="CREATE INDEX idx ON t(x)",
            out="""\
CreateIndexStmt
  index_name: "idx"
  schema: null
  table_name: "t"
  is_unique: FALSE
  if_not_exists: FALSE
  columns: OrderByList[1]
    OrderingTerm
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
  where: null
""",
        )

    def test_create_unique_index(self):
        return AstTestBlueprint(
            sql="CREATE UNIQUE INDEX idx ON t(x)",
            out="""\
CreateIndexStmt
  index_name: "idx"
  schema: null
  table_name: "t"
  is_unique: TRUE
  if_not_exists: FALSE
  columns: OrderByList[1]
    OrderingTerm
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
  where: null
""",
        )

    def test_create_index_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE INDEX IF NOT EXISTS idx ON t(x)",
            out="""\
CreateIndexStmt
  index_name: "idx"
  schema: null
  table_name: "t"
  is_unique: FALSE
  if_not_exists: TRUE
  columns: OrderByList[1]
    OrderingTerm
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
  where: null
""",
        )

    def test_create_index_with_where(self):
        return AstTestBlueprint(
            sql="CREATE INDEX idx ON t(x) WHERE x > 0",
            out="""\
CreateIndexStmt
  index_name: "idx"
  schema: null
  table_name: "t"
  is_unique: FALSE
  if_not_exists: FALSE
  columns: OrderByList[1]
    OrderingTerm
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
  where: BinaryExpr
    op: GT
    left: ColumnRef
      column: "x"
      table: null
      schema: null
    right: Literal
      literal_type: INTEGER
      source: "0"
""",
        )

    def test_create_index_with_schema(self):
        return AstTestBlueprint(
            sql="CREATE INDEX main.idx ON t(x)",
            out="""\
CreateIndexStmt
  index_name: "idx"
  schema: "main"
  table_name: "t"
  is_unique: FALSE
  if_not_exists: FALSE
  columns: OrderByList[1]
    OrderingTerm
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
  where: null
""",
        )

    def test_create_index_multi_column(self):
        return AstTestBlueprint(
            sql="CREATE INDEX idx ON t(x, y DESC)",
            out="""\
CreateIndexStmt
  index_name: "idx"
  schema: null
  table_name: "t"
  is_unique: FALSE
  if_not_exists: FALSE
  columns: OrderByList[2]
    OrderingTerm
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
    OrderingTerm
      expr: ColumnRef
        column: "y"
        table: null
        schema: null
      sort_order: DESC
      nulls_order: NONE
  where: null
""",
        )


class CreateViewStmts(TestSuite):
    """CREATE VIEW statement tests."""

    def test_create_view(self):
        return AstTestBlueprint(
            sql="CREATE VIEW v AS SELECT * FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  column_names: null
  select: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
        expr: null
    from_clause: TableRef
      table_name: "t"
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

    def test_create_temp_view(self):
        return AstTestBlueprint(
            sql="CREATE TEMP VIEW v AS SELECT * FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: TRUE
  if_not_exists: FALSE
  column_names: null
  select: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
        expr: null
    from_clause: TableRef
      table_name: "t"
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

    def test_create_view_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE VIEW IF NOT EXISTS v AS SELECT * FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: FALSE
  if_not_exists: TRUE
  column_names: null
  select: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
        expr: null
    from_clause: TableRef
      table_name: "t"
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

    def test_create_view_with_columns(self):
        return AstTestBlueprint(
            sql="CREATE VIEW v(a, b) AS SELECT x, y FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  column_names: ExprList[2]
    ColumnRef
      column: "a"
      table: null
      schema: null
    ColumnRef
      column: "b"
      table: null
      schema: null
  select: SelectStmt
    flags: (none)
    columns: ResultColumnList[2]
      ResultColumn
        flags: (none)
        alias: null
        expr: ColumnRef
          column: "x"
          table: null
          schema: null
      ResultColumn
        flags: (none)
        alias: null
        expr: ColumnRef
          column: "y"
          table: null
          schema: null
    from_clause: TableRef
      table_name: "t"
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
