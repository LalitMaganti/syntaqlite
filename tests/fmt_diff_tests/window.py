from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class WindowFunctionFormat(TestSuite):
    def test_over_order_by(self):
        return AstTestBlueprint(
            sql="select row_number() over (order by id) from t",
            out="SELECT row_number() OVER (ORDER BY id) FROM t",
        )

    def test_over_partition_by(self):
        return AstTestBlueprint(
            sql="select count(*) over (partition by a) from t",
            out="SELECT count(*) OVER (PARTITION BY a) FROM t",
        )

    def test_over_partition_and_order(self):
        return AstTestBlueprint(
            sql="select sum(x) over (partition by a order by b) from t",
            out="SELECT sum(x) OVER (PARTITION BY a ORDER BY b) FROM t",
        )

    def test_over_named_window(self):
        return AstTestBlueprint(
            sql="select sum(x) over w from t window w as (order by x)",
            out="SELECT sum(x) OVER w FROM t WINDOW w AS (ORDER BY x)",
        )

    def test_multiple_named_windows(self):
        return AstTestBlueprint(
            sql="select sum(x) over w1, avg(y) over w2 from t window w1 as (order by a), w2 as (partition by b order by c)",
            out="""\
                SELECT sum(x) OVER w1, avg(y) OVER w2
                FROM t
                WINDOW w1 AS (ORDER BY a), w2 AS (PARTITION BY b ORDER BY c)
            """,
        )


class FilterOverFormat(TestSuite):
    def test_filter_only(self):
        return AstTestBlueprint(
            sql="select count(*) filter (where x > 0) from t",
            out="SELECT count(*) FILTER (WHERE x > 0) FROM t",
        )

    def test_filter_with_over(self):
        return AstTestBlueprint(
            sql="select sum(x) filter (where x > 0) over (order by y) from t",
            out="SELECT sum(x) FILTER (WHERE x > 0) OVER (ORDER BY y) FROM t",
        )

    def test_filter_with_named_window(self):
        return AstTestBlueprint(
            sql="select sum(x) filter (where x > 0) over w from t window w as (order by y)",
            out="SELECT sum(x) FILTER (WHERE x > 0) OVER w FROM t WINDOW w AS (ORDER BY y)",
        )


class FrameSpecFormat(TestSuite):
    def test_rows_between(self):
        return AstTestBlueprint(
            sql="select sum(x) over (order by y rows between 1 preceding and 1 following) from t",
            out="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) FROM t",
        )

    def test_range_unbounded(self):
        return AstTestBlueprint(
            sql="select sum(x) over (order by y range between unbounded preceding and current row) from t",
            out="""\
                SELECT
                  sum(x) OVER (ORDER BY y RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
                FROM t
            """,
        )

    def test_groups_with_exclude(self):
        return AstTestBlueprint(
            sql="select sum(x) over (order by y groups between unbounded preceding and unbounded following exclude ties) from t",
            out="""\
                SELECT
                  sum(x) OVER (ORDER BY y GROUPS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING EXCLUDE TIES)
                FROM t
            """,
        )

    def test_rows_single_bound(self):
        return AstTestBlueprint(
            sql="select sum(x) over (order by y rows 2 preceding) from t",
            out="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 2 PRECEDING AND CURRENT ROW) FROM t",
        )
