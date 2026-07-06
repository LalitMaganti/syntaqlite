# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Column reference expression AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class ColumnRefBasic(TestSuite):
    """Column reference tests."""

    def test_simple_column(self):
        return DiffTestBlueprint(
            sql="SELECT x",
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
                        column: "x"
                        table: (none)
                        schema: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_qualified_column(self):
        return DiffTestBlueprint(
            sql="SELECT t.x",
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
                        column: "x"
                        table: "t"
                        schema: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_fully_qualified_column(self):
        return DiffTestBlueprint(
            sql="SELECT s.t.x",
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
                        column: "x"
                        table: "t"
                        schema: "s"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_multiple_columns(self):
        return DiffTestBlueprint(
            sql="SELECT a, b, c",
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
                      ColumnRef
                        column: "c"
                        table: (none)
                        schema: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_table_star(self):
        return DiffTestBlueprint(
            sql="SELECT t.*",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr:
                      IdentName
                        source: "t"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_mixed_expressions(self):
        return DiffTestBlueprint(
            sql="SELECT a, t.b, 1 + x",
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
                        table: "t"
                        schema: (none)
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
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class ColumnRefQuoting(TestSuite):
    """Quoted-identifier spans dump as the identifier value.

    The parser strips enclosing quotes from all four styles ("x", [x],
    `x`, 'x'); the dump additionally collapses SQLite's doubled-quote
    escape so the same name dumps identically however it was encoded.
    """

    def test_single_quoted_column_dumps_bare(self):
        return DiffTestBlueprint(
            sql="SELECT 'x' FROM ('x')",
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
                        source: "'x'"
              from_clause:
                TableRef
                  table_name: "x"
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

    def test_escaped_quotes_dump_identifier_value(self):
        return DiffTestBlueprint(
            sql='SELECT "a""b" FROM \'a\'\'b\'',
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
                        column: "a"b"
                        table: (none)
                        schema: (none)
              from_clause:
                TableRef
                  table_name: "a'b"
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
