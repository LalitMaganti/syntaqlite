# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Compound SELECT, subquery, and IN expression AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class CompoundSelect(TestSuite):
    """Compound SELECT statement tests."""

    def test_union(self):
        return AstTestBlueprint(
            sql="SELECT 1 UNION SELECT 2",
            out="""\
CompoundSelect
  op: UNION
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "1"
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_union_all(self):
        return AstTestBlueprint(
            sql="SELECT 1 UNION ALL SELECT 2",
            out="""\
CompoundSelect
  op: UNION_ALL
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "1"
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_intersect(self):
        return AstTestBlueprint(
            sql="SELECT 1 INTERSECT SELECT 2",
            out="""\
CompoundSelect
  op: INTERSECT
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "1"
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_except(self):
        return AstTestBlueprint(
            sql="SELECT 1 EXCEPT SELECT 2",
            out="""\
CompoundSelect
  op: EXCEPT
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "1"
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )


class SubqueryExpr(TestSuite):
    """Subquery expression tests."""

    def test_scalar_subquery(self):
        return AstTestBlueprint(
            sql="SELECT (SELECT 1)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      SubqueryExpr
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

    def test_exists(self):
        return AstTestBlueprint(
            sql="SELECT EXISTS (SELECT 1)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      ExistsExpr
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


class InExpr(TestSuite):
    """IN expression tests."""

    def test_in_list(self):
        return AstTestBlueprint(
            sql="SELECT 1 IN (1, 2, 3)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      InExpr
        negated: 0
        Literal
          literal_type: INTEGER
          source: "1"
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

    def test_not_in_list(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOT IN (1, 2, 3)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      InExpr
        negated: 1
        Literal
          literal_type: INTEGER
          source: "1"
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

    def test_in_subquery(self):
        return AstTestBlueprint(
            sql="SELECT 1 IN (SELECT 1)",
            out="""\
SelectStmt
  flags: (none)
  ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      InExpr
        negated: 0
        Literal
          literal_type: INTEGER
          source: "1"
        SubqueryExpr
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
