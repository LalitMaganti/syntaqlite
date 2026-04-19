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
