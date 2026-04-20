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
