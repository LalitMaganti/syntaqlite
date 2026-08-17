# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CAST, PTR, QNUMBER, and row value AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CastExpr(TestSuite):
    """CAST expression tests."""

    def test_cast_integer(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(1 AS INTEGER)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        type_name: "INTEGER"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cast_text(self):
        return DiffTestBlueprint(
            sql="SELECT CAST('hello' AS TEXT)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          Literal
                            literal_type: STRING
                            source: "'hello'"
                        type_name: "TEXT"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cast_real(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(3.14 AS REAL)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          Literal
                            literal_type: FLOAT
                            source: "3.14"
                        type_name: "REAL"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cast_varchar_precision(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(x AS VARCHAR(100))",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        type_name: "VARCHAR(100)"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cast_decimal_scale(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(x AS DECIMAL(10,2))",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        type_name: "DECIMAL(10,2)"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cast_multi_word_type(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(x AS DOUBLE PRECISION)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        type_name: "DOUBLE PRECISION"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cast_empty_type(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(1 AS )",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      CastExpr
                        expr:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        type_name: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class PtrExpr(TestSuite):
    """PTR (JSON ->) operator tests."""

    def test_ptr_strings(self):
        return DiffTestBlueprint(
            sql="SELECT '{\"a\":1}' -> '$.a'",
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
                        op: PTR
                        left:
                          Literal
                            literal_type: STRING
                            source: "'{"a":1}'"
                        right:
                          Literal
                            literal_type: STRING
                            source: "'$.a'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_ptr_column(self):
        return DiffTestBlueprint(
            sql="SELECT j -> '$.name'",
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
                        op: PTR
                        left:
                          ColumnRef
                            column: "j"
                            table: (none)
                            schema: (none)
                        right:
                          Literal
                            literal_type: STRING
                            source: "'$.name'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


    def test_ptr2_extract_text(self):
        return DiffTestBlueprint(
            sql="SELECT j ->> '$.name'",
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
                        op: PTR2
                        left:
                          ColumnRef
                            column: "j"
                            table: (none)
                            schema: (none)
                        right:
                          Literal
                            literal_type: STRING
                            source: "'$.name'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_ptr_and_ptr2_chain(self):
        return DiffTestBlueprint(
            sql="SELECT j -> '$.a' ->> '$.b'",
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
                        op: PTR2
                        left:
                          BinaryExpr
                            op: PTR
                            left:
                              ColumnRef
                                column: "j"
                                table: (none)
                                schema: (none)
                            right:
                              Literal
                                literal_type: STRING
                                source: "'$.a'"
                        right:
                          Literal
                            literal_type: STRING
                            source: "'$.b'"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class QnumberLiteral(TestSuite):
    """QNUMBER (digit-separated number) literal tests."""

    def test_qnumber_integer(self):
        return DiffTestBlueprint(
            sql="SELECT 1_000_000",
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
                        literal_type: QNUMBER
                        source: "1_000_000"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_qnumber_float(self):
        return DiffTestBlueprint(
            sql="SELECT 1_000.50",
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
                        literal_type: QNUMBER
                        source: "1_000.50"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class RowValue(TestSuite):
    """Row value tuple tests."""

    def test_two_elements(self):
        return DiffTestBlueprint(
            sql="SELECT (1, 2)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      RowValue
                        items:
                          ExprList [2 items]
                            Literal
                              literal_type: INTEGER
                              source: "1"
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

    def test_three_elements(self):
        return DiffTestBlueprint(
            sql="SELECT (1, 2, 3)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      RowValue
                        items:
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
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
