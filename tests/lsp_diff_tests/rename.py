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

    def test_prepare_rename_on_table_definition(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>users (id INT);\nSELECT * FROM users;",
            op="prepare-rename",
            out="""\
            0:13..0:18 "users"
""",
        )

    def test_prepare_rename_on_column_definition(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (<|>id INT);\nSELECT id FROM users;",
            op="prepare-rename",
            out="""\
            0:20..0:22 "id"
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

    def test_rename_table_renames_create_index_target(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>users (id INT);\nCREATE INDEX idx ON users (id);",
            op="rename",
            new_name="accounts",
            out="""\
            0:13..0:18 -> "accounts"
            1:20..1:25 -> "accounts"
""",
        )

    def test_rename_column_renames_create_index_columns(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>id INT);\nCREATE INDEX i ON t (id) WHERE id > 0;",
            op="rename",
            new_name="key",
            out="""\
            0:16..0:18 -> "key"
            1:21..1:23 -> "key"
            1:31..1:33 -> "key"
""",
        )

    def test_rename_column_renames_check(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>qty INT CHECK (qty > 0));",
            op="rename",
            new_name="amount",
            out="""\
            0:16..0:19 -> "amount"
            0:31..0:34 -> "amount"
""",
        )

    def test_rename_column_renames_primary_key(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>id INT, PRIMARY KEY (id));",
            op="rename",
            new_name="key",
            out="""\
            0:16..0:18 -> "key"
            0:37..0:39 -> "key"
""",
        )

    def test_rename_table_renames_references(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>p (id INT);\nCREATE TABLE c (pid INT REFERENCES p (id));",
            op="rename",
            new_name="parent",
            out="""\
            0:13..0:14 -> "parent"
            1:35..1:36 -> "parent"
""",
        )

    def test_rename_table_renames_drop_target(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>t (a INT);\nDROP TABLE t;",
            op="rename",
            new_name="u",
            out="""\
            0:13..0:14 -> "u"
            1:11..1:12 -> "u"
""",
        )

    def test_rename_column_through_table_alias(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>qty INT);\nSELECT c.qty FROM t c;",
            op="rename",
            new_name="amount",
            out="""\
            0:16..0:19 -> "amount"
            1:9..1:12 -> "amount"
""",
        )
