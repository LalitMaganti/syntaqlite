# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: CTEs are transparent — origin traces through to base tables."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Cte(TestSuite):
    def test_cte_single_reference(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "WITH u AS (SELECT id, name FROM users) SELECT name FROM u;\n"
            ),
            out="""\
            Lineage
              statement: 0
              status: complete
              target: users (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 1
              status: complete
              target: (none)
              columns:
                name <- users.name
              relations:
                users (table)
              physical_tables:
                users
              partial_reasons: (none)
""",
        )

    def test_cte_rename_passthrough(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE t (a INTEGER);\n"
                "WITH c(renamed) AS (SELECT a FROM t) SELECT renamed FROM c;\n"
            ),
            out="""\
            Lineage
              statement: 0
              status: complete
              target: t (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 1
              status: complete
              target: (none)
              columns:
                renamed <- (transformed)
              relations:
                t (table)
              physical_tables:
                t
              partial_reasons: (none)
""",
        )
