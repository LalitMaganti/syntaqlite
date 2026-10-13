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


class CreateIndexTarget(TestSuite):
    def test_known_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER); CREATE INDEX i ON t (a);",
            strict_schema=True,
            out="",
        )

    def test_quoted_known_table(self):
        return DiffTestBlueprint(
            sql='CREATE TABLE t (a INTEGER); CREATE INDEX i ON "t" (a);',
            strict_schema=True,
            out="",
        )

    def test_unknown_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER); CREATE INDEX i ON tt (a);",
            strict_schema=True,
            out="""\
            error: unknown table 'tt'
             --> <stdin>:1:47
              |
            1 | CREATE TABLE t (a INTEGER); CREATE INDEX i ON tt (a);
              |                                               ^~
              = help: did you mean 't'?
""",
        )

    def test_table_defined_after_index(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX i ON t (a); CREATE TABLE t (a INTEGER);",
            strict_schema=True,
            out="""\
            error: unknown table 't'
             --> <stdin>:1:19
              |
            1 | CREATE INDEX i ON t (a); CREATE TABLE t (a INTEGER);
              |                   ^
""",
        )


class CreateIndexColumns(TestSuite):
    def test_known_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); CREATE INDEX i ON t (a, lower(b)) WHERE b > 0;",
            strict_schema=True,
            out="",
        )

    def test_unknown_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); CREATE INDEX i ON t (c);",
            strict_schema=True,
            out="""\
            error: unknown column 'c'
             --> <stdin>:1:45
              |
            1 | CREATE TABLE t (a, b); CREATE INDEX i ON t (c);
              |                                             ^
              = help: did you mean 'a'?
            """,
        )

    def test_unknown_column_in_where(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b); CREATE INDEX i ON t (a) WHERE nope > 0;",
            strict_schema=True,
            out="""\
            error: unknown column 'nope'
             --> <stdin>:1:54
              |
            1 | CREATE TABLE t (a, b); CREATE INDEX i ON t (a) WHERE nope > 0;
              |                                                      ^~~~
            """,
        )

    def test_rowid_not_indexable(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); CREATE INDEX i ON t (rowid);",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:1:42
              |
            1 | CREATE TABLE t (a); CREATE INDEX i ON t (rowid);
              |                                          ^~~~~
            """,
        )

    def test_unknown_table_does_not_flag_columns(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX i ON t (a);",
            strict_schema=True,
            out="""\
            error: unknown table 't'
             --> <stdin>:1:19
              |
            1 | CREATE INDEX i ON t (a);
              |                   ^
""",
        )


class CreateTableColumnConstraints(TestSuite):
    def test_check_sees_all_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a CHECK (a > b AND rowid > 0), b);",
            strict_schema=True,
            out="",
        )

    def test_check_unknown_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a CHECK (nope > 0));",
            strict_schema=True,
            out="""\
            error: unknown column 'nope'
             --> <stdin>:1:26
              |
            1 | CREATE TABLE t (a CHECK (nope > 0));
              |                          ^~~~
            """,
        )

    def test_check_rowid_in_without_rowid_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a PRIMARY KEY CHECK (rowid > 0)) WITHOUT ROWID;",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:1:38
              |
            1 | CREATE TABLE t (a PRIMARY KEY CHECK (rowid > 0)) WITHOUT ROWID;
              |                                      ^~~~~
            """,
        )

    def test_generated_unknown_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b AS (nope + 1));",
            strict_schema=True,
            out="""\
            error: unknown column 'nope'
             --> <stdin>:1:26
              |
            1 | CREATE TABLE t (a, b AS (nope + 1));
              |                          ^~~~
            """,
        )

    def test_generated_cannot_use_rowid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b AS (rowid));",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:1:26
              |
            1 | CREATE TABLE t (a, b AS (rowid));
              |                          ^~~~~
            """,
        )

    def test_default_and_references_are_not_resolved(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a DEFAULT nope, b REFERENCES p (x));",
            strict_schema=True,
            out="",
        )


class CreateTableTableConstraints(TestSuite):
    def test_known_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b, PRIMARY KEY (a, b), UNIQUE (b COLLATE nocase), CHECK (a > rowid));",
            strict_schema=True,
            out="",
        )

    def test_primary_key_unknown_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, PRIMARY KEY (nope));",
            strict_schema=True,
            out="""\
            error: unknown column 'nope'
             --> <stdin>:1:33
              |
            1 | CREATE TABLE t (a, PRIMARY KEY (nope));
              |                                 ^~~~
            """,
        )

    def test_unique_cannot_use_rowid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, UNIQUE (rowid));",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:1:28
              |
            1 | CREATE TABLE t (a, UNIQUE (rowid));
              |                            ^~~~~
            """,
        )

    def test_check_unknown_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, CHECK (nope > 0));",
            strict_schema=True,
            out="""\
            error: unknown column 'nope'
             --> <stdin>:1:27
              |
            1 | CREATE TABLE t (a, CHECK (nope > 0));
              |                           ^~~~
            """,
        )

    def test_check_rowid_in_without_rowid_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a PRIMARY KEY, CHECK (rowid > 0)) WITHOUT ROWID;",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:1:39
              |
            1 | CREATE TABLE t (a PRIMARY KEY, CHECK (rowid > 0)) WITHOUT ROWID;
              |                                       ^~~~~
            """,
        )


class CreateTableForeignKeyChildColumns(TestSuite):
    def test_known_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, FOREIGN KEY (a) REFERENCES p (x));",
            strict_schema=True,
            out="",
        )

    def test_unknown_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, FOREIGN KEY (nope) REFERENCES p (x));",
            strict_schema=True,
            out="""\
            error: unknown column 'nope'
             --> <stdin>:1:33
              |
            1 | CREATE TABLE t (a, FOREIGN KEY (nope) REFERENCES p (x));
              |                                 ^~~~
            """,
        )

    def test_cannot_use_rowid(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, FOREIGN KEY (rowid) REFERENCES p (x));",
            strict_schema=True,
            out="""\
            error: unknown column 'rowid'
             --> <stdin>:1:33
              |
            1 | CREATE TABLE t (a, FOREIGN KEY (rowid) REFERENCES p (x));
              |                                 ^~~~~
            """,
        )


class CreateTableForeignKeyParent(TestSuite):
    def test_unknown_parent_table_is_accepted(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE c (x REFERENCES nosuch (id));",
            strict_schema=True,
            out="",
        )

    def test_unknown_parent_column_is_accepted(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE p (id); CREATE TABLE c (x, FOREIGN KEY (x) REFERENCES p (nope));",
            strict_schema=True,
            out="",
        )

    def test_parent_defined_later_is_accepted(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE c (x REFERENCES p (id)); CREATE TABLE p (id);",
            strict_schema=True,
            out="",
        )


class DropTable(TestSuite):
    def test_select_after_drop_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); DROP TABLE t; SELECT a FROM t;",
            strict_schema=True,
            out="""\
            error: unknown table 't'
             --> <stdin>:1:49
              |
            1 | CREATE TABLE t (a); DROP TABLE t; SELECT a FROM t;
              |                                                 ^
            """,
        )

    def test_create_index_after_drop_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); DROP TABLE t; CREATE INDEX i ON t (a);",
            strict_schema=True,
            out="""\
            error: unknown table 't'
             --> <stdin>:1:53
              |
            1 | CREATE TABLE t (a); DROP TABLE t; CREATE INDEX i ON t (a);
              |                                                     ^
            """,
        )

    def test_drop_view(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1; DROP VIEW v; SELECT * FROM v;",
            strict_schema=True,
            out="""\
            error: unknown table 'v'
             --> <stdin>:1:55
              |
            1 | CREATE VIEW v AS SELECT 1; DROP VIEW v; SELECT * FROM v;
              |                                                       ^
            """,
        )

    def test_recreate_after_drop(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); DROP TABLE t; CREATE TABLE t (b); SELECT b FROM t;",
            strict_schema=True,
            out="",
        )

    def test_drop_unknown_table(self):
        return DiffTestBlueprint(
            sql="DROP TABLE nosuch;",
            strict_schema=True,
            out="""\
            error: unknown table 'nosuch'
             --> <stdin>:1:12
              |
            1 | DROP TABLE nosuch;
              |            ^~~~~~
            """,
        )

    def test_drop_if_exists_unknown_table(self):
        return DiffTestBlueprint(
            sql="DROP TABLE IF EXISTS nosuch;",
            strict_schema=True,
            out="",
        )

    def test_drop_table_on_view_keeps_view(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1 AS x; DROP TABLE v; SELECT x FROM v;",
            strict_schema=True,
            out="",
        )

    def test_builtin_table_survives_drop(self):
        return DiffTestBlueprint(
            sql="DROP TABLE sqlite_master; SELECT name FROM sqlite_master;",
            strict_schema=True,
            out="",
        )


class QualifiedRef(TestSuite):
    def test_index_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); CREATE INDEX i ON t (lower(t.a));",
            out="""\
            error: qualified name 't.a' is not allowed in an index expression
             --> <stdin>:1:48
              |
            1 | CREATE TABLE t (a); CREATE INDEX i ON t (lower(t.a));
              |                                                ^
            """,
        )

    def test_index_where_allows_qualified(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); CREATE INDEX i ON t (a) WHERE t.a > 0;",
            out="",
        )

    def test_generated_column(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, b AS (t.a + 1));",
            out="""\
            error: qualified name 't.a' is not allowed in a generated column
             --> <stdin>:1:26
              |
            1 | CREATE TABLE t (a, b AS (t.a + 1));
              |                          ^
            """,
        )

    def test_primary_key(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, PRIMARY KEY (t.a));",
            out="""\
            error: qualified name 't.a' is not allowed in an index expression
             --> <stdin>:1:33
              |
            1 | CREATE TABLE t (a, PRIMARY KEY (t.a));
              |                                 ^
            """,
        )

    def test_check_allows_qualified(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a, CHECK (t.a > 0));",
            out="",
        )

    def test_other_qualifier_is_unknown_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a); CREATE INDEX i ON t (x.a);",
            strict_schema=True,
            out="""\
            error: unknown table 'x'
             --> <stdin>:1:42
              |
            1 | CREATE TABLE t (a); CREATE INDEX i ON t (x.a);
              |                                          ^
              = help: did you mean 't'?
            """,
        )


class IndexOnView(TestSuite):
    def test_index_on_view(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1 AS a; CREATE INDEX i ON v (a);",
            out="""\
            error: cannot index view 'v'
             --> <stdin>:1:51
              |
            1 | CREATE VIEW v AS SELECT 1 AS a; CREATE INDEX i ON v (a);
              |                                                   ^
            """,
        )

    def test_index_on_table_named_like_dropped_view(self):
        return DiffTestBlueprint(
            sql="CREATE VIEW v AS SELECT 1 AS a; DROP VIEW v; CREATE TABLE v (a); CREATE INDEX i ON v (a);",
            out="",
        )
