# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Definition(TestSuite):
    def test_ddl_table_reference(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER); SELECT id FROM <|>users;",
            op="definition",
            out="""\
target: 0:13..0:18
""",
        )

    def test_cte_reference(self):
        return LspDiffTestBlueprint(
            sql="WITH cte AS (SELECT 1) SELECT * FROM <|>cte;",
            op="definition",
            out="""\
target: 0:5..0:8
""",
        )

    def test_cte_shadows_ddl(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER); WITH t AS (SELECT 1 AS id) SELECT * FROM <|>t;",
            op="definition",
            out="""\
            target: 0:34..0:35
""",
        )

    def test_unknown_table_returns_none(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM <|>nonexistent;",
            op="definition",
            out="""\
(no definition)
""",
        )

    def test_column_in_ddl_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT <|>name FROM users;",
            op="definition",
            out="""\
target: 0:32..0:36
""",
        )

    def test_unknown_column_returns_none(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (a INT);\nSELECT <|>b FROM t;",
            op="definition",
            out="""\
(no definition)
""",
        )

    def test_cte_column_inferred_from_alias(self):
        return LspDiffTestBlueprint(
            sql="WITH foo AS (SELECT 1 AS a)\nSELECT <|>a FROM foo;",
            op="definition",
            out="""\
            target: 0:25..0:26
""",
        )

    def test_cte_column_from_declared_list(self):
        return LspDiffTestBlueprint(
            sql="WITH foo(x) AS (SELECT 1)\nSELECT <|>x FROM foo;",
            op="definition",
            out="""\
            target: 0:9..0:10
""",
        )

    def test_create_index_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER); CREATE INDEX idx ON <|>users (id);",
            op="definition",
            out="""\
target: 0:13..0:18
""",
        )

    def test_create_index_column(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER); CREATE INDEX i ON t (<|>id);",
            op="definition",
            out="""\
target: 0:16..0:18
""",
        )

    def test_check_column(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b INT CHECK (<|>a < b));",
            op="definition",
            out="""\
target: 0:16..0:17
""",
        )

    def test_references_parent_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE p (id INT); CREATE TABLE c (pid INT REFERENCES <|>p (id));",
            op="definition",
            out="""\
target: 0:13..0:14
""",
        )

    def test_self_referencing_foreign_key_column(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (id INT, parent INT REFERENCES t (<|>id));",
            op="definition",
            out="""\
target: 0:16..0:18
""",
        )

    def test_column_through_table_alias(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (qty INT); SELECT c.<|>qty FROM t c;",
            op="definition",
            out="""\
target: 0:16..0:19
""",
        )

    def test_view_declared_column(self):
        return LspDiffTestBlueprint(
            sql="CREATE VIEW v (a) AS SELECT 1 AS x; SELECT <|>a FROM v;",
            op="definition",
            out="""\
target: 0:15..0:16
""",
        )

    def test_cte_column_passed_through(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (qty INT); WITH w AS (SELECT qty FROM t) SELECT <|>qty FROM w;",
            op="definition",
            out="""\
target: 0:44..0:47
""",
        )
