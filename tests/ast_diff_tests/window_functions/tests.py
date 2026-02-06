# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Window function AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class WindowFunctionBasic(TestSuite):
    """Basic window function tests."""

    def test_row_number_over_order(self):
        return AstTestBlueprint(
            sql="SELECT row_number() OVER (ORDER BY id) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "row_number"
        flags: 0
        WindowDef
          base_window_name: null
          OrderByList[1]
            OrderingTerm
              ColumnRef
                column: "id"
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

    def test_count_star_over(self):
        return AstTestBlueprint(
            sql="SELECT count(*) OVER (PARTITION BY a) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "count"
        flags: 2
        WindowDef
          base_window_name: null
          ExprList[1]
            ColumnRef
              column: "a"
              table: null
              schema: null
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_over_named_window(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER w FROM t WINDOW w AS (ORDER BY x)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        WindowDef
          base_window_name: "w"
  TableRef
    table_name: "t"
    schema: null
    alias: null
  NamedWindowDefList[1]
    NamedWindowDef
      window_name: "w"
      WindowDef
        base_window_name: null
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "x"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
""",
        )


class FilterClause(TestSuite):
    """FILTER clause tests."""

    def test_filter_only(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) FILTER (WHERE x > 0) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        BinaryExpr
          op: GT
          ColumnRef
            column: "x"
            table: null
            schema: null
          Literal
            literal_type: INTEGER
            source: "0"
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_filter_and_over(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) FILTER (WHERE x > 0) OVER (ORDER BY y) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        BinaryExpr
          op: GT
          ColumnRef
            column: "x"
            table: null
            schema: null
          Literal
            literal_type: INTEGER
            source: "0"
        WindowDef
          base_window_name: null
          OrderByList[1]
            OrderingTerm
              ColumnRef
                column: "y"
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


class FrameSpecification(TestSuite):
    """Frame specification tests."""

    def test_rows_between(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        WindowDef
          base_window_name: null
          OrderByList[1]
            OrderingTerm
              ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          FrameSpec
            frame_type: ROWS
            exclude: NONE
            FrameBound
              bound_type: EXPR_PRECEDING
              Literal
                literal_type: INTEGER
                source: "1"
            FrameBound
              bound_type: EXPR_FOLLOWING
              Literal
                literal_type: INTEGER
                source: "1"
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_range_unbounded(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        WindowDef
          base_window_name: null
          OrderByList[1]
            OrderingTerm
              ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          FrameSpec
            frame_type: RANGE
            exclude: NONE
            FrameBound
              bound_type: UNBOUNDED_PRECEDING
            FrameBound
              bound_type: CURRENT_ROW
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_groups_with_exclude(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y GROUPS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING EXCLUDE TIES) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        WindowDef
          base_window_name: null
          OrderByList[1]
            OrderingTerm
              ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          FrameSpec
            frame_type: GROUPS
            exclude: TIES
            FrameBound
              bound_type: UNBOUNDED_PRECEDING
            FrameBound
              bound_type: UNBOUNDED_FOLLOWING
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_rows_single_bound(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y ROWS 2 PRECEDING) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        WindowDef
          base_window_name: null
          OrderByList[1]
            OrderingTerm
              ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          FrameSpec
            frame_type: ROWS
            exclude: NONE
            FrameBound
              bound_type: EXPR_PRECEDING
              Literal
                literal_type: INTEGER
                source: "2"
            FrameBound
              bound_type: CURRENT_ROW
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )


class WindowClause(TestSuite):
    """WINDOW clause tests."""

    def test_window_clause_basic(self):
        return AstTestBlueprint(
            sql="SELECT * FROM t WINDOW w AS (ORDER BY x)",
            out="""\
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
  NamedWindowDefList[1]
    NamedWindowDef
      window_name: "w"
      WindowDef
        base_window_name: null
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "x"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
""",
        )

    def test_multiple_named_windows(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER w1, avg(y) OVER w2 FROM t WINDOW w1 AS (ORDER BY a), w2 AS (PARTITION BY b ORDER BY c)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[2]
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "sum"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        WindowDef
          base_window_name: "w1"
    ResultColumn
      flags: 0
      alias: null
      FunctionCall
        func_name: "avg"
        flags: 0
        ExprList[1]
          ColumnRef
            column: "y"
            table: null
            schema: null
        WindowDef
          base_window_name: "w2"
  TableRef
    table_name: "t"
    schema: null
    alias: null
  NamedWindowDefList[2]
    NamedWindowDef
      window_name: "w1"
      WindowDef
        base_window_name: null
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "a"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
    NamedWindowDef
      window_name: "w2"
      WindowDef
        base_window_name: null
        ExprList[1]
          ColumnRef
            column: "b"
            table: null
            schema: null
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "c"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
""",
        )


class AggregateWithWindowFunction(TestSuite):
    """Aggregate function calls with FILTER/OVER."""

    def test_aggregate_with_filter_over(self):
        return AstTestBlueprint(
            sql="SELECT group_concat(x, ',' ORDER BY y) FILTER (WHERE z > 0) OVER (PARTITION BY a) FROM t",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      AggregateFunctionCall
        func_name: "group_concat"
        flags: 0
        ExprList[2]
          ColumnRef
            column: "x"
            table: null
            schema: null
          Literal
            literal_type: STRING
            source: "','"
        OrderByList[1]
          OrderingTerm
            ColumnRef
              column: "y"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        BinaryExpr
          op: GT
          ColumnRef
            column: "z"
            table: null
            schema: null
          Literal
            literal_type: INTEGER
            source: "0"
        WindowDef
          base_window_name: null
          ExprList[1]
            ColumnRef
              column: "a"
              table: null
              schema: null
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )
