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
                        alias_as: FALSE
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
                        alias_as: FALSE
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
                        alias_as: FALSE
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
                        alias_as: FALSE
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
                        alias_as: FALSE
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
                        alias_as: FALSE
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

    # -- Pipelines and node expansion --

    def test_pipeline_statement_is_not_expanded(self):
        """A pipeline on its own is a statement, not a node to expand."""
        return DiffTestBlueprint(
            sql="FROM t |> DROP a, b",
            out="""\
            PerfettoPipeline
              from:
                PerfettoPipeSource
                  table_name: "t"
                  schema: (none)
                  select: (none)
                  alias: (none)
                  alias_as: FALSE
              stages:
                PerfettoPipeStageList [1 items]
                  PerfettoPipeDrop
                    columns:
                      PerfettoPipeNameList [2 items]
                        PerfettoPipeName
                          name: "a"
                        PerfettoPipeName
                          name: "b"
""",
        )

    def test_pipeline_subquery_is_expanded(self):
        """A pipeline in a FROM clause is replaced by the expander."""
        return DiffTestBlueprint(
            sql="SELECT x.c FROM (FROM t |> DROP a) AS x",
            out="""\
            expanding "FROM t |> DROP a" in "SELECT x.c FROM (FROM t |> DROP a) AS x"
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      ColumnRef
                        column: "c"
                        table: "x"
                        schema: (none)
              from_clause:
                SubqueryTableSource
                  select:
                    PerfettoPipeline
                      from:
                        PerfettoPipeSource
                          table_name: "t"
                          schema: (none)
                          select: (none)
                          alias: (none)
                          alias_as: FALSE
                      stages:
                        PerfettoPipeStageList [1 items]
                          PerfettoPipeDrop
                            columns:
                              PerfettoPipeNameList [1 items]
                                PerfettoPipeName
                                  name: "a"
                  alias:
                    IdentName
                      source: "x"
                  alias_as: TRUE
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
            expanded pipeline in source: "FROM t |> DROP a" -> "SELECT * FROM expanded_1"
""",
        )

    def test_pipeline_cte_is_expanded(self):
        """A pipeline as a CTE is replaced by the expander."""
        return DiffTestBlueprint(
            sql="WITH p AS (FROM t |> DROP a) SELECT * FROM p",
            out="""\
            expanding "FROM t |> DROP a" in "WITH p AS (FROM t |> DROP a) SELECT * FROM p"
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "p"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      PerfettoPipeline
                        from:
                          PerfettoPipeSource
                            table_name: "t"
                            schema: (none)
                            select: (none)
                            alias: (none)
                            alias_as: FALSE
                        stages:
                          PerfettoPipeStageList [1 items]
                            PerfettoPipeDrop
                              columns:
                                PerfettoPipeNameList [1 items]
                                  PerfettoPipeName
                                    name: "a"
              select:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: STAR
                        alias: (none)
                        alias_as: FALSE
                        expr: (none)
                  from_clause:
                    TableRef
                      table_name: "p"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      alias_as: FALSE
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
            expanded pipeline in source: "FROM t |> DROP a" -> "SELECT * FROM expanded_1"
""",
        )

    def test_pipeline_source_is_expanded_inside_pipeline(self):
        """A pipeline read by another pipeline is expanded first; the outer
        expansion covers it."""
        return DiffTestBlueprint(
            sql="SELECT * FROM (FROM (FROM t |> DROP a) |> DROP b)",
            out="""\
            expanding "FROM t |> DROP a" in "SELECT * FROM (FROM (FROM t |> DROP a) |> DROP b)"
            expanding "FROM (FROM t |> DROP a) |> DROP b" in "SELECT * FROM (FROM (FROM t |> DROP a) |> DROP b)"
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    alias_as: FALSE
                    expr: (none)
              from_clause:
                SubqueryTableSource
                  select:
                    PerfettoPipeline
                      from:
                        PerfettoPipeSource
                          table_name: (none)
                          schema: (none)
                          select:
                            PerfettoPipeline
                              from:
                                PerfettoPipeSource
                                  table_name: "t"
                                  schema: (none)
                                  select: (none)
                                  alias: (none)
                                  alias_as: FALSE
                              stages:
                                PerfettoPipeStageList [1 items]
                                  PerfettoPipeDrop
                                    columns:
                                      PerfettoPipeNameList [1 items]
                                        PerfettoPipeName
                                          name: "a"
                          alias: (none)
                          alias_as: FALSE
                      stages:
                        PerfettoPipeStageList [1 items]
                          PerfettoPipeDrop
                            columns:
                              PerfettoPipeNameList [1 items]
                                PerfettoPipeName
                                  name: "b"
                  alias: (none)
                  alias_as: FALSE
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
            expanded pipeline in source: "FROM t |> DROP a" -> "SELECT * FROM expanded_1"
            expanded pipeline in source: "FROM (FROM t |> DROP a) |> DROP b" -> "SELECT * FROM expanded_2"
""",
        )

    def test_pipeline_after_join_is_expanded(self):
        """Pipelines joined to other sources are each expanded where written."""
        return DiffTestBlueprint(
            sql="SELECT * FROM t JOIN (FROM u) AS a USING (id) JOIN (FROM v) AS b USING (id)",
            out="""\
            expanding "FROM u" in "SELECT * FROM t JOIN (FROM u) AS a USING (id) JOIN (FROM v) AS b USING (id)"
            expanding "FROM v" in "SELECT * FROM t JOIN (FROM u) AS a USING (id) JOIN (FROM v) AS b USING (id)"
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    alias_as: FALSE
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: INNER
                  modifiers: (none)
                  left:
                    JoinClause
                      join_type: INNER
                      modifiers: (none)
                      left:
                        TableRef
                          table_name: "t"
                          schema: (none)
                          has_parens: FALSE
                          alias: (none)
                          alias_as: FALSE
                          args: (none)
                          index_hint: DEFAULT
                          index_name: (none)
                      right:
                        SubqueryTableSource
                          select:
                            PerfettoPipeline
                              from:
                                PerfettoPipeSource
                                  table_name: "u"
                                  schema: (none)
                                  select: (none)
                                  alias: (none)
                                  alias_as: FALSE
                              stages: (none)
                          alias:
                            IdentName
                              source: "a"
                          alias_as: TRUE
                      on_expr: (none)
                      using_columns:
                        ExprList [1 items]
                          ColumnRef
                            column: "id"
                            table: (none)
                            schema: (none)
                  right:
                    SubqueryTableSource
                      select:
                        PerfettoPipeline
                          from:
                            PerfettoPipeSource
                              table_name: "v"
                              schema: (none)
                              select: (none)
                              alias: (none)
                              alias_as: FALSE
                          stages: (none)
                      alias:
                        IdentName
                          source: "b"
                      alias_as: TRUE
                  on_expr: (none)
                  using_columns:
                    ExprList [1 items]
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
            expanded pipeline in source: "FROM u" -> "SELECT * FROM expanded_1"
            expanded pipeline in source: "FROM v" -> "SELECT * FROM expanded_2"
""",
        )

    def test_pipeline_expansion_failure_fails_the_parse(self):
        """A failed expansion fails the parse, naming what was expanded."""
        return DiffTestBlueprint(
            sql="SELECT * FROM (FROM (FROM unexpandable) |> DROP a)",
            out="""\
            expanding "FROM unexpandable" in "SELECT * FROM (FROM (FROM unexpandable) |> DROP a)"
            parse error: expanding pipeline failed
""",
        )

    def test_pipeline_expansion_sees_the_whole_statement(self):
        """Nodes are expanded once their statement is parsed, so the expander
        sees all of it, such as the query after a CTE."""
        return DiffTestBlueprint(
            sql="WITH p AS (FROM t |> DROP a) SELECT * FROM p WHERE x > 1",
            out="""\
            expanding "FROM t |> DROP a" in "WITH p AS (FROM t |> DROP a) SELECT * FROM p WHERE x > 1"
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "p"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      PerfettoPipeline
                        from:
                          PerfettoPipeSource
                            table_name: "t"
                            schema: (none)
                            select: (none)
                            alias: (none)
                            alias_as: FALSE
                        stages:
                          PerfettoPipeStageList [1 items]
                            PerfettoPipeDrop
                              columns:
                                PerfettoPipeNameList [1 items]
                                  PerfettoPipeName
                                    name: "a"
              select:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: STAR
                        alias: (none)
                        alias_as: FALSE
                        expr: (none)
                  from_clause:
                    TableRef
                      table_name: "p"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      alias_as: FALSE
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
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
                          source: "1"
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
            expanded pipeline in source: "FROM t |> DROP a" -> "SELECT * FROM expanded_1"
""",
        )

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
                    alias_as: FALSE
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
              temporary: NONE
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
