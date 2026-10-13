# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class InsertValues(TestSuite):
    def test_too_few_values_without_column_list(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t VALUES(1);",
            out="""\
            error: table 't' has 2 columns but 1 values were supplied
             --> <stdin>:1:38
              |
            1 | CREATE TABLE t (a, b); INSERT INTO t VALUES(1);
              |                                      ^~~~~~~~~
            """,
        )

    def test_too_many_values_for_column_list(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t(a) VALUES(1, 2);",
            out="""\
            error: 2 values for 1 columns
             --> <stdin>:1:41
              |
            1 | CREATE TABLE t (a, b); INSERT INTO t(a) VALUES(1, 2);
              |                                         ^~~~~~~~~~~~
            """,
        )

    def test_matching_counts(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t VALUES(1, 2), (3, 4); INSERT INTO t(b) VALUES(1);",
            out="",
        )

    def test_generated_columns_are_not_filled(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b AS (a * 2)); INSERT INTO t VALUES(1); INSERT INTO t VALUES(1, 2);",
            out="""\
            error: table 't' has 1 columns but 2 values were supplied
             --> <stdin>:1:74
              |
            1 | CREATE TABLE t (a, b AS (a * 2)); INSERT INTO t VALUES(1); INSERT INTO t VALUES(1, 2);
              |                                                                          ^~~~~~~~~~~~
            """,
        )

    def test_generated_column_added_by_alter(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); ALTER TABLE t ADD COLUMN b AS (a); INSERT INTO t VALUES(1);",
            out="",
        )

    def test_select_source(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t SELECT 1 UNION SELECT 2;",
            out="""\
            error: table 't' has 2 columns but 1 values were supplied
             --> <stdin>:1:38
              |
            1 | CREATE TABLE t (a, b); INSERT INTO t SELECT 1 UNION SELECT 2;
              |                                      ^~~~~~~~~~~~~~~~~~~~~~~
            """,
        )

    def test_with_source(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t WITH c AS (SELECT 1) SELECT 1, 2, 3 FROM c;",
            out="""\
            error: table 't' has 2 columns but 3 values were supplied
             --> <stdin>:1:38
              |
            1 | CREATE TABLE t (a, b); INSERT INTO t WITH c AS (SELECT 1) SELECT 1, 2, 3 FROM c;
              |                                      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            """,
        )

    def test_star_source_is_expanded(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); CREATE TABLE u (x); INSERT INTO t SELECT * FROM u;",
            out="""\
            error: table 't' has 2 columns but 1 values were supplied
             --> <stdin>:1:58
              |
            1 | CREATE TABLE t (a, b); CREATE TABLE u (x); INSERT INTO t SELECT * FROM u;
              |                                                          ^~~~~~~~~~~~~~~
            """,
        )

    def test_default_values(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t DEFAULT VALUES;",
            out="",
        )

    def test_trigger_body(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); CREATE TRIGGER tr AFTER DELETE ON t BEGIN INSERT INTO t VALUES(old.a); END;",
            out="""\
            error: table 't' has 2 columns but 1 values were supplied
             --> <stdin>:1:80
              |
            1 | CREATE TABLE t (a, b); CREATE TRIGGER tr AFTER DELETE ON t BEGIN INSERT INTO t VALUES(old.a); END;
              |                                                                                ^~~~~~~~~~~~~
            """,
        )

    def test_generated_column_renamed_then_dropped(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b AS (a), c); ALTER TABLE t RENAME COLUMN b TO bb; ALTER TABLE t DROP COLUMN bb; INSERT INTO t VALUES(1, 2);",
            out="",
        )

    def test_uneven_values_rows_are_reported_once(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); INSERT INTO t VALUES(1, 2), (3);",
            out="""\
            error: VALUES row has 1 terms, but the first row has 2
             --> <stdin>:1:53
              |
            1 | CREATE TABLE t (a, b); INSERT INTO t VALUES(1, 2), (3);
              |                                                     ^
            """,
        )

    def test_copying_a_table_of_the_same_width(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE src (a AS (b), b); CREATE TABLE dst (x AS (y), y); INSERT INTO dst SELECT * FROM src;",
            out="",
        )

    def test_copying_a_table_into_itself_counts_values(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b AS (a)); INSERT INTO t SELECT * FROM t;",
            out="""\
            error: table 't' has 1 columns but 2 values were supplied
             --> <stdin>:1:45
              |
            1 | CREATE TABLE t (a, b AS (a)); INSERT INTO t SELECT * FROM t;
              |                                             ^~~~~~~~~~~~~~~
            """,
        )
