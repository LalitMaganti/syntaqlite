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
  pragma_form: 0
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
  pragma_form: 0
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
  pragma_form: 1
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
  pragma_form: 2
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
  pragma_form: 1
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
  is_reindex: 0
""",
        )

    def test_analyze_table(self):
        return AstTestBlueprint(
            sql="ANALYZE t",
            out="""\
AnalyzeStmt
  target_name: "t"
  schema: null
  is_reindex: 0
""",
        )

    def test_analyze_with_schema(self):
        return AstTestBlueprint(
            sql="ANALYZE main.t",
            out="""\
AnalyzeStmt
  target_name: "t"
  schema: "main"
  is_reindex: 0
""",
        )

    def test_reindex_bare(self):
        return AstTestBlueprint(
            sql="REINDEX",
            out="""\
AnalyzeStmt
  target_name: null
  schema: null
  is_reindex: 1
""",
        )

    def test_reindex_table(self):
        return AstTestBlueprint(
            sql="REINDEX t",
            out="""\
AnalyzeStmt
  target_name: "t"
  schema: null
  is_reindex: 1
""",
        )


class AttachDetachStmts(TestSuite):
    """ATTACH and DETACH statement tests."""

    def test_attach(self):
        return AstTestBlueprint(
            sql="ATTACH 'file.db' AS db2",
            out="""\
AttachStmt
  Literal
    literal_type: STRING
    source: "'file.db'"
  ColumnRef
    column: "db2"
    table: null
    schema: null
""",
        )

    def test_attach_database(self):
        return AstTestBlueprint(
            sql="ATTACH DATABASE 'file.db' AS db2",
            out="""\
AttachStmt
  Literal
    literal_type: STRING
    source: "'file.db'"
  ColumnRef
    column: "db2"
    table: null
    schema: null
""",
        )

    def test_detach(self):
        return AstTestBlueprint(
            sql="DETACH db2",
            out="""\
DetachStmt
  ColumnRef
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
  ColumnRef
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
""",
        )

    def test_vacuum_into(self):
        return AstTestBlueprint(
            sql="VACUUM INTO 'backup.db'",
            out="""\
VacuumStmt
  schema: null
  Literal
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
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_explain_query_plan(self):
        return AstTestBlueprint(
            sql="EXPLAIN QUERY PLAN SELECT * FROM t",
            out="""\
ExplainStmt
  explain_mode: QUERY_PLAN
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
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
  is_unique: 0
  if_not_exists: 0
  OrderByList[1]
    OrderingTerm
      ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
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
  is_unique: 1
  if_not_exists: 0
  OrderByList[1]
    OrderingTerm
      ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
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
  is_unique: 0
  if_not_exists: 1
  OrderByList[1]
    OrderingTerm
      ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
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
  is_unique: 0
  if_not_exists: 0
  OrderByList[1]
    OrderingTerm
      ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
  BinaryExpr
    op: GT
    ColumnRef
      column: "x"
      table: null
      schema: null
    Literal
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
  is_unique: 0
  if_not_exists: 0
  OrderByList[1]
    OrderingTerm
      ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
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
  is_unique: 0
  if_not_exists: 0
  OrderByList[2]
    OrderingTerm
      ColumnRef
        column: "x"
        table: null
        schema: null
      sort_order: ASC
      nulls_order: NONE
    OrderingTerm
      ColumnRef
        column: "y"
        table: null
        schema: null
      sort_order: DESC
      nulls_order: NONE
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
  is_temp: 0
  if_not_exists: 0
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )

    def test_create_temp_view(self):
        return AstTestBlueprint(
            sql="CREATE TEMP VIEW v AS SELECT * FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: 1
  if_not_exists: 0
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )

    def test_create_view_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE VIEW IF NOT EXISTS v AS SELECT * FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: 0
  if_not_exists: 1
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )

    def test_create_view_with_columns(self):
        return AstTestBlueprint(
            sql="CREATE VIEW v(a, b) AS SELECT x, y FROM t",
            out="""\
CreateViewStmt
  view_name: "v"
  schema: null
  is_temp: 0
  if_not_exists: 0
  ExprList[2]
    ColumnRef
      column: "a"
      table: null
      schema: null
    ColumnRef
      column: "b"
      table: null
      schema: null
  SelectStmt
    flags: (none)
    ResultColumnList[2]
      ResultColumn
        flags: (none)
        alias: null
        ColumnRef
          column: "x"
          table: null
          schema: null
      ResultColumn
        flags: (none)
        alias: null
        ColumnRef
          column: "y"
          table: null
          schema: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )
