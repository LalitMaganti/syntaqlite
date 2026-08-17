# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation integration tests for SQLite full (self-contained) mode.

Verifies that the full amalgamation (runtime inlined into dialect) compiles
and parses correctly without any external dependencies.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SqliteAmalgFull(TestSuite):
    """SQLite parsing through full self-contained amalgamation."""

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
                  index_hint: DEFAULT
                  index_name: (none)
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
