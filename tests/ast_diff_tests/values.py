# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""VALUES clause AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class ValuesClause(TestSuite):
    """VALUES clause tests."""

    def test_single_row(self):
        return DiffTestBlueprint(
            sql="VALUES (1, 2, 3)",
            out="""\
            ValuesClause
              rows:
                ValuesRowList [1 items]
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
""",
        )

    def test_multi_row(self):
        return DiffTestBlueprint(
            sql="VALUES (1, 2), (3, 4)",
            out="""\
            ValuesClause
              rows:
                ValuesRowList [2 items]
                  ExprList [2 items]
                    Literal
                      literal_type: INTEGER
                      source: "1"
                    Literal
                      literal_type: INTEGER
                      source: "2"
                  ExprList [2 items]
                    Literal
                      literal_type: INTEGER
                      source: "3"
                    Literal
                      literal_type: INTEGER
                      source: "4"
""",
        )

    def test_three_rows(self):
        return DiffTestBlueprint(
            sql="VALUES (1), (2), (3)",
            out="""\
            ValuesClause
              rows:
                ValuesRowList [3 items]
                  ExprList [1 items]
                    Literal
                      literal_type: INTEGER
                      source: "1"
                  ExprList [1 items]
                    Literal
                      literal_type: INTEGER
                      source: "2"
                  ExprList [1 items]
                    Literal
                      literal_type: INTEGER
                      source: "3"
""",
        )

    def test_with_expressions(self):
        return DiffTestBlueprint(
            sql="VALUES (1+2, 'hello')",
            out="""\
            ValuesClause
              rows:
                ValuesRowList [1 items]
                  ExprList [2 items]
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
                    Literal
                      literal_type: STRING
                      source: "'hello'"
""",
        )

    def test_in_compound(self):
        return DiffTestBlueprint(
            sql="SELECT 1 UNION VALUES (2)",
            out="""\
            CompoundSelect
              op: UNION
              left:
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
              right:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "2"
              orderby: (none)
              limit_clause: (none)
""",
        )
