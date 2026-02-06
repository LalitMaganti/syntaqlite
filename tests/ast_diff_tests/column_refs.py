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
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "x"
        table: null
        schema: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_qualified_column(self):
        return AstTestBlueprint(
            sql="SELECT t.x",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "x"
        table: "t"
        schema: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_fully_qualified_column(self):
        return AstTestBlueprint(
            sql="SELECT s.t.x",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "x"
        table: "t"
        schema: "s"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_multiple_columns(self):
        return AstTestBlueprint(
            sql="SELECT a, b, c",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[3]
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "a"
        table: null
        schema: null
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "b"
        table: null
        schema: null
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "c"
        table: null
        schema: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_table_star(self):
        return AstTestBlueprint(
            sql="SELECT t.*",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: "t"
      expr: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_mixed_expressions(self):
        return AstTestBlueprint(
            sql="SELECT a, t.b, 1 + x",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[3]
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "a"
        table: null
        schema: null
    ResultColumn
      flags: (none)
      alias: null
      expr: ColumnRef
        column: "b"
        table: "t"
        schema: null
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: PLUS
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: ColumnRef
          column: "x"
          table: null
          schema: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
