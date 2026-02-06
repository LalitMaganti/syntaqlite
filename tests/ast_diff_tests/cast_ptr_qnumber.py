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
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: Literal
          literal_type: INTEGER
          source: "1"
        type_name: "INTEGER"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cast_text(self):
        return AstTestBlueprint(
            sql="SELECT CAST('hello' AS TEXT)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: Literal
          literal_type: STRING
          source: "'hello'"
        type_name: "TEXT"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cast_real(self):
        return AstTestBlueprint(
            sql="SELECT CAST(3.14 AS REAL)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: Literal
          literal_type: FLOAT
          source: "3.14"
        type_name: "REAL"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cast_varchar_precision(self):
        return AstTestBlueprint(
            sql="SELECT CAST(x AS VARCHAR(100))",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: ColumnRef
          column: "x"
          table: null
          schema: null
        type_name: "VARCHAR(100)"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cast_decimal_scale(self):
        return AstTestBlueprint(
            sql="SELECT CAST(x AS DECIMAL(10,2))",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: ColumnRef
          column: "x"
          table: null
          schema: null
        type_name: "DECIMAL(10,2)"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cast_multi_word_type(self):
        return AstTestBlueprint(
            sql="SELECT CAST(x AS DOUBLE PRECISION)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: ColumnRef
          column: "x"
          table: null
          schema: null
        type_name: "DOUBLE PRECISION"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cast_empty_type(self):
        return AstTestBlueprint(
            sql="SELECT CAST(1 AS )",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: CastExpr
        expr: Literal
          literal_type: INTEGER
          source: "1"
        type_name: null
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class PtrExpr(TestSuite):
    """PTR (JSON ->) operator tests."""

    def test_ptr_strings(self):
        return AstTestBlueprint(
            sql="SELECT '{\"a\":1}' -> '$.a'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: PTR
        left: Literal
          literal_type: STRING
          source: "'{\\"a\\":1}'"
        right: Literal
          literal_type: STRING
          source: "'$.a'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_ptr_column(self):
        return AstTestBlueprint(
            sql="SELECT j -> '$.name'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: PTR
        left: ColumnRef
          column: "j"
          table: null
          schema: null
        right: Literal
          literal_type: STRING
          source: "'$.name'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class QnumberLiteral(TestSuite):
    """QNUMBER (digit-separated number) literal tests."""

    def test_qnumber_integer(self):
        return AstTestBlueprint(
            sql="SELECT 1_000_000",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: QNUMBER
        source: "1_000_000"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_qnumber_float(self):
        return AstTestBlueprint(
            sql="SELECT 1_000.50",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: QNUMBER
        source: "1_000.50"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class RowValue(TestSuite):
    """Row value tuple tests."""

    def test_two_elements(self):
        return AstTestBlueprint(
            sql="SELECT (1, 2)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: ExprList[2]
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_three_elements(self):
        return AstTestBlueprint(
            sql="SELECT (1, 2, 3)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: ExprList[3]
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
        Literal
          literal_type: INTEGER
          source: "3"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
