# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation integration tests for base SQLite dialect.

These tests verify that the amalgamated syntaqlite_sqlite.{h,c} compiles
and produces correct AST output — the same as the non-amalgamated CLI.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SqliteAmalgBasic(TestSuite):
    """Basic parsing through amalgamated sqlite dialect."""

    def test_select_literal(self):
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

    def test_select_from_where(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM t WHERE x = 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "a"
                        table: (none)
                        schema: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              where_clause:
                BinaryExpr
                  op: EQ
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
""",
        )

    def test_create_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT NOT NULL)",
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
                        source: "id"
                    type_name: "INTEGER"
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
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "name"
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
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )

    def test_multiple_statements(self):
        return DiffTestBlueprint(
            sql="SELECT 1; SELECT 2",
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
            ----
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
                        source: "2"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
