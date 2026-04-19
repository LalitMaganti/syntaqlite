# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class UnknownColumn(TestSuite):
    def test_flagged_on_known_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER); SELECT missing FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'missing'
             --> <stdin>:1:37
              |
            1 | CREATE TABLE t (id INTEGER); SELECT missing FROM t;
              |                                     ^~~~~~~
""",
        )

    def test_known_column_no_error(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER, name TEXT); SELECT id, name FROM t;",
            strict_schema=True,
            out="",
        )

    def test_without_rowid_table_rejects_rowid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE kv (key TEXT, value TEXT) WITHOUT ROWID;\nSELECT rowid FROM kv;",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:2:8
              |
            2 | SELECT rowid FROM kv;
              |        ^~~~~
""",
        )

    def test_regular_table_accepts_rowid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT rowid FROM users;",
            strict_schema=True,
            out="",
        )

    def test_unknown_table_suppresses_column_errors(self):
        return DiffTestBlueprint(
            sql="SELECT id, name, email FROM users WHERE age >= 0",
            strict_schema=True,
            out="""\
            error: unknown table 'users'
             --> <stdin>:1:29
              |
            1 | SELECT id, name, email FROM users WHERE age >= 0
              |                             ^~~~~
""",
        )

    def test_insert_into_unknown_table_no_column_errors(self):
        return DiffTestBlueprint(
            sql="INSERT INTO unknown_tbl(a, b, c) VALUES(1, 2, 3)",
            strict_schema=True,
            out="""\
            error: unknown table 'unknown_tbl'
             --> <stdin>:1:13
              |
            1 | INSERT INTO unknown_tbl(a, b, c) VALUES(1, 2, 3)
              |             ^~~~~~~~~~~
""",
        )

    def test_update_unknown_table_no_column_errors(self):
        return DiffTestBlueprint(
            sql="UPDATE unknown_tbl SET stat='val' WHERE idx='t1a'",
            strict_schema=True,
            out="""\
            error: unknown table 'unknown_tbl'
             --> <stdin>:1:8
              |
            1 | UPDATE unknown_tbl SET stat='val' WHERE idx='t1a'
              |        ^~~~~~~~~~~
""",
        )

    def test_delete_from_unknown_table_no_column_errors(self):
        return DiffTestBlueprint(
            sql="DELETE FROM unknown_tbl WHERE idx='t1a'",
            strict_schema=True,
            out="""\
            error: unknown table 'unknown_tbl'
             --> <stdin>:1:13
              |
            1 | DELETE FROM unknown_tbl WHERE idx='t1a'
              |             ^~~~~~~~~~~
""",
        )

    def test_subquery_with_unknown_inner_table(self):
        return DiffTestBlueprint(
            sql="SELECT sub.id FROM (SELECT id FROM users) AS sub",
            strict_schema=True,
            out="""\
            error: unknown table 'users'
             --> <stdin>:1:36
              |
            1 | SELECT sub.id FROM (SELECT id FROM users) AS sub
              |                                    ^~~~~
""",
        )

    def test_select_true_and_false(self):
        return DiffTestBlueprint(
            sql="SELECT true, False, TRUE, false;",
            strict_schema=True,
            out="",
        )

    def test_attach_schema_name_not_column(self):
        return DiffTestBlueprint(
            sql="ATTACH ':memory:' AS scratch",
            strict_schema=True,
            out="",
        )
