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
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"a","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":{"name":"b","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":2,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"x","index":0,"origin":{"table":"a","column":"x"}}],"relations":[{"name":"a","kind":"table"}],"physical_tables":[{"name":"a"}]}
""",
        )
