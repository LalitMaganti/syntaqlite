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
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: ISNULL
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_notnull(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOTNULL",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: NOTNULL
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_not_null(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOT NULL",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: NOTNULL
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_is_null(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NULL",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: IS
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: NULL
          source: "NULL"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_is_not_null(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NOT NULL",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: IS_NOT
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: NULL
          source: "NULL"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_is_expr(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: IS
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: INTEGER
          source: "2"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_is_not_expr(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NOT 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: IS_NOT
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: INTEGER
          source: "2"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_is_not_distinct_from(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS NOT DISTINCT FROM 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: IS_NOT_DISTINCT
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: INTEGER
          source: "2"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_is_distinct_from(self):
        return AstTestBlueprint(
            sql="SELECT 1 IS DISTINCT FROM 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: IsExpr
        op: IS_DISTINCT
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: INTEGER
          source: "2"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class BetweenExprBasic(TestSuite):
    """BETWEEN expression tests."""

    def test_between(self):
        return AstTestBlueprint(
            sql="SELECT 1 BETWEEN 0 AND 10",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BetweenExpr
        negated: FALSE
        operand: Literal
          literal_type: INTEGER
          source: "1"
        low: Literal
          literal_type: INTEGER
          source: "0"
        high: Literal
          literal_type: INTEGER
          source: "10"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_not_between(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOT BETWEEN 0 AND 10",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BetweenExpr
        negated: TRUE
        operand: Literal
          literal_type: INTEGER
          source: "1"
        low: Literal
          literal_type: INTEGER
          source: "0"
        high: Literal
          literal_type: INTEGER
          source: "10"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class LikeExprBasic(TestSuite):
    """LIKE expression tests."""

    def test_like(self):
        return AstTestBlueprint(
            sql="SELECT 'abc' LIKE 'a%'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: LikeExpr
        negated: FALSE
        operand: Literal
          literal_type: STRING
          source: "'abc'"
        pattern: Literal
          literal_type: STRING
          source: "'a%'"
        escape: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_not_like(self):
        return AstTestBlueprint(
            sql="SELECT 'abc' NOT LIKE 'a%'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: LikeExpr
        negated: TRUE
        operand: Literal
          literal_type: STRING
          source: "'abc'"
        pattern: Literal
          literal_type: STRING
          source: "'a%'"
        escape: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_like_escape(self):
        return AstTestBlueprint(
            sql="SELECT 'abc' LIKE 'a%' ESCAPE '\\'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: LikeExpr
        negated: FALSE
        operand: Literal
          literal_type: STRING
          source: "'abc'"
        pattern: Literal
          literal_type: STRING
          source: "'a%'"
        escape: Literal
          literal_type: STRING
          source: "'\\\\'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class CaseExprBasic(TestSuite):
    """CASE expression tests."""

    def test_simple_case(self):
        return AstTestBlueprint(
            sql="SELECT CASE WHEN 1 THEN 'a' ELSE 'b' END",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CaseExpr
        operand: null
        else_expr: Literal
          literal_type: STRING
          source: "'b'"
        whens: CaseWhenList[1]
          CaseWhen
            when_expr: Literal
              literal_type: INTEGER
              source: "1"
            then_expr: Literal
              literal_type: STRING
              source: "'a'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_case_with_operand(self):
        return AstTestBlueprint(
            sql="SELECT CASE 1 WHEN 1 THEN 'yes' WHEN 2 THEN 'no' END",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CaseExpr
        operand: Literal
          literal_type: INTEGER
          source: "1"
        else_expr: null
        whens: CaseWhenList[2]
          CaseWhen
            when_expr: Literal
              literal_type: INTEGER
              source: "1"
            then_expr: Literal
              literal_type: STRING
              source: "'yes'"
          CaseWhen
            when_expr: Literal
              literal_type: INTEGER
              source: "2"
            then_expr: Literal
              literal_type: STRING
              source: "'no'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_case_no_else(self):
        return AstTestBlueprint(
            sql="SELECT CASE WHEN 1 THEN 'a' END",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CaseExpr
        operand: null
        else_expr: null
        whens: CaseWhenList[1]
          CaseWhen
            when_expr: Literal
              literal_type: INTEGER
              source: "1"
            then_expr: Literal
              literal_type: STRING
              source: "'a'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
