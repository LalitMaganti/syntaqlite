# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: joins trace origins through each joined source."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Joins(TestSuite):
    def test_inner_join_qualified(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "CREATE TABLE orders (id INTEGER, user_id INTEGER);\n"
                "SELECT users.name, orders.id FROM users JOIN orders ON users.id = orders.user_id;\n"
            ),
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"users","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":{"name":"orders","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":2,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"name","index":0,"origin":{"table":"users","column":"name"}},{"name":"id","index":1,"origin":{"table":"orders","column":"id"}}],"relations":[{"name":"orders","kind":"table"},{"name":"users","kind":"table"}],"physical_tables":[{"name":"orders"},{"name":"users"}]}
""",
        )

    def test_two_table_relations(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE a (x INTEGER);\n"
                "CREATE TABLE b (y INTEGER);\n"
                "SELECT a.x, b.y FROM a, b;\n"
            ),
            out="""\
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":0,"status":"complete","partial_reasons":[],"target":{"name":"a","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":1,"status":"complete","partial_reasons":[],"target":{"name":"b","kind":"table"},"columns":[],"relations":[],"physical_tables":[]}
            {"kind":"lineage","schema_version":0,"file":"<stdin>","statement_index":2,"status":"complete","partial_reasons":[],"target":null,"columns":[{"name":"x","index":0,"origin":{"table":"a","column":"x"}},{"name":"y","index":1,"origin":{"table":"b","column":"y"}}],"relations":[{"name":"a","kind":"table"},{"name":"b","kind":"table"}],"physical_tables":[{"name":"a"},{"name":"b"}]}
""",
        )
