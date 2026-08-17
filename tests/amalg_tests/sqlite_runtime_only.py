# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation integration tests for runtime-only mode.

Verifies that the runtime-only amalgamation (syntaqlite_runtime.{h,c})
compiles and works as a standalone parser using the built-in sqlite dialect.
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SqliteAmalgRuntimeOnly(TestSuite):
    """SQLite parsing through the runtime-only amalgamation."""

    def test_select_literal(self):
        return DiffTestBlueprint(
            sql="SELECT 42",
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
                        source: "42"
              from_clause: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_create_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT NOT NULL)",
            out="""\
            CreateTableStmt
              table_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              flags: (none)
              columns:
                ColumnDefList [2 items]
                  ColumnDef
                    column_name:
                      IdentName
                        source: "id"
                    type_name: "INTEGER"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: PRIMARY_KEY
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_has_parens: FALSE
                          generated_always: FALSE
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
                  ColumnDef
                    column_name:
                      IdentName
                        source: "name"
                    type_name: "TEXT"
                    constraints:
                      ColumnConstraintList [1 items]
                        ColumnConstraint
                          kind: NOT_NULL
                          constraint_name: (none)
                          onconf: DEFAULT
                          sort_order: ASC
                          is_autoincrement: FALSE
                          collation_name: (none)
                          generated_storage: VIRTUAL
                          deferrable: UNSET
                          initial_defer: UNSET
                          default_has_parens: FALSE
                          generated_always: FALSE
                          default_expr: (none)
                          check_expr: (none)
                          generated_expr: (none)
                          fk_clause: (none)
              table_constraints: (none)
              as_select: (none)
""",
        )
