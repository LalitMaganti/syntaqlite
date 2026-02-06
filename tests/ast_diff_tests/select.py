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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
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

    def test_binary_eq(self):
        return AstTestBlueprint(
            sql="SELECT 1 = 2",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      BinaryExpr
        op: EQ
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_binary_and(self):
        return AstTestBlueprint(
            sql="SELECT 1 AND 0",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      BinaryExpr
        op: AND
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "0"
""",
        )

    def test_binary_or(self):
        return AstTestBlueprint(
            sql="SELECT 1 OR 0",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      BinaryExpr
        op: OR
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "0"
""",
        )

    def test_binary_concat(self):
        return AstTestBlueprint(
            sql="SELECT 'a' || 'b'",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      BinaryExpr
        op: CONCAT
        Literal
          literal_type: STRING
          source: "'a'"
        Literal
          literal_type: STRING
          source: "'b'"
""",
        )

    def test_unary_minus(self):
        return AstTestBlueprint(
            sql="SELECT -5",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      UnaryExpr
        op: MINUS
        Literal
          literal_type: INTEGER
          source: "5"
""",
        )

    def test_unary_not(self):
        return AstTestBlueprint(
            sql="SELECT NOT 1",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      UnaryExpr
        op: NOT
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )
