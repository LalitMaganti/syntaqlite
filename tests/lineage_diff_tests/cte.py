# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: CTEs are transparent — origin traces through to base tables."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Cte(TestSuite):
    def test_cte_single_reference(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "WITH u AS (SELECT id, name FROM users) SELECT name FROM u;\n"
            ),
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"name","index":0,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
""",
        )

    def test_cte_rename_passthrough(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE t (a INTEGER);\n"
                "WITH c(renamed) AS (SELECT a FROM t) SELECT renamed FROM c;\n"
            ),
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"t","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"renamed","index":0,"origin":null}],"relations":[{"name":"t","kind":"table"}],"physical_tables":[{"name":"t"}]}
""",
        )
