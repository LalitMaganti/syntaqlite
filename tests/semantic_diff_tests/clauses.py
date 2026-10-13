# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class OrderByAlias(TestSuite):
    def test_select_alias_resolves(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT, active INT); SELECT COUNT(*) AS cnt FROM users ORDER BY cnt",
            strict_schema=True,
            out="",
        )

    def test_alias_with_group_by(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE employees (id INTEGER, dept TEXT, salary REAL); SELECT dept, SUM(salary) AS total_salary FROM employees GROUP BY dept ORDER BY total_salary DESC",
            strict_schema=True,
            out="",
        )


class HavingAlias(TestSuite):
    def test_select_alias_resolves(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, active INT); SELECT COUNT(*) AS n FROM users HAVING n > 0",
            strict_schema=True,
            out="",
        )


class WhereAlias(TestSuite):
    def test_select_alias_resolves(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b INT); SELECT a + b AS total FROM t WHERE total > 10",
            strict_schema=True,
            out="",
        )


class GroupByAlias(TestSuite):
    def test_select_alias_resolves(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b INT); SELECT a + b AS total, COUNT(*) FROM t GROUP BY total",
            strict_schema=True,
            out="",
        )


class CompoundColumns(TestSuite):
    def test_union_column_count_mismatch(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE a (x, y); SELECT x FROM a UNION ALL SELECT x, y FROM a;",
            out="""\
            error: SELECTs to the left and right of UNION ALL have 1 and 2 result columns
             --> <stdin>:1:50
              |
            1 | CREATE TABLE a (x, y); SELECT x FROM a UNION ALL SELECT x, y FROM a;
              |                                                  ^~~~~~~~~~~~~~~~~~
            """,
        )

    def test_matching_counts(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE a (x, y); SELECT x FROM a UNION SELECT y FROM a;",
            out="",
        )

    def test_star_arm_is_skipped(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE a (x, y); SELECT * FROM a EXCEPT SELECT x FROM a;",
            out="",
        )
