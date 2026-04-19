# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class MultipleStatements(TestSuite):
    def test_independent_selects(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER); SELECT id FROM users; SELECT id FROM users;",
            strict_schema=True,
            out="",
        )

    def test_diagnostics_isolated_per_statement(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER); SELECT id FROM users; SELECT * FROM missing;",
            strict_schema=True,
            out="""\
            error: unknown table 'missing'
             --> <stdin>:1:70
              |
            1 | CREATE TABLE users (id INTEGER); SELECT id FROM users; SELECT * FROM missing;
              |                                                                      ^~~~~~~
""",
        )

    def test_pragma_no_errors(self):
        return DiffTestBlueprint(
            sql="PRAGMA journal_mode;",
            strict_schema=True,
            out="",
        )

    def test_empty_source_no_errors(self):
        return DiffTestBlueprint(
            sql="",
            strict_schema=True,
            out="",
        )

    def test_clean_select_no_diagnostics(self):
        return DiffTestBlueprint(
            sql="SELECT 1;",
            strict_schema=True,
            out="",
        )

    def test_parse_error_reported(self):
        return DiffTestBlueprint(
            sql="SELECT;",
            strict_schema=True,
            out="""\
            error: syntax error near ';'
             --> <stdin>:1:7
              |
            1 | SELECT;
              |       ^
""",
        )
