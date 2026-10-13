# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CorrelatedSubqueries(TestSuite):
    def test_outer_column_resolves(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);"
                "CREATE TABLE orders (id INTEGER, user_id INTEGER);"
                "SELECT * FROM users WHERE EXISTS ("
                "SELECT 1 FROM orders WHERE name = 'Alice');"
            ),
            strict_schema=True,
            out="",
        )

    def test_inner_shadows_outer(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);"
                "CREATE TABLE orders (id INTEGER, user_id INTEGER);"
                "SELECT * FROM users WHERE EXISTS ("
                "SELECT 1 FROM orders WHERE id = 1);"
            ),
            strict_schema=True,
            out="",
        )

    def test_column_in_neither_scope_flagged(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);"
                "CREATE TABLE orders (id INTEGER, user_id INTEGER);"
                "SELECT * FROM users WHERE EXISTS ("
                "SELECT 1 FROM orders WHERE bogus = 1);"
            ),
            strict_schema=True,
            out="""\
            error: unknown column 'bogus'
             --> <stdin>:1:155
              |
            1 | CREATE TABLE users (id INTEGER, name TEXT);CREATE TABLE orders (id INTEGER, user_id INTEGER);SELECT * FROM users WHERE EXISTS (SELECT 1 FROM orders WHERE bogus = 1);
              |                                                                                                                                                           ^~~~~
""",
        )


class UnaliasedSubqueries(TestSuite):
    def test_columns_visible(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t1(a INTEGER, b TEXT); SELECT a FROM (SELECT a, b FROM t1);",
            strict_schema=True,
            out="",
        )

    def test_rejects_missing_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t1(a INTEGER, b TEXT); SELECT missing FROM (SELECT a FROM t1);",
            strict_schema=True,
            out="""\
            error: unknown column 'missing'
             --> <stdin>:1:44
              |
            1 | CREATE TABLE t1(a INTEGER, b TEXT); SELECT missing FROM (SELECT a FROM t1);
              |                                            ^~~~~~~
""",
        )

    def test_compound_accepts_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t1(a INTEGER); SELECT x FROM (SELECT a AS x FROM t1 UNION ALL SELECT a FROM t1);",
            strict_schema=True,
            out="",
        )

    def test_join_columns_visible(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t1(a INTEGER); CREATE TABLE t2(x INTEGER); SELECT a, x FROM t1 LEFT JOIN (SELECT x FROM t2) ON a = x;",
            strict_schema=True,
            out="",
        )


class AliasedSubqueries(TestSuite):
    def test_columns_visible(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t1(a INTEGER, b TEXT); SELECT sq.a FROM (SELECT a, b FROM t1) AS sq;",
            strict_schema=True,
            out="",
        )


class SubqueryColumns(TestSuite):
    def test_scalar_subquery_returns_two_columns(self):
        return DiffTestBlueprint(
            sql="SELECT 1 + (SELECT 1, 2);",
            out="""\
            error: subquery returns 2 columns; expected 1
             --> <stdin>:1:12
              |
            1 | SELECT 1 + (SELECT 1, 2);
              |            ^~~~~~~~~~~~~
            """,
        )

    def test_in_subquery_narrower_than_row_value(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); SELECT 1 WHERE (1, 2) IN (SELECT a FROM t);",
            out="""\
            error: subquery returns 1 columns; expected 2
             --> <stdin>:1:50
              |
            1 | CREATE TABLE t (a, b); SELECT 1 WHERE (1, 2) IN (SELECT a FROM t);
              |                                                  ^~~~~~~~~~~~~~~
            """,
        )

    def test_in_table_counts_its_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); SELECT 1 WHERE 1 IN t;",
            out="""\
            error: subquery returns 2 columns; expected 1
             --> <stdin>:1:39
              |
            1 | CREATE TABLE t (a, b); SELECT 1 WHERE 1 IN t;
              |                                       ^~~~~~
            """,
        )

    def test_in_star_subquery(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); SELECT 1 WHERE 1 IN (SELECT * FROM t);",
            out="""\
            error: subquery returns 2 columns; expected 1
             --> <stdin>:1:45
              |
            1 | CREATE TABLE t (a, b); SELECT 1 WHERE 1 IN (SELECT * FROM t);
              |                                             ^~~~~~~~~~~~~~~
            """,
        )

    def test_row_value_positions_take_wide_subqueries(self):
        return DiffTestBlueprint(
            sql=(
            "CREATE TABLE t (a, b); "
            "SELECT (SELECT 1, 2) = (1, 2), (1, 2) IN (SELECT a, b FROM t), "
            "CASE (SELECT 1, 2) WHEN (1, 2) THEN 1 END, "
            "(1, 2) BETWEEN (SELECT 1, 2) AND (3, 4); "
            "UPDATE t SET (a, b) = (SELECT 1, 2);"
            ),
            out="",
        )

    def test_is_null_tests_one_value(self):
        return DiffTestBlueprint(
            sql="SELECT 1 WHERE (SELECT 1, 2) IS NULL;",
            out="""\
            error: subquery returns 2 columns; expected 1
             --> <stdin>:1:16
              |
            1 | SELECT 1 WHERE (SELECT 1, 2) IS NULL;
              |                ^~~~~~~~~~~~~
            """,
        )

    def test_exists_takes_any_width(self):
        return DiffTestBlueprint(
            sql="SELECT EXISTS (SELECT 1, 2);",
            out="",
        )

    def test_empty_in_list(self):
        return DiffTestBlueprint(
            sql="SELECT 1 IN (), (1, 2) IN ();",
            out="",
        )
