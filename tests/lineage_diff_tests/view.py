# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: views.

In-source view definitions are expanded transitively.
Views registered without a body appear as unexpanded_views (status=partial).
"""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class View(TestSuite):
    def test_view_defined_inline_is_expanded(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "CREATE VIEW u AS SELECT id, name FROM users;\n"
                "SELECT name FROM u;\n"
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
              target: u (view)
              columns:
                id <- users.id
                name <- users.name
              relations:
                users (table)
              physical_tables:
                users
              partial_reasons: (none)
            Lineage
              statement: 2
              status: partial
              target: (none)
              columns:
                name <- (transformed)
              relations:
                u (view)
              physical_tables:
                u
              partial_reasons:
                unexpanded_view: u
""",
        )
