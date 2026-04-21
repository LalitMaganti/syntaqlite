# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT clause AST tests: WHERE, GROUP BY, HAVING, ORDER BY, LIMIT."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SelectWhere(TestSuite):
    """SELECT with WHERE clause tests."""

    def test_where_simple(self):
        return DiffTestBlueprint(
            sql="SELECT 1 WHERE 1 > 0",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause:
                BinaryExpr
                  op: GT
                  left:
                    Literal
                      literal_type: INTEGER
                      source: "1"
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "0"
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class SelectGroupBy(TestSuite):
    """SELECT with GROUP BY clause tests."""

    def test_groupby_single(self):
        return DiffTestBlueprint(
            sql="SELECT 1 GROUP BY 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby:
                ExprList [1 items]
                  Literal
                    literal_type: INTEGER
                    source: "1"
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_groupby_multiple(self):
        return DiffTestBlueprint(
            sql="SELECT a, b, count(*) FROM t GROUP BY a, b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [3 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "a"
                        table: (none)
                        schema: (none)
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "b"
                        table: (none)
                        schema: (none)
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
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              where_clause: (none)
              groupby:
                ExprList [2 items]
                  ColumnRef
                    column: "a"
                    table: (none)
                    schema: (none)
                  ColumnRef
                    column: "b"
                    table: (none)
                    schema: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_groupby_having(self):
        return DiffTestBlueprint(
            sql="SELECT 1 GROUP BY 1 HAVING 1 > 0",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby:
                ExprList [1 items]
                  Literal
                    literal_type: INTEGER
                    source: "1"
              having:
                BinaryExpr
                  op: GT
                  left:
                    Literal
                      literal_type: INTEGER
                      source: "1"
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "0"
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class SelectOrderBy(TestSuite):
    """SELECT with ORDER BY clause tests."""

    def test_orderby_simple(self):
        return DiffTestBlueprint(
            sql="SELECT 1 ORDER BY 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                    sort_order: ASC
                    nulls_order: NONE
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_orderby_desc(self):
        return DiffTestBlueprint(
            sql="SELECT 1 ORDER BY 1 DESC",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                    sort_order: DESC
                    nulls_order: NONE
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_orderby_nulls_first(self):
        return DiffTestBlueprint(
            sql="SELECT 1 ORDER BY 1 NULLS FIRST",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                    sort_order: ASC
                    nulls_order: FIRST
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_orderby_desc_nulls_last(self):
        return DiffTestBlueprint(
            sql="SELECT 1 ORDER BY 1 DESC NULLS LAST",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                    sort_order: DESC
                    nulls_order: LAST
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_orderby_multiple(self):
        return DiffTestBlueprint(
            sql="SELECT 1 ORDER BY 1 ASC, 2 DESC",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby:
                OrderByList [2 items]
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                    sort_order: ASC
                    nulls_order: NONE
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "2"
                    sort_order: DESC
                    nulls_order: NONE
              limit_clause: (none)
              window_clause: (none)
""",
        )


class SelectLimit(TestSuite):
    """SELECT with LIMIT clause tests."""

    def test_limit_simple(self):
        return DiffTestBlueprint(
            sql="SELECT 1 LIMIT 10",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause:
                LimitClause
                  limit:
                    Literal
                      literal_type: INTEGER
                      source: "10"
                  offset: (none)
              window_clause: (none)
""",
        )

    def test_limit_offset(self):
        return DiffTestBlueprint(
            sql="SELECT 1 LIMIT 10 OFFSET 5",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause:
                LimitClause
                  limit:
                    Literal
                      literal_type: INTEGER
                      source: "10"
                  offset:
                    Literal
                      literal_type: INTEGER
                      source: "5"
              window_clause: (none)
""",
        )

    def test_limit_comma(self):
        return DiffTestBlueprint(
            sql="SELECT 1 LIMIT 5, 10",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause:
                LimitClause
                  limit:
                    Literal
                      literal_type: INTEGER
                      source: "10"
                  offset:
                    Literal
                      literal_type: INTEGER
                      source: "5"
              window_clause: (none)
""",
        )


class SelectWindow(TestSuite):
    """SELECT with WINDOW clause tests."""

    def test_window_clause(self):
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
                            base_window_name: "w"
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


class SelectCombined(TestSuite):
    """SELECT with multiple clauses combined."""

    def test_where_orderby_limit(self):
        return DiffTestBlueprint(
            sql="SELECT 1 WHERE 1 > 0 ORDER BY 1 LIMIT 10",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause:
                BinaryExpr
                  op: GT
                  left:
                    Literal
                      literal_type: INTEGER
                      source: "1"
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "0"
              groupby: (none)
              having: (none)
              orderby:
                OrderByList [1 items]
                  OrderingTerm
                    expr:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                    sort_order: ASC
                    nulls_order: NONE
              limit_clause:
                LimitClause
                  limit:
                    Literal
                      literal_type: INTEGER
                      source: "10"
                  offset: (none)
              window_clause: (none)
""",
        )

    def test_all_clauses(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM t WHERE x > 0 GROUP BY a HAVING count(*) > 1 ORDER BY a LIMIT 10 OFFSET 5",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "a"
                        table: (none)
                        schema: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              where_clause:
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
              groupby:
                ExprList [1 items]
                  ColumnRef
                    column: "a"
                    table: (none)
                    schema: (none)
              having:
                BinaryExpr
                  op: GT
                  left:
                    FunctionCall
                      func_name: "count"
                      flags: STAR
                      args: (none)
                      filter_clause: (none)
                      over_clause: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "1"
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
              limit_clause:
                LimitClause
                  limit:
                    Literal
                      literal_type: INTEGER
                      source: "10"
                  offset:
                    Literal
                      literal_type: INTEGER
                      source: "5"
              window_clause: (none)
""",
        )
