# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Function call expression AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class FunctionCallBasic(TestSuite):
    """Basic function call tests."""

    def test_no_args(self):
        return DiffTestBlueprint(
            sql="SELECT random()",
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
                        func_name: "random"
                        flags: (none)
                        args: (none)
                        filter_clause: (none)
                        over_clause: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_one_arg(self):
        return DiffTestBlueprint(
            sql="SELECT abs(1)",
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
                        func_name: "abs"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            Literal
                              literal_type: INTEGER
                              source: "1"
                        filter_clause: (none)
                        over_clause: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_multiple_args(self):
        return DiffTestBlueprint(
            sql="SELECT max(1, 2, 3)",
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
                        func_name: "max"
                        flags: (none)
                        args:
                          ExprList [3 items]
                            Literal
                              literal_type: INTEGER
                              source: "1"
                            Literal
                              literal_type: INTEGER
                              source: "2"
                            Literal
                              literal_type: INTEGER
                              source: "3"
                        filter_clause: (none)
                        over_clause: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_count_star(self):
        return DiffTestBlueprint(
            sql="SELECT count(*)",
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
                        over_clause: (none)
              from_clause: (none)
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
            sql="SELECT count(DISTINCT 1)",
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
                        flags: DISTINCT
                        args:
                          ExprList [1 items]
                            Literal
                              literal_type: INTEGER
                              source: "1"
                        filter_clause: (none)
                        over_clause: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_filter_clause(self):
        return DiffTestBlueprint(
            sql="SELECT count(*) FILTER (WHERE x > 0) FROM t",
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

    def test_complex_args(self):
        return DiffTestBlueprint(
            sql="SELECT abs(1 + 2)",
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
                        func_name: "abs"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            BinaryExpr
                              op: PLUS
                              left:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                              right:
                                Literal
                                  literal_type: INTEGER
                                  source: "2"
                        filter_clause: (none)
                        over_clause: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_nested(self):
        return DiffTestBlueprint(
            sql="SELECT abs(max(1, 2))",
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
                        func_name: "abs"
                        flags: (none)
                        args:
                          ExprList [1 items]
                            FunctionCall
                              func_name: "max"
                              flags: (none)
                              args:
                                ExprList [2 items]
                                  Literal
                                    literal_type: INTEGER
                                    source: "1"
                                  Literal
                                    literal_type: INTEGER
                                    source: "2"
                              filter_clause: (none)
                              over_clause: (none)
                        filter_clause: (none)
                        over_clause: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
