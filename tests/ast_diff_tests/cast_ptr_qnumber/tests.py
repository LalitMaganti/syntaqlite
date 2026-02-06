# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CAST, PTR, QNUMBER, and row value AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class CastExpr(TestSuite):
    """CAST expression tests."""

    def test_cast_integer(self):
        return AstTestBlueprint(
            sql="SELECT CAST(1 AS INTEGER)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        Literal
          literal_type: INTEGER
          source: "1"
        type_name: "INTEGER"
""",
        )

    def test_cast_text(self):
        return AstTestBlueprint(
            sql="SELECT CAST('hello' AS TEXT)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        Literal
          literal_type: STRING
          source: "'hello'"
        type_name: "TEXT"
""",
        )

    def test_cast_real(self):
        return AstTestBlueprint(
            sql="SELECT CAST(3.14 AS REAL)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        Literal
          literal_type: FLOAT
          source: "3.14"
        type_name: "REAL"
""",
        )

    def test_cast_varchar_precision(self):
        return AstTestBlueprint(
            sql="SELECT CAST(x AS VARCHAR(100))",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        ColumnRef
          column: "x"
          table: null
          schema: null
        type_name: "VARCHAR(100)"
""",
        )

    def test_cast_decimal_scale(self):
        return AstTestBlueprint(
            sql="SELECT CAST(x AS DECIMAL(10,2))",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        ColumnRef
          column: "x"
          table: null
          schema: null
        type_name: "DECIMAL(10,2)"
""",
        )

    def test_cast_multi_word_type(self):
        return AstTestBlueprint(
            sql="SELECT CAST(x AS DOUBLE PRECISION)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        ColumnRef
          column: "x"
          table: null
          schema: null
        type_name: "DOUBLE PRECISION"
""",
        )

    def test_cast_empty_type(self):
        return AstTestBlueprint(
            sql="SELECT CAST(1 AS )",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      CastExpr
        Literal
          literal_type: INTEGER
          source: "1"
        type_name: null
""",
        )


class PtrExpr(TestSuite):
    """PTR (JSON ->) operator tests."""

    def test_ptr_strings(self):
        return AstTestBlueprint(
            sql="SELECT '{\"a\":1}' -> '$.a'",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BinaryExpr
        op: PTR
        Literal
          literal_type: STRING
          source: "'{\\"a\\":1}'"
        Literal
          literal_type: STRING
          source: "'$.a'"
""",
        )

    def test_ptr_column(self):
        return AstTestBlueprint(
            sql="SELECT j -> '$.name'",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      BinaryExpr
        op: PTR
        ColumnRef
          column: "j"
          table: null
          schema: null
        Literal
          literal_type: STRING
          source: "'$.name'"
""",
        )


class QnumberLiteral(TestSuite):
    """QNUMBER (digit-separated number) literal tests."""

    def test_qnumber_integer(self):
        return AstTestBlueprint(
            sql="SELECT 1_000_000",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      Literal
        literal_type: QNUMBER
        source: "1_000_000"
""",
        )

    def test_qnumber_float(self):
        return AstTestBlueprint(
            sql="SELECT 1_000.50",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      Literal
        literal_type: QNUMBER
        source: "1_000.50"
""",
        )


class RowValue(TestSuite):
    """Row value tuple tests."""

    def test_two_elements(self):
        return AstTestBlueprint(
            sql="SELECT (1, 2)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      ExprList[2]
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_three_elements(self):
        return AstTestBlueprint(
            sql="SELECT (1, 2, 3)",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      ExprList[3]
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
        Literal
          literal_type: INTEGER
          source: "3"
""",
        )
