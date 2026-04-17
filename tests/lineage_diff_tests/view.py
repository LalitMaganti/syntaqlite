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
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":{"name":"u","kind":"view"},"columns":[{"name":"id","index":0,"origin":{"table":"users","column":"id"}},{"name":"name","index":1,"origin":{"table":"users","column":"name"}}],"relations":[{"name":"users","kind":"table"}],"physical_tables":[{"name":"users"}]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":2,"status":"partial","partial_reasons":[{"code":"unexpanded_view","view":"u"}],"target":null,"columns":[{"name":"name","index":0,"origin":null}],"relations":[{"name":"u","kind":"view"}],"physical_tables":[{"name":"u"}]}
""",
        )
