# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Rename(TestSuite):
    def test_prepare_rename_for_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INT);\nSELECT * FROM <|>users;",
            op="prepare-rename",
            out="""\
            1:14..1:19 "users"
""",
        )

    def test_rename_table_in_single_file(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INT);\nSELECT * FROM <|>users;\nDELETE FROM users;",
            op="rename",
            new_name="accounts",
            out="""\
            0:13..0:18 -> "accounts"
            1:14..1:19 -> "accounts"
            2:12..2:17 -> "accounts"
""",
        )

    def test_rename_column_in_single_file(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (id INT, name TEXT);\nSELECT <|>id FROM t;\nSELECT id, name FROM t;",
            op="rename",
            new_name="user_id",
            out="""\
            0:16..0:18 -> "user_id"
            1:7..1:9 -> "user_id"
            2:7..2:9 -> "user_id"
""",
        )

    def test_prepare_rename_on_unknown_returns_none(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM <|>nonexistent;",
            op="prepare-rename",
            out="""\
(no rename)
""",
        )
