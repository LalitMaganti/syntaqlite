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
  left: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
          literal_type: INTEGER
          source: "1"
    from_clause: null
    where: null
    groupby: null
    having: null
    orderby: null
    limit_clause: null
    window_clause: null
  right: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
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

    def test_union_all(self):
        return AstTestBlueprint(
            sql="SELECT 1 UNION ALL SELECT 2",
            out="""\
CompoundSelect
  op: UNION_ALL
  left: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
          literal_type: INTEGER
          source: "1"
    from_clause: null
    where: null
    groupby: null
    having: null
    orderby: null
    limit_clause: null
    window_clause: null
  right: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
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

    def test_intersect(self):
        return AstTestBlueprint(
            sql="SELECT 1 INTERSECT SELECT 2",
            out="""\
CompoundSelect
  op: INTERSECT
  left: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
          literal_type: INTEGER
          source: "1"
    from_clause: null
    where: null
    groupby: null
    having: null
    orderby: null
    limit_clause: null
    window_clause: null
  right: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
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

    def test_except(self):
        return AstTestBlueprint(
            sql="SELECT 1 EXCEPT SELECT 2",
            out="""\
CompoundSelect
  op: EXCEPT
  left: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
          literal_type: INTEGER
          source: "1"
    from_clause: null
    where: null
    groupby: null
    having: null
    orderby: null
    limit_clause: null
    window_clause: null
  right: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: Literal
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


class SubqueryExpr(TestSuite):
    """Subquery expression tests."""

    def test_scalar_subquery(self):
        return AstTestBlueprint(
            sql="SELECT (SELECT 1)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: SubqueryExpr
        select: SelectStmt
          flags: (none)
          columns: ResultColumnList[1]
            ResultColumn
              flags: (none)
              alias: null
              expr: Literal
                literal_type: INTEGER
                source: "1"
          from_clause: null
          where: null
          groupby: null
          having: null
          orderby: null
          limit_clause: null
          window_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_exists(self):
        return AstTestBlueprint(
            sql="SELECT EXISTS (SELECT 1)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: ExistsExpr
        select: SelectStmt
          flags: (none)
          columns: ResultColumnList[1]
            ResultColumn
              flags: (none)
              alias: null
              expr: Literal
                literal_type: INTEGER
                source: "1"
          from_clause: null
          where: null
          groupby: null
          having: null
          orderby: null
          limit_clause: null
          window_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
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
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: InExpr
        negated: 0
        operand: Literal
          literal_type: INTEGER
          source: "1"
        source: ExprList[3]
          Literal
            literal_type: INTEGER
            source: "1"
          Literal
            literal_type: INTEGER
            source: "2"
          Literal
            literal_type: INTEGER
            source: "3"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_not_in_list(self):
        return AstTestBlueprint(
            sql="SELECT 1 NOT IN (1, 2, 3)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: InExpr
        negated: 1
        operand: Literal
          literal_type: INTEGER
          source: "1"
        source: ExprList[3]
          Literal
            literal_type: INTEGER
            source: "1"
          Literal
            literal_type: INTEGER
            source: "2"
          Literal
            literal_type: INTEGER
            source: "3"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_in_subquery(self):
        return AstTestBlueprint(
            sql="SELECT 1 IN (SELECT 1)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: InExpr
        negated: 0
        operand: Literal
          literal_type: INTEGER
          source: "1"
        source: SubqueryExpr
          select: SelectStmt
            flags: (none)
            columns: ResultColumnList[1]
              ResultColumn
                flags: (none)
                alias: null
                expr: Literal
                  literal_type: INTEGER
                  source: "1"
            from_clause: null
            where: null
            groupby: null
            having: null
            orderby: null
            limit_clause: null
            window_clause: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
