# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Miscellaneous expression AST tests: variables, COLLATE, CTIME_KW."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class BindParameters(TestSuite):
    """Bind parameter (VARIABLE) tests."""

    def test_question_mark(self):
        return AstTestBlueprint(
            sql="SELECT ?",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Variable
        source: "?"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_numbered_parameter(self):
        return AstTestBlueprint(
            sql="SELECT ?1",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Variable
        source: "?1"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_named_colon(self):
        return AstTestBlueprint(
            sql="SELECT :name",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Variable
        source: ":name"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_named_at(self):
        return AstTestBlueprint(
            sql="SELECT @name",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Variable
        source: "@name"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_named_dollar(self):
        return AstTestBlueprint(
            sql="SELECT $name",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Variable
        source: "$name"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class CollateExpressions(TestSuite):
    """COLLATE expression tests."""

    def test_collate_nocase(self):
        return AstTestBlueprint(
            sql="SELECT 1 COLLATE NOCASE",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CollateExpr
        expr: Literal
          literal_type: INTEGER
          source: "1"
        collation: "NOCASE"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_collate_binary(self):
        return AstTestBlueprint(
            sql="SELECT 'hello' COLLATE BINARY",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CollateExpr
        expr: Literal
          literal_type: STRING
          source: "'hello'"
        collation: "BINARY"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class DateTimeKeywords(TestSuite):
    """CTIME_KW (CURRENT_TIME, CURRENT_DATE, CURRENT_TIMESTAMP) tests."""

    def test_current_time(self):
        return AstTestBlueprint(
            sql="SELECT CURRENT_TIME",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: CURRENT
        source: "CURRENT_TIME"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_current_date(self):
        return AstTestBlueprint(
            sql="SELECT CURRENT_DATE",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: CURRENT
        source: "CURRENT_DATE"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_current_timestamp(self):
        return AstTestBlueprint(
            sql="SELECT CURRENT_TIMESTAMP",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: CURRENT
        source: "CURRENT_TIMESTAMP"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
