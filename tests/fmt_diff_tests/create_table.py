# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CreateTableFormat(TestSuite):
    def test_basic(self):
        return DiffTestBlueprint(
            sql="create table t (a integer, b text)",
            out="CREATE TABLE t(a integer, b text);",
        )

    def test_single_column(self):
        return DiffTestBlueprint(
            sql="create table t(a int)",
            out="CREATE TABLE t(a int);",
        )

    def test_no_type(self):
        return DiffTestBlueprint(
            sql="create table t(a, b, c)",
            out="CREATE TABLE t(a, b, c);",
        )

    def test_temp(self):
        return DiffTestBlueprint(
            sql="create temp table t(a int)",
            out="CREATE TEMP TABLE t(a int);",
        )

    def test_if_not_exists(self):
        return DiffTestBlueprint(
            sql="create table if not exists t(a int)",
            out="CREATE TABLE IF NOT EXISTS t(a int);",
        )

    def test_schema_prefix(self):
        return DiffTestBlueprint(
            sql="create table main.t(a int)",
            out="CREATE TABLE main.t(a int);",
        )

    def test_as_select(self):
        return DiffTestBlueprint(
            sql="create table t2 as select * from t1",
            out="""\
                CREATE TABLE t2 AS
                SELECT * FROM t1;
            """,
        )

    def test_without_rowid(self):
        return DiffTestBlueprint(
            sql="create table t(a int primary key) without rowid",
            out="CREATE TABLE t(a int PRIMARY KEY) WITHOUT ROWID;",
        )

    def test_strict(self):
        return DiffTestBlueprint(
            sql="create table t(a int) strict",
            out="CREATE TABLE t(a int) STRICT;",
        )

    def test_without_rowid_strict(self):
        return DiffTestBlueprint(
            sql="create table t(a int primary key) without rowid, strict",
            out="CREATE TABLE t(a int PRIMARY KEY) WITHOUT ROWID, STRICT;",
        )


class ColumnConstraintFormat(TestSuite):
    def test_primary_key(self):
        return DiffTestBlueprint(
            sql="create table t(a int primary key)",
            out="CREATE TABLE t(a int PRIMARY KEY);",
        )

    def test_primary_key_autoincrement(self):
        return DiffTestBlueprint(
            sql="create table t(a integer primary key autoincrement)",
            out="CREATE TABLE t(a integer PRIMARY KEY AUTOINCREMENT);",
        )

    def test_primary_key_desc(self):
        return DiffTestBlueprint(
            sql="create table t(a int primary key desc)",
            out="CREATE TABLE t(a int PRIMARY KEY DESC);",
        )

    def test_not_null(self):
        return DiffTestBlueprint(
            sql="create table t(a text not null)",
            out="CREATE TABLE t(a text NOT NULL);",
        )

    def test_unique(self):
        return DiffTestBlueprint(
            sql="create table t(a text unique)",
            out="CREATE TABLE t(a text UNIQUE);",
        )

    def test_default_integer(self):
        return DiffTestBlueprint(
            sql="create table t(a int default 42)",
            out="CREATE TABLE t(a int DEFAULT (42));",
        )

    def test_default_string(self):
        return DiffTestBlueprint(
            sql="create table t(a text default 'hello')",
            out="CREATE TABLE t(a text DEFAULT ('hello'));",
        )

    def test_check(self):
        return DiffTestBlueprint(
            sql="create table t(a int check(a > 0))",
            out="CREATE TABLE t(a int CHECK(a > 0));",
        )

    def test_collate(self):
        return DiffTestBlueprint(
            sql="create table t(a text collate nocase)",
            out="CREATE TABLE t(a text COLLATE nocase);",
        )

    def test_named_constraint(self):
        return DiffTestBlueprint(
            sql="create table t(a int constraint nn not null)",
            out="CREATE TABLE t(a int CONSTRAINT nn NOT NULL);",
        )

    def test_generated_stored(self):
        return DiffTestBlueprint(
            sql="create table t(a int, b int as (a * 2) stored)",
            out="CREATE TABLE t(a int, b int AS (a * 2) STORED);",
        )

    def test_generated_virtual(self):
        return DiffTestBlueprint(
            sql="create table t(a int, b int as (a + 1))",
            out="CREATE TABLE t(a int, b int AS (a + 1));",
        )

    def test_multiple_constraints(self):
        return DiffTestBlueprint(
            sql="create table t(a text not null unique)",
            out="CREATE TABLE t(a text NOT NULL UNIQUE);",
        )


class ForeignKeyFormat(TestSuite):
    def test_references_simple(self):
        return DiffTestBlueprint(
            sql="create table t(a int references other(id))",
            out="CREATE TABLE t(a int REFERENCES other(id));",
        )

    def test_references_on_delete_cascade(self):
        return DiffTestBlueprint(
            sql="create table t(a int references other(id) on delete cascade)",
            out="CREATE TABLE t(a int REFERENCES other(id) ON DELETE CASCADE);",
        )

    def test_references_on_update_set_null(self):
        return DiffTestBlueprint(
            sql="create table t(a int references other(id) on update set null)",
            out="CREATE TABLE t(a int REFERENCES other(id) ON UPDATE SET NULL);",
        )

    def test_references_deferred(self):
        return DiffTestBlueprint(
            sql="create table t(a int references other(id) deferrable initially deferred)",
            out="CREATE TABLE t(a int REFERENCES other(id) DEFERRABLE INITIALLY DEFERRED);",
        )

    def test_long_column_constraints_wrap(self):
        """Column constraints should wrap when they exceed the line width."""
        return DiffTestBlueprint(
            sql="create table measurements(sensor_id text not null references sensors(id) on delete cascade on update set null deferrable initially deferred)",
            out="""\
                CREATE TABLE measurements(
                  sensor_id text
                    NOT NULL
                    REFERENCES sensors(id) ON DELETE CASCADE ON UPDATE SET NULL
                    DEFERRABLE INITIALLY DEFERRED
                );
            """,
        )


class TableConstraintFormat(TestSuite):
    def test_table_pk(self):
        return DiffTestBlueprint(
            sql="create table t(a int, b int, primary key(a, b))",
            out="CREATE TABLE t(a int, b int, PRIMARY KEY(a, b));",
        )

    def test_named_table_pk(self):
        return DiffTestBlueprint(
            sql="create table t(a int, constraint pk primary key(a))",
            out="CREATE TABLE t(a int, CONSTRAINT pk PRIMARY KEY(a));",
        )

    def test_table_unique(self):
        return DiffTestBlueprint(
            sql="create table t(a int, b int, unique(a, b))",
            out="CREATE TABLE t(a int, b int, UNIQUE(a, b));",
        )

    def test_table_check(self):
        return DiffTestBlueprint(
            sql="create table t(a int, b int, check(a > b))",
            out="CREATE TABLE t(a int, b int, CHECK(a > b));",
        )

    def test_table_fk(self):
        return DiffTestBlueprint(
            sql="create table t(a int, foreign key(a) references other(id))",
            out="CREATE TABLE t(a int, FOREIGN KEY(a) REFERENCES other(id));",
        )

    def test_table_fk_with_actions(self):
        return DiffTestBlueprint(
            sql="create table t(a int, foreign key(a) references other(id) on delete cascade on update set null)",
            out="""\
                CREATE TABLE t(
                  a int,
                  FOREIGN KEY(a) REFERENCES other(id) ON DELETE CASCADE ON UPDATE SET NULL
                );
            """,
        )

    def test_table_fk_with_deferrable(self):
        """Table-level FK with DEFERRABLE should keep space before it."""
        return DiffTestBlueprint(
            sql="create table t(a int, foreign key(a) references other(id) on delete cascade deferrable initially deferred)",
            out="""\
                CREATE TABLE t(
                  a int,
                  FOREIGN KEY(a) REFERENCES other(id) ON DELETE CASCADE DEFERRABLE INITIALLY DEFERRED
                );
            """,
        )

    def test_table_pk_columns_stay_inline(self):
        """PRIMARY KEY column list should not wrap when outer group breaks."""
        return DiffTestBlueprint(
            sql="create table really_long_table_name (id integer, first_name text, last_name text, primary key(first_name, last_name))",
            out="""\
                CREATE TABLE really_long_table_name(
                  id integer,
                  first_name text,
                  last_name text,
                  PRIMARY KEY(first_name, last_name)
                );
            """,
        )

    def test_table_unique_columns_stay_inline(self):
        """UNIQUE column list should not wrap when outer group breaks."""
        return DiffTestBlueprint(
            sql="create table really_long_table_name (id integer primary key, first_name text, last_name text, unique(first_name, last_name))",
            out="""\
                CREATE TABLE really_long_table_name(
                  id integer PRIMARY KEY,
                  first_name text,
                  last_name text,
                  UNIQUE(first_name, last_name)
                );
            """,
        )

    def test_table_fk_columns_stay_inline(self):
        """FOREIGN KEY and REFERENCES column lists should not wrap when outer group breaks."""
        return DiffTestBlueprint(
            sql="create table really_long_table_name (a integer, b integer, foreign key(a, b) references other_table(x, y))",
            out="""\
                CREATE TABLE really_long_table_name(
                  a integer,
                  b integer,
                  FOREIGN KEY(a, b) REFERENCES other_table(x, y)
                );
            """,
        )

    # ── Quoted identifiers ────────────────────────────────────────────

    def test_quoted_table_name(self):
        return DiffTestBlueprint(
            sql='create table "my table" (a int)',
            out='CREATE TABLE "my table"(a int);',
        )

    def test_quoted_column_name(self):
        return DiffTestBlueprint(
            sql='create table t (id integer, "set" text)',
            out='CREATE TABLE t(id integer, "set" text);',
        )

    def test_backtick_table_normalizes(self):
        return DiffTestBlueprint(
            sql="create table `my table` (a int)",
            out='CREATE TABLE "my table"(a int);',
        )

    def test_bracket_table_normalizes(self):
        return DiffTestBlueprint(
            sql="create table [my table] (a int)",
            out='CREATE TABLE "my table"(a int);',
        )

    def test_backtick_column_normalizes(self):
        return DiffTestBlueprint(
            sql="create table t (id integer, `set` text)",
            out='CREATE TABLE t(id integer, "set" text);',
        )

    def test_bracket_column_normalizes(self):
        return DiffTestBlueprint(
            sql="create table t (id integer, [set] text)",
            out='CREATE TABLE t(id integer, "set" text);',
        )