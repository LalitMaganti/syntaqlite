# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Miscellaneous expression AST tests: variables, COLLATE, CTIME_KW."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class BindParameters(TestSuite):
    """Bind parameter (VARIABLE) tests."""

    def test_question_mark(self):
        return DiffTestBlueprint(
            sql="SELECT ?",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Variable
                        source: "?"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_numbered_parameter(self):
        return DiffTestBlueprint(
            sql="SELECT ?1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Variable
                        source: "?1"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_named_colon(self):
        return DiffTestBlueprint(
            sql="SELECT :name",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Variable
                        source: ":name"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_named_at(self):
        return DiffTestBlueprint(
            sql="SELECT @name",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Variable
                        source: "@name"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_named_dollar(self):
        return DiffTestBlueprint(
            sql="SELECT $name",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Variable
                        source: "$name"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class CollateExpressions(TestSuite):
    """COLLATE expression tests."""

    def test_collate_nocase(self):
        return DiffTestBlueprint(
            sql="SELECT 1 COLLATE NOCASE",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      CollateExpr
                        expr:
                          Literal
                            literal_type: INTEGER
                            source: "1"
                        collation: "NOCASE"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_collate_binary(self):
        return DiffTestBlueprint(
            sql="SELECT 'hello' COLLATE BINARY",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      CollateExpr
                        expr:
                          Literal
                            literal_type: STRING
                            source: "'hello'"
                        collation: "BINARY"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class DateTimeKeywords(TestSuite):
    """CTIME_KW (CURRENT_TIME, CURRENT_DATE, CURRENT_TIMESTAMP) tests."""

    def test_current_time(self):
        return DiffTestBlueprint(
            sql="SELECT CURRENT_TIME",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Literal
                        literal_type: CURRENT
                        source: "CURRENT_TIME"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_current_date(self):
        return DiffTestBlueprint(
            sql="SELECT CURRENT_DATE",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Literal
                        literal_type: CURRENT
                        source: "CURRENT_DATE"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_current_timestamp(self):
        return DiffTestBlueprint(
            sql="SELECT CURRENT_TIMESTAMP",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    alias_as: FALSE
                    expr:
                      Literal
                        literal_type: CURRENT
                        source: "CURRENT_TIMESTAMP"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
