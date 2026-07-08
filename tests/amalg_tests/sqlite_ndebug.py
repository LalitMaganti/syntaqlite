# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation integration tests for release (-DNDEBUG) builds.

The full amalgamation must compile with -DNDEBUG, the mode downstream
consumers use for release builds. Lemon guards ParseTrace behind
`#ifndef NDEBUG`, so the inline dispatch macros must degrade to no-ops
in release builds rather than calling an undeclared symbol.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SqliteAmalgNdebug(TestSuite):
    """SQLite parsing through a full amalgamation compiled with -DNDEBUG."""

    def test_select_literal(self):
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
