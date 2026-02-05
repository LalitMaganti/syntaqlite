# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Column reference expression AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class ColumnRefBasic(TestSuite):
    """Column reference tests."""

    def test_simple_column(self):
        return AstTestBlueprint(
            sql="SELECT x",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "x"
        table: null
        schema: null
""",
        )

    def test_qualified_column(self):
        return AstTestBlueprint(
            sql="SELECT t.x",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "x"
        table: "t"
        schema: null
""",
        )

    def test_fully_qualified_column(self):
        return AstTestBlueprint(
            sql="SELECT s.t.x",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "x"
        table: "t"
        schema: "s"
""",
        )

    def test_multiple_columns(self):
        return AstTestBlueprint(
            sql="SELECT a, b, c",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[3]
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "a"
        table: null
        schema: null
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "b"
        table: null
        schema: null
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "c"
        table: null
        schema: null
""",
        )

    def test_table_star(self):
        return AstTestBlueprint(
            sql="SELECT t.*",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 1
      alias: "t"
""",
        )

    def test_mixed_expressions(self):
        return AstTestBlueprint(
            sql="SELECT a, t.b, 1 + x",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[3]
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "a"
        table: null
        schema: null
    ResultColumn
      flags: 0
      alias: null
      ColumnRef
        column: "b"
        table: "t"
        schema: null
    ResultColumn
      flags: 0
      alias: null
      BinaryExpr
        op: PLUS
        Literal
          literal_type: INTEGER
          source: "1"
        ColumnRef
          column: "x"
          table: null
          schema: null
""",
        )
