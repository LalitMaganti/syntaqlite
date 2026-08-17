# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Conditional expression AST tests: IS, BETWEEN, LIKE, CASE."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class IsExprBasic(TestSuite):
    """IS/ISNULL/NOTNULL expression tests."""

    def test_isnull(self):
        return DiffTestBlueprint(
            sql="SELECT 1 ISNULL",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS_NULL
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_notnull(self):
        return DiffTestBlueprint(
            sql="SELECT 1 NOTNULL",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: NOT_NULL
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_not_null(self):
        return DiffTestBlueprint(
            sql="SELECT 1 NOT NULL",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: NOT_NULL
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_is_null(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IS NULL",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: NULL
                            source: "NULL"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_is_not_null(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IS NOT NULL",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS_NOT
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: NULL
                            source: "NULL"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_is_expr(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IS 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "2"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_is_not_expr(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IS NOT 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS_NOT
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "2"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_is_not_distinct_from(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IS NOT DISTINCT FROM 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS_NOT_DISTINCT
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "2"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_is_distinct_from(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IS DISTINCT FROM 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      IsExpr
                        op: IS_DISTINCT
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "2"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class BetweenExprBasic(TestSuite):
    """BETWEEN expression tests."""

    def test_between(self):
        return DiffTestBlueprint(
            sql="SELECT 1 BETWEEN 0 AND 10",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BetweenExpr
                        negated: FALSE
                        operand:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        low:
                          Literal
                            literal_type: INTEGER
                            source: "0"
                        high:
                          Literal
                            literal_type: INTEGER
                            source: "10"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_between_expressions(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t WHERE x BETWEEN y+1 AND z-1",
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
              where_clause:
                BetweenExpr
                  negated: FALSE
                  operand:
                    ColumnRef
                      column: "x"
                      table: (none)
                      schema: (none)
                  low:
                    BinaryExpr
                      op: PLUS
                      left:
                        ColumnRef
                          column: "y"
                          table: (none)
                          schema: (none)
                      right:
                        Literal
                          literal_type: INTEGER
                          source: "1"
                  high:
                    BinaryExpr
                      op: MINUS
                      left:
                        ColumnRef
                          column: "z"
                          table: (none)
                          schema: (none)
                      right:
                        Literal
                          literal_type: INTEGER
                          source: "1"
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_not_between(self):
        return DiffTestBlueprint(
            sql="SELECT 1 NOT BETWEEN 0 AND 10",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BetweenExpr
                        negated: TRUE
                        operand:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        low:
                          Literal
                            literal_type: INTEGER
                            source: "0"
                        high:
                          Literal
                            literal_type: INTEGER
                            source: "10"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class LikeExprBasic(TestSuite):
    """LIKE expression tests."""

    def test_like(self):
        return DiffTestBlueprint(
            sql="SELECT 'abc' LIKE 'a%'",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      LikeExpr
                        negated: FALSE
                        keyword: LIKE
                        operand:
                          Literal
                            literal_type: STRING
                            source: "'abc'"
                        pattern:
                          Literal
                            literal_type: STRING
                            source: "'a%'"
                        escape: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_not_like(self):
        return DiffTestBlueprint(
            sql="SELECT 'abc' NOT LIKE 'a%'",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      LikeExpr
                        negated: TRUE
                        keyword: LIKE
                        operand:
                          Literal
                            literal_type: STRING
                            source: "'abc'"
                        pattern:
                          Literal
                            literal_type: STRING
                            source: "'a%'"
                        escape: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_like_column_operands(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t WHERE a LIKE b",
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
              where_clause:
                LikeExpr
                  negated: FALSE
                  keyword: LIKE
                  operand:
                    ColumnRef
                      column: "a"
                      table: (none)
                      schema: (none)
                  pattern:
                    ColumnRef
                      column: "b"
                      table: (none)
                      schema: (none)
                  escape: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_like_escape(self):
        return DiffTestBlueprint(
            sql="SELECT 'abc' LIKE 'a%' ESCAPE '\\'",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      LikeExpr
                        negated: FALSE
                        keyword: LIKE
                        operand:
                          Literal
                            literal_type: STRING
                            source: "'abc'"
                        pattern:
                          Literal
                            literal_type: STRING
                            source: "'a%'"
                        escape:
                          Literal
                            literal_type: STRING
                            source: "'\\'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class CaseExprBasic(TestSuite):
    """CASE expression tests."""

    def test_simple_case(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN 1 THEN 'a' ELSE 'b' END",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CaseExpr
                        operand: (none)
                        else_expr:
                          Literal
                            literal_type: STRING
                            source: "'b'"
                        whens:
                          CaseWhenList [1 items]
                            CaseWhen
                              when_expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'a'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_case_with_operand(self):
        return DiffTestBlueprint(
            sql="SELECT CASE 1 WHEN 1 THEN 'yes' WHEN 2 THEN 'no' END",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CaseExpr
                        operand:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        else_expr: (none)
                        whens:
                          CaseWhenList [2 items]
                            CaseWhen
                              when_expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'yes'"
                            CaseWhen
                              when_expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "2"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'no'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_case_no_else(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN 1 THEN 'a' END",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CaseExpr
                        operand: (none)
                        else_expr: (none)
                        whens:
                          CaseWhenList [1 items]
                            CaseWhen
                              when_expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'a'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_nested_case(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN x > 0 THEN CASE WHEN y > 0 THEN 1 ELSE 2 END ELSE 3 END",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CaseExpr
                        operand: (none)
                        else_expr:
                          Literal
                            literal_type: INTEGER
                            source: "3"
                        whens:
                          CaseWhenList [1 items]
                            CaseWhen
                              when_expr:
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
                              then_expr:
                                CaseExpr
                                  operand: (none)
                                  else_expr:
                                    Literal
                                      literal_type: INTEGER
                                      source: "2"
                                  whens:
                                    CaseWhenList [1 items]
                                      CaseWhen
                                        when_expr:
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
                                        then_expr:
                                          Literal
                                            literal_type: INTEGER
                                            source: "1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_case_three_whens(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN x = 1 THEN 'a' WHEN x = 2 THEN 'b' WHEN x = 3 THEN 'c' ELSE 'd' END",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CaseExpr
                        operand: (none)
                        else_expr:
                          Literal
                            literal_type: STRING
                            source: "'d'"
                        whens:
                          CaseWhenList [3 items]
                            CaseWhen
                              when_expr:
                                BinaryExpr
                                  op: EQ
                                  left:
                                    ColumnRef
                                      column: "x"
                                      table: (none)
                                      schema: (none)
                                  right:
                                    Literal
                                      literal_type: INTEGER
                                      source: "1"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'a'"
                            CaseWhen
                              when_expr:
                                BinaryExpr
                                  op: EQ
                                  left:
                                    ColumnRef
                                      column: "x"
                                      table: (none)
                                      schema: (none)
                                  right:
                                    Literal
                                      literal_type: INTEGER
                                      source: "2"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'b'"
                            CaseWhen
                              when_expr:
                                BinaryExpr
                                  op: EQ
                                  left:
                                    ColumnRef
                                      column: "x"
                                      table: (none)
                                      schema: (none)
                                  right:
                                    Literal
                                      literal_type: INTEGER
                                      source: "3"
                              then_expr:
                                Literal
                                  literal_type: STRING
                                  source: "'c'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
