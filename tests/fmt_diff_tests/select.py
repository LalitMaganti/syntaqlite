from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class SelectFormat(TestSuite):
    def test_literal(self):
        return AstTestBlueprint(
            sql="SELECT 1",
            out="SELECT 1",
        )

    def test_columns(self):
        return AstTestBlueprint(
            sql="select a, b, c from t",
            out="""\
                SELECT a, b, c
                FROM
                  t
            """,
        )

    def test_where(self):
        return AstTestBlueprint(
            sql="select a from t where x = 1",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  x = 1
            """,
        )

    def test_order_by(self):
        return AstTestBlueprint(
            sql="select a from t order by a desc",
            out="""\
                SELECT a
                FROM
                  t
                ORDER BY
                  a DESC
            """,
        )

    def test_group_by_having(self):
        return AstTestBlueprint(
            sql="select count(*) from t group by a having count(*) > 5",
            out="""\
                SELECT count(*)
                FROM
                  t
                GROUP BY
                  a
                HAVING
                  count(*) > 5
            """,
        )

    def test_limit_offset(self):
        return AstTestBlueprint(
            sql="select a from t limit 10 offset 5",
            out="""\
                SELECT a
                FROM
                  t
                LIMIT
                  10 OFFSET 5
            """,
        )

    def test_distinct(self):
        return AstTestBlueprint(
            sql="select distinct a, b from t",
            out="""\
                SELECT DISTINCT a, b
                FROM
                  t
            """,
        )

    def test_star(self):
        return AstTestBlueprint(
            sql="select * from t",
            out="""\
                SELECT *
                FROM
                  t
            """,
        )

    def test_alias(self):
        return AstTestBlueprint(
            sql="select a as x from t",
            out="""\
                SELECT a
                FROM
                  t
            """,
        )

    def test_compound_union_all(self):
        return AstTestBlueprint(
            sql="select a from t1 union all select b from t2",
            out="""\
                SELECT a
                FROM
                  t1
                UNION ALL
                SELECT b
                FROM
                  t2
            """,
        )


class ExprFormat(TestSuite):
    def test_binary_ops(self):
        return AstTestBlueprint(
            sql="select 1 + 2 * 3",
            out="SELECT 1 + 2 * 3",
        )

    def test_unary_minus(self):
        return AstTestBlueprint(
            sql="select -x from t",
            out="""\
                SELECT -x
                FROM
                  t
            """,
        )

    def test_and_or(self):
        return AstTestBlueprint(
            sql="select a from t where x = 1 and y = 2",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  x = 1 AND y = 2
            """,
        )

    def test_between(self):
        return AstTestBlueprint(
            sql="select a from t where x between 1 and 10",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  x BETWEEN 1 AND 10
            """,
        )

    def test_like(self):
        return AstTestBlueprint(
            sql="select a from t where x like '%foo%'",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  x LIKE '%foo%'
            """,
        )

    def test_in_list(self):
        return AstTestBlueprint(
            sql="select a from t where x in (1, 2, 3)",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  x IN (1, 2, 3)
            """,
        )

    def test_case(self):
        return AstTestBlueprint(
            sql="select case when x > 0 then 'pos' else 'neg' end from t",
            out="""\
                SELECT CASE WHEN x > 0 THEN 'pos' ELSE 'neg' END
                FROM
                  t
            """,
        )

    def test_cast(self):
        return AstTestBlueprint(
            sql="select cast(x as integer) from t",
            out="""\
                SELECT CAST(x AS integer)
                FROM
                  t
            """,
        )

    def test_exists(self):
        return AstTestBlueprint(
            sql="select a from t where exists (select 1 from t2)",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  EXISTS (SELECT 1
                  FROM
                    t2)
            """,
        )

    def test_function_call(self):
        return AstTestBlueprint(
            sql="select max(a), min(b) from t",
            out="""\
                SELECT max(a), min(b)
                FROM
                  t
            """,
        )

    def test_is_null(self):
        return AstTestBlueprint(
            sql="select a from t where x is null",
            out="""\
                SELECT a
                FROM
                  t
                WHERE
                  x IS null
            """,
        )
