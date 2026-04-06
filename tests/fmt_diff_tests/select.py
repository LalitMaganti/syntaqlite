# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class SelectFormat(TestSuite):
    def test_literal(self):
        return DiffTestBlueprint(
            sql="SELECT 1",
            out="SELECT 1;",
        )

    def test_columns(self):
        return DiffTestBlueprint(
            sql="select a, b, c from t",
            out="SELECT a, b, c FROM t;",
        )

    def test_where(self):
        return DiffTestBlueprint(
            sql="select a from t where x = 1",
            out="SELECT a FROM t WHERE x = 1;",
        )

    def test_order_by(self):
        return DiffTestBlueprint(
            sql="select a from t order by a desc",
            out="SELECT a FROM t ORDER BY a DESC;",
        )

    def test_group_by_having(self):
        return DiffTestBlueprint(
            sql="select count(*) from t group by a having count(*) > 5",
            out="SELECT count(*) FROM t GROUP BY a HAVING count(*) > 5;",
        )

    def test_limit_offset(self):
        return DiffTestBlueprint(
            sql="select a from t limit 10 offset 5",
            out="SELECT a FROM t LIMIT 10 OFFSET 5;",
        )

    def test_distinct(self):
        return DiffTestBlueprint(
            sql="select distinct a, b from t",
            out="SELECT DISTINCT a, b FROM t;",
        )

    def test_star(self):
        return DiffTestBlueprint(
            sql="select * from t",
            out="SELECT * FROM t;",
        )

    def test_alias(self):
        return DiffTestBlueprint(
            sql="select a as x from t",
            out="SELECT a AS x FROM t;",
        )

    def test_compound_union_all(self):
        return DiffTestBlueprint(
            sql="select a from t1 union all select b from t2",
            out="""\
                SELECT a FROM t1
                UNION ALL
                SELECT b FROM t2;
            """,
        )

    def test_long_query_breaks(self):
        return DiffTestBlueprint(
            sql="select a, b, c, d, e, f, g, h, i, j from very_long_table_name where a = 1 and b = 2 and c = 3",
            out="""\
                SELECT a, b, c, d, e, f, g, h, i, j
                FROM very_long_table_name
                WHERE
                  a = 1
                  AND b = 2
                  AND c = 3;
            """,
        )

    def test_long_where_clause(self):
        return DiffTestBlueprint(
            sql="select a from t where x = 1 and y = 2 and z = 3 and w = 4 and v = 5 and u = 6 and q = 7 and r = 8 and s = 9 and p = 10",
            out="""\
                SELECT a
                FROM t
                WHERE
                  x = 1
                  AND y = 2
                  AND z = 3
                  AND w = 4
                  AND v = 5
                  AND u = 6
                  AND q = 7
                  AND r = 8
                  AND s = 9
                  AND p = 10;
            """,
        )


class ExprFormat(TestSuite):
    def test_binary_ops(self):
        return DiffTestBlueprint(
            sql="select 1 + 2 * 3",
            out="SELECT 1 + 2 * 3;",
        )

    def test_unary_minus(self):
        return DiffTestBlueprint(
            sql="select -x from t",
            out="SELECT -x FROM t;",
        )

    def test_and_or(self):
        return DiffTestBlueprint(
            sql="select a from t where x = 1 and y = 2",
            out="SELECT a FROM t WHERE x = 1 AND y = 2;",
        )

    def test_between(self):
        return DiffTestBlueprint(
            sql="select a from t where x between 1 and 10",
            out="SELECT a FROM t WHERE x BETWEEN 1 AND 10;",
        )

    def test_like(self):
        return DiffTestBlueprint(
            sql="select a from t where x like '%foo%'",
            out="SELECT a FROM t WHERE x LIKE '%foo%';",
        )

    def test_in_list(self):
        return DiffTestBlueprint(
            sql="select a from t where x in (1, 2, 3)",
            out="SELECT a FROM t WHERE x IN (1, 2, 3);",
        )

    def test_case(self):
        return DiffTestBlueprint(
            sql="select case when x > 0 then 'pos' else 'neg' end from t",
            out="SELECT CASE WHEN x > 0 THEN 'pos' ELSE 'neg' END FROM t;",
        )

    def test_case_multiline(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN status = 'ACTIVE' THEN 'active' WHEN status = 'INACTIVE' THEN 'inactive' WHEN status = 'PENDING' THEN 'pending' WHEN status = 'DELETED' THEN 'deleted' ELSE 'unknown' END FROM users",
            out="""\
                SELECT
                  CASE
                    WHEN status = 'ACTIVE' THEN 'active'
                    WHEN status = 'INACTIVE' THEN 'inactive'
                    WHEN status = 'PENDING' THEN 'pending'
                    WHEN status = 'DELETED' THEN 'deleted'
                    ELSE 'unknown'
                  END
                FROM users;
            """,
        )

    def test_cast(self):
        return DiffTestBlueprint(
            sql="select cast(x as integer) from t",
            out="SELECT CAST(x AS integer) FROM t;",
        )

    def test_exists(self):
        return DiffTestBlueprint(
            sql="select a from t where exists (select 1 from t2)",
            out="SELECT a FROM t WHERE EXISTS (SELECT 1 FROM t2);",
        )

    def test_function_call(self):
        return DiffTestBlueprint(
            sql="select max(a), min(b) from t",
            out="SELECT max(a), min(b) FROM t;",
        )

    def test_is_null(self):
        return DiffTestBlueprint(
            sql="select a from t where x is null",
            out="SELECT a FROM t WHERE x IS null;",
        )


class JoinUsingFormat(TestSuite):
    def test_comma_join_using_columns_stay_inline(self):
        """USING column list in comma-join should not wrap when outer group breaks."""
        return DiffTestBlueprint(
            sql="select * from long_table_name_one, long_table_name_two using (col_a, col_b) where x = 1 and y = 2 and z = 3",
            out="""\
                SELECT *
                FROM long_table_name_one, long_table_name_two USING (col_a, col_b)
                WHERE
                  x = 1
                  AND y = 2
                  AND z = 3;
            """,
        )


class TableValuedFunctionFormat(TestSuite):
    def test_tvf_basic(self):
        return DiffTestBlueprint(
            sql="select * from generate_series(1, 10)",
            out="SELECT * FROM generate_series(1, 10);",
        )

    def test_tvf_with_alias(self):
        return DiffTestBlueprint(
            sql="select * from json_each('[]') as j",
            out="SELECT * FROM json_each('[]') AS j;",
        )

    def test_tvf_in_join(self):
        return DiffTestBlueprint(
            sql="select * from t join json_each(t.col) as j on 1",
            out="SELECT *\nFROM t\nJOIN json_each(t.col) AS j ON 1;",
        )

    # ── Quoted identifiers ────────────────────────────────────────────

    def test_double_quoted_table(self):
        return DiffTestBlueprint(
            sql='SELECT * FROM "my table"',
            out='SELECT * FROM "my table";',
        )

    def test_double_quoted_column(self):
        return DiffTestBlueprint(
            sql='SELECT "set" FROM t',
            out='SELECT "set" FROM t;',
        )

    def test_backtick_table_normalizes_to_double_quote(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM `set`",
            out='SELECT * FROM "set";',
        )

    def test_bracket_table_normalizes_to_double_quote(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM [set]",
            out='SELECT * FROM "set";',
        )

    def test_backtick_column_normalizes_to_double_quote(self):
        return DiffTestBlueprint(
            sql="SELECT `set` FROM t",
            out='SELECT "set" FROM t;',
        )

    def test_bracket_column_normalizes_to_double_quote(self):
        return DiffTestBlueprint(
            sql="SELECT [set] FROM t",
            out='SELECT "set" FROM t;',
        )

    def test_qualified_quoted_column(self):
        return DiffTestBlueprint(
            sql='SELECT t."set" FROM t',
            out='SELECT t."set" FROM t;',
        )

    def test_unquoted_identifier_stays_unquoted(self):
        return DiffTestBlueprint(
            sql="SELECT name FROM users",
            out="SELECT name FROM users;",
        )