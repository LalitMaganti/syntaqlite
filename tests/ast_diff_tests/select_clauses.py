# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT clause AST tests: WHERE, GROUP BY, HAVING, ORDER BY, LIMIT."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class SelectWhere(TestSuite):
    """SELECT with WHERE clause tests."""

    def test_where_simple(self):
        return AstTestBlueprint(
            sql="SELECT 1 WHERE 1 > 0",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: INTEGER
        source: "1"
  from_clause: null
  where: BinaryExpr
    op: GT
    left: Literal
      literal_type: INTEGER
      source: "1"
    right: Literal
      literal_type: INTEGER
      source: "0"
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class SelectGroupBy(TestSuite):
    """SELECT with GROUP BY clause tests."""

    def test_groupby_single(self):
        return AstTestBlueprint(
            sql="SELECT 1 GROUP BY 1",
            out="""\
SelectStmt
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
  groupby: ExprList[1]
    Literal
      literal_type: INTEGER
      source: "1"
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_groupby_having(self):
        return AstTestBlueprint(
            sql="SELECT 1 GROUP BY 1 HAVING 1 > 0",
            out="""\
SelectStmt
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
  groupby: ExprList[1]
    Literal
      literal_type: INTEGER
      source: "1"
  having: BinaryExpr
    op: GT
    left: Literal
      literal_type: INTEGER
      source: "1"
    right: Literal
      literal_type: INTEGER
      source: "0"
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class SelectOrderBy(TestSuite):
    """SELECT with ORDER BY clause tests."""

    def test_orderby_simple(self):
        return AstTestBlueprint(
            sql="SELECT 1 ORDER BY 1",
            out="""\
SelectStmt
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
  orderby: OrderByList[1]
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "1"
      sort_order: ASC
      nulls_order: NONE
  limit_clause: null
  window_clause: null
""",
        )

    def test_orderby_desc(self):
        return AstTestBlueprint(
            sql="SELECT 1 ORDER BY 1 DESC",
            out="""\
SelectStmt
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
  orderby: OrderByList[1]
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "1"
      sort_order: DESC
      nulls_order: NONE
  limit_clause: null
  window_clause: null
""",
        )

    def test_orderby_nulls_first(self):
        return AstTestBlueprint(
            sql="SELECT 1 ORDER BY 1 NULLS FIRST",
            out="""\
SelectStmt
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
  orderby: OrderByList[1]
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "1"
      sort_order: ASC
      nulls_order: FIRST
  limit_clause: null
  window_clause: null
""",
        )

    def test_orderby_desc_nulls_last(self):
        return AstTestBlueprint(
            sql="SELECT 1 ORDER BY 1 DESC NULLS LAST",
            out="""\
SelectStmt
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
  orderby: OrderByList[1]
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "1"
      sort_order: DESC
      nulls_order: LAST
  limit_clause: null
  window_clause: null
""",
        )

    def test_orderby_multiple(self):
        return AstTestBlueprint(
            sql="SELECT 1 ORDER BY 1 ASC, 2 DESC",
            out="""\
SelectStmt
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
  orderby: OrderByList[2]
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "1"
      sort_order: ASC
      nulls_order: NONE
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "2"
      sort_order: DESC
      nulls_order: NONE
  limit_clause: null
  window_clause: null
""",
        )


class SelectLimit(TestSuite):
    """SELECT with LIMIT clause tests."""

    def test_limit_simple(self):
        return AstTestBlueprint(
            sql="SELECT 1 LIMIT 10",
            out="""\
SelectStmt
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
  limit_clause: LimitClause
    limit: Literal
      literal_type: INTEGER
      source: "10"
    offset: null
  window_clause: null
""",
        )

    def test_limit_offset(self):
        return AstTestBlueprint(
            sql="SELECT 1 LIMIT 10 OFFSET 5",
            out="""\
SelectStmt
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
  limit_clause: LimitClause
    limit: Literal
      literal_type: INTEGER
      source: "10"
    offset: Literal
      literal_type: INTEGER
      source: "5"
  window_clause: null
""",
        )

    def test_limit_comma(self):
        return AstTestBlueprint(
            sql="SELECT 1 LIMIT 5, 10",
            out="""\
SelectStmt
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
  limit_clause: LimitClause
    limit: Literal
      literal_type: INTEGER
      source: "10"
    offset: Literal
      literal_type: INTEGER
      source: "5"
  window_clause: null
""",
        )


class SelectCombined(TestSuite):
    """SELECT with multiple clauses combined."""

    def test_where_orderby_limit(self):
        return AstTestBlueprint(
            sql="SELECT 1 WHERE 1 > 0 ORDER BY 1 LIMIT 10",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: INTEGER
        source: "1"
  from_clause: null
  where: BinaryExpr
    op: GT
    left: Literal
      literal_type: INTEGER
      source: "1"
    right: Literal
      literal_type: INTEGER
      source: "0"
  groupby: null
  having: null
  orderby: OrderByList[1]
    OrderingTerm
      expr: Literal
        literal_type: INTEGER
        source: "1"
      sort_order: ASC
      nulls_order: NONE
  limit_clause: LimitClause
    limit: Literal
      literal_type: INTEGER
      source: "10"
    offset: null
  window_clause: null
""",
        )
