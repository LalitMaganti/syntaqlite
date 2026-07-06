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

    # `SQLite` also accepts a string literal wherever an identifier is
    # expected (table names, column names, aliases). The parser strips
    # "double quotes", [brackets], and `backticks`, but retains the single
    # quotes verbatim, so the analyzer normalizes those separately.

    def test_table_single_quoted_ddl_bare_query(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE 't1' (a INTEGER); SELECT * FROM t1",
            strict_schema=True,
            out="",
        )

    def test_table_single_quoted_ddl_bracket_query(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE '@abc' (a INTEGER); SELECT * FROM [@abc]",
            strict_schema=True,
            out="",
        )

    def test_table_bare_ddl_single_quoted_query(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t1 (a INTEGER); DELETE FROM 't1'",
            strict_schema=True,
            out="",
        )

    def test_table_single_quoted_escaped_ddl_double_quoted_query(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE \'don\'\'t\' (a INTEGER); SELECT * FROM "don\'t"',
            strict_schema=True,
            out="",
        )

    def test_col_single_quoted_ddl_unquoted_query(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t ('col' INTEGER); SELECT col FROM t",
            strict_schema=True,
            out="",
        )

    def test_aliased_col_single_quoted(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM (SELECT avg(x) AS 'a' FROM (SELECT 1 AS x)) t1;",
            strict_schema=True,
            out="",
        )

    def test_order_by_single_quoted_alias(self):
        return DiffTestBlueprint(
            sql="SELECT 2 AS 'x' ORDER BY x;",
            strict_schema=True,
            out="",
        )

    def test_source_alias_single_quoted(self):
        return DiffTestBlueprint(
            sql="SELECT s.a FROM (SELECT 1 AS a) AS 's';",
            strict_schema=True,
            out="",
        )

    def test_qualified_col_single_quoted_table_and_column(self):
        # Both the table qualifier and the column name are single-quoted
        # string-literal identifiers (quote.test); the qualifier must
        # normalize to match the source's scope key, not just the column.
        return DiffTestBlueprint(
            sql="CREATE TABLE '@abc' ('!pqr' INTEGER); SELECT '@abc'.'!pqr' FROM '@abc';",
            strict_schema=True,
            out="",
        )

    # Escaped quotes inside *any* quote style must compare by identifier
    # value, not by the raw inner text the zero-copy span points at:
    # `"x""y"`, `[x"y]`, and `'x"y'` all name the same table.

    def test_table_double_quoted_escaped_ddl_bracket_query(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE "x""y" (a INTEGER); SELECT * FROM [x"y]',
            strict_schema=True,
            out="",
        )

    def test_table_bracket_ddl_double_quoted_escaped_query(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE [x"y] (a INTEGER); SELECT * FROM "x""y"',
            strict_schema=True,
            out="",
        )

    def test_table_backtick_escaped_ddl_double_quoted_query(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE `x``y` (a INTEGER); SELECT * FROM "x`y"',
            strict_schema=True,
            out="",
        )

    def test_col_double_quoted_escaped_ddl_bracket_query(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE t ("a""b" INTEGER); SELECT [a"b] FROM t',
            strict_schema=True,
            out="",
        )

    def test_alias_double_quoted_escaped(self):
        return DiffTestBlueprint(
            sql='SELECT 1 AS "x""y" ORDER BY [x"y];',
            strict_schema=True,
            out="",
        )
