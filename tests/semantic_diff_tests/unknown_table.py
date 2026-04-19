# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class UnknownTable(TestSuite):
    def test_strict_is_error(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM missing_table",
            strict_schema=True,
            out="""\
            error: unknown table 'missing_table'
             --> <stdin>:1:15
              |
            1 | SELECT * FROM missing_table
              |               ^~~~~~~~~~~~~
""",
        )

    def test_lenient_is_warning(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM missing_table",
            out="""\
            warning: unknown table 'missing_table'
             --> <stdin>:1:15
              |
            1 | SELECT * FROM missing_table
              |               ^~~~~~~~~~~~~
""",
        )

    def test_fuzzy_suggestion(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER); SELECT * FROM usres",
            strict_schema=True,
            out="""\
            error: unknown table 'usres'
             --> <stdin>:1:48
              |
            1 | CREATE TABLE users (id INTEGER); SELECT * FROM usres
              |                                                ^~~~~
              = help: did you mean 'users'?
""",
        )

    def test_virtual_table_not_flagged(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE fts USING fts5(content);\nSELECT * FROM fts;",
            strict_schema=True,
            out="",
        )

    def test_known_table_no_error(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT); SELECT id FROM users;",
            strict_schema=True,
            out="",
        )

    def test_two_unknown_tables_in_join(self):
        return DiffTestBlueprint(
            sql="SELECT u.id, o.total FROM users u JOIN orders o ON u.id = o.user_id",
            strict_schema=True,
            out="""\
            error: unknown table 'users'
             --> <stdin>:1:27
              |
            1 | SELECT u.id, o.total FROM users u JOIN orders o ON u.id = o.user_id
              |                           ^~~~~
            error: unknown table 'orders'
             --> <stdin>:1:40
              |
            1 | SELECT u.id, o.total FROM users u JOIN orders o ON u.id = o.user_id
              |                                        ^~~~~~
""",
        )
