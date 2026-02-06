# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Function call expression AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class FunctionCallBasic(TestSuite):
    """Basic function call tests."""

    def test_no_args(self):
        return AstTestBlueprint(
            sql="SELECT random()",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "random"
        flags: (none)
        args: null
        filter_clause: null
        over_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_one_arg(self):
        return AstTestBlueprint(
            sql="SELECT abs(1)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "abs"
        flags: (none)
        args: ExprList[1]
          Literal
            literal_type: INTEGER
            source: "1"
        filter_clause: null
        over_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_multiple_args(self):
        return AstTestBlueprint(
            sql="SELECT max(1, 2, 3)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "max"
        flags: (none)
        args: ExprList[3]
          Literal
            literal_type: INTEGER
            source: "1"
          Literal
            literal_type: INTEGER
            source: "2"
          Literal
            literal_type: INTEGER
            source: "3"
        filter_clause: null
        over_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_count_star(self):
        return AstTestBlueprint(
            sql="SELECT count(*)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "count"
        flags: STAR
        args: null
        filter_clause: null
        over_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_distinct(self):
        return AstTestBlueprint(
            sql="SELECT count(DISTINCT 1)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "count"
        flags: DISTINCT
        args: ExprList[1]
          Literal
            literal_type: INTEGER
            source: "1"
        filter_clause: null
        over_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_nested(self):
        return AstTestBlueprint(
            sql="SELECT abs(max(1, 2))",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "abs"
        flags: (none)
        args: ExprList[1]
          FunctionCall
            func_name: "max"
            flags: (none)
            args: ExprList[2]
              Literal
                literal_type: INTEGER
                source: "1"
              Literal
                literal_type: INTEGER
                source: "2"
            filter_clause: null
            over_clause: null
        filter_clause: null
        over_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
