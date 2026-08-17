# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Utility statement AST tests (PRAGMA, ANALYZE, ATTACH, DETACH, VACUUM, REINDEX, EXPLAIN, CREATE INDEX, CREATE VIEW)."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class PragmaStmts(TestSuite):
    """PRAGMA statement tests."""

    def test_pragma_bare(self):
        return DiffTestBlueprint(
            sql="PRAGMA journal_mode",
            out="""\
            PragmaStmt
              pragma_name: "journal_mode"
              schema: (none)
              value: (none)
              pragma_form: BARE
""",
        )

    def test_pragma_with_schema(self):
        return DiffTestBlueprint(
            sql="PRAGMA main.journal_mode",
            out="""\
            PragmaStmt
              pragma_name: "journal_mode"
              schema: "main"
              value: (none)
              pragma_form: BARE
""",
        )

    def test_pragma_eq_value(self):
        return DiffTestBlueprint(
            sql="PRAGMA journal_mode = wal",
            out="""\
            PragmaStmt
              pragma_name: "journal_mode"
              schema: (none)
              value: "wal"
              pragma_form: EQ
""",
        )

    def test_pragma_function_form(self):
        return DiffTestBlueprint(
            sql="PRAGMA table_info(t)",
            out="""\
            PragmaStmt
              pragma_name: "table_info"
              schema: (none)
              value: "t"
              pragma_form: CALL
""",
        )

    def test_pragma_negative_value(self):
        return DiffTestBlueprint(
            sql="PRAGMA cache_size = -2000",
            out="""\
            PragmaStmt
              pragma_name: "cache_size"
              schema: (none)
              value: "-2000"
              pragma_form: EQ
""",
        )


class AnalyzeReindexStmts(TestSuite):
    """ANALYZE and REINDEX statement tests."""

    def test_analyze_bare(self):
        return DiffTestBlueprint(
            sql="ANALYZE",
            out="""\
            AnalyzeOrReindexStmt
              target_name: (none)
              schema: (none)
              kind: ANALYZE
""",
        )

    def test_analyze_table(self):
        return DiffTestBlueprint(
            sql="ANALYZE t",
            out="""\
            AnalyzeOrReindexStmt
              target_name: "t"
              schema: (none)
              kind: ANALYZE
""",
        )

    def test_analyze_with_schema(self):
        return DiffTestBlueprint(
            sql="ANALYZE main.t",
            out="""\
            AnalyzeOrReindexStmt
              target_name: "t"
              schema: "main"
              kind: ANALYZE
""",
        )

    def test_reindex_bare(self):
        return DiffTestBlueprint(
            sql="REINDEX",
            out="""\
            AnalyzeOrReindexStmt
              target_name: (none)
              schema: (none)
              kind: REINDEX
""",
        )

    def test_reindex_table(self):
        return DiffTestBlueprint(
            sql="REINDEX t",
            out="""\
            AnalyzeOrReindexStmt
              target_name: "t"
              schema: (none)
              kind: REINDEX
""",
        )

    def test_reindex_schema_qualified(self):
        return DiffTestBlueprint(
            sql="REINDEX main.t",
            out="""\
            AnalyzeOrReindexStmt
              target_name: "t"
              schema: "main"
              kind: REINDEX
""",
        )


class AttachDetachStmts(TestSuite):
    """ATTACH and DETACH statement tests."""

    def test_attach(self):
        return DiffTestBlueprint(
            sql="ATTACH 'file.db' AS db2",
            out="""\
            AttachStmt
              filename:
                Literal
                  literal_type: STRING
                  source: "'file.db'"
              db_name:
                ColumnRef
                  column: "db2"
                  table: (none)
                  schema: (none)
              key: (none)
""",
        )

    def test_attach_database(self):
        return DiffTestBlueprint(
            sql="ATTACH DATABASE 'file.db' AS db2",
            out="""\
            AttachStmt
              filename:
                Literal
                  literal_type: STRING
                  source: "'file.db'"
              db_name:
                ColumnRef
                  column: "db2"
                  table: (none)
                  schema: (none)
              key: (none)
""",
        )

    def test_detach(self):
        return DiffTestBlueprint(
            sql="DETACH db2",
            out="""\
            DetachStmt
              db_name:
                ColumnRef
                  column: "db2"
                  table: (none)
                  schema: (none)
""",
        )

    def test_detach_database(self):
        return DiffTestBlueprint(
            sql="DETACH DATABASE db2",
            out="""\
            DetachStmt
              db_name:
                ColumnRef
                  column: "db2"
                  table: (none)
                  schema: (none)
""",
        )


class VacuumStmts(TestSuite):
    """VACUUM statement tests."""

    def test_vacuum_bare(self):
        return DiffTestBlueprint(
            sql="VACUUM",
            out="""\
            VacuumStmt
              schema: (none)
              filename: (none)
""",
        )

    def test_vacuum_into(self):
        return DiffTestBlueprint(
            sql="VACUUM INTO 'backup.db'",
            out="""\
            VacuumStmt
              schema: (none)
              filename:
                Literal
                  literal_type: STRING
                  source: "'backup.db'"
""",
        )

    def test_vacuum_schema(self):
        return DiffTestBlueprint(
            sql="VACUUM main",
            out="""\
            VacuumStmt
              schema: "main"
              filename: (none)
""",
        )

    def test_vacuum_schema_into(self):
        return DiffTestBlueprint(
            sql="VACUUM main INTO 'backup.db'",
            out="""\
            VacuumStmt
              schema: "main"
              filename:
                Literal
                  literal_type: STRING
                  source: "'backup.db'"
""",
        )


class ExplainStmts(TestSuite):
    """EXPLAIN statement tests."""

    def test_explain(self):
        return DiffTestBlueprint(
            sql="EXPLAIN SELECT 1",
            out="""\
            ExplainStmt
              explain_mode: EXPLAIN
              stmt:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: (none)
                        alias: (none)
                        expr:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                  from_clause: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_explain_delete(self):
        return DiffTestBlueprint(
            sql="EXPLAIN DELETE FROM t",
            out="""\
            ExplainStmt
              explain_mode: EXPLAIN
              stmt:
                DeleteStmt
                  with_ctes: (none)
                  with_recursive: FALSE
                  table:
                    TableRef
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  index_hint: DEFAULT
                  index_name: (none)
                  where_clause: (none)
                  orderby: (none)
                  limit_clause: (none)
                  returning: (none)
""",
        )

    def test_explain_query_plan(self):
        return DiffTestBlueprint(
            sql="EXPLAIN QUERY PLAN SELECT * FROM t",
            out="""\
            ExplainStmt
              explain_mode: QUERY_PLAN
              stmt:
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
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )


class CreateIndexStmts(TestSuite):
    """CREATE INDEX statement tests."""

    def test_create_index(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX idx ON t(x)",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: (none)
              table_name: "t"
              is_unique: FALSE
              if_not_exists: FALSE
              columns:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
                    sort_order: ASC
                    nulls_order: NONE
              where_clause: (none)
""",
        )

    def test_create_unique_index(self):
        return DiffTestBlueprint(
            sql="CREATE UNIQUE INDEX idx ON t(x)",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: (none)
              table_name: "t"
              is_unique: TRUE
              if_not_exists: FALSE
              columns:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
                    sort_order: ASC
                    nulls_order: NONE
              where_clause: (none)
""",
        )

    def test_create_index_if_not_exists(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX IF NOT EXISTS idx ON t(x)",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: (none)
              table_name: "t"
              is_unique: FALSE
              if_not_exists: TRUE
              columns:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
                    sort_order: ASC
                    nulls_order: NONE
              where_clause: (none)
""",
        )

    def test_create_index_with_where(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX idx ON t(x) WHERE x > 0",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: (none)
              table_name: "t"
              is_unique: FALSE
              if_not_exists: FALSE
              columns:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
                    sort_order: ASC
                    nulls_order: NONE
              where_clause:
                BinaryExpr
                  op: GT
                  left:
                    ColumnRef
                      column: "x"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "0"
""",
        )

    def test_create_index_with_schema(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX main.idx ON t(x)",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: "main"
              table_name: "t"
              is_unique: FALSE
              if_not_exists: FALSE
              columns:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
                    sort_order: ASC
                    nulls_order: NONE
              where_clause: (none)
""",
        )

    def test_create_index_expr_column(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX idx ON t(lower(x))",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: (none)
              table_name: "t"
              is_unique: FALSE
              if_not_exists: FALSE
              columns:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      FunctionCall
                        func_name: "lower"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause: (none)
                    sort_order: ASC
                    nulls_order: NONE
              where_clause: (none)
""",
        )

    def test_create_index_multi_column(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX idx ON t(x, y DESC)",
            out="""\
            CreateIndexStmt
              index_name: "idx"
              schema: (none)
              table_name: "t"
              is_unique: FALSE
              if_not_exists: FALSE
              columns:
                OrderByList [2 items]
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
                    sort_order: ASC
                    nulls_order: NONE
                  OrderingTerm
                    expr:
                      ColumnRef
                        column: "y"
                        table: (none)
                        schema: (none)
                    sort_order: DESC
                    nulls_order: NONE
              where_clause: (none)
""",
        )


class CreateViewStmts(TestSuite):
    """CREATE VIEW statement tests."""

    def test_create_view(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT * FROM t",
            out="""\
            CreateViewStmt
              view_name: "v"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              column_names: (none)
              select:
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
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_create_temp_view(self):
        return DiffTestBlueprint(
            sql="CREATE TEMP VIEW v AS SELECT * FROM t",
            out="""\
            CreateViewStmt
              view_name: "v"
              schema: (none)
              is_temp: TRUE
              if_not_exists: FALSE
              column_names: (none)
              select:
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
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_create_view_if_not_exists(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW IF NOT EXISTS v AS SELECT * FROM t",
            out="""\
            CreateViewStmt
              view_name: "v"
              schema: (none)
              is_temp: FALSE
              if_not_exists: TRUE
              column_names: (none)
              select:
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
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_create_view_with_columns(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v(a, b) AS SELECT x, y FROM t",
            out="""\
            CreateViewStmt
              view_name: "v"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              column_names:
                ExprList [2 items]
                  ColumnRef
                    column: "a"
                    table: (none)
                    schema: (none)
                  ColumnRef
                    column: "b"
                    table: (none)
                    schema: (none)
              select:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [2 items]
                      ResultColumn
                        flags: (none)
                        alias: (none)
                        expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                      ResultColumn
                        flags: (none)
                        alias: (none)
                        expr:
                          ColumnRef
                            column: "y"
                            table: (none)
                            schema: (none)
                  from_clause:
                    TableRef
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )
