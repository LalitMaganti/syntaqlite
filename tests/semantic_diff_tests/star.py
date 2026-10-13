# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite

SCHEMA = "CREATE TABLE a (x, y); CREATE TABLE b (x, z); "


class StarExpansion(TestSuite):
    def test_natural_join_shows_shared_columns_once(self):
        return DiffTestBlueprint(
            sql=SCHEMA + "SELECT * FROM a NATURAL JOIN b UNION SELECT 1, 2, 3, 4;",
            out="""\
            error: SELECTs to the left and right of UNION have 3 and 4 result columns
             --> <stdin>:1:84
              |
            1 | CREATE TABLE a (x, y); CREATE TABLE b (x, z); SELECT * FROM a NATURAL JOIN b UNION SELECT 1, 2, 3, 4;
              |                                                                                    ^~~~~~~~~~~~~~~~~
            """,
        )

    def test_using_shows_its_columns_once(self):
        return DiffTestBlueprint(
            sql=SCHEMA + "SELECT * FROM a JOIN b USING (x) UNION SELECT 1, 2;",
            out="""\
            error: SELECTs to the left and right of UNION have 3 and 2 result columns
             --> <stdin>:1:86
              |
            1 | CREATE TABLE a (x, y); CREATE TABLE b (x, z); SELECT * FROM a JOIN b USING (x) UNION SELECT 1, 2;
              |                                                                                      ^~~~~~~~~~~
            """,
        )

    def test_qualified_star_keeps_using_columns(self):
        return DiffTestBlueprint(
            sql=SCHEMA + "SELECT b.* FROM a JOIN b USING (x) UNION SELECT 1;",
            out="""\
            error: SELECTs to the left and right of UNION have 2 and 1 result columns
             --> <stdin>:1:88
              |
            1 | CREATE TABLE a (x, y); CREATE TABLE b (x, z); SELECT b.* FROM a JOIN b USING (x) UNION SELECT 1;
              |                                                                                        ^~~~~~~~
            """,
        )

    def test_subquery_source(self):
        return DiffTestBlueprint(
            sql=SCHEMA + "SELECT * FROM a, (SELECT 1 AS q) UNION SELECT 1, 2;",
            out="""\
            error: SELECTs to the left and right of UNION have 3 and 2 result columns
             --> <stdin>:1:86
              |
            1 | CREATE TABLE a (x, y); CREATE TABLE b (x, z); SELECT * FROM a, (SELECT 1 AS q) UNION SELECT 1, 2;
              |                                                                                      ^~~~~~~~~~~
            """,
        )

    def test_generated_columns_are_included(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE g (p, q AS (p)); SELECT * FROM g UNION SELECT 1;",
            out="""\
            error: SELECTs to the left and right of UNION have 2 and 1 result columns
             --> <stdin>:1:53
              |
            1 | CREATE TABLE g (p, q AS (p)); SELECT * FROM g UNION SELECT 1;
              |                                                     ^~~~~~~~
            """,
        )

    def test_unknown_source_is_skipped(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM nosuch UNION SELECT 1;",
            out="""\
            warning: unknown table 'nosuch'
             --> <stdin>:1:15
              |
            1 | SELECT * FROM nosuch UNION SELECT 1;
              |               ^~~~~~
            """,
        )

    def test_view_of_a_join(self):
        return DiffTestBlueprint(
            sql=SCHEMA + "CREATE VIEW v AS SELECT * FROM a JOIN b USING (x); SELECT v.z, v.w FROM v;",
            out="""\
            warning: unknown column 'w' in table 'v'
             --> <stdin>:1:112
              |
            1 | CREATE TABLE a (x, y); CREATE TABLE b (x, z); CREATE VIEW v AS SELECT * FROM a JOIN b USING (x); SELECT v.z, v.w FROM v;
              |                                                                                                                ^
              = help: did you mean 'x'?
            """,
        )
