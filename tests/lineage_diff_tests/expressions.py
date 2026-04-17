# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: transformed columns have origin=null."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Expressions(TestSuite):
    """Expression-produced columns carry no origin."""

    def test_arithmetic_has_no_origin(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER, b INTEGER);\nSELECT a + b FROM t;\n",
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
                a + b <- (transformed)
              relations:
                t (table)
              physical_tables:
                t
              partial_reasons: (none)
""",
        )

    def test_cast_has_no_origin(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER);\nSELECT CAST(a AS TEXT) FROM t;\n",
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
                a as text <- (transformed)
              relations:
                t (table)
              physical_tables:
                t
              partial_reasons: (none)
""",
        )

    def test_aggregate_has_no_origin(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER);\nSELECT SUM(a) FROM t;\n",
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
                sum(a <- (transformed)
              relations:
                t (table)
              physical_tables:
                t
              partial_reasons: (none)
""",
        )

    def test_literal_has_no_origin(self):
        return DiffTestBlueprint(
            sql="SELECT 1 AS x, 'lit' AS y;\n",
            out="""\
            Lineage
              statement: 0
              status: complete
              target: (none)
              columns:
                x <- (transformed)
                y <- (transformed)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
""",
        )
