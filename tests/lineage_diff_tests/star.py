# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: SELECT * expansion."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Star(TestSuite):
    def test_star_expands_to_each_column(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "SELECT * FROM users;\n"
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
                id <- users.id
                name <- users.name
              relations:
                users (table)
              physical_tables:
                users
              partial_reasons: (none)
""",
        )

    def test_star_follows_join_order(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE a (x, y);\n"
                "CREATE TABLE b (x, z);\n"
                "SELECT * FROM a JOIN b USING (x);\n"
            ),
            out="""\
            Lineage
              statement: 0
              status: complete
              target: a (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 1
              status: complete
              target: b (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 2
              status: complete
              target: (none)
              columns:
                x <- a.x
                y <- a.y
                z <- b.z
              relations:
                a (table)
                b (table)
              physical_tables:
                a
                b
              partial_reasons: (none)
            """,
        )

    def test_qualified_star_expands_one_table(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE a (x, y);\n"
                "CREATE TABLE b (x, z);\n"
                "SELECT b.* FROM a, b;\n"
            ),
            out="""\
            Lineage
              statement: 0
              status: complete
              target: a (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 1
              status: complete
              target: b (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 2
              status: complete
              target: (none)
              columns:
                x <- b.x
                z <- b.z
              relations:
                a (table)
                b (table)
              physical_tables:
                a
                b
              partial_reasons: (none)
            """,
        )
