# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Aggregate function ORDER BY and RAISE expression AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class AggregateFunctionOrderBy(TestSuite):
    """Aggregate function calls with ORDER BY clause."""

    def test_basic_order_by(self):
        return AstTestBlueprint(
            sql="SELECT GROUP_CONCAT(name ORDER BY name) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: AggregateFunctionCall
        func_name: "GROUP_CONCAT"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "name"
            table: null
            schema: null
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "name"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        filter_clause: null
        over_clause: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_order_by_desc(self):
        return AstTestBlueprint(
            sql="SELECT GROUP_CONCAT(name, ',' ORDER BY name DESC) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: AggregateFunctionCall
        func_name: "GROUP_CONCAT"
        flags: (none)
        args: ExprList[2]
          ColumnRef
            column: "name"
            table: null
            schema: null
          Literal
            literal_type: STRING
            source: "','"
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "name"
              table: null
              schema: null
            sort_order: DESC
            nulls_order: NONE
        filter_clause: null
        over_clause: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_distinct_order_by(self):
        return AstTestBlueprint(
            sql="SELECT GROUP_CONCAT(DISTINCT name ORDER BY name) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: AggregateFunctionCall
        func_name: "GROUP_CONCAT"
        flags: DISTINCT
        args: ExprList[1]
          ColumnRef
            column: "name"
            table: null
            schema: null
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "name"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        filter_clause: null
        over_clause: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class RaiseExpression(TestSuite):
    """RAISE expression tests."""

    def test_raise_ignore(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(IGNORE)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: RaiseExpr
        raise_type: IGNORE
        error_message: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_raise_rollback(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(ROLLBACK, 'error message')",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: RaiseExpr
        raise_type: ROLLBACK
        error_message: Literal
          literal_type: STRING
          source: "'error message'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_raise_abort(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(ABORT, 'constraint failed')",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: RaiseExpr
        raise_type: ABORT
        error_message: Literal
          literal_type: STRING
          source: "'constraint failed'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_raise_fail(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(FAIL, 'error')",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: RaiseExpr
        raise_type: FAIL
        error_message: Literal
          literal_type: STRING
          source: "'error'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
