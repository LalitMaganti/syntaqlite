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
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "random"
        flags: 0
""",
        )

    def test_one_arg(self):
        return AstTestBlueprint(
            sql="SELECT abs(1)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "abs"
        flags: 0
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
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "max"
        flags: 0
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
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "count"
        flags: 2
""",
        )

    def test_distinct(self):
        return AstTestBlueprint(
            sql="SELECT count(DISTINCT 1)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "count"
        flags: 1
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
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "abs"
        flags: 0
        ExprList[1]
          FunctionCall
            func_name: "max"
            flags: 0
            ExprList[2]
              Literal
                literal_type: INTEGER
                source: "1"
              Literal
                literal_type: INTEGER
                source: "2"
""",
        )
