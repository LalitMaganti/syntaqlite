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
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":{"name":"scratch","kind":"table"},"columns":[{"name":"name","index":0,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
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
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":{"name":"scratch","kind":"table"},"columns":[{"name":"name","index":0,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":2,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"name","index":0,"origin":{"table":"scratch","column":"name"}}],"relations":[{"name":"scratch","kind":"table"}],"physical_tables":[{"name":"scratch"}]}
""",
        )
