# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


# ── Comment-only input ─────────────────────────────────────────────────────────


class CommentOnlyInput(TestSuite):
    def test_block_comment_only(self):
        return DiffTestBlueprint(
            sql="/* select 1 */",
            out="/* select 1 */",
        )

    def test_line_comment_only(self):
        return DiffTestBlueprint(
            sql="-- noop",
            out="-- noop",
        )


# ── Trailing comments ──────────────────────────────────────────────────────────


class TrailingLineComment(TestSuite):
    def test_end_of_statement(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM t -- trailing",
            out="SELECT a FROM t; -- trailing",
        )

    def test_after_column(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT
                  a, -- first col
                  b
                FROM t
            """,
            out="""\
                SELECT
                  a, -- first col
                  b
                FROM t;
            """,
        )

    def test_after_where(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM t WHERE x = 1 -- filter active",
            out="SELECT a FROM t WHERE x = 1; -- filter active",
        )

    def test_after_select(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT -- trailing
                1
            """,
            out="""\
                SELECT -- trailing
                  1;
            """,
        )


# ── Leading comments ──────────────────────────────────────────────────────────


class LeadingLineComment(TestSuite):
    def test_before_statement(self):
        return DiffTestBlueprint(
            sql="""\
                -- main query
                SELECT a FROM t
            """,
            out="""\
                -- main query
                SELECT a FROM t;
            """,
        )

    def test_before_clause(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a
                -- apply filter
                FROM t
                WHERE x = 1
            """,
            out="""\
                SELECT a
                -- apply filter
                FROM t
                WHERE
                  x = 1;
            """,
        )


# ── Block comments ─────────────────────────────────────────────────────────────


class BlockComment(TestSuite):
    def test_before_statement(self):
        return DiffTestBlueprint(
            sql="/* count */ SELECT count(*) FROM t",
            out="""\
                /* count */
                SELECT count(*) FROM t;
            """,
        )

    def test_trailing_block(self):
        return DiffTestBlueprint(
            sql="SELECT a /* col */ FROM t",
            out="""\
                SELECT a
                /* col */ FROM t;
            """,
        )

    def test_trailing_block_after_from(self):
        return DiffTestBlueprint(
            sql="SELECT 1 FROM t /* block */ WHERE x = 1",
            out="""\
                SELECT 1
                FROM t
                /* block */ WHERE
                  x = 1;
            """,
        )

    def test_inline_block_comments(self):
        return DiffTestBlueprint(
            sql="SELECT /* c1 */ a, /* c2 */ b /* c3 */ FROM t",
            out="""\
                SELECT
                  /* c1 */ a,
                  /* c2 */ b
                /* c3 */ FROM t;
            """,
        )


# ── Multiple comments ─────────────────────────────────────────────────────────


class MultipleComments(TestSuite):
    def test_two_trailing(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT
                  a, -- first
                  b -- second
                FROM t
            """,
            out="""\
                SELECT
                  a, -- first
                  b -- second
                FROM t;
            """,
        )


# ── SELECT clause comments (every position) ───────────────────────────────────


class SelectClauseComment(TestSuite):
    """Comments before/between every clause of a SELECT statement."""

    def test_before_from(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1
                -- c
                FROM t
            """,
            out="""\
                SELECT 1
                -- c
                FROM t;
            """,
        )

    def test_before_column(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT
                -- c
                a FROM t
            """,
            out="""\
                SELECT
                  -- c
                  a
                FROM t;
            """,
        )

    def test_before_where(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t
                -- c
                WHERE x = 1
            """,
            out="""\
                SELECT 1
                FROM t
                -- c
                WHERE
                  x = 1;
            """,
        )

    def test_inside_where(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t WHERE
                -- c
                x = 1
            """,
            out="""\
                SELECT a
                FROM t
                WHERE
                  -- c
                  x = 1;
            """,
        )

    def test_between_and_conditions(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t WHERE x = 1 AND
                -- c
                y = 2
            """,
            out="""\
                SELECT a
                FROM t
                WHERE
                  x = 1
                  AND\x20
                  -- c
                  y = 2;
            """,
        )

    def test_before_group_by(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a, count(*) FROM t
                -- c
                GROUP BY a
            """,
            out="""\
                SELECT a, count(*)
                FROM t
                -- c
                GROUP BY
                  a;
            """,
        )

    def test_inside_group_by(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t GROUP BY
                -- c
                a
            """,
            out="""\
                SELECT a
                FROM t
                GROUP BY
                  -- c
                  a;
            """,
        )

    def test_before_having(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a, count(*) FROM t GROUP BY a
                -- c
                HAVING count(*) > 1
            """,
            out="""\
                SELECT a, count(*)
                FROM t
                GROUP BY
                  a
                -- c
                HAVING
                  count(*) > 1;
            """,
        )

    def test_before_order_by(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t
                -- c
                ORDER BY a
            """,
            out="""\
                SELECT a
                FROM t
                -- c
                ORDER BY
                  a;
            """,
        )

    def test_inside_order_by(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t ORDER BY
                -- c
                a DESC
            """,
            out="""\
                SELECT a
                FROM t
                ORDER BY
                  -- c
                  a DESC;
            """,
        )

    def test_before_limit(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t
                -- c
                LIMIT 10
            """,
            out="""\
                SELECT a
                FROM t
                -- c
                LIMIT 10;
            """,
        )

    def test_column_alias(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a AS
                -- c
                x FROM t
            """,
            out="""\
                SELECT
                  a AS\x20
                  -- c
                  x
                FROM t;
            """,
        )

    def test_union_all_comments(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1
                -- c1
                UNION ALL
                -- c2
                SELECT 2
            """,
            out="""\
                SELECT 1
                -- c1
                UNION ALL
                -- c2
                SELECT 2;
            """,
        )


# ── JOIN comment preservation ──────────────────────────────────────────────────


class JoinComment(TestSuite):
    """Comments at every position within JOIN clauses."""

    def test_before_join(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a
                -- c
                JOIN t2 b ON a.id = b.id
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                JOIN t2 AS b
                  ON a.id = b.id;
            """,
        )

    def test_before_on(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a JOIN t2 b
                -- c
                ON a.id = b.id
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                JOIN t2 AS b
                  -- c
                  ON a.id = b.id;
            """,
        )

    def test_before_using(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 JOIN t2 USING
                -- c
                (id)
            """,
            out="""\
                SELECT 1
                FROM t1
                JOIN t2
                -- c
                 USING (
                  id
                );
            """,
        )

    def test_inside_left_join(self):
        """Comment between LEFT and JOIN (multi-word keyword interior)."""
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a LEFT
                -- c
                JOIN t2 b ON a.id = b.id
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                LEFT JOIN t2 AS b
                  ON a.id = b.id;
            """,
        )

    def test_block_inside_left_join(self):
        return DiffTestBlueprint(
            sql="SELECT 1 FROM t1 a LEFT /* mid */ JOIN t2 b ON a.id = b.id",
            out="""\
                SELECT 1
                FROM t1 AS a
                /* mid */ LEFT JOIN t2 AS b
                  ON a.id = b.id;
            """,
        )

    def test_inside_right_join(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a RIGHT
                -- c
                JOIN t2 b ON a.id = b.id
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                RIGHT JOIN t2 AS b
                  ON a.id = b.id;
            """,
        )

    def test_inside_full_join(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a FULL
                -- c
                JOIN t2 b ON a.id = b.id
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                FULL JOIN t2 AS b
                  ON a.id = b.id;
            """,
        )

    def test_inside_cross_join(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a CROSS
                -- c
                JOIN t2 b
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                CROSS JOIN t2 AS b;
            """,
        )

    def test_inside_natural_join(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a NATURAL
                -- c
                JOIN t2 b
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                NATURAL JOIN t2 AS b;
            """,
        )

    def test_inside_natural_left_join(self):
        """3-word keyword: NATURAL LEFT JOIN."""
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a NATURAL LEFT
                -- c
                JOIN t2 b ON a.id = b.id
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c
                NATURAL LEFT JOIN t2 AS b
                  ON a.id = b.id;
            """,
        )

    def test_multiple_joins_with_comments(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM t1 a
                -- c1
                JOIN t2 b ON a.id = b.id
                -- c2
                LEFT JOIN t3 c ON c.id = a.id
                -- c3
                WHERE a.x = 1
            """,
            out="""\
                SELECT 1
                FROM t1 AS a
                -- c1
                JOIN t2 AS b
                  ON a.id = b.id
                -- c2
                LEFT JOIN t3 AS c
                  ON c.id = a.id
                -- c3
                WHERE
                  a.x = 1;
            """,
        )

    def test_multi_join_with_interior_comments(self):
        """Comments before ON and inside multi-word LEFT JOIN keyword."""
        return DiffTestBlueprint(
            sql="""\
                SELECT 1
                FROM orders o
                -- foo
                JOIN order_line_items li
                -- z
                ON li.order_id = o.order_id
                LEFT
                -- foo
                JOIN customers c
                -- x
                ON c.customer_id = o.customer_id
            """,
            out="""\
                SELECT 1
                FROM orders AS o
                -- foo
                JOIN order_line_items AS li
                  -- z
                  ON li.order_id = o.order_id
                -- foo
                LEFT JOIN customers AS c
                  -- x
                  ON c.customer_id = o.customer_id;
            """,
        )

    def test_original_bug_repro(self):
        """The original reported bug: comments between alias/JOIN and LEFT/JOIN."""
        return DiffTestBlueprint(
            sql="""\
                SELECT 1
                FROM orders o
                -- foo
                JOIN order_line_items li ON li.order_id = o.order_id
                LEFT
                -- bar
                JOIN customers c ON c.customer_id = o.customer_id
            """,
            out="""\
                SELECT 1
                FROM orders AS o
                -- foo
                JOIN order_line_items AS li
                  ON li.order_id = o.order_id
                -- bar
                LEFT JOIN customers AS c
                  ON c.customer_id = o.customer_id;
            """,
        )


# ── Table alias comments ──────────────────────────────────────────────────────


class TableAliasComment(TestSuite):
    def test_between_table_and_alias_no_as(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM orders
                -- c
                o
            """,
            out="""\
                SELECT 1
                FROM orders AS\x20
                -- c
                o;
            """,
        )

    def test_between_as_and_alias(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT 1 FROM orders AS
                -- c
                o
            """,
            out="""\
                SELECT 1
                FROM orders AS\x20
                -- c
                o;
            """,
        )


# ── DML statement comments ────────────────────────────────────────────────────


class DeleteComment(TestSuite):
    def test_before_where(self):
        return DiffTestBlueprint(
            sql="""\
                DELETE FROM t
                -- c
                WHERE x = 1
            """,
            out="""\
                DELETE FROM t
                -- c
                WHERE
                  x = 1;
            """,
        )


class UpdateComment(TestSuite):
    def test_before_set(self):
        return DiffTestBlueprint(
            sql="""\
                UPDATE t SET
                -- c
                x = 1
            """,
            out="""\
                UPDATE t
                SET
                  -- c
                  x = 1;
            """,
        )

    def test_before_where(self):
        return DiffTestBlueprint(
            sql="""\
                UPDATE t SET x = 1
                -- c
                WHERE y = 2
            """,
            out="""\
                UPDATE t
                SET
                  x = 1
                -- c
                WHERE
                  y = 2;
            """,
        )


class InsertComment(TestSuite):
    def test_before_values(self):
        return DiffTestBlueprint(
            sql="""\
                INSERT INTO t
                -- c
                VALUES (1)
            """,
            out="""\
                INSERT INTO t
                -- c
                VALUES (1);
            """,
        )

    def test_before_values_with_columns(self):
        return DiffTestBlueprint(
            sql="""\
                INSERT INTO t(a, b)
                -- c
                VALUES (1, 2)
            """,
            out="""\
                INSERT INTO t(a, b)
                -- c
                VALUES (1, 2);
            """,
        )


# ── Expression-level comments ─────────────────────────────────────────────────


class ExprComment(TestSuite):
    def test_before_not(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM t WHERE
                -- c
                NOT x = 1
            """,
            out="""\
                SELECT a
                FROM t
                WHERE
                  -- c
                  NOT (x = 1);
            """,
            idempotent=False,
        )

    def test_leading_comment_before_case_when(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT
                  CASE value
                    -- Display.STATE_OFF
                    WHEN 1 THEN 'off'
                    -- Display.STATE_ON
                    WHEN 2 THEN 'on'
                    ELSE 'unknown'
                  END AS name
                FROM t
            """,
            out="""\
                SELECT
                  CASE value
                    -- Display.STATE_OFF
                    WHEN 1 THEN 'off'
                    -- Display.STATE_ON
                    WHEN 2 THEN 'on'
                    ELSE 'unknown'
                  END AS name
                FROM t;
            """,
        )


# ── CREATE TABLE comments ─────────────────────────────────────────────────────


class CreateTableComment(TestSuite):
    def test_before_column_defs(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE TABLE t (
                -- c
                a int, b text)
            """,
            out="""\
                CREATE TABLE t(
                  -- c
                  a int,
                  b text
                );
            """,
        )


# ── CTE comments ──────────────────────────────────────────────────────────────


# ── Comment block spacing ─────────────────────────────────────────────────────


class CommentBlockSpacing(TestSuite):
    """Blank lines between separate comment blocks are preserved,
    but NOT between code and comments or within a single block."""

    def test_blank_line_between_comment_blocks_preserved(self):
        return DiffTestBlueprint(
            sql="""\
                -- block one
                -- block one cont

                -- block two
                -- block two cont
                SELECT 1
            """,
            out="""\
                -- block one
                -- block one cont

                -- block two
                -- block two cont
                SELECT 1;
            """,
        )

    def test_no_blank_line_within_contiguous_block(self):
        return DiffTestBlueprint(
            sql="""\
                -- line 1
                -- line 2
                -- line 3
                SELECT 1
            """,
            out="""\
                -- line 1
                -- line 2
                -- line 3
                SELECT 1;
            """,
        )

    def test_no_blank_line_between_code_and_comment(self):
        """After UNION ALL, no blank line before comment."""
        return DiffTestBlueprint(
            sql="""\
                SELECT 1

                UNION ALL

                -- part two
                SELECT 2
            """,
            out="""\
                SELECT 1
                UNION ALL
                -- part two
                SELECT 2;
            """,
        )

    def test_multi_block_comments_between_statements(self):
        return DiffTestBlueprint(
            sql="""\
                -- copyright header
                -- license text

                -- module docs
                -- more docs
                SELECT 1
            """,
            out="""\
                -- copyright header
                -- license text

                -- module docs
                -- more docs
                SELECT 1;
            """,
        )

    def test_leading_comment_inside_parens_no_blank_line(self):
        # A leading comment immediately inside an opening paren
        # shouldn't render with a blank line between the enclosing
        # keyword and the comment.  The paren itself is preserved via
        # `ParenExpr`, so the comment lands inside the paren body.
        return DiffTestBlueprint(
            sql="""\
                SELECT (
                -- c
                1)
            """,
            out="""\
                SELECT
                  (
                  -- c
                  1);
            """,
        )

    def test_blank_line_between_file_header_and_first_statement(self):
        # A file-level header comment block separated from the first
        # statement by a blank line keeps that blank line on output.
        return DiffTestBlueprint(
            sql="""\
                -- header a
                -- header b

                SELECT 1
            """,
            out="""\
                -- header a
                -- header b

                SELECT 1;
            """,
        )


class ListCommentSpacing(TestSuite):
    """Multi-line comments inside list nodes (e.g., column defs)
    should not get spurious blank lines between comment lines."""

    def test_multiline_comment_before_column_def(self):
        return DiffTestBlueprint(
            sql="""\
                CREATE TABLE t (
                -- line 1
                -- line 2
                a int,
                b text)
            """,
            out="""\
                CREATE TABLE t(
                  -- line 1
                  -- line 2
                  a int,
                  b text
                );
            """,
        )

    def test_multiline_comment_between_columns(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT
                  a,
                  -- comment line 1
                  -- comment line 2
                  b
                FROM t
            """,
            out="""\
                SELECT
                  a,
                  -- comment line 1
                  -- comment line 2
                  b
                FROM t;
            """,
        )


# ── Star column comments ─────────────────────────────────────────────────────


class StarColumnComment(TestSuite):
    def test_after_star(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT *
                -- about from
                FROM t
            """,
            out="""\
                SELECT *
                -- about from
                FROM t;
            """,
        )

    def test_trailing_on_select(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT -- pick cols
                a FROM t
            """,
            out="""\
                SELECT -- pick cols
                  a
                FROM t;
            """,
        )

    def test_trailing_not_dropped_when_followed_by_line_comment(self):
        return DiffTestBlueprint(
            sql="""\
                select a, b
                -- y
                from t -- x
                -- z
                where c = 1
            """,
            out="""\
                SELECT a, b
                -- y
                FROM t -- x
                -- z
                WHERE
                  c = 1;
            """,
        )


# ── JOIN comments (simple) ───────────────────────────────────────────────────


class SimpleJoinComment(TestSuite):
    def test_before_join_no_on(self):
        return DiffTestBlueprint(
            sql="""\
                SELECT a FROM slice
                -- before join
                JOIN track
            """,
            out="""\
                SELECT a
                FROM slice
                -- before join
                JOIN track;
            """,
        )


# ── Multi-statement comments ────────────────────────────────────────────────


class MultiStatementComment(TestSuite):
    def test_basic(self):
        return DiffTestBlueprint(
            sql="SELECT 1;\nSELECT 2",
            out="""\
                SELECT 1;

                SELECT 2;
            """,
        )

    def test_comment_between(self):
        return DiffTestBlueprint(
            sql="SELECT 1;\n-- between\nSELECT 2",
            out="""\
                SELECT 1;

                -- between
                SELECT 2;
            """,
        )

    def test_trailing_after_first(self):
        return DiffTestBlueprint(
            sql="SELECT 1; -- after first\nSELECT 2",
            out="""\
                SELECT 1; -- after first

                SELECT 2;
            """,
        )

    def test_comment_before_first(self):
        return DiffTestBlueprint(
            sql="-- header\nSELECT 1",
            out="""\
                -- header
                SELECT 1;
            """,
        )

    def test_multi_stmt_three_comments(self):
        return DiffTestBlueprint(
            sql="SELECT 1;\n\n-- foo\nselect\n-- foo bar\n1\n-- foo\nfrom slice;",
            out="""\
                SELECT 1;

                -- foo
                SELECT
                  -- foo bar
                  1
                -- foo
                FROM slice;
            """,
        )


class CteComment(TestSuite):
    def test_before_cte_body(self):
        return DiffTestBlueprint(
            sql="""\
                WITH cte AS
                -- c
                (SELECT 1)
                SELECT * FROM cte
            """,
            out="""\
                WITH
                  cte AS\x20
                  -- c
                  (
                    SELECT 1
                  )
                SELECT * FROM cte;
            """,
        )
