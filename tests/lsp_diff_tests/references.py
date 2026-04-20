# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class References(TestSuite):
    def test_table_in_single_file(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INT);\nSELECT * FROM <|>users;\nDELETE FROM users;",
            op="references",
            include_declaration=False,
            out="""\
            1:14..1:19
            2:12..2:17
""",
        )

    def test_table_include_declaration(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INT);\nSELECT * FROM <|>users;\nDELETE FROM users;",
            op="references",
            out="""\
            0:13..0:18
            1:14..1:19
            2:12..2:17
""",
        )

    def test_column_in_single_file(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (id INT, name TEXT);\nSELECT <|>id FROM t;\nSELECT id, name FROM t;",
            op="references",
            include_declaration=False,
            out="""\
            1:7..1:9
            2:7..2:9
""",
        )

    def test_cursor_on_definition(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>users (id INT);\nSELECT * FROM users;",
            op="references",
            include_declaration=False,
            out="""\
            1:14..1:19
""",
        )

    def test_cursor_on_definition_include_declaration(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>users (id INT);\nSELECT * FROM users;",
            op="references",
            out="""\
            0:13..0:18
            1:14..1:19
""",
        )
