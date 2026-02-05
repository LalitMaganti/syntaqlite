# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class SelectBasic(TestSuite):
    """Basic SELECT statement tests."""

    def test_integer_literal(self):
        return AstTestBlueprint(
            sql="SELECT 1",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      Literal
        literal_type: INTEGER
        source: "1"
""",
        )

    def test_float_literal(self):
        return AstTestBlueprint(
            sql="SELECT 3.14",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      Literal
        literal_type: FLOAT
        source: "3.14"
""",
        )

    def test_string_literal(self):
        return AstTestBlueprint(
            sql="SELECT 'hello'",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      Literal
        literal_type: STRING
        source: "'hello'"
""",
        )

    def test_null_literal(self):
        return AstTestBlueprint(
            sql="SELECT NULL",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      Literal
        literal_type: NULL
        source: "NULL"
""",
        )

    def test_binary_plus(self):
        return AstTestBlueprint(
            sql="SELECT 1 + 2",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BinaryExpr
        op: PLUS
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_binary_star(self):
        return AstTestBlueprint(
            sql="SELECT 3 * 4",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BinaryExpr
        op: STAR
        Literal
          literal_type: INTEGER
          source: "3"
        Literal
          literal_type: INTEGER
          source: "4"
""",
        )

    def test_binary_lt(self):
        return AstTestBlueprint(
            sql="SELECT 1 < 2",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BinaryExpr
        op: LT
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )
