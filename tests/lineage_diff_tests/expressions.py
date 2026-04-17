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
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"t","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"a + b","index":0,"origin":null}],"relations":[{"name":"t","kind":"table"}],"physical_tables":[{"name":"t"}]}
""",
        )

    def test_cast_has_no_origin(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER);\nSELECT CAST(a AS TEXT) FROM t;\n",
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"t","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"a as text","index":0,"origin":null}],"relations":[{"name":"t","kind":"table"}],"physical_tables":[{"name":"t"}]}
""",
        )

    def test_aggregate_has_no_origin(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER);\nSELECT SUM(a) FROM t;\n",
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"t","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"sum(a","index":0,"origin":null}],"relations":[{"name":"t","kind":"table"}],"physical_tables":[{"name":"t"}]}
""",
        )

    def test_literal_has_no_origin(self):
        return DiffTestBlueprint(
            sql="SELECT 1 AS x, 'lit' AS y;\n",
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"x","index":0,"origin":null},{"name":"y","index":1,"origin":null}],"relations":[],"physical_tables":[]}
""",
        )
