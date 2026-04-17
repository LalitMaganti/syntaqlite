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
{"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
{"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"name","index":0,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
""",
        )

    def test_multiple_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT id, name FROM users;\n",
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"id","index":0,"origin":{"table":"users","column":"id"}},{"name":"name","index":1,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
""",
        )

    def test_alias_is_passthrough(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER);\nSELECT a AS aa FROM t;\n",
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"t","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"aa","index":0,"origin":{"table":"t","column":"a"}}],"relations":[{"name":"t","kind":"table"}],"physical_tables":[{"name":"t"}]}
""",
        )
