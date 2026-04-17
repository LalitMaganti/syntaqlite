# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: UNION/INTERSECT columns are merges — origin=null."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Union(TestSuite):
    def test_union_all_columns_have_no_origin(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE a (x INTEGER);\n"
                "CREATE TABLE b (y INTEGER);\n"
                "SELECT x FROM a UNION ALL SELECT y FROM b;\n"
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
              relations:
                a (table)
              physical_tables:
                a
              partial_reasons: (none)
""",
        )
