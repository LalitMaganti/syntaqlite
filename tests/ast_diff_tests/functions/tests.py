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
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      FunctionCall
        func_name: "random"
        flags: (none)
""",
        )

    def test_one_arg(self):
        return AstTestBlueprint(
            sql="SELECT abs(1)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      FunctionCall
        func_name: "abs"
        flags: (none)
        ExprList[1]
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_multiple_args(self):
        return AstTestBlueprint(
            sql="SELECT max(1, 2, 3)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      FunctionCall
        func_name: "max"
        flags: (none)
        ExprList[3]
          Literal
            literal_type: INTEGER
            source: "1"
          Literal
            literal_type: INTEGER
            source: "2"
          Literal
            literal_type: INTEGER
            source: "3"
""",
        )

    def test_count_star(self):
        return AstTestBlueprint(
            sql="SELECT count(*)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      FunctionCall
        func_name: "count"
        flags: STAR
""",
        )

    def test_distinct(self):
        return AstTestBlueprint(
            sql="SELECT count(DISTINCT 1)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      FunctionCall
        func_name: "count"
        flags: DISTINCT
        ExprList[1]
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_nested(self):
        return AstTestBlueprint(
            sql="SELECT abs(max(1, 2))",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      FunctionCall
        func_name: "abs"
        flags: (none)
        ExprList[1]
          FunctionCall
            func_name: "max"
            flags: (none)
            ExprList[2]
              Literal
                literal_type: INTEGER
                source: "1"
              Literal
                literal_type: INTEGER
                source: "2"
""",
        )
