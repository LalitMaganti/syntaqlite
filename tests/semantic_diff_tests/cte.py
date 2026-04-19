# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CteColumnList(TestSuite):
    def test_without_column_list_no_error(self):
        return DiffTestBlueprint(
            sql="WITH cte AS (SELECT 1 AS x) SELECT x FROM cte",
            strict_schema=True,
            out="",
        )

    def test_count_matches_no_error(self):
        return DiffTestBlueprint(
            sql="WITH cte(a, b) AS (SELECT 1, 2) SELECT a, b FROM cte",
            strict_schema=True,
            out="",
        )

    def test_count_mismatch_too_few_declared(self):
        return DiffTestBlueprint(
            sql="WITH cte(a) AS (SELECT 1, 2) SELECT a FROM cte",
            strict_schema=True,
            out="""\
            error: table 'cte' has 2 values for 1 columns
             --> <stdin>:1:6
              |
            1 | WITH cte(a) AS (SELECT 1, 2) SELECT a FROM cte
              |      ^~~
""",
        )

    def test_count_mismatch_too_many_declared(self):
        return DiffTestBlueprint(
            sql="WITH cte(a, b, c) AS (SELECT 1, 2) SELECT a FROM cte",
            strict_schema=True,
            out="""\
            error: table 'cte' has 2 values for 3 columns
             --> <stdin>:1:6
              |
            1 | WITH cte(a, b, c) AS (SELECT 1, 2) SELECT a FROM cte
              |      ^~~
""",
        )

    def test_declared_column_names_visible_in_outer(self):
        return DiffTestBlueprint(
            sql="WITH cte(x, y) AS (SELECT 1, 2) SELECT z FROM cte",
            strict_schema=True,
            out="""\
            error: unknown column 'z'
             --> <stdin>:1:40
              |
            1 | WITH cte(x, y) AS (SELECT 1, 2) SELECT z FROM cte
              |                                        ^
              = help: did you mean 'x'?
""",
        )

    def test_declared_columns_valid_ref_ok(self):
        return DiffTestBlueprint(
            sql="WITH cte(x, y) AS (SELECT 1, 2) SELECT x FROM cte",
            strict_schema=True,
            out="",
        )

    def test_star_body_skips_count_check(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER, b TEXT); WITH cte(x, y) AS (SELECT * FROM t) SELECT x FROM cte",
            strict_schema=True,
            out="",
        )


class CteInferredColumns(TestSuite):
    def test_invalid_ref_errors(self):
        return DiffTestBlueprint(
            sql="WITH cte AS (SELECT 1 AS x) SELECT z FROM cte",
            strict_schema=True,
            out="""\
            error: unknown column 'z'
             --> <stdin>:1:36
              |
            1 | WITH cte AS (SELECT 1 AS x) SELECT z FROM cte
              |                                    ^
              = help: did you mean 'x'?
""",
        )

    def test_valid_ref_ok(self):
        return DiffTestBlueprint(
            sql="WITH cte AS (SELECT 1 AS x) SELECT x FROM cte",
            strict_schema=True,
            out="",
        )

    def test_star_body_accepts_any_ref(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER, b TEXT); WITH cte AS (SELECT * FROM t) SELECT anything FROM cte",
            strict_schema=True,
            out="",
        )
