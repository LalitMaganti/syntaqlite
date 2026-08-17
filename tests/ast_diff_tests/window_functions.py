# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Window function AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class WindowFunctionBasic(TestSuite):
    """Basic window function tests."""

    def test_row_number_over_order(self):
        return DiffTestBlueprint(
            sql="SELECT row_number() OVER (ORDER BY id) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "row_number"
                        flags: (none)
                        args: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "id"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_count_star_over(self):
        return DiffTestBlueprint(
            sql="SELECT count(*) OVER (PARTITION BY a) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "count"
                        flags: STAR
                        args: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by:
                              ExprList [1 items]
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                            orderby: (none)
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_over_named_window(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER w FROM t WINDOW w AS (ORDER BY x)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: "w"
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause:
                NamedWindowDefList [1 items]
                  NamedWindowDef
                    window_name: "w"
                    window_def:
                      WindowDef
                        ref_window_name: (none)
                        base_window_name: (none)
                        partition_by: (none)
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "x"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        frame: (none)
""",
        )


    def test_empty_over_clause(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER () FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_partition_by_order_by_combined(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (PARTITION BY a ORDER BY b) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by:
                              ExprList [1 items]
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "b"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_multiple_partition_expressions(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (PARTITION BY a, b ORDER BY c) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by:
                              ExprList [2 items]
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                                ColumnRef
                                  column: "b"
                                  table: (none)
                                  schema: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "c"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_order_by_desc_nulls_last(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY a DESC NULLS LAST) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "a"
                                      table: (none)
                                      schema: (none)
                                  sort_order: DESC
                                  nulls_order: LAST
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class FilterClause(TestSuite):
    """FILTER clause tests."""

    def test_filter_only(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) FILTER (WHERE x > 0) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause:
                          BinaryExpr
                            op: GT
                            left:
                              ColumnRef
                                column: "x"
                                table: (none)
                                schema: (none)
                            right:
                              Literal
                                literal_type: INTEGER
                                source: "0"
                        over_clause: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_filter_and_over(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) FILTER (WHERE x > 0) OVER (ORDER BY y) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause:
                          BinaryExpr
                            op: GT
                            left:
                              ColumnRef
                                column: "x"
                                table: (none)
                                schema: (none)
                            right:
                              Literal
                                literal_type: INTEGER
                                source: "0"
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "y"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class FrameSpecification(TestSuite):
    """Frame specification tests."""

    def test_rows_between(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "y"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame:
                              FrameSpec
                                frame_type: ROWS
                                exclude: NONE
                                start_bound:
                                  FrameBound
                                    bound_type: EXPR_PRECEDING
                                    expr:
                                      Literal
                                        literal_type: INTEGER
                                        source: "1"
                                end_bound:
                                  FrameBound
                                    bound_type: EXPR_FOLLOWING
                                    expr:
                                      Literal
                                        literal_type: INTEGER
                                        source: "1"
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_range_unbounded(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "y"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame:
                              FrameSpec
                                frame_type: RANGE
                                exclude: NONE
                                start_bound:
                                  FrameBound
                                    bound_type: UNBOUNDED_PRECEDING
                                    expr: (none)
                                end_bound:
                                  FrameBound
                                    bound_type: CURRENT_ROW
                                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_groups_with_exclude(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y GROUPS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING EXCLUDE TIES) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "y"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame:
                              FrameSpec
                                frame_type: GROUPS
                                exclude: TIES
                                start_bound:
                                  FrameBound
                                    bound_type: UNBOUNDED_PRECEDING
                                    expr: (none)
                                end_bound:
                                  FrameBound
                                    bound_type: UNBOUNDED_FOLLOWING
                                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_frame_exclude_no_others(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE NO OTHERS) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame:
                              FrameSpec
                                frame_type: ROWS
                                exclude: NO_OTHERS
                                start_bound:
                                  FrameBound
                                    bound_type: UNBOUNDED_PRECEDING
                                    expr: (none)
                                end_bound:
                                  FrameBound
                                    bound_type: CURRENT_ROW
                                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_frame_exclude_current_row(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE CURRENT ROW) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame:
                              FrameSpec
                                frame_type: ROWS
                                exclude: CURRENT_ROW
                                start_bound:
                                  FrameBound
                                    bound_type: UNBOUNDED_PRECEDING
                                    expr: (none)
                                end_bound:
                                  FrameBound
                                    bound_type: CURRENT_ROW
                                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_frame_exclude_group(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW EXCLUDE GROUP) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame:
                              FrameSpec
                                frame_type: ROWS
                                exclude: GROUP
                                start_bound:
                                  FrameBound
                                    bound_type: UNBOUNDED_PRECEDING
                                    expr: (none)
                                end_bound:
                                  FrameBound
                                    bound_type: CURRENT_ROW
                                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_rows_single_bound(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (ORDER BY y ROWS 2 PRECEDING) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by: (none)
                            orderby:
                              OrderByList [1 items]
                                OrderingTerm
                                  expr:
                                    ColumnRef
                                      column: "y"
                                      table: (none)
                                      schema: (none)
                                  sort_order: ASC
                                  nulls_order: NONE
                            frame:
                              FrameSpec
                                frame_type: ROWS
                                exclude: NONE
                                start_bound:
                                  FrameBound
                                    bound_type: EXPR_PRECEDING
                                    expr:
                                      Literal
                                        literal_type: INTEGER
                                        source: "2"
                                end_bound:
                                  FrameBound
                                    bound_type: CURRENT_ROW
                                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class WindowClause(TestSuite):
    """WINDOW clause tests."""

    def test_window_clause_basic(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t WINDOW w AS (ORDER BY x)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause:
                NamedWindowDefList [1 items]
                  NamedWindowDef
                    window_name: "w"
                    window_def:
                      WindowDef
                        ref_window_name: (none)
                        base_window_name: (none)
                        partition_by: (none)
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "x"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        frame: (none)
""",
        )

    def test_multiple_named_windows(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER w1, avg(y) OVER w2 FROM t WINDOW w1 AS (ORDER BY a), w2 AS (PARTITION BY b ORDER BY c)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [2 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "sum"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: "w1"
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame: (none)
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      FunctionCall
                        func_name: "avg"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "y"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
                        over_clause:
                          WindowDef
                            ref_window_name: "w2"
                            base_window_name: (none)
                            partition_by: (none)
                            orderby: (none)
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause:
                NamedWindowDefList [2 items]
                  NamedWindowDef
                    window_name: "w1"
                    window_def:
                      WindowDef
                        ref_window_name: (none)
                        base_window_name: (none)
                        partition_by: (none)
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        frame: (none)
                  NamedWindowDef
                    window_name: "w2"
                    window_def:
                      WindowDef
                        ref_window_name: (none)
                        base_window_name: (none)
                        partition_by:
                          ExprList [1 items]
                            ColumnRef
                              column: "b"
                              table: (none)
                              schema: (none)
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "c"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        frame: (none)
""",
        )


class AggregateWithWindowFunction(TestSuite):
    """Aggregate function calls with FILTER/OVER."""

    def test_aggregate_with_filter_over(self):
        return DiffTestBlueprint(
            sql="SELECT group_concat(x, ',' ORDER BY y) FILTER (WHERE z > 0) OVER (PARTITION BY a) FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      AggregateFunctionCall
                        func_name: "group_concat"
                        flags: (none)
                        args:
                          ExprList [2 items]
                            ColumnRef
                              column: "x"
                              table: (none)
                              schema: (none)
                            Literal
                              literal_type: STRING
                              source: "','"
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "y"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        filter_clause:
                          BinaryExpr
                            op: GT
                            left:
                              ColumnRef
                                column: "z"
                                table: (none)
                                schema: (none)
                            right:
                              Literal
                                literal_type: INTEGER
                                source: "0"
                        over_clause:
                          WindowDef
                            ref_window_name: (none)
                            base_window_name: (none)
                            partition_by:
                              ExprList [1 items]
                                ColumnRef
                                  column: "a"
                                  table: (none)
                                  schema: (none)
                            orderby: (none)
                            frame: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
