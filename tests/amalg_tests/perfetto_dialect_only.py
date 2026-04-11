# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation integration tests for Perfetto dialect-only mode.

Verifies that the dialect-only amalgamation (syntaqlite_perfetto.{h,c}
referencing an external syntaqlite_runtime.{h,c}) compiles and parses
correctly when the runtime is provided separately at compile time.
This is the primary test for the dialect-only build mode since extension
dialects are the meaningful use case.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class PerfettoAmalgDialectOnly(TestSuite):
    """Perfetto extension parsing through dialect-only amalgamation + external runtime."""

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

    def test_create_perfetto_table(self):
        return DiffTestBlueprint(
            sql="CREATE PERFETTO TABLE foo AS SELECT 1",
            out="""\
            CreatePerfettoTableStmt
              table_name: "foo"
              or_replace: FALSE
              schema: (none)
              select:
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
              select_span: "SELECT 1"
""",
        )
