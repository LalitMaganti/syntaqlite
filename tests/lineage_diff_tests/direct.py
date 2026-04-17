# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: direct column references."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Direct(TestSuite):
    """Direct column reference lineage."""

    def test_single_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT name FROM users;\n",
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

    def test_multiple_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT id, name FROM users;\n",
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

    def test_alias_is_passthrough(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER);\nSELECT a AS aa FROM t;\n",
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
                aa <- t.a
              relations:
                t (table)
              physical_tables:
                t
              partial_reasons: (none)
""",
        )
