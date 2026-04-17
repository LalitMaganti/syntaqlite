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
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"id","index":0,"origin":{"table":"users","column":"id"}},{"name":"name","index":1,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
""",
        )
