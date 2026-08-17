# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class WindowFunctionFormat(TestSuite):
    def test_over_order_by(self):
        return DiffTestBlueprint(
            sql="select row_number() over (order by id) from t",
            out="SELECT row_number() OVER (ORDER BY id) FROM t;",
        )

    def test_over_partition_by(self):
        return DiffTestBlueprint(
            sql="select count(*) over (partition by a) from t",
            out="SELECT count(*) OVER (PARTITION BY a) FROM t;",
        )

    def test_over_partition_and_order(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (partition by a order by b) from t",
            out="SELECT sum(x) OVER (PARTITION BY a ORDER BY b) FROM t;",
        )

    def test_over_named_window(self):
        return DiffTestBlueprint(
            sql="select sum(x) over w from t window w as (order by x)",
            out="SELECT sum(x) OVER w FROM t WINDOW w AS (ORDER BY x);",
        )

    def test_multiple_named_windows(self):
        return DiffTestBlueprint(
            sql="select sum(x) over w1, avg(y) over w2 from t window w1 as (order by a), w2 as (partition by b order by c)",
            out="""\
                SELECT sum(x) OVER w1, avg(y) OVER w2
                FROM t
                WINDOW
                  w1 AS (ORDER BY a),
                  w2 AS (PARTITION BY b ORDER BY c);
            """,
        )

    def test_long_over_partition_wraps(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT
                  customer_id,
                  order_id,
                  order_total,
                  rank() OVER (PARTITION BY customer_iddffkjllfjksljdfdklsfjklsfjkljfdklsdsjklfjkslfjljskdfjkl ORDER BY order_total DESC) AS customer_rank,
                  dense_rank() OVER (PARTITION BY customer_id ORDER BY order_total DESC) AS customer_dense_rank,
                  count(*) OVER (PARTITION BY customer_id) AS customer_order_count
                FROM orders
                WHERE
                  order_total > 0;
            """,
            out="""\
                SELECT
                  customer_id,
                  order_id,
                  order_total,
                  rank() OVER (
                    PARTITION BY
                      customer_iddffkjllfjksljdfdklsfjklsfjkljfdklsdsjklfjkslfjljskdfjkl
                    ORDER BY order_total DESC
                  ) AS customer_rank,
                  dense_rank() OVER (PARTITION BY customer_id ORDER BY order_total DESC) AS customer_dense_rank,
                  count(*) OVER (PARTITION BY customer_id) AS customer_order_count
                FROM orders
                WHERE
                  order_total > 0;
            """,
        )

    def test_long_named_window_def_wraps(self):
        return DiffTestBlueprint(
            sql="""\
                select sum(order_total) over w
                from orders
                window w as (partition by customer_iddffkjllfjksljdfdklsfjklsfjkljfdklsdsjklfjkslfjljskdfjkl order by order_total desc)
            """,
            out="""\
                SELECT sum(order_total) OVER w
                FROM orders
                WINDOW
                  w AS (
                    PARTITION BY
                      customer_iddffkjllfjksljdfdklsfjklsfjkljfdklsdsjklfjkslfjljskdfjkl
                    ORDER BY order_total DESC
                  );
            """,
        )


    def test_short_partition_by_stays_inline(self):
        return DiffTestBlueprint(
            sql="SELECT sum(x) OVER (PARTITION BY id ORDER BY ts) FROM t",
            out="SELECT sum(x) OVER (PARTITION BY id ORDER BY ts) FROM t;",
        )

    def test_order_by_multi_col_stays_inline(self):
        return DiffTestBlueprint(
            sql="select row_number() over (order by a, b, c) from t",
            out="SELECT row_number() OVER (ORDER BY a, b, c) FROM t;",
        )

    def test_order_by_long_list_wraps(self):
        return DiffTestBlueprint(
            sql="SELECT row_number() OVER (ORDER BY some_really_long_column_name DESC, another_long_column_name ASC) FROM t",
            out="""\
                SELECT
                  row_number() OVER (
                    ORDER BY some_really_long_column_name DESC, another_long_column_name
                  )
                FROM t;
            """,
        )

    def test_order_by_very_long_list_wraps(self):
        return DiffTestBlueprint(
            sql="SELECT row_number() OVER (ORDER BY some_really_long_column_name DESC, another_really_long_column_name_here ASC) FROM t",
            out="""\
                SELECT
                  row_number() OVER (
                    ORDER BY
                      some_really_long_column_name DESC,
                      another_really_long_column_name_here
                  )
                FROM t;
            """,
        )

    def test_order_by_expr(self):
        return DiffTestBlueprint(
            sql="select row_number() over (order by a + b desc) from t",
            out="SELECT row_number() OVER (ORDER BY a + b DESC) FROM t;",
        )

    def test_partition_order_frame_all_present(self):
        return DiffTestBlueprint(
            sql="SELECT SUM(x) OVER (PARTITION BY dept ORDER BY hire_date ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM emp",
            out="""\
                SELECT
                  SUM(x) OVER (
                    PARTITION BY
                      dept
                    ORDER BY hire_date
                    ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
                  )
                FROM emp;
            """,
        )

    def test_partition_only_multi_col_stays_inline(self):
        return DiffTestBlueprint(
            sql="select count(*) over (partition by region, department, team, project) from emp",
            out="SELECT count(*) OVER (PARTITION BY region, department, team, project) FROM emp;",
        )

    def test_multiple_windows_mixed(self):
        return DiffTestBlueprint(
            sql="SELECT SUM(x) OVER (PARTITION BY a ORDER BY b), AVG(y) OVER (ORDER BY c ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t",
            out="""\
                SELECT
                  SUM(x) OVER (PARTITION BY a ORDER BY b),
                  AVG(y) OVER (ORDER BY c ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
                FROM t;
            """,
        )


class FilterOverFormat(TestSuite):
    def test_filter_only(self):
        return DiffTestBlueprint(
            sql="select count(*) filter (where x > 0) from t",
            out="SELECT count(*) FILTER (WHERE x > 0) FROM t;",
        )

    def test_filter_with_over(self):
        return DiffTestBlueprint(
            sql="select sum(x) filter (where x > 0) over (order by y) from t",
            out="SELECT sum(x) FILTER (WHERE x > 0) OVER (ORDER BY y) FROM t;",
        )

    def test_filter_with_named_window(self):
        return DiffTestBlueprint(
            sql="select sum(x) filter (where x > 0) over w from t window w as (order by y)",
            out="SELECT sum(x) FILTER (WHERE x > 0) OVER w FROM t WINDOW w AS (ORDER BY y);",
        )


class FrameSpecFormat(TestSuite):
    def test_rows_between(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (order by y rows between 1 preceding and 1 following) from t",
            out="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) FROM t;",
        )

    def test_range_unbounded(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (order by y range between unbounded preceding and current row) from t",
            out="""\
                SELECT
                  sum(x) OVER (ORDER BY y RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
                FROM t;
            """,
        )

    def test_groups_with_exclude(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (order by y groups between unbounded preceding and unbounded following exclude ties) from t",
            out="""\
                SELECT
                  sum(x) OVER (
                    ORDER BY y
                    GROUPS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING EXCLUDE TIES
                  )
                FROM t;
            """,
        )

    def test_rows_single_bound(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (order by y rows 2 preceding) from t",
            out="SELECT sum(x) OVER (ORDER BY y ROWS BETWEEN 2 PRECEDING AND CURRENT ROW) FROM t;",
        )

    def test_frame_only_no_orderby(self):
        return DiffTestBlueprint(
            sql="SELECT SUM(x) OVER (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t",
            out="SELECT SUM(x) OVER (ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM t;",
        )

    def test_frame_exclude_current_row(self):
        return DiffTestBlueprint(
            sql="SELECT SUM(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING EXCLUDE CURRENT ROW) FROM t",
            out="""\
                SELECT
                  SUM(x) OVER (
                    ORDER BY y
                    ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING EXCLUDE CURRENT ROW
                  )
                FROM t;
            """,
        )

    def test_frame_exclude_no_others(self):
        return DiffTestBlueprint(
            sql="SELECT SUM(x) OVER (ORDER BY y ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING EXCLUDE NO OTHERS) FROM t",
            out="""\
                SELECT
                  SUM(x) OVER (
                    ORDER BY y
                    ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING EXCLUDE NO OTHERS
                  )
                FROM t;
            """,
        )

    def test_frame_range_type(self):
        return DiffTestBlueprint(
            sql="select avg(x) over (order by y range between unbounded preceding and current row) from t",
            out="""\
                SELECT
                  avg(x) OVER (ORDER BY y RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
                FROM t;
            """,
        )

    def test_frame_groups_type(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (order by y groups between 1 preceding and 1 following) from t",
            out="""\
                SELECT sum(x) OVER (ORDER BY y GROUPS BETWEEN 1 PRECEDING AND 1 FOLLOWING)
                FROM t;
            """,
        )


class WindowBaseName(TestSuite):
    def test_base_name_with_orderby(self):
        """A base window name lives inside the parens, alongside the rest."""
        return DiffTestBlueprint(
            sql="select sum(x) over (w1 order by b) from t window w1 as (partition by a)",
            out=(
                "SELECT sum(x) OVER (w1 ORDER BY b) "
                "FROM t WINDOW w1 AS (PARTITION BY a);"
            ),
        )

    def test_base_name_with_partition_by(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (w1 partition by a) from t window w1 as (order by b)",
            out=(
                "SELECT sum(x) OVER (w1 PARTITION BY a) "
                "FROM t WINDOW w1 AS (ORDER BY b);"
            ),
        )

    def test_base_name_with_frame(self):
        return DiffTestBlueprint(
            sql="select sum(x) over (w1 rows between 1 preceding and current row) from t window w1 as (order by b)",
            out="""\
                SELECT sum(x) OVER (w1 ROWS BETWEEN 1 PRECEDING AND CURRENT ROW)
                FROM t
                WINDOW
                  w1 AS (ORDER BY b);
            """,
        )

    def test_base_name_in_window_clause(self):
        """`w2 AS w1` is not valid SQL; the parens must survive."""
        return DiffTestBlueprint(
            sql="select sum(x) from t window w1 as (partition by a), w2 as (w1 order by b)",
            out=(
                "SELECT sum(x) FROM t "
                "WINDOW w1 AS (PARTITION BY a), w2 AS (w1 ORDER BY b);"
            ),
        )

    def test_bare_over_name_is_not_parenthesised(self):
        return DiffTestBlueprint(
            sql="select sum(x) over w1 from t window w1 as (partition by a)",
            out="SELECT sum(x) OVER w1 FROM t WINDOW w1 AS (PARTITION BY a);",
        )

    def test_empty_over(self):
        return DiffTestBlueprint(
            sql="select sum(x) over () from t",
            out="SELECT sum(x) OVER () FROM t;",
        )
