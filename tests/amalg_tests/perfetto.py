# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation integration tests for perfetto dialect extension.

These tests verify that dialect extensions (additional .y and .synq files)
are correctly merged with the base SQLite grammar and produce a working
amalgamated parser.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class PerfettoExtension(TestSuite):
    """Tests for perfetto dialect extension syntax."""

    # -- CREATE PERFETTO TABLE --

    def test_create_perfetto_table_as_select(self):
        """CREATE PERFETTO TABLE with AS select."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO TABLE foo AS SELECT 1",
            out="""\
            CreatePerfettoTableStmt
              table_name: "foo"
              or_replace: FALSE
              schema: (none)
              select:
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
              select_span: "SELECT 1"
""",
        )

    def test_create_or_replace_perfetto_table(self):
        """CREATE OR REPLACE PERFETTO TABLE."""
        return DiffTestBlueprint(
            sql="CREATE OR REPLACE PERFETTO TABLE foo AS SELECT 1",
            out="""\
            CreatePerfettoTableStmt
              table_name: "foo"
              or_replace: TRUE
              schema: (none)
              select:
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
              select_span: "SELECT 1"
""",
        )

    # -- CREATE PERFETTO VIEW --

    def test_create_perfetto_view(self):
        """CREATE PERFETTO VIEW with AS select."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO VIEW v AS SELECT 1",
            out="""\
            CreatePerfettoViewStmt
              view_name: "v"
              or_replace: FALSE
              schema: (none)
              select:
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
              select_span: "SELECT 1"
""",
        )

    # -- CREATE PERFETTO FUNCTION --

    def test_create_perfetto_function_scalar(self):
        """CREATE PERFETTO FUNCTION returning a scalar type."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO FUNCTION f(x INT) RETURNS BOOL AS SELECT 1",
            out="""\
            CreatePerfettoFunctionStmt
              function_name: "f"
              or_replace: FALSE
              args:
                PerfettoArgDefList [1 items]
                  PerfettoArgDef
                    arg_name:
                      IdentName
                        source: "x"
                    arg_type: "INT"
                    is_variadic: FALSE
              return_type:
                PerfettoReturnType
                  kind: SCALAR
                  scalar_type: "BOOL"
                  table_columns: (none)
              select:
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
              select_span: "SELECT 1"
""",
        )

    def test_create_perfetto_function_no_args(self):
        """CREATE PERFETTO FUNCTION with no arguments."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO FUNCTION f() RETURNS INT AS SELECT 42",
            out="""\
            CreatePerfettoFunctionStmt
              function_name: "f"
              or_replace: FALSE
              args: (none)
              return_type:
                PerfettoReturnType
                  kind: SCALAR
                  scalar_type: "INT"
                  table_columns: (none)
              select:
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
                            source: "42"
                  from_clause: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
              select_span: "SELECT 42"
""",
        )

    def test_create_perfetto_table_select_span_leading_ws(self):
        """select_span must exclude leading/trailing whitespace around the
        body, demonstrating that the BEFORE marker captures the start of
        the first real token (not the end of `AS`) and the AFTER marker
        captures the end of the last real token (not the start of `;`).
        """
        return DiffTestBlueprint(
            sql="CREATE PERFETTO TABLE foo AS    SELECT 1   ",
            out="""\
            CreatePerfettoTableStmt
              table_name: "foo"
              or_replace: FALSE
              schema: (none)
              select:
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
              select_span: "SELECT 1"
""",
        )

    # -- CREATE PERFETTO INDEX --

    def test_create_perfetto_index(self):
        """CREATE PERFETTO INDEX on a single column."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO INDEX idx ON t(col)",
            out="""\
            CreatePerfettoIndexStmt
              index_name: "idx"
              or_replace: FALSE
              table_name: "t"
              columns:
                PerfettoIndexedColumnList [1 items]
                  PerfettoIndexedColumn
                    column_name: "col"
""",
        )

    def test_create_perfetto_index_multi_column(self):
        """CREATE PERFETTO INDEX on multiple columns."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO INDEX idx ON t(a, b, c)",
            out="""\
            CreatePerfettoIndexStmt
              index_name: "idx"
              or_replace: FALSE
              table_name: "t"
              columns:
                PerfettoIndexedColumnList [3 items]
                  PerfettoIndexedColumn
                    column_name: "a"
                  PerfettoIndexedColumn
                    column_name: "b"
                  PerfettoIndexedColumn
                    column_name: "c"
""",
        )

    # -- CREATE PERFETTO MACRO --

    def test_create_perfetto_macro(self):
        """CREATE PERFETTO MACRO with arguments and body."""
        return DiffTestBlueprint(
            sql="CREATE PERFETTO MACRO m(x TableOrSubquery) RETURNS TableOrSubquery AS x",
            out="""\
            CreatePerfettoMacroStmt
              macro_name: "m"
              or_replace: FALSE
              return_type: "TableOrSubquery"
              body: "x"
              args:
                PerfettoMacroArgList [1 items]
                  PerfettoMacroArg
                    arg_name: "x"
                    arg_type: "TableOrSubquery"
""",
        )

    # -- INCLUDE PERFETTO MODULE --

    def test_include_perfetto_module(self):
        """INCLUDE PERFETTO MODULE with dotted path."""
        return DiffTestBlueprint(
            sql="INCLUDE PERFETTO MODULE foo.bar",
            out="""\
            IncludePerfettoModuleStmt
              module_name: "foo.bar"
""",
        )

    def test_include_perfetto_module_simple(self):
        """INCLUDE PERFETTO MODULE single name."""
        return DiffTestBlueprint(
            sql="INCLUDE PERFETTO MODULE metrics",
            out="""\
            IncludePerfettoModuleStmt
              module_name: "metrics"
""",
        )

    # -- DROP PERFETTO INDEX --

    def test_drop_perfetto_index(self):
        """DROP PERFETTO INDEX on a table."""
        return DiffTestBlueprint(
            sql="DROP PERFETTO INDEX idx ON t",
            out="""\
            DropPerfettoIndexStmt
              index_name: "idx"
              table_name: "t"
""",
        )

    # -- Base SQLite still works --

    def test_base_select_still_works(self):
        """Base SQLite syntax must still work in an extended dialect."""
        return DiffTestBlueprint(
            sql="SELECT 1",
            out="""\
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

    def test_base_create_table_still_works(self):
        """Regular CREATE TABLE must coexist with CREATE PERFETTO TABLE."""
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER)",
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
                        source: "id"
                    type_name: "INTEGER"
                    constraints: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )
