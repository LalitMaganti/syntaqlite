# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""WITH/CTE (Common Table Expression) AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class WithClause(TestSuite):
    """WITH clause tests."""

    def test_simple_cte(self):
        return AstTestBlueprint(
            sql="WITH t AS (SELECT 1) SELECT * FROM t",
            out="""\
WithClause
  recursive: 0
  CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: 0
      SelectStmt
        flags: 0
        ResultColumnList[1]
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "1"
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 1
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )

    def test_cte_with_columns(self):
        return AstTestBlueprint(
            sql="WITH t(a, b) AS (SELECT 1, 2) SELECT * FROM t",
            out="""\
WithClause
  recursive: 0
  CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: 0
      ExprList[2]
        ColumnRef
          column: "a"
          table: null
          schema: null
        ColumnRef
          column: "b"
          table: null
          schema: null
      SelectStmt
        flags: 0
        ResultColumnList[2]
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "1"
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "2"
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 1
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )

    def test_recursive_cte(self):
        return AstTestBlueprint(
            sql="WITH RECURSIVE cnt(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM cnt) SELECT x FROM cnt",
            out="""\
WithClause
  recursive: 1
  CteList[1]
    CteDefinition
      cte_name: "cnt"
      materialized: 0
      ExprList[1]
        ColumnRef
          column: "x"
          table: null
          schema: null
      CompoundSelect
        op: UNION_ALL
        SelectStmt
          flags: 0
          ResultColumnList[1]
            ResultColumn
              flags: 0
              alias: null
              Literal
                literal_type: INTEGER
                source: "1"
        SelectStmt
          flags: 0
          ResultColumnList[1]
            ResultColumn
              flags: 0
              alias: null
              BinaryExpr
                op: PLUS
                ColumnRef
                  column: "x"
                  table: null
                  schema: null
                Literal
                  literal_type: INTEGER
                  source: "1"
          TableRef
            table_name: "cnt"
            schema: null
            alias: null
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 0
        alias: null
        ColumnRef
          column: "x"
          table: null
          schema: null
    TableRef
      table_name: "cnt"
      schema: null
      alias: null
""",
        )

    def test_multiple_ctes(self):
        return AstTestBlueprint(
            sql="WITH a AS (SELECT 1), b AS (SELECT 2) SELECT * FROM a",
            out="""\
WithClause
  recursive: 0
  CteList[2]
    CteDefinition
      cte_name: "a"
      materialized: 0
      SelectStmt
        flags: 0
        ResultColumnList[1]
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "1"
    CteDefinition
      cte_name: "b"
      materialized: 0
      SelectStmt
        flags: 0
        ResultColumnList[1]
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "2"
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 1
        alias: null
    TableRef
      table_name: "a"
      schema: null
      alias: null
""",
        )

    def test_materialized(self):
        return AstTestBlueprint(
            sql="WITH t AS MATERIALIZED (SELECT 1) SELECT * FROM t",
            out="""\
WithClause
  recursive: 0
  CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: 1
      SelectStmt
        flags: 0
        ResultColumnList[1]
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "1"
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 1
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )

    def test_not_materialized(self):
        return AstTestBlueprint(
            sql="WITH t AS NOT MATERIALIZED (SELECT 1) SELECT * FROM t",
            out="""\
WithClause
  recursive: 0
  CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: 2
      SelectStmt
        flags: 0
        ResultColumnList[1]
          ResultColumn
            flags: 0
            alias: null
            Literal
              literal_type: INTEGER
              source: "1"
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 1
        alias: null
    TableRef
      table_name: "t"
      schema: null
      alias: null
""",
        )
