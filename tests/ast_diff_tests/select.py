# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SelectBasic(TestSuite):
    """Basic SELECT statement tests."""

    def test_integer_literal(self):
        return DiffTestBlueprint(
            sql="SELECT 1",
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
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_float_literal(self):
        return DiffTestBlueprint(
            sql="SELECT 3.14",
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
                        literal_type: FLOAT
                        source: "3.14"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_string_literal(self):
        return DiffTestBlueprint(
            sql="SELECT 'hello'",
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
                        literal_type: STRING
                        source: "'hello'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_null_literal(self):
        return DiffTestBlueprint(
            sql="SELECT NULL",
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

    def test_binary_plus(self):
        return DiffTestBlueprint(
            sql="SELECT 1 + 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
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
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_binary_star(self):
        return DiffTestBlueprint(
            sql="SELECT 3 * 4",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: STAR
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "3"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "4"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_binary_lt(self):
        return DiffTestBlueprint(
            sql="SELECT 1 < 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: LT
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

    def test_binary_eq(self):
        return DiffTestBlueprint(
            sql="SELECT 1 = 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: EQ
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

    def test_binary_and(self):
        return DiffTestBlueprint(
            sql="SELECT 1 AND 0",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: AND
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "0"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_binary_or(self):
        return DiffTestBlueprint(
            sql="SELECT 1 OR 0",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: OR
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "0"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_binary_concat(self):
        return DiffTestBlueprint(
            sql="SELECT 'a' || 'b'",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: CONCAT
                        left:
                          Literal
                            literal_type: STRING
                            source: "'a'"
                        right:
                          Literal
                            literal_type: STRING
                            source: "'b'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_unary_minus(self):
        return DiffTestBlueprint(
            sql="SELECT -5",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      UnaryExpr
                        op: MINUS
                        operand:
                          Literal
                            literal_type: INTEGER
                            source: "5"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_unary_not(self):
        return DiffTestBlueprint(
            sql="SELECT NOT 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      UnaryExpr
                        op: NOT
                        operand:
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

    def test_column_alias_as(self):
        return DiffTestBlueprint(
            sql="SELECT a AS x, b AS y FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [2 items]
                  ResultColumn
                    flags: (none)
                    alias:
                      IdentName
                        source: "x"
                    expr:
                      ColumnRef
                        column: "a"
                        table: (none)
                        schema: (none)
                  ResultColumn
                    flags: (none)
                    alias:
                      IdentName
                        source: "y"
                    expr:
                      ColumnRef
                        column: "b"
                        table: (none)
                        schema: (none)
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
              window_clause: (none)
""",
        )

    def test_column_alias_implicit(self):
        return DiffTestBlueprint(
            sql="SELECT a x FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias:
                      IdentName
                        source: "x"
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
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_select_distinct(self):
        return DiffTestBlueprint(
            sql="SELECT DISTINCT x FROM t",
            out="""\
            SelectStmt
              flags: DISTINCT
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
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
              window_clause: (none)
""",
        )

    def test_select_distinct_multiple_cols(self):
        return DiffTestBlueprint(
            sql="SELECT DISTINCT a, b FROM t",
            out="""\
            SelectStmt
              flags: DISTINCT
              columns:
                ResultColumnList [2 items]
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
              window_clause: (none)
""",
        )

    def test_select_star(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t",
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
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_blob_literal(self):
        return DiffTestBlueprint(
            sql="SELECT x'ABCD'",
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
                        literal_type: BLOB
                        source: "x'ABCD'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_precedence_mul_over_add(self):
        return DiffTestBlueprint(
            sql="SELECT 1 + 2 * 3",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: PLUS
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        right:
                          BinaryExpr
                            op: STAR
                            left:
                              Literal
                                literal_type: INTEGER
                                source: "2"
                            right:
                              Literal
                                literal_type: INTEGER
                                source: "3"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_precedence_parens(self):
        return DiffTestBlueprint(
            sql="SELECT (1 + 2) * 3",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: STAR
                        left:
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
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "3"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class BinaryOps(TestSuite):
    """Binary operator variant tests."""

    def test_minus(self):
        return DiffTestBlueprint(
            sql="SELECT 1 - 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: MINUS
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

    def test_slash(self):
        return DiffTestBlueprint(
            sql="SELECT 6 / 3",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: SLASH
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "6"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "3"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_rem(self):
        return DiffTestBlueprint(
            sql="SELECT 7 % 3",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: REM
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "7"
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "3"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_le(self):
        return DiffTestBlueprint(
            sql="SELECT 1 <= 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: LE
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

    def test_ge(self):
        return DiffTestBlueprint(
            sql="SELECT 1 >= 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: GE
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

    def test_ne(self):
        return DiffTestBlueprint(
            sql="SELECT 1 != 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: NE
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

    def test_bit_and(self):
        return DiffTestBlueprint(
            sql="SELECT 3 & 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: BIT_AND
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "3"
                        right:
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

    def test_bit_or(self):
        return DiffTestBlueprint(
            sql="SELECT 3 | 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: BIT_OR
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "3"
                        right:
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

    def test_lshift(self):
        return DiffTestBlueprint(
            sql="SELECT 1 << 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: LSHIFT
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

    def test_rshift(self):
        return DiffTestBlueprint(
            sql="SELECT 8 >> 2",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      BinaryExpr
                        op: RSHIFT
                        left:
                          Literal
                            literal_type: INTEGER
                            source: "8"
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


class UnaryOps(TestSuite):
    """Unary operator variant tests."""

    def test_plus(self):
        return DiffTestBlueprint(
            sql="SELECT +5",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      UnaryExpr
                        op: PLUS
                        operand:
                          Literal
                            literal_type: INTEGER
                            source: "5"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_bit_not(self):
        return DiffTestBlueprint(
            sql="SELECT ~5",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      UnaryExpr
                        op: BIT_NOT
                        operand:
                          Literal
                            literal_type: INTEGER
                            source: "5"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
