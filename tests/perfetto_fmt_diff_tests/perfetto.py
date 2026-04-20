# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class PerfettoIndexFormat(TestSuite):
    def test_create_perfetto_index_multiline_columns(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE PERFETTO INDEX idx_hot_slices ON hot_slices(ts,
                dur,
                track_id)
            """,
            out="CREATE PERFETTO INDEX idx_hot_slices ON hot_slices(ts, dur, track_id)",
        )


class PerfettoMacroFormat(TestSuite):
    def test_create_perfetto_macro_body_preserved(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE PERFETTO MACRO m(x TableOrSubquery) RETURNS TableOrSubquery AS x
            """,
            out="""\
                CREATE PERFETTO MACRO m(x TableOrSubquery)
                RETURNS TableOrSubquery
                AS x
            """,
        )

    def test_create_perfetto_macro_body_select(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE PERFETTO MACRO my_macro(t TableOrSubquery) RETURNS TableOrSubquery AS (SELECT * FROM $t)
            """,
            out="""\
                CREATE PERFETTO MACRO my_macro(t TableOrSubquery)
                RETURNS TableOrSubquery
                AS (SELECT * FROM $t)
            """,
        )

    def test_create_perfetto_macro_long_args_indented(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE PERFETTO MACRO _viz_flamegraph_filter_frames(source TableOrSubquery, show_from_frame_bits Expr) RETURNS TableOrSubquery AS $source
            """,
            out="""\
                CREATE PERFETTO MACRO _viz_flamegraph_filter_frames(
                  source TableOrSubquery,
                  show_from_frame_bits Expr
                )
                RETURNS TableOrSubquery
                AS $source
            """,
        )

    def test_create_or_replace_perfetto_macro_body(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE OR REPLACE PERFETTO MACRO m(x Expr) RETURNS Expr AS $x
            """,
            out="""\
                CREATE OR REPLACE PERFETTO MACRO m(x Expr)
                RETURNS Expr
                AS $x
            """,
        )


class PerfettoMacroCallFormat(TestSuite):
    def test_macro_call_in_select(self):
        return DiffTestBlueprint(
            sql="SELECT foo!(1 + 2), 3",
            out="SELECT foo!(1 + 2), 3",
        )

    def test_macro_arg_expression_canonicalized(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(1+2)",
            out="SELECT cast_int!(1 + 2)",
        )

    def test_macro_call_in_from(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM my_macro!(t1)",
            out="SELECT * FROM my_macro!(t1)",
        )

    def test_macro_call_nested_parens(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM graph_reachable_dfs!((SELECT id FROM t), (SELECT id FROM s))",
            out="SELECT * FROM graph_reachable_dfs!((SELECT id FROM t), (SELECT id FROM s))",
        )

    def test_macro_call_no_args(self):
        return DiffTestBlueprint(
            sql="SELECT my_macro!()",
            out="SELECT my_macro!()",
        )

    def test_macro_call_multi_node(self):
        return DiffTestBlueprint(
            sql="SELECT my_fn!(a, b)",
            out="SELECT my_fn!(a, b)",
        )

    def test_macro_call_multi_node_no_extra_separator(self):
        return DiffTestBlueprint(
            sql="SELECT foo!(a, b), c",
            out="SELECT foo!(a, b), c",
        )

    def test_macro_multiline_reindented(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT *
                FROM graph_next_sibling!(
                        (
                          SELECT id, parent_id, ts
                          FROM slice
                          WHERE dur = 0
                        )
                    )
            """,
            out="""\
                SELECT *
                FROM graph_next_sibling!((SELECT id, parent_id, ts FROM slice WHERE dur = 0))
            """,
        )

    def test_macro_parens_in_strings_ignored(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT *
                FROM my_macro!(
                  (
                    SELECT '(((' AS x
                    FROM t
                  )
                )
            """,
            out="""\
                SELECT *
                FROM my_macro!(
                  (
                    SELECT '(((' AS x
                    FROM t
                  )
                )
            """,
        )

    def test_macro_with_function_calls(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT *
                FROM scan!(
                  (
                    SELECT
                      IIF(
                        x > 0,
                        1,
                        0
                      ) AS flag
                    FROM t
                  )
                )
            """,
            out="SELECT * FROM scan!((SELECT IIF(x > 0, 1, 0) AS flag FROM t))",
        )

    def test_macro_comma_separated_args(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT *
                FROM scan!(
                    edges,
                    inits,
                    (a, b, c),
                    (
                      SELECT id
                      FROM t
                    )
                  )
            """,
            out="SELECT * FROM scan!(edges, inits, (a, b, c), (SELECT id FROM t))",
        )

    def test_macro_call_with_alias(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(value) AS x FROM t",
            out="SELECT cast_int!(value) AS x FROM t",
        )

    def test_macro_call_with_alias_multiple(self):
        return DiffTestBlueprint(
            sql="SELECT foo!(a) AS x, bar!(b) AS y FROM t",
            out="SELECT foo!(a) AS x, bar!(b) AS y FROM t",
        )

    def test_macro_call_in_from_with_alias(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM my_macro!(t) AS mt",
            out="SELECT * FROM my_macro!(t) AS mt",
        )

    def test_macro_call_in_update_set(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET col = cast_int!(value)",
            out="UPDATE t SET col = cast_int!(value)",
        )

    def test_macro_call_in_values(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (cast_int!(value))",
            out="INSERT INTO t VALUES (cast_int!(value))",
        )

    def test_macro_call_in_cte(self):
        return DiffTestBlueprint(
            sql="WITH c AS (SELECT cast_int!(value) AS x FROM t) SELECT * FROM c",
            out="""\
                WITH c AS (SELECT cast_int!(value) AS x FROM t)
                SELECT * FROM c
            """,
        )

    # ── Similar-style risks: trailing keywords/spans on the macro's parent ──

    def test_macro_call_with_trailing_collate(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(value) COLLATE NOCASE FROM t",
            out="SELECT cast_int!(value) COLLATE NOCASE FROM t",
        )

    def test_macro_call_with_trailing_sort_order(self):
        return DiffTestBlueprint(
            sql="SELECT x FROM t ORDER BY cast_int!(value) DESC",
            out="SELECT x FROM t ORDER BY cast_int!(value) DESC",
        )

    def test_macro_call_with_trailing_nulls_order(self):
        return DiffTestBlueprint(
            sql="SELECT x FROM t ORDER BY cast_int!(value) NULLS FIRST",
            out="SELECT x FROM t ORDER BY cast_int!(value) NULLS FIRST",
        )

    # ── Macro nested inside an outer wrapper (inner position) ──

    def test_macro_inside_cast(self):
        return DiffTestBlueprint(
            sql="SELECT CAST(cast_int!(value) AS INT) FROM t",
            out="SELECT CAST(cast_int!(value) AS INT) FROM t",
        )

    def test_macro_inside_binary_expr(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(a) + cast_int!(b) FROM t",
            out="SELECT cast_int!(a) + cast_int!(b) FROM t",
        )

    def test_macro_inside_not_expr(self):
        return DiffTestBlueprint(
            sql="SELECT NOT cast_int!(value) FROM t",
            out="SELECT NOT cast_int!(value) FROM t",
        )

    def test_macro_inside_case_arm(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN x > 0 THEN cast_int!(value) ELSE 0 END FROM t",
            out="SELECT CASE WHEN x > 0 THEN cast_int!(value) ELSE 0 END FROM t",
        )

    def test_macro_inside_in_list(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t WHERE x IN (cast_int!(a), cast_int!(b))",
            out="SELECT * FROM t WHERE x IN (cast_int!(a), cast_int!(b))",
        )

    # ── Macro in DML contexts ──

    def test_macro_in_delete_where(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t WHERE id = cast_int!(value)",
            out="DELETE FROM t WHERE id = cast_int!(value)",
        )

    def test_macro_in_update_where(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET col = 1 WHERE id = cast_int!(value)",
            out="UPDATE t SET col = 1 WHERE id = cast_int!(value)",
        )

    def test_macro_in_returning_with_alias(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t RETURNING cast_int!(value) AS x",
            out="DELETE FROM t RETURNING cast_int!(value) AS x",
        )

    def test_macro_in_insert_values_multi(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (cast_int!(a), cast_int!(b))",
            out="INSERT INTO t VALUES (cast_int!(a), cast_int!(b))",
        )

    # ── Macro with surrounding trivia ──

    def test_macro_call_with_line_comment_after(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(value) -- trailing comment\nFROM t",
            out="""\
                SELECT cast_int!(value) -- trailing comment
                FROM t
            """,
        )

    def test_macro_call_with_block_comment_before_alias(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(value) /* inline */ AS x FROM t",
            out="SELECT cast_int!(value) /* inline */ AS x FROM t",
        )

    # ── Nested macros ──

    def test_nested_macro_calls(self):
        return DiffTestBlueprint(
            sql="SELECT cast_int!(cast_int!(value)) FROM t",
            out="SELECT cast_int!(cast_int!(value)) FROM t",
        )

    def test_macro_in_frame_bound_preserves_following(self):
        return DiffTestBlueprint(
            sql="SELECT count() OVER (ORDER BY ts RANGE BETWEEN CURRENT ROW AND my_macro!(x) FOLLOWING) FROM t",
            out="""\
                SELECT
                  count() OVER (
                    ORDER BY ts
                    RANGE BETWEEN CURRENT ROW AND my_macro!(x) FOLLOWING
                  )
                FROM t
            """,
        )


    def test_macro_partition_by_multi_arg_nests(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT last_value(thread.start_ts) OVER (
                  PARTITION BY upid, android_standardize_thread_name(thread.name)
                  ORDER BY thread.start_ts
                  RANGE BETWEEN CURRENT ROW AND cast_int!($sliding_window_dur) FOLLOWING
                ) FROM thread
            """,
            out="""\
                SELECT
                  last_value(thread.start_ts) OVER (
                    PARTITION BY
                      upid,
                      android_standardize_thread_name(thread.name)
                    ORDER BY thread.start_ts
                    RANGE BETWEEN CURRENT ROW AND cast_int!($sliding_window_dur) FOLLOWING
                  )
                FROM thread
            """,
        )


class PerfettoFunctionFormat(TestSuite):
    def test_create_perfetto_function_returns_on_newline(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE PERFETTO FUNCTION top_slice_count(cpu INT, min_dur INT) RETURNS INT AS
                SELECT count(*) FROM slice WHERE cpu = $cpu AND dur >= $min_dur;
            """,
            out="""\
                CREATE PERFETTO FUNCTION top_slice_count(cpu INT, min_dur INT)
                RETURNS INT
                AS
                SELECT count(*) FROM slice WHERE cpu = $cpu AND dur >= $min_dur
            """,
        )
