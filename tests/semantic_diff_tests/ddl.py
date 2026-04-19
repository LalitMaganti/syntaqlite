# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class DDLAccumulation(TestSuite):
    def test_create_table_then_select(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER); SELECT id FROM t;",
            strict_schema=True,
            out="",
        )

    def test_create_view_then_select(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER); CREATE VIEW vw AS SELECT id FROM users; SELECT id FROM vw;",
            strict_schema=True,
            out="",
        )

    def test_create_table_unknown_column_flagged(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (id INTEGER); SELECT missing FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'missing'
             --> <stdin>:1:37
              |
            1 | CREATE TABLE t (id INTEGER); SELECT missing FROM t;
              |                                     ^~~~~~~
""",
        )


class CreateTableAsSelect(TestSuite):
    def test_aliased_column_valid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1 AS x; SELECT x FROM t;",
            strict_schema=True,
            out="",
        )

    def test_aliased_column_invalid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1 AS x; SELECT z FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'z'
             --> <stdin>:1:41
              |
            1 | CREATE TABLE t AS SELECT 1 AS x; SELECT z FROM t;
              |                                         ^
              = help: did you mean 'x'?
""",
        )

    def test_unaliased_literal_flags_wrong_name(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1; SELECT t.order_id FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'order_id' in table 't'
             --> <stdin>:1:38
              |
            1 | CREATE TABLE t AS SELECT 1; SELECT t.order_id FROM t;
              |                                      ^~~~~~~~
""",
        )

    def test_unaliased_null_flags_wrong_name(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT NULL; SELECT t.order_id FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'order_id' in table 't'
             --> <stdin>:1:41
              |
            1 | CREATE TABLE t AS SELECT NULL; SELECT t.order_id FROM t;
              |                                         ^~~~~~~~
""",
        )

    def test_unaliased_binary_expr_flags_wrong_name(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1+2; SELECT t.order_id FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'order_id' in table 't'
             --> <stdin>:1:40
              |
            1 | CREATE TABLE t AS SELECT 1+2; SELECT t.order_id FROM t;
              |                                        ^~~~~~~~
""",
        )

    def test_unaliased_function_call_flags_wrong_name(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT abs(1); SELECT t.order_id FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'order_id' in table 't'
             --> <stdin>:1:43
              |
            1 | CREATE TABLE t AS SELECT abs(1); SELECT t.order_id FROM t;
              |                                           ^~~~~~~~
""",
        )

    def test_multiple_unnamed_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1, 2; SELECT t.order_id FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'order_id' in table 't'
             --> <stdin>:1:41
              |
            1 | CREATE TABLE t AS SELECT 1, 2; SELECT t.order_id FROM t;
              |                                         ^~~~~~~~
""",
        )

    def test_mixed_named_unnamed_named_valid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1, 2 AS y; SELECT t.y FROM t;",
            strict_schema=True,
            out="",
        )

    def test_mixed_named_unnamed_wrong(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t AS SELECT 1, 2 AS y; SELECT t.z FROM t;",
            strict_schema=True,
            out="""\
            error: unknown column 'z' in table 't'
             --> <stdin>:1:46
              |
            1 | CREATE TABLE t AS SELECT 1, 2 AS y; SELECT t.z FROM t;
              |                                              ^
              = help: did you mean '1'?
""",
        )

    def test_star_accepts_any_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE src (id INTEGER); CREATE TABLE t AS SELECT * FROM src; SELECT t.anything FROM t;",
            strict_schema=True,
            out="",
        )


class CreateViewAsSelect(TestSuite):
    def test_aliased_column_valid(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1 AS x; SELECT x FROM v;",
            strict_schema=True,
            out="",
        )

    def test_aliased_column_invalid(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1 AS x; SELECT z FROM v;",
            strict_schema=True,
            out="""\
            error: unknown column 'z'
             --> <stdin>:1:40
              |
            1 | CREATE VIEW v AS SELECT 1 AS x; SELECT z FROM v;
              |                                        ^
              = help: did you mean 'x'?
""",
        )

    def test_unaliased_literal_flags_wrong_name(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1; SELECT v.order_id FROM v;",
            strict_schema=True,
            out="""\
            error: unknown column 'order_id' in table 'v'
             --> <stdin>:1:37
              |
            1 | CREATE VIEW v AS SELECT 1; SELECT v.order_id FROM v;
              |                                     ^~~~~~~~
""",
        )
