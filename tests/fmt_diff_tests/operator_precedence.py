# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


# Precedence levels (from _common.y, low to high):
#   1: OR
#   2: AND                (paren_boundary)
#   3: EQ, NE, IS, LIKE, GLOB, MATCH, REGEXP, BETWEEN, IN
#   4: LT, GT, LE, GE
#   5: BIT_AND, BIT_OR, LSHIFT, RSHIFT
#   6: PLUS, MINUS
#   7: STAR, SLASH, REM
#   8: CONCAT, PTR
#   9: COLLATE
#
# Operator groups (cross-group always gets parens for readability):
#   STANDARD (0): OR, AND, EQ, NE, LT, GT, LE, GE, PLUS, MINUS, STAR, SLASH,
#                 REM, CONCAT, PTR, IS, LIKE, BETWEEN, IN, COLLATE
#   BITWISE  (1): BIT_AND, BIT_OR, LSHIFT, RSHIFT
#
# Paren boundary: AND has the paren_boundary flag. When AND appears as a child
# of a different-precedence operator in the same group, readability parens are
# added. This gives us `(a AND b) OR c` without adding parens everywhere.


class OrAndPrecedence(TestSuite):
    """OR (prec 1) vs AND (prec 2): AND has paren_boundary."""

    def test_and_in_or_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a AND b OR c AND d",
            out="SELECT (a AND b) OR (c AND d);",
        )

    def test_or_in_and_left(self):
        return DiffTestBlueprint(
            sql="SELECT (a OR b) AND c",
            out="SELECT (a OR b) AND c;",
        )

    def test_or_in_and_right(self):
        return DiffTestBlueprint(
            sql="SELECT a AND (b OR c)",
            out="SELECT a AND (b OR c);",
        )

    def test_or_in_and_both(self):
        return DiffTestBlueprint(
            sql="SELECT (a OR b) AND (c OR d)",
            out="SELECT (a OR b) AND (c OR d);",
        )

    def test_chained_or(self):
        return DiffTestBlueprint(
            sql="SELECT a OR b OR c",
            out="SELECT a OR b OR c;",
        )

    def test_chained_and(self):
        return DiffTestBlueprint(
            sql="SELECT a AND b AND c",
            out="SELECT a AND b AND c;",
        )

    def test_three_ands_in_or(self):
        """a AND b AND c OR d → only one set of parens around the AND chain."""
        return DiffTestBlueprint(
            sql="SELECT a AND b AND c OR d",
            out="SELECT (a AND b AND c) OR d;",
        )


class EqualityComparisonPrecedence(TestSuite):
    """EQ/NE (prec 3) vs LT/GT/LE/GE (prec 4): same group, no readability parens."""

    def test_comparison_in_equality_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a > b = c < d",
            out="SELECT a > b = c < d;",
        )

    def test_eq_in_comparison(self):
        return DiffTestBlueprint(
            sql="SELECT (a = b) > (c = d)",
            out="SELECT (a = b) > (c = d);",
        )

    def test_ne_in_lt(self):
        return DiffTestBlueprint(
            sql="SELECT (a != b) < c",
            out="SELECT (a != b) < c;",
        )

    def test_eq_and_ne(self):
        return DiffTestBlueprint(
            sql="SELECT a = b != c",
            out="SELECT a = b != c;",
        )

    def test_right_assoc_eq_in_ge(self):
        return DiffTestBlueprint(
            sql="SELECT a >= (b = c)",
            out="SELECT a >= (b = c);",
        )


class AndEqualityPrecedence(TestSuite):
    """AND (prec 2) vs EQ/NE (prec 3): same group, no readability parens."""

    def test_eq_in_and_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a = 1 AND b = 2",
            out="SELECT a = 1 AND b = 2;",
        )

    def test_and_in_eq(self):
        return DiffTestBlueprint(
            sql="SELECT (a AND b) = (c AND d)",
            out="SELECT (a AND b) = (c AND d);",
        )

    def test_eq_in_or_no_boundary(self):
        """EQ has no paren_boundary, so no readability parens inside OR."""
        return DiffTestBlueprint(
            sql="SELECT a = 1 OR b = 2",
            out="SELECT a = 1 OR b = 2;",
        )


class ArithmeticPrecedence(TestSuite):
    """PLUS/MINUS (prec 6) vs STAR/SLASH/REM (prec 7): same group."""

    def test_mul_in_add_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a + b * c",
            out="SELECT a + b * c;",
        )

    def test_add_in_mul(self):
        return DiffTestBlueprint(
            sql="SELECT (a + b) * c",
            out="SELECT (a + b) * c;",
        )

    def test_sub_in_div(self):
        return DiffTestBlueprint(
            sql="SELECT (a - b) / c",
            out="SELECT (a - b) / c;",
        )

    def test_mul_add_mul(self):
        return DiffTestBlueprint(
            sql="SELECT a * b + c * d",
            out="SELECT a * b + c * d;",
        )

    def test_rem_in_add(self):
        return DiffTestBlueprint(
            sql="SELECT a + b % c",
            out="SELECT a + b % c;",
        )

    def test_add_in_rem(self):
        return DiffTestBlueprint(
            sql="SELECT (a + b) % c",
            out="SELECT (a + b) % c;",
        )


class SamePrecAssociativity(TestSuite):
    """Same-precedence left-associativity: right-child needs parens."""

    def test_sub_right_assoc(self):
        return DiffTestBlueprint(
            sql="SELECT a - (b + c)",
            out="SELECT a - (b + c);",
        )

    def test_sub_right_assoc_sub(self):
        return DiffTestBlueprint(
            sql="SELECT a - (b - c)",
            out="SELECT a - (b - c);",
        )

    def test_sub_left_assoc_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a - b + c",
            out="SELECT a - b + c;",
        )

    def test_div_right_assoc(self):
        return DiffTestBlueprint(
            sql="SELECT a / (b * c)",
            out="SELECT a / (b * c);",
        )

    def test_div_right_assoc_rem(self):
        return DiffTestBlueprint(
            sql="SELECT a / (b % c)",
            out="SELECT a / (b % c);",
        )

    def test_mul_left_assoc_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a * b / c",
            out="SELECT a * b / c;",
        )


class BitwiseOpsPrecedence(TestSuite):
    """BIT_AND, BIT_OR, LSHIFT, RSHIFT all at prec 5, same group."""

    def test_bitand_bitor_left_assoc(self):
        return DiffTestBlueprint(
            sql="SELECT a & b | c",
            out="SELECT a & b | c;",
        )

    def test_bitor_in_bitand_right(self):
        return DiffTestBlueprint(
            sql="SELECT a & (b | c)",
            out="SELECT a & (b | c);",
        )

    def test_lshift_rshift_same_prec(self):
        return DiffTestBlueprint(
            sql="SELECT a << b >> c",
            out="SELECT a << b >> c;",
        )

    def test_rshift_in_lshift_right(self):
        return DiffTestBlueprint(
            sql="SELECT a << (b >> c)",
            out="SELECT a << (b >> c);",
        )

    def test_bitand_in_lshift_same_prec(self):
        return DiffTestBlueprint(
            sql="SELECT a << b & c",
            out="SELECT a << b & c;",
        )

    def test_lshift_in_bitand_right(self):
        return DiffTestBlueprint(
            sql="SELECT a & (b << c)",
            out="SELECT a & (b << c);",
        )


class BitwiseVsStandardPrecedence(TestSuite):
    """Bitwise (group 1) vs standard (group 0): cross-group, parens added."""

    def test_add_in_bitand_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a + b & c + d",
            out="SELECT (a + b) & (c + d);",
        )

    def test_bitand_in_add(self):
        return DiffTestBlueprint(
            sql="SELECT (a & b) + c",
            out="SELECT (a & b) + c;",
        )

    def test_mul_in_bitor_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a * b | c * d",
            out="SELECT (a * b) | (c * d);",
        )

    def test_lshift_in_mul(self):
        return DiffTestBlueprint(
            sql="SELECT (a << b) * c",
            out="SELECT (a << b) * c;",
        )

    def test_concat_in_bitand(self):
        return DiffTestBlueprint(
            sql="SELECT a || b & c || d",
            out="SELECT (a || b) & (c || d);",
        )

    def test_bitand_in_gt_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a & b < c & d",
            out="SELECT (a & b) < (c & d);",
        )

    def test_lt_in_bitor(self):
        return DiffTestBlueprint(
            sql="SELECT (a < b) | c",
            out="SELECT (a < b) | c;",
        )


class ConcatPtrPrecedence(TestSuite):
    """CONCAT/PTR (prec 8) — highest among binary ops, standard group."""

    def test_concat_chain(self):
        return DiffTestBlueprint(
            sql="SELECT a || b || c",
            out="SELECT a || b || c;",
        )

    def test_concat_right_assoc(self):
        return DiffTestBlueprint(
            sql="SELECT a || (b || c)",
            out="SELECT a || (b || c);",
        )

    def test_add_in_concat(self):
        return DiffTestBlueprint(
            sql="SELECT (a + b) || c",
            out="SELECT (a + b) || c;",
        )

    def test_concat_in_add_no_parens(self):
        """Concat is same group as arithmetic — higher prec, no readability parens."""
        return DiffTestBlueprint(
            sql="SELECT a || b + c || d",
            out="SELECT a || b + c || d;",
        )

    def test_ptr_and_concat_same_prec(self):
        return DiffTestBlueprint(
            sql="SELECT a -> b || c",
            out="SELECT a -> b || c;",
        )

    def test_concat_in_ptr_right(self):
        return DiffTestBlueprint(
            sql="SELECT a -> (b || c)",
            out="SELECT a -> (b || c);",
        )

    def test_ptr2_preserved(self):
        return DiffTestBlueprint(
            sql="SELECT a ->> b",
            out="SELECT a ->> b;",
        )

    def test_ptr_and_ptr2_chain(self):
        return DiffTestBlueprint(
            sql="SELECT a -> b ->> c",
            out="SELECT a -> b ->> c;",
        )

    def test_ptr2_with_string(self):
        return DiffTestBlueprint(
            sql="SELECT data ->> '$.name'",
            out="SELECT data ->> '$.name';",
        )


class ArithmeticVsComparisonPrecedence(TestSuite):
    """Arithmetic (prec 6/7) vs comparison (prec 4): same group, no readability parens."""

    def test_add_in_gt_no_parens(self):
        """a + b > c - d: arithmetic binds tighter, same group → no parens."""
        return DiffTestBlueprint(
            sql="SELECT a + b > c - d",
            out="SELECT a + b > c - d;",
        )

    def test_gt_in_add(self):
        return DiffTestBlueprint(
            sql="SELECT (a > b) + c",
            out="SELECT (a > b) + c;",
        )

    def test_mul_in_le_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a * b <= c / d",
            out="SELECT a * b <= c / d;",
        )


class NotWithBinaryExpr(TestSuite):
    """NOT (unary) wrapping binary expressions."""

    def test_not_and(self):
        return DiffTestBlueprint(
            sql="SELECT NOT (a AND b)",
            out="SELECT NOT (a AND b);",
        )

    def test_not_or(self):
        return DiffTestBlueprint(
            sql="SELECT NOT (a OR b)",
            out="SELECT NOT (a OR b);",
        )

    def test_not_eq(self):
        return DiffTestBlueprint(
            sql="SELECT NOT (a = b)",
            out="SELECT NOT (a = b);",
        )

    def test_not_gt(self):
        return DiffTestBlueprint(
            sql="SELECT NOT (a > b)",
            out="SELECT NOT (a > b);",
        )

    def test_not_add(self):
        return DiffTestBlueprint(
            sql="SELECT NOT (a + b)",
            out="SELECT NOT (a + b);",
        )

    def test_not_concat(self):
        return DiffTestBlueprint(
            sql="SELECT NOT (a || b)",
            out="SELECT NOT (a || b);",
        )


class IsExprPrecedence(TestSuite):
    """IS/ISNULL/NOTNULL (prec 3, group 0) in the global precedence system."""

    def test_is_null_no_parens_in_and(self):
        return DiffTestBlueprint(
            sql="SELECT a ISNULL AND b NOTNULL",
            out="SELECT a ISNULL AND b NOTNULL;",
        )

    def test_is_in_or_no_boundary(self):
        """IS has no boundary flag — no readability parens inside OR."""
        return DiffTestBlueprint(
            sql="SELECT a IS NULL OR b IS NOT NULL",
            out="SELECT a IS NULL OR b IS NOT NULL;",
        )

    def test_or_in_is_gets_parens(self):
        """OR (prec 1) inside IS (prec 3) needs correctness parens."""
        return DiffTestBlueprint(
            sql="SELECT (a OR b) IS NULL",
            out="SELECT (a OR b) IS NULL;",
        )

    def test_and_in_is_gets_parens(self):
        """AND (prec 2) inside IS (prec 3) needs correctness parens."""
        return DiffTestBlueprint(
            sql="SELECT (a AND b) IS NOT NULL",
            out="SELECT (a AND b) IS NOT NULL;",
        )

    def test_add_in_isnull_no_parens(self):
        """Arithmetic (prec 6) in ISNULL (prec 3) — higher prec, no parens."""
        return DiffTestBlueprint(
            sql="SELECT a + b ISNULL",
            out="SELECT a + b ISNULL;",
        )

    def test_is_distinct_with_comparison(self):
        return DiffTestBlueprint(
            sql="SELECT a IS DISTINCT FROM b AND c IS NOT DISTINCT FROM d",
            out="SELECT a IS DISTINCT FROM b AND c IS NOT DISTINCT FROM d;",
        )


class LikeExprPrecedence(TestSuite):
    """LIKE/GLOB/MATCH/REGEXP (prec 3, group 0) in the global precedence system."""

    def test_like_in_and_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a LIKE 'foo' AND b LIKE 'bar'",
            out="SELECT a LIKE 'foo' AND b LIKE 'bar';",
        )

    def test_like_in_or_no_boundary(self):
        return DiffTestBlueprint(
            sql="SELECT a LIKE 'foo' OR b LIKE 'bar'",
            out="SELECT a LIKE 'foo' OR b LIKE 'bar';",
        )

    def test_or_in_like_gets_parens(self):
        """OR (prec 1) inside LIKE (prec 3) needs correctness parens."""
        return DiffTestBlueprint(
            sql="SELECT (a OR b) LIKE 'foo'",
            out="SELECT (a OR b) LIKE 'foo';",
        )

    def test_and_in_like_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT (a AND b) LIKE 'foo'",
            out="SELECT (a AND b) LIKE 'foo';",
        )

    def test_add_in_like_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a + b LIKE 'foo'",
            out="SELECT a + b LIKE 'foo';",
        )

    def test_glob_preserves_keyword(self):
        """GLOB keyword should be preserved (not rewritten to LIKE)."""
        return DiffTestBlueprint(
            sql="SELECT a GLOB 'foo*'",
            out="SELECT a GLOB 'foo*';",
        )

    def test_like_with_escape(self):
        return DiffTestBlueprint(
            sql="SELECT a LIKE 'foo%' ESCAPE '\\'",
            out="SELECT a LIKE 'foo%' ESCAPE '\\';",
        )

    def test_not_like_in_and(self):
        return DiffTestBlueprint(
            sql="SELECT a NOT LIKE 'foo' AND b NOT LIKE 'bar'",
            out="SELECT a NOT LIKE 'foo' AND b NOT LIKE 'bar';",
        )


class BetweenExprPrecedence(TestSuite):
    """BETWEEN (prec 3, group 0) in the global precedence system."""

    def test_between_in_and_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a BETWEEN 1 AND 10 AND b BETWEEN 20 AND 30",
            out="SELECT a BETWEEN 1 AND 10 AND b BETWEEN 20 AND 30;",
        )

    def test_between_in_or_no_boundary(self):
        return DiffTestBlueprint(
            sql="SELECT a BETWEEN 1 AND 10 OR b BETWEEN 20 AND 30",
            out="SELECT a BETWEEN 1 AND 10 OR b BETWEEN 20 AND 30;",
        )

    def test_or_in_between_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT (a OR b) BETWEEN 1 AND 10",
            out="SELECT (a OR b) BETWEEN 1 AND 10;",
        )

    def test_add_in_between_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a + b BETWEEN 1 AND 10",
            out="SELECT a + b BETWEEN 1 AND 10;",
        )

    def test_not_between(self):
        return DiffTestBlueprint(
            sql="SELECT a NOT BETWEEN 1 AND 10",
            out="SELECT a NOT BETWEEN 1 AND 10;",
        )


class InExprPrecedence(TestSuite):
    """IN (prec 3, group 0) in the global precedence system."""

    def test_in_in_and_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a IN (1, 2) AND b IN (3, 4)",
            out="SELECT a IN (1, 2) AND b IN (3, 4);",
        )

    def test_in_in_or_no_boundary(self):
        return DiffTestBlueprint(
            sql="SELECT a IN (1, 2) OR b IN (3, 4)",
            out="SELECT a IN (1, 2) OR b IN (3, 4);",
        )

    def test_or_in_in_gets_parens(self):
        return DiffTestBlueprint(
            sql="SELECT (a OR b) IN (1, 2)",
            out="SELECT (a OR b) IN (1, 2);",
        )

    def test_add_in_in_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a + b IN (1, 2)",
            out="SELECT a + b IN (1, 2);",
        )

    def test_not_in(self):
        return DiffTestBlueprint(
            sql="SELECT a NOT IN (1, 2, 3)",
            out="SELECT a NOT IN (1, 2, 3);",
        )

    def test_in_bare_table_name(self):
        # `x IN table` tests membership in the (single-column) table.
        # Wrapping as `x IN (table)` would change the meaning to a
        # one-element value list with `table` resolved as a column or
        # variable — so the formatter must keep the bare form on both
        # the input and the output.
        return DiffTestBlueprint(
            sql="SELECT x IN my_table FROM t",
            out="SELECT x IN my_table FROM t;",
        )

    def test_not_in_bare_table_name(self):
        return DiffTestBlueprint(
            sql="SELECT x NOT IN my_table FROM t",
            out="SELECT x NOT IN my_table FROM t;",
        )

    def test_in_schema_qualified_table(self):
        return DiffTestBlueprint(
            sql="SELECT x IN my_schema.my_table FROM t",
            out="SELECT x IN my_schema.my_table FROM t;",
        )

    def test_not_in_schema_qualified_table(self):
        return DiffTestBlueprint(
            sql="SELECT x NOT IN my_schema.my_table FROM t",
            out="SELECT x NOT IN my_schema.my_table FROM t;",
        )

    def test_in_table_valued_function_single_arg(self):
        # `x IN tvf(a)` — table-valued function, bare form too.
        return DiffTestBlueprint(
            sql="SELECT x IN my_tvf(1) FROM t",
            out="SELECT x IN my_tvf(1) FROM t;",
        )

    def test_in_table_valued_function_multi_arg(self):
        return DiffTestBlueprint(
            sql="SELECT x IN my_tvf(1, 2, 3) FROM t",
            out="SELECT x IN my_tvf(1, 2, 3) FROM t;",
        )

    def test_in_table_valued_function_no_args_preserves_parens(self):
        # `tvf()` (empty parens) and `tvf` (bare name) are distinct
        # productions in the SQLite grammar: the first is a zero-arg
        # table-valued function call, the second is a bare table
        # reference. Both forms resolve to the same thing for a TVF
        # that takes zero args, but the formatter must round-trip the
        # authored syntax.
        return DiffTestBlueprint(
            sql="SELECT x IN my_tvf() FROM t",
            out="SELECT x IN my_tvf() FROM t;",
        )

    def test_from_table_valued_function_no_args_preserves_parens(self):
        # Same distinction in `FROM` position.
        return DiffTestBlueprint(
            sql="SELECT * FROM my_tvf()",
            out="SELECT * FROM my_tvf();",
        )

    def test_from_bare_table_no_parens(self):
        # And the bare form keeps its bare form.
        return DiffTestBlueprint(
            sql="SELECT * FROM my_tbl",
            out="SELECT * FROM my_tbl;",
        )

    def test_in_schema_qualified_tvf(self):
        return DiffTestBlueprint(
            sql="SELECT x IN my_schema.my_tvf(1) FROM t",
            out="SELECT x IN my_schema.my_tvf(1) FROM t;",
        )

    def test_not_in_tvf(self):
        return DiffTestBlueprint(
            sql="SELECT x NOT IN my_tvf(1) FROM t",
            out="SELECT x NOT IN my_tvf(1) FROM t;",
        )

    def test_in_bare_table_chained_with_and(self):
        # Two IN-bare-table expressions joined by AND — each must keep
        # its bare form independently.
        return DiffTestBlueprint(
            sql="SELECT x IN tbl_a AND y IN tbl_b FROM t",
            out="SELECT x IN tbl_a AND y IN tbl_b FROM t;",
        )

    def test_in_bare_table_in_where(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t WHERE id IN valid_ids",
            out="SELECT * FROM t WHERE id IN valid_ids;",
        )

    def test_in_bare_table_inside_case(self):
        return DiffTestBlueprint(
            sql="SELECT CASE WHEN x IN tbl THEN 1 ELSE 0 END FROM t",
            out="SELECT CASE WHEN x IN tbl THEN 1 ELSE 0 END FROM t;",
        )

    def test_in_bare_table_vs_list_distinct(self):
        # The value-list form `IN (a)` MUST stay parenthesised — it's
        # grammatically and semantically distinct from `IN a`. Keeping
        # the existing single-element-list test as a sibling assertion
        # that the formatter hasn't merged the two forms.
        return DiffTestBlueprint(
            sql="SELECT x IN (1) FROM t",
            out="SELECT x IN (1) FROM t;",
        )

    def test_in_bare_table_iif_perfetto_pattern(self):
        # The exact shape that triggered the original stdlib bug:
        # `iif(constant IN _table, ...)`. Locks in the canonical form.
        return DiffTestBlueprint(
            sql="SELECT iif(0 IN _device_policies, 1, 0) AS flag FROM t",
            out="SELECT iif(0 IN _device_policies, 1, 0) AS flag FROM t;",
        )


class CollateExprPrecedence(TestSuite):
    """COLLATE (prec 9, group 0) in the global precedence system."""

    def test_collate_in_eq_no_parens(self):
        return DiffTestBlueprint(
            sql="SELECT a COLLATE nocase = b COLLATE nocase",
            out="SELECT a COLLATE nocase = b COLLATE nocase;",
        )

    def test_add_in_collate_gets_parens(self):
        """Arithmetic (prec 6) inside COLLATE (prec 9) needs correctness parens."""
        return DiffTestBlueprint(
            sql="SELECT (a + b) COLLATE nocase",
            out="SELECT (a + b) COLLATE nocase;",
        )

    def test_collate_in_add_no_parens(self):
        """COLLATE (prec 9) inside add (prec 6) — higher prec, no parens."""
        return DiffTestBlueprint(
            sql="SELECT a COLLATE nocase + b",
            out="SELECT a COLLATE nocase + b;",
        )

    def test_concat_in_collate_gets_parens(self):
        """Concat (prec 8) inside COLLATE (prec 9) needs correctness parens."""
        return DiffTestBlueprint(
            sql="SELECT (a || b) COLLATE nocase",
            out="SELECT (a || b) COLLATE nocase;",
        )


class DeepNesting(TestSuite):
    """Multi-level nesting across precedence boundaries."""

    def test_three_levels(self):
        return DiffTestBlueprint(
            sql="SELECT (a + b) * c > d AND e",
            out="SELECT (a + b) * c > d AND e;",
        )

    def test_or_and_eq_add_mul(self):
        return DiffTestBlueprint(
            sql="SELECT a * b + c = d AND e OR f",
            out="SELECT (a * b + c = d AND e) OR f;",
        )

    def test_complex_parens_preserved(self):
        return DiffTestBlueprint(
            sql="SELECT (a OR b) AND (c + d) > (e * f)",
            out="SELECT (a OR b) AND c + d > e * f;",
        )

    def test_bitwise_in_comparison_in_and(self):
        return DiffTestBlueprint(
            sql="SELECT a & b > 0 AND c | d < 10",
            out="SELECT (a & b) > 0 AND (c | d) < 10;",
        )

    def test_all_levels(self):
        return DiffTestBlueprint(
            sql="SELECT a || b * c + d & e > f = g AND h OR i",
            out="SELECT (((a || b * c + d) & e) > f = g AND h) OR i;",
        )

    def test_like_and_between_in_or(self):
        return DiffTestBlueprint(
            sql="SELECT a LIKE 'foo' AND b BETWEEN 1 AND 10 OR c IN (1, 2)",
            out="SELECT (a LIKE 'foo' AND b BETWEEN 1 AND 10) OR c IN (1, 2);",
        )


class InWhereClause(TestSuite):
    """Precedence in WHERE clause context (common real-world usage)."""

    def test_where_and_or(self):
        return DiffTestBlueprint(
            sql="SELECT x FROM t WHERE a = 1 AND (b = 2 OR c = 3)",
            out="SELECT x FROM t WHERE a = 1 AND (b = 2 OR c = 3);",
        )

    def test_where_arithmetic_comparison(self):
        """Arithmetic and comparison are same group — no readability parens."""
        return DiffTestBlueprint(
            sql="SELECT x FROM t WHERE a + b > c * d",
            out="SELECT x FROM t WHERE a + b > c * d;",
        )

    def test_where_not_compound(self):
        return DiffTestBlueprint(
            sql="SELECT x FROM t WHERE NOT (a = 1 OR b = 2)",
            out="SELECT x FROM t WHERE NOT (a = 1 OR b = 2);",
        )

    def test_where_like_and_in(self):
        return DiffTestBlueprint(
            sql="SELECT x FROM t WHERE name LIKE 'foo%' AND id IN (1, 2, 3)",
            out="SELECT x FROM t WHERE name LIKE 'foo%' AND id IN (1, 2, 3);",
        )
