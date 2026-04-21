# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation tests with SYNTAQLITE_OMIT_MACROS defined.

Verifies that the parser works correctly when macro support is compiled
out — all macro-related code is absent, and basic SQL parsing still
produces the expected AST.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SqliteAmalgOmitMacros(TestSuite):
    """Parsing through amalgamated sqlite dialect with macros omitted."""

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
