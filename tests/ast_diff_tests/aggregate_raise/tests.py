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
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      AggregateFunctionCall
        func_name: "GROUP_CONCAT"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "name"
            table: null
            schema: null
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "name"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_order_by_desc(self):
        return AstTestBlueprint(
            sql="SELECT GROUP_CONCAT(name, ',' ORDER BY name DESC) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      AggregateFunctionCall
        func_name: "GROUP_CONCAT"
        flags: 0
        ExprList[2]
          ColumnRef
            column: "name"
            table: null
            schema: null
          Literal
            literal_type: STRING
            source: "','"
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "name"
              table: null
              schema: null
            sort_order: DESC
            nulls_order: NONE
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_distinct_order_by(self):
        return AstTestBlueprint(
            sql="SELECT GROUP_CONCAT(DISTINCT name ORDER BY name) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      AggregateFunctionCall
        func_name: "GROUP_CONCAT"
        flags: 1
        ExprList[1]
          ColumnRef
            column: "name"
            table: null
            schema: null
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "name"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )


class RaiseExpression(TestSuite):
    """RAISE expression tests."""

    def test_raise_ignore(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(IGNORE)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      RaiseExpr
        raise_type: IGNORE
""",
        )

    def test_raise_rollback(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(ROLLBACK, 'error message')",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      RaiseExpr
        raise_type: ROLLBACK
        Literal
          literal_type: STRING
          source: "'error message'"
""",
        )

    def test_raise_abort(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(ABORT, 'constraint failed')",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      RaiseExpr
        raise_type: ABORT
        Literal
          literal_type: STRING
          source: "'constraint failed'"
""",
        )

    def test_raise_fail(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(FAIL, 'error')",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      RaiseExpr
        raise_type: FAIL
        Literal
          literal_type: STRING
          source: "'error'"
""",
        )
