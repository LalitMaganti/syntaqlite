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
