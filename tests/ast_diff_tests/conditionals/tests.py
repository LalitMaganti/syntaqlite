# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Conditional expression AST tests: IS, BETWEEN, LIKE, CASE."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class IsExprBasic(TestSuite):
    """IS/ISNULL/NOTNULL expression tests."""

    def test_isnull(self):
        return AstTestBlueprint(
            sql="SELECT 1 ISNULL",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: ISNULL
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_notnull(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOTNULL",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: NOTNULL
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_not_null(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOT NULL",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: NOTNULL
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_is_null(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NULL",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: IS
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: NULL
          source: "NULL"
""",
        )

    def test_is_not_null(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NOT NULL",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: IS_NOT
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: NULL
          source: "NULL"
""",
        )

    def test_is_expr(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS 2",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: IS
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_is_not_expr(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NOT 2",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: IS_NOT
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_is_not_distinct_from(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NOT DISTINCT FROM 2",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: IS_NOT_DISTINCT
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_is_distinct_from(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS DISTINCT FROM 2",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      IsExpr
        op: IS_DISTINCT
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )


class BetweenExprBasic(TestSuite):
    """BETWEEN expression tests."""

    def test_between(self):
        return AstTestBlueprint(
            sql="SELECT 1 BETWEEN 0 AND 10",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BetweenExpr
        negated: 0
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "0"
        Literal
          literal_type: INTEGER
          source: "10"
""",
        )

    def test_not_between(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOT BETWEEN 0 AND 10",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BetweenExpr
        negated: 1
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "0"
        Literal
          literal_type: INTEGER
          source: "10"
""",
        )


class LikeExprBasic(TestSuite):
    """LIKE expression tests."""

    def test_like(self):
        return AstTestBlueprint(
            sql="SELECT 'abc' LIKE 'a%'",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      LikeExpr
        negated: 0
        Literal
          literal_type: STRING
          source: "'abc'"
        Literal
          literal_type: STRING
          source: "'a%'"
""",
        )

    def test_not_like(self):
        return AstTestBlueprint(
            sql="SELECT 'abc' NOT LIKE 'a%'",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      LikeExpr
        negated: 1
        Literal
          literal_type: STRING
          source: "'abc'"
        Literal
          literal_type: STRING
          source: "'a%'"
""",
        )

    def test_like_escape(self):
        return AstTestBlueprint(
            sql="SELECT 'abc' LIKE 'a%' ESCAPE '\\'",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      LikeExpr
        negated: 0
        Literal
          literal_type: STRING
          source: "'abc'"
        Literal
          literal_type: STRING
          source: "'a%'"
        Literal
          literal_type: STRING
          source: "'\\\\'"
""",
        )


class CaseExprBasic(TestSuite):
    """CASE expression tests."""

    def test_simple_case(self):
        return AstTestBlueprint(
            sql="SELECT CASE WHEN 1 THEN 'a' ELSE 'b' END",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CaseExpr
        Literal
          literal_type: STRING
          source: "'b'"
        CaseWhenList[1]
          CaseWhen
            Literal
              literal_type: INTEGER
              source: "1"
            Literal
              literal_type: STRING
              source: "'a'"
""",
        )

    def test_case_with_operand(self):
        return AstTestBlueprint(
            sql="SELECT CASE 1 WHEN 1 THEN 'yes' WHEN 2 THEN 'no' END",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CaseExpr
        Literal
          literal_type: INTEGER
          source: "1"
        CaseWhenList[2]
          CaseWhen
            Literal
              literal_type: INTEGER
              source: "1"
            Literal
              literal_type: STRING
              source: "'yes'"
          CaseWhen
            Literal
              literal_type: INTEGER
              source: "2"
            Literal
              literal_type: STRING
              source: "'no'"
""",
        )

    def test_case_no_else(self):
        return AstTestBlueprint(
            sql="SELECT CASE WHEN 1 THEN 'a' END",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CaseExpr
        CaseWhenList[1]
          CaseWhen
            Literal
              literal_type: INTEGER
              source: "1"
            Literal
              literal_type: STRING
              source: "'a'"
""",
        )
