# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Aggregate function ORDER BY and RAISE expression AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class AggregateFunctionOrderBy(TestSuite):
    """Aggregate function calls with ORDER BY clause."""

    def test_basic_order_by(self):
        return DiffTestBlueprint(
            sql="SELECT GROUP_CONCAT(name ORDER BY name) FROM t",
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
                        func_name: "GROUP_CONCAT"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "name"
                              table: (none)
                              schema: (none)
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "name"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        filter_clause: (none)
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

    def test_order_by_desc(self):
        return DiffTestBlueprint(
            sql="SELECT GROUP_CONCAT(name, ',' ORDER BY name DESC) FROM t",
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
                        func_name: "GROUP_CONCAT"
                        flags: (none)
                        args:
                          ExprList [2 items]
                            ColumnRef
                              column: "name"
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
                                  column: "name"
                                  table: (none)
                                  schema: (none)
                              sort_order: DESC
                              nulls_order: NONE
                        filter_clause: (none)
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

    def test_distinct_order_by(self):
        return DiffTestBlueprint(
            sql="SELECT GROUP_CONCAT(DISTINCT name ORDER BY name) FROM t",
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
                        func_name: "GROUP_CONCAT"
                        flags: DISTINCT
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "name"
                              table: (none)
                              schema: (none)
                        orderby:
                          OrderByList [1 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "name"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        filter_clause: (none)
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


    def test_without_order_by(self):
        return DiffTestBlueprint(
            sql="SELECT group_concat(name) FROM t",
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
                        func_name: "group_concat"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            ColumnRef
                              column: "name"
                              table: (none)
                              schema: (none)
                        filter_clause: (none)
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

    def test_multiple_ordering_terms(self):
        return DiffTestBlueprint(
            sql="SELECT group_concat(name ORDER BY last_name, first_name) FROM t",
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
                          ExprList [1 items]
                            ColumnRef
                              column: "name"
                              table: (none)
                              schema: (none)
                        orderby:
                          OrderByList [2 items]
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "last_name"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                            OrderingTerm
                              expr:
                                ColumnRef
                                  column: "first_name"
                                  table: (none)
                                  schema: (none)
                              sort_order: ASC
                              nulls_order: NONE
                        filter_clause: (none)
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


class RaiseExpression(TestSuite):
    """RAISE expression tests."""

    def test_raise_ignore(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(IGNORE)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      RaiseExpr
                        raise_type: IGNORE
                        error_message: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_raise_rollback(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(ROLLBACK, 'error message')",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      RaiseExpr
                        raise_type: ROLLBACK
                        error_message:
                          Literal
                            literal_type: STRING
                            source: "'error message'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_raise_abort(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(ABORT, 'constraint failed')",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      RaiseExpr
                        raise_type: ABORT
                        error_message:
                          Literal
                            literal_type: STRING
                            source: "'constraint failed'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_raise_fail(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(FAIL, 'error')",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      RaiseExpr
                        raise_type: FAIL
                        error_message:
                          Literal
                            literal_type: STRING
                            source: "'error'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


ORDERED_SET_CFLAGS = ["SQLITE_ENABLE_ORDERED_SET_AGGREGATES"]


class OrderedSetFunctionCall(TestSuite):
    """WITHIN GROUP ordered-set aggregate function tests.

    Requires SQLITE_ENABLE_ORDERED_SET_AGGREGATES cflag.
    """

    def test_basic(self):
        return DiffTestBlueprint(
            sql="SELECT percentile_cont(0.5) WITHIN GROUP (ORDER BY x) FROM t",
            cflags=ORDERED_SET_CFLAGS,
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      OrderedSetFunctionCall
                        func_name: "percentile_cont"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            Literal
                              literal_type: FLOAT
                              source: "0.5"
                        orderby_expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        filter_clause: (none)
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

    def test_distinct(self):
        return DiffTestBlueprint(
            sql="SELECT percentile_cont(DISTINCT 0.5) WITHIN GROUP (ORDER BY x) FROM t",
            cflags=ORDERED_SET_CFLAGS,
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      OrderedSetFunctionCall
                        func_name: "percentile_cont"
                        flags: DISTINCT
                        args:
                          ExprList [1 items]
                            Literal
                              literal_type: FLOAT
                              source: "0.5"
                        orderby_expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        filter_clause: (none)
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

    def test_with_filter(self):
        return DiffTestBlueprint(
            sql="SELECT percentile_cont(0.5) WITHIN GROUP (ORDER BY x) FILTER (WHERE y > 0) FROM t",
            cflags=ORDERED_SET_CFLAGS,
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      OrderedSetFunctionCall
                        func_name: "percentile_cont"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            Literal
                              literal_type: FLOAT
                              source: "0.5"
                        orderby_expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        filter_clause:
                          BinaryExpr
                            op: GT
                            left:
                              ColumnRef
                                column: "y"
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

    def test_with_over(self):
        return DiffTestBlueprint(
            sql="SELECT percentile_cont(0.5) WITHIN GROUP (ORDER BY x) OVER () FROM t",
            cflags=ORDERED_SET_CFLAGS,
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      OrderedSetFunctionCall
                        func_name: "percentile_cont"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            Literal
                              literal_type: FLOAT
                              source: "0.5"
                        orderby_expr:
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
