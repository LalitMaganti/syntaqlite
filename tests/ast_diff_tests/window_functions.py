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
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "row_number"
        flags: (none)
        args: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: null
          partition_by: null
          orderby: OrderByList[1]
            OrderingTerm
              expr: ColumnRef
                column: "id"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          frame: null
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

    def test_count_star_over(self):
        return AstTestBlueprint(
            sql="SELECT count(*) OVER (PARTITION BY a) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "count"
        flags: STAR
        args: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: null
          partition_by: ExprList[1]
            ColumnRef
              column: "a"
              table: null
              schema: null
          orderby: null
          frame: null
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

    def test_over_named_window(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER w FROM t WINDOW w AS (ORDER BY x)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: "w"
          partition_by: null
          orderby: null
          frame: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: NamedWindowDefList[1]
    NamedWindowDef
      window_name: "w"
      window_def: WindowDef
        base_window_name: null
        partition_by: null
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "x"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        frame: null
""",
        )


class FilterClause(TestSuite):
    """FILTER clause tests."""

    def test_filter_only(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) FILTER (WHERE x > 0) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: BinaryExpr
          op: GT
          left: ColumnRef
            column: "x"
            table: null
            schema: null
          right: Literal
            literal_type: INTEGER
            source: "0"
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

    def test_filter_and_over(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) FILTER (WHERE x > 0) OVER (ORDER BY y) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: BinaryExpr
          op: GT
          left: ColumnRef
            column: "x"
            table: null
            schema: null
          right: Literal
            literal_type: INTEGER
            source: "0"
        over_clause: WindowDef
          base_window_name: null
          partition_by: null
          orderby: OrderByList[1]
            OrderingTerm
              expr: ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          frame: null
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


class FrameSpecification(TestSuite):
    """Frame specification tests."""

    def test_rows_between(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: null
          partition_by: null
          orderby: OrderByList[1]
            OrderingTerm
              expr: ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          frame: FrameSpec
            frame_type: ROWS
            exclude: NONE
            start_bound: FrameBound
              bound_type: EXPR_PRECEDING
              expr: Literal
                literal_type: INTEGER
                source: "1"
            end_bound: FrameBound
              bound_type: EXPR_FOLLOWING
              expr: Literal
                literal_type: INTEGER
                source: "1"
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

    def test_range_unbounded(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: null
          partition_by: null
          orderby: OrderByList[1]
            OrderingTerm
              expr: ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          frame: FrameSpec
            frame_type: RANGE
            exclude: NONE
            start_bound: FrameBound
              bound_type: UNBOUNDED_PRECEDING
              expr: null
            end_bound: FrameBound
              bound_type: CURRENT_ROW
              expr: null
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

    def test_groups_with_exclude(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y GROUPS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING EXCLUDE TIES) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: null
          partition_by: null
          orderby: OrderByList[1]
            OrderingTerm
              expr: ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          frame: FrameSpec
            frame_type: GROUPS
            exclude: TIES
            start_bound: FrameBound
              bound_type: UNBOUNDED_PRECEDING
              expr: null
            end_bound: FrameBound
              bound_type: UNBOUNDED_FOLLOWING
              expr: null
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

    def test_rows_single_bound(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y ROWS 2 PRECEDING) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: null
          partition_by: null
          orderby: OrderByList[1]
            OrderingTerm
              expr: ColumnRef
                column: "y"
                table: null
                schema: null
              sort_order: ASC
              nulls_order: NONE
          frame: FrameSpec
            frame_type: ROWS
            exclude: NONE
            start_bound: FrameBound
              bound_type: EXPR_PRECEDING
              expr: Literal
                literal_type: INTEGER
                source: "2"
            end_bound: FrameBound
              bound_type: CURRENT_ROW
              expr: null
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


class WindowClause(TestSuite):
    """WINDOW clause tests."""

    def test_window_clause_basic(self):
        return AstTestBlueprint(
            sql="SELECT * FROM t WINDOW w AS (ORDER BY x)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: NamedWindowDefList[1]
    NamedWindowDef
      window_name: "w"
      window_def: WindowDef
        base_window_name: null
        partition_by: null
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "x"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        frame: null
""",
        )

    def test_multiple_named_windows(self):
        return AstTestBlueprint(
            sql="SELECT sum(x) OVER w1, avg(y) OVER w2 FROM t WINDOW w1 AS (ORDER BY a), w2 AS (PARTITION BY b ORDER BY c)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[2]
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "sum"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "x"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: "w1"
          partition_by: null
          orderby: null
          frame: null
    ResultColumn
      flags: (none)
      alias: null
      expr: FunctionCall
        func_name: "avg"
        flags: (none)
        args: ExprList[1]
          ColumnRef
            column: "y"
            table: null
            schema: null
        filter_clause: null
        over_clause: WindowDef
          base_window_name: "w2"
          partition_by: null
          orderby: null
          frame: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: NamedWindowDefList[2]
    NamedWindowDef
      window_name: "w1"
      window_def: WindowDef
        base_window_name: null
        partition_by: null
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "a"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        frame: null
    NamedWindowDef
      window_name: "w2"
      window_def: WindowDef
        base_window_name: null
        partition_by: ExprList[1]
          ColumnRef
            column: "b"
            table: null
            schema: null
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "c"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        frame: null
""",
        )


class AggregateWithWindowFunction(TestSuite):
    """Aggregate function calls with FILTER/OVER."""

    def test_aggregate_with_filter_over(self):
        return AstTestBlueprint(
            sql="SELECT group_concat(x, ',' ORDER BY y) FILTER (WHERE z > 0) OVER (PARTITION BY a) FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: AggregateFunctionCall
        func_name: "group_concat"
        flags: (none)
        args: ExprList[2]
          ColumnRef
            column: "x"
            table: null
            schema: null
          Literal
            literal_type: STRING
            source: "','"
        orderby: OrderByList[1]
          OrderingTerm
            expr: ColumnRef
              column: "y"
              table: null
              schema: null
            sort_order: ASC
            nulls_order: NONE
        filter_clause: BinaryExpr
          op: GT
          left: ColumnRef
            column: "z"
            table: null
            schema: null
          right: Literal
            literal_type: INTEGER
            source: "0"
        over_clause: WindowDef
          base_window_name: null
          partition_by: ExprList[1]
            ColumnRef
              column: "a"
              table: null
              schema: null
          orderby: null
          frame: null
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
