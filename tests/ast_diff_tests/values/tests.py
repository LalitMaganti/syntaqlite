# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""VALUES clause AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class ValuesClause(TestSuite):
    """VALUES clause tests."""

    def test_single_row(self):
        return AstTestBlueprint(
            sql="VALUES (1, 2, 3)",
            out="""\
ValuesClause
  ValuesRowList[1]
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

    def test_multi_row(self):
        return AstTestBlueprint(
            sql="VALUES (1, 2), (3, 4)",
            out="""\
ValuesClause
  ValuesRowList[2]
    ExprList[2]
      Literal
        literal_type: INTEGER
        source: "1"
      Literal
        literal_type: INTEGER
        source: "2"
    ExprList[2]
      Literal
        literal_type: INTEGER
        source: "3"
      Literal
        literal_type: INTEGER
        source: "4"
""",
        )

    def test_three_rows(self):
        return AstTestBlueprint(
            sql="VALUES (1), (2), (3)",
            out="""\
ValuesClause
  ValuesRowList[3]
    ExprList[1]
      Literal
        literal_type: INTEGER
        source: "1"
    ExprList[1]
      Literal
        literal_type: INTEGER
        source: "2"
    ExprList[1]
      Literal
        literal_type: INTEGER
        source: "3"
""",
        )

    def test_with_expressions(self):
        return AstTestBlueprint(
            sql="VALUES (1+2, 'hello')",
            out="""\
ValuesClause
  ValuesRowList[1]
    ExprList[2]
      BinaryExpr
        op: PLUS
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
      Literal
        literal_type: STRING
        source: "'hello'"
""",
        )

    def test_in_compound(self):
        return AstTestBlueprint(
            sql="SELECT 1 UNION VALUES (2)",
            out="""\
CompoundSelect
  op: UNION
  SelectStmt
    flags: 0
    ResultColumnList[1]
      ResultColumn
        flags: 0
        alias: null
        Literal
          literal_type: INTEGER
          source: "1"
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )
