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

    def test_table_in_create_index(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>users (id INT);\nCREATE INDEX idx ON users (id);",
            op="references",
            include_declaration=False,
            out="""\
            1:20..1:25
""",
        )

    def test_column_in_create_index(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>id INT, n INT);\nCREATE INDEX i ON t (id, n) WHERE id > 0;",
            op="references",
            include_declaration=False,
            out="""\
            1:21..1:23
            1:34..1:36
""",
        )

    def test_column_in_check_and_generated(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>qty INT CHECK (qty > 0), total AS (qty * 2));",
            op="references",
            include_declaration=False,
            out="""\
            0:31..0:34
            0:51..0:54
""",
        )

    def test_column_in_table_constraints(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>id INT, PRIMARY KEY (id), UNIQUE (id), CHECK (id > 0));",
            op="references",
            include_declaration=False,
            out="""\
            0:37..0:39
            0:50..0:52
            0:62..0:64
""",
        )

    def test_column_in_foreign_key(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>pid INT, FOREIGN KEY (pid) REFERENCES p (id));",
            op="references",
            include_declaration=False,
            out="""\
            0:38..0:41
""",
        )

    def test_parent_column_in_references(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE p (<|>id INT);\nCREATE TABLE c (pid INT REFERENCES p (id), FOREIGN KEY (pid) REFERENCES p (id));",
            op="references",
            include_declaration=False,
            out="""\
            1:38..1:40
            1:75..1:77
""",
        )

    def test_parent_table_in_references(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>p (id INT);\nCREATE TABLE c (pid INT REFERENCES p (id));",
            op="references",
            include_declaration=False,
            out="""\
            1:35..1:36
""",
        )

    def test_column_through_table_alias(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>qty INT);\nSELECT c.qty FROM t c;\nSELECT qty FROM t AS c;",
            op="references",
            include_declaration=False,
            out="""\
            1:9..1:12
            2:7..2:10
""",
        )

    def test_column_through_self_join_aliases(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>id INT);\nSELECT a.id FROM t a JOIN t b ON a.id = b.id;",
            op="references",
            include_declaration=False,
            out="""\
            1:35..1:37
            1:42..1:44
            1:9..1:11
""",
        )

    def test_table_and_column_in_trigger(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE t (<|>qty INT);\nCREATE TRIGGER trg AFTER UPDATE OF qty ON t WHEN OLD.qty <> NEW.qty BEGIN SELECT NEW.qty; END;",
            op="references",
            include_declaration=False,
            out="""\
            1:35..1:38
            1:53..1:56
            1:64..1:67
            1:85..1:88
""",
        )

    def test_trigger_target_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE <|>t (qty INT);\nCREATE TRIGGER trg AFTER INSERT ON t BEGIN SELECT 1; END;",
            op="references",
            include_declaration=False,
            out="""\
            1:35..1:36
""",
        )
