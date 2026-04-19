# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Dequoting(TestSuite):
    def test_col_backtick_ddl_unquoted_query(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (`col` INTEGER); SELECT col FROM t",
            strict_schema=True,
            out="",
        )

    def test_col_bracket_ddl_unquoted_query(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t ([col] INTEGER); SELECT col FROM t",
            strict_schema=True,
            out="",
        )

    def test_col_unquoted_ddl_double_quoted_query(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE t (col INTEGER); SELECT "col" FROM t',
            strict_schema=True,
            out="",
        )

    def test_reserved_word_column_qualified(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE t (id INTEGER, "set" TEXT); SELECT t."set" FROM t',
            strict_schema=True,
            out="",
        )

    def test_expr_span_col_double_quoted(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE t AS SELECT 1; SELECT t."1" FROM t;',
            strict_schema=True,
            out="",
        )

    def test_expr_span_col_backtick(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1; SELECT t.`1` FROM t;",
            strict_schema=True,
            out="",
        )

    def test_expr_span_col_bracket(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1; SELECT t.[1] FROM t;",
            strict_schema=True,
            out="",
        )

    def test_aliased_col_double_quoted(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE t AS SELECT 1 AS x; SELECT t."x" FROM t;',
            strict_schema=True,
            out="",
        )
