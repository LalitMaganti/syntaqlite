# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class SelectBasic(TestSuite):
    """Basic SELECT statement tests."""

    def test_integer_literal(self):
        return AstTestBlueprint(
            sql="SELECT 1",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: INTEGER
        source: "1"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_float_literal(self):
        return AstTestBlueprint(
            sql="SELECT 3.14",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: FLOAT
        source: "3.14"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_string_literal(self):
        return AstTestBlueprint(
            sql="SELECT 'hello'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: STRING
        source: "'hello'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_null_literal(self):
        return AstTestBlueprint(
            sql="SELECT NULL",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: Literal
        literal_type: NULL
        source: "NULL"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_binary_plus(self):
        return AstTestBlueprint(
            sql="SELECT 1 + 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: PLUS
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
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

    def test_binary_star(self):
        return AstTestBlueprint(
            sql="SELECT 3 * 4",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: STAR
        left: Literal
          literal_type: INTEGER
          source: "3"
        right: Literal
          literal_type: INTEGER
          source: "4"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_binary_lt(self):
        return AstTestBlueprint(
            sql="SELECT 1 < 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: LT
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
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

    def test_binary_eq(self):
        return AstTestBlueprint(
            sql="SELECT 1 = 2",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: EQ
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
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

    def test_binary_and(self):
        return AstTestBlueprint(
            sql="SELECT 1 AND 0",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: AND
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: INTEGER
          source: "0"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_binary_or(self):
        return AstTestBlueprint(
            sql="SELECT 1 OR 0",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: OR
        left: Literal
          literal_type: INTEGER
          source: "1"
        right: Literal
          literal_type: INTEGER
          source: "0"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_binary_concat(self):
        return AstTestBlueprint(
            sql="SELECT 'a' || 'b'",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: BinaryExpr
        op: CONCAT
        left: Literal
          literal_type: STRING
          source: "'a'"
        right: Literal
          literal_type: STRING
          source: "'b'"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_unary_minus(self):
        return AstTestBlueprint(
            sql="SELECT -5",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: UnaryExpr
        op: MINUS
        operand: Literal
          literal_type: INTEGER
          source: "5"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_unary_not(self):
        return AstTestBlueprint(
            sql="SELECT NOT 1",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: (none)
      alias: null
      expr: UnaryExpr
        op: NOT
        operand: Literal
          literal_type: INTEGER
          source: "1"
  from_clause: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
