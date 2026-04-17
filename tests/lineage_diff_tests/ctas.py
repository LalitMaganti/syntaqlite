# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: CREATE TABLE AS SELECT — target + body lineage."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Ctas(TestSuite):
    def test_ctas_target_and_sources(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "CREATE TABLE scratch AS SELECT name FROM users;\n"
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
              target: scratch (table)
              columns:
                name <- users.name
              relations:
                users (table)
              physical_tables:
                users
              partial_reasons: (none)
""",
        )

    def test_temp_table_is_opaque_boundary(self):
        """A temp table created inline is a materialization boundary.

        Reads from it stop there rather than chaining back to the source.
        """
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "CREATE TEMP TABLE scratch AS SELECT name FROM users;\n"
                "SELECT name FROM scratch;\n"
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
              target: scratch (table)
              columns:
                name <- users.name
              relations:
                users (table)
              physical_tables:
                users
              partial_reasons: (none)
            Lineage
              statement: 2
              status: complete
              target: (none)
              columns:
                name <- scratch.name
              relations:
                scratch (table)
              physical_tables:
                scratch
              partial_reasons: (none)
""",
        )
