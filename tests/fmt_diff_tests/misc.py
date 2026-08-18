# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class VariableFormat(TestSuite):
    def test_positional(self):
        return DiffTestBlueprint(
            sql="SELECT ?1",
            out="SELECT ?1;",
        )

    def test_named_colon(self):
        return DiffTestBlueprint(
            sql="SELECT :name",
            out="SELECT :name;",
        )

    def test_named_at(self):
        return DiffTestBlueprint(
            sql="SELECT @var",
            out="SELECT @var;",
        )

    def test_named_dollar(self):
        return DiffTestBlueprint(
            sql="SELECT $param",
            out="SELECT $param;",
        )


class CollateFormat(TestSuite):
    def test_collate(self):
        return DiffTestBlueprint(
            sql="select x collate nocase from t",
            out="SELECT x COLLATE nocase FROM t;",
        )


class ValuesFormat(TestSuite):
    def test_single_row(self):
        return DiffTestBlueprint(
            sql="values (1, 2, 3)",
            out="VALUES (1, 2, 3);",
        )

    def test_multiple_rows(self):
        return DiffTestBlueprint(
            sql="values (1, 2), (3, 4)",
            out="VALUES (1, 2), (3, 4);",
        )


class CteFormat(TestSuite):
    def test_basic_cte(self):
        return DiffTestBlueprint(
            sql="with cte as (select 1) select * from cte",
            out="""\
                WITH cte AS (SELECT 1)
                SELECT * FROM cte;
            """,
        )

    def test_recursive_cte(self):
        return DiffTestBlueprint(
            sql="with recursive cte as (select 1) select * from cte",
            out="""\
                WITH RECURSIVE cte AS (SELECT 1)
                SELECT * FROM cte;
            """,
        )

    def test_cte_with_columns(self):
        return DiffTestBlueprint(
            sql="with cte(a, b) as (select 1, 2) select * from cte",
            out="""\
                WITH cte(a, b) AS (SELECT 1, 2)
                SELECT * FROM cte;
            """,
        )

    def test_materialized_cte(self):
        return DiffTestBlueprint(
            sql="with cte as materialized (select 1) select * from cte",
            out="""\
                WITH cte AS MATERIALIZED (SELECT 1)
                SELECT * FROM cte;
            """,
        )

    def test_not_materialized_cte(self):
        return DiffTestBlueprint(
            sql="with cte as not materialized (select 1) select * from cte",
            out="""\
                WITH cte AS NOT MATERIALIZED (SELECT 1)
                SELECT * FROM cte;
            """,
        )

    def test_cte_body_indented(self):
        return DiffTestBlueprint(
            sql="""\
                WITH cte AS (
                    SELECT a, b, c, d, e, f, g, h, i, j, k
                    FROM some_table
                    WHERE some_column = 1 AND another_col = 2
                )
                SELECT * FROM cte
            """,
            out="""\
                WITH
                  cte AS (
                    SELECT a, b, c, d, e, f, g, h, i, j, k
                    FROM some_table
                    WHERE
                      some_column = 1
                      AND another_col = 2
                  )
                SELECT * FROM cte;
            """,
        )

    def test_multi_cte_with_compound(self):
        return DiffTestBlueprint(
            sql="""\
                WITH a AS (SELECT id, name FROM users WHERE active = 1),
                b AS (SELECT customer_id AS id FROM orders),
                c AS (SELECT id FROM a INTERSECT SELECT id FROM b),
                d AS (SELECT id FROM a EXCEPT SELECT id FROM c)
                SELECT id, name FROM users WHERE id IN (SELECT id FROM d)
                UNION ALL
                SELECT id, name FROM users WHERE id IN (SELECT id FROM c) ORDER BY name
            """,
            out="""\
                WITH
                  a AS (SELECT id, name FROM users WHERE active = 1),
                  b AS (SELECT customer_id AS id FROM orders),
                  c AS (
                    SELECT id FROM a
                    INTERSECT
                    SELECT id FROM b
                  ),
                  d AS (
                    SELECT id FROM a
                    EXCEPT
                    SELECT id FROM c
                  )
                SELECT id, name FROM users WHERE id IN (SELECT id FROM d)
                UNION ALL
                SELECT id, name FROM users WHERE id IN (SELECT id FROM c) ORDER BY name;
            """,
        )


class JoinFormat(TestSuite):
    def test_inner_join(self):
        return DiffTestBlueprint(
            sql="select * from a join b on a.id = b.id",
            out="""\
                SELECT *
                FROM a
                JOIN b ON a.id = b.id;
            """,
        )

    def test_left_join(self):
        return DiffTestBlueprint(
            sql="select * from a left join b on a.id = b.id",
            out="""\
                SELECT *
                FROM a
                LEFT JOIN b ON a.id = b.id;
            """,
        )

    def test_cross_join(self):
        return DiffTestBlueprint(
            sql="select * from a cross join b",
            out="""\
                SELECT *
                FROM a
                CROSS JOIN b;
            """,
        )

    def test_natural_join(self):
        return DiffTestBlueprint(
            sql="select * from a natural join b",
            out="""\
                SELECT *
                FROM a
                NATURAL JOIN b;
            """,
        )

    def test_natural_cross_join(self):
        # NATURAL CROSS JOIN joins on common columns; CROSS JOIN does not.
        return DiffTestBlueprint(
            sql="select * from a natural cross join b",
            out="""\
                SELECT *
                FROM a
                NATURAL CROSS JOIN b;
            """,
        )

    def test_natural_left_outer_join(self):
        return DiffTestBlueprint(
            sql="select * from a natural left outer join b",
            out="""\
                SELECT *
                FROM a
                NATURAL LEFT JOIN b;
            """,
        )

    def test_natural_full_join(self):
        return DiffTestBlueprint(
            sql="select * from a natural full join b",
            out="""\
                SELECT *
                FROM a
                NATURAL FULL JOIN b;
            """,
        )

    def test_join_keywords_are_a_set_not_a_sequence(self):
        # Keywords are a set, so this is exactly NATURAL LEFT OUTER JOIN.
        return DiffTestBlueprint(
            sql="select * from a outer left natural join b",
            out="""\
                SELECT *
                FROM a
                NATURAL LEFT JOIN b;
            """,
        )

    def test_cross_natural_join_reordered(self):
        return DiffTestBlueprint(
            sql="select * from a cross natural join b",
            out="""\
                SELECT *
                FROM a
                NATURAL CROSS JOIN b;
            """,
        )

    def test_unknown_join_type_falls_back_to_inner(self):
        # sqlite3JoinType() falls back to JT_INNER after erroring; we match that.
        return DiffTestBlueprint(
            sql="select * from a left bogus join b",
            out="""\
                SELECT *
                FROM a
                JOIN b;
            """,
        )

    def test_join_using(self):
        return DiffTestBlueprint(
            sql="select * from a join b using (id)",
            out="""\
                SELECT *
                FROM a
                JOIN b USING (id);
            """,
        )

    def test_comma_join(self):
        return DiffTestBlueprint(
            sql="select * from a, b",
            out="SELECT * FROM a, b;",
        )


class ParenthesizedFromFormat(TestSuite):
    """Parens around a FROM term group the join and are load-bearing."""

    def test_nested_join_keeps_parens(self):
        # Flattened, the trailing ON has nothing to attach to and will not parse.
        return DiffTestBlueprint(
            sql="select count(*) from a left join (b join c on b.x=c.x) on a.x=b.x",
            out="""\
                SELECT count(*)
                FROM a
                LEFT JOIN (
                  b
                  JOIN c ON b.x = c.x
                )
                  ON a.x = b.x;
            """,
        )

    def test_alias_on_table_list(self):
        return DiffTestBlueprint(
            sql="select zz.a from (t1,t2) as zz",
            out="SELECT zz.a FROM (t1, t2) AS zz;",
        )

    def test_alias_on_single_table(self):
        return DiffTestBlueprint(
            sql="select zz.a from (t1) as zz",
            out="SELECT zz.a FROM (t1) AS zz;",
        )

    def test_cosmetic_parens_are_dropped(self):
        # Upstream discards these: whole FROM clause, no alias, no ON/USING.
        return DiffTestBlueprint(
            sql="select * from (t1,t2)",
            out="SELECT * FROM t1, t2;",
        )

    def test_cosmetic_parens_around_join_are_dropped(self):
        return DiffTestBlueprint(
            sql="select * from (a join b on a.x=b.x)",
            out="""\
                SELECT *
                FROM a
                JOIN b ON a.x = b.x;
            """,
        )


class SubqueryFormat(TestSuite):
    def test_subquery_table_source(self):
        return DiffTestBlueprint(
            sql="select * from (select 1) as t",
            out="SELECT * FROM (SELECT 1) AS t;",
        )

    def test_scalar_subquery(self):
        return DiffTestBlueprint(
            sql="select (select 1)",
            out="SELECT (SELECT 1);",
        )

    def test_in_subquery(self):
        return DiffTestBlueprint(
            sql="select a from t where x in (select id from t2)",
            out="SELECT a FROM t WHERE x IN (SELECT id FROM t2);",
        )

    def test_not_in_subquery(self):
        return DiffTestBlueprint(
            sql="select a from t where x not in (select id from t2)",
            out="SELECT a FROM t WHERE x NOT IN (SELECT id FROM t2);",
        )

    def test_subquery_table_source_breaks(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM (SELECT a, b FROM t WHERE x = 1 AND y = 2) AS sub",
            line_width=30,
            out="""\
                SELECT *
                FROM (
                  SELECT a, b
                  FROM t
                  WHERE
                    x = 1
                    AND y = 2
                ) AS sub;
            """,
        )

    def test_subquery_table_source_no_alias_breaks(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM (SELECT a, b FROM t WHERE x = 1 AND y = 2)",
            line_width=30,
            out="""\
                SELECT *
                FROM (
                  SELECT a, b
                  FROM t
                  WHERE
                    x = 1
                    AND y = 2
                );
            """,
        )


class RaiseFormat(TestSuite):
    def test_raise_ignore(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(IGNORE)",
            out="SELECT RAISE(IGNORE);",
        )

    def test_raise_rollback(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(ROLLBACK, 'error message')",
            out="SELECT RAISE(ROLLBACK, 'error message');",
        )

    def test_raise_abort(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(ABORT, 'constraint failed')",
            out="SELECT RAISE(ABORT, 'constraint failed');",
        )

    def test_raise_fail(self):
        return DiffTestBlueprint(
            sql="SELECT RAISE(FAIL, 'error')",
            out="SELECT RAISE(FAIL, 'error');",
        )


class AggregateFunctionFormat(TestSuite):
    def test_count_star(self):
        return DiffTestBlueprint(
            sql="select count(*) from t",
            out="SELECT count(*) FROM t;",
        )

    def test_sum_distinct(self):
        return DiffTestBlueprint(
            sql="select sum(distinct x) from t",
            out="SELECT sum(DISTINCT x) FROM t;",
        )


class TriggerFormat(TestSuite):
    def test_basic_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger tr before insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_after_delete_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger tr after delete on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr AFTER DELETE ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_instead_of_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger tr instead of insert on v begin select 1; end",
            out="""\
                CREATE TRIGGER tr INSTEAD OF INSERT ON v
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_trigger_insert_upsert_do_nothing(self):
        return DiffTestBlueprint(
            sql=(
                "create trigger tr after insert on t begin "
                "insert into u values(1) on conflict do nothing; end"
            ),
            out="""\
                CREATE TRIGGER tr AFTER INSERT ON t
                BEGIN
                  INSERT INTO u VALUES (1) ON CONFLICT DO NOTHING;
                END;
            """,
        )

    def test_trigger_insert_upsert_do_update(self):
        return DiffTestBlueprint(
            sql=(
                "create trigger tr after insert on t begin "
                "insert into u values(1) on conflict(x) do update set y=2 where z=3; end"
            ),
            out="""\
                CREATE TRIGGER tr AFTER INSERT ON t
                BEGIN
                  INSERT INTO u VALUES (1) ON CONFLICT (x) DO UPDATE SET y = 2 WHERE z = 3;
                END;
            """,
        )

    def test_temp_trigger(self):
        return DiffTestBlueprint(
            sql="create temp trigger tr before insert on t begin select 1; end",
            out="""\
                CREATE TEMP TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_if_not_exists_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger if not exists tr before insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER IF NOT EXISTS tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_schema_qualified_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger main.tr before insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER main.tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_update_of_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger tr before update of col1, col2 on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE UPDATE OF col1, col2 ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_when_clause_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger tr before insert on t when new.x > 0 begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                WHEN new.x > 0
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_multiple_commands_trigger(self):
        return DiffTestBlueprint(
            sql="create trigger tr before insert on t begin select 1; select 2; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                  SELECT 2;
                END;
            """,
        )

    def test_trigger_with_dml(self):
        return DiffTestBlueprint(
            sql="create trigger tr before insert on t begin update t2 set a = 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  UPDATE t2 SET a = 1;
                END;
            """,
        )


class VirtualTableFormat(TestSuite):
    def test_basic_virtual_table(self):
        return DiffTestBlueprint(
            sql="create virtual table vt using fts5(content)",
            out="CREATE VIRTUAL TABLE vt USING fts5(content);",
        )

    def test_no_args(self):
        return DiffTestBlueprint(
            sql="create virtual table vt using mod",
            out="CREATE VIRTUAL TABLE vt USING mod;",
        )

    def test_if_not_exists(self):
        return DiffTestBlueprint(
            sql="create virtual table if not exists vt using fts5(content)",
            out="CREATE VIRTUAL TABLE IF NOT EXISTS vt USING fts5(content);",
        )

    def test_schema_qualified(self):
        return DiffTestBlueprint(
            sql="create virtual table main.vt using fts5",
            out="CREATE VIRTUAL TABLE main.vt USING fts5;",
        )


class IndentWidthFormat(TestSuite):
    def test_where_clause_indent_4(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM t WHERE x = 1 AND y = 2 AND z = 3",
            indent_width=4,
            line_width=30,
            out="""\
                SELECT a
                FROM t
                WHERE
                    x = 1
                    AND y = 2
                    AND z = 3;
            """,
        )

    def test_create_table_indent_4(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER NOT NULL, b TEXT, c REAL)",
            indent_width=4,
            line_width=30,
            out="""\
                CREATE TABLE t(
                    a INTEGER NOT NULL,
                    b TEXT,
                    c REAL
                );
            """,
        )

    def test_trigger_indent_4(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            indent_width=4,
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                    SELECT 1;
                END;
            """,
        )

    def test_cte_indent_4(self):
        return DiffTestBlueprint(
            sql="""\
                WITH cte AS (
                    SELECT a, b, c, d, e, f, g, h, i, j, k
                    FROM some_table
                    WHERE some_column = 1 AND another_col = 2
                )
                SELECT * FROM cte
            """,
            indent_width=4,
            out="""\
                WITH
                    cte AS (
                        SELECT a, b, c, d, e, f, g, h, i, j, k
                        FROM some_table
                        WHERE
                            some_column = 1
                            AND another_col = 2
                    )
                SELECT * FROM cte;
            """,
        )

    def test_indent_width_1(self):
        return DiffTestBlueprint(
            sql="SELECT a FROM t WHERE x = 1 AND y = 2 AND z = 3",
            indent_width=1,
            line_width=30,
            out="""\
                SELECT a
                FROM t
                WHERE
                 x = 1
                 AND y = 2
                 AND z = 3;
            """,
        )

    def test_indent_width_8(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INTEGER, b TEXT)",
            indent_width=8,
            line_width=20,
            out="""\
                CREATE TABLE t(
                        a INTEGER,
                        b TEXT
                );
            """,
        )


class ParenBreakFormat(TestSuite):
    """Verify parenthesized lists indent correctly when they break."""

    def test_primary_key_breaks(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b INT, c INT, PRIMARY KEY(a_col, b_col, c_col))",
            line_width=30,
            out="""\
                CREATE TABLE t(
                  a INT,
                  b INT,
                  c INT,
                  PRIMARY KEY(
                    a_col,
                    b_col,
                    c_col
                  )
                );
            """,
        )

    def test_unique_breaks(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b INT, UNIQUE(first_name, last_name, email))",
            line_width=30,
            out="""\
                CREATE TABLE t(
                  a INT,
                  b INT,
                  UNIQUE(
                    first_name,
                    last_name,
                    email
                  )
                );
            """,
        )

    def test_foreign_key_breaks(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, FOREIGN KEY(col_a, col_b) REFERENCES other(x, y))",
            line_width=30,
            out="""\
                CREATE TABLE t(
                  a INT,
                  FOREIGN KEY(col_a, col_b) REFERENCES other(
                    x,
                    y
                  )
                );
            """,
        )

    def test_check_constraint_breaks(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE t (a INT, b INT, CHECK(a > 0 AND b > 0 AND a <> b))",
            line_width=30,
            out="""\
                CREATE TABLE t(
                  a INT,
                  b INT,
                  CHECK(
                    a > 0
                    AND b > 0
                    AND a != b
                  )
                );
            """,
        )

    def test_create_index_breaks(self):
        return DiffTestBlueprint(
            sql="CREATE INDEX idx ON t (col_a, col_b, col_c, col_d)",
            line_width=25,
            out="""\
                CREATE INDEX idx ON t (
                  col_a,
                  col_b,
                  col_c,
                  col_d
                );
            """,
        )

    def test_values_row_breaks(self):
        return DiffTestBlueprint(
            sql="VALUES (1, 2, 3, 4, 5, 6, 7, 8)",
            line_width=15,
            out="""\
                VALUES
                  (
                    1,
                    2,
                    3,
                    4,
                    5,
                    6,
                    7,
                    8
                  );
            """,
        )

    def test_filter_where_breaks(self):
        return DiffTestBlueprint(
            sql="SELECT count(*) FILTER (WHERE status > 1 AND type > 2) FROM t",
            line_width=35,
            out="""\
                SELECT
                  count(*) FILTER (
                    WHERE status > 1
                    AND type > 2
                  )
                FROM t;
            """,
        )

    def test_on_conflict_columns_break(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT (col_a, col_b, col_c) DO NOTHING",
            line_width=30,
            out="""\
                INSERT INTO t
                VALUES (1)
                ON CONFLICT (
                  col_a,
                  col_b,
                  col_c
                ) DO NOTHING;
            """,
        )

class CanonicalSpellingFormat(TestSuite):
    """Equivalent spellings SQLite accepts collapse onto one form.

    Each pair below was verified to compile to identical bytecode, so the
    rewrite is semantically inert. See concepts/formatting.md.
    """

    def test_trigger_drops_for_each_row(self):
        return DiffTestBlueprint(
            sql="create trigger tr after insert on t for each row begin select 1; end",
            out="""\
                CREATE TRIGGER tr AFTER INSERT ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_trigger_gains_default_timing(self):
        return DiffTestBlueprint(
            sql="create trigger tr insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END;
            """,
        )

    def test_begin_commit_drop_transaction(self):
        return DiffTestBlueprint(
            sql="begin transaction; commit transaction;",
            out="BEGIN;\n\nCOMMIT;",
        )

    def test_end_becomes_commit(self):
        return DiffTestBlueprint(
            sql="end transaction;",
            out="COMMIT;",
        )

    def test_attach_detach_drop_database(self):
        return DiffTestBlueprint(
            sql="attach database 'f' as x; detach database x;",
            out="ATTACH 'f' AS x;\n\nDETACH x;",
        )

    def test_rename_gains_column(self):
        return DiffTestBlueprint(
            sql="alter table t rename a to b",
            out="ALTER TABLE t RENAME COLUMN a TO b;",
        )

    def test_savepoint_keyword_added(self):
        return DiffTestBlueprint(
            sql="release sp; rollback to sp;",
            out="RELEASE SAVEPOINT sp;\n\nROLLBACK TO SAVEPOINT sp;",
        )

    def test_generated_column_drops_virtual(self):
        return DiffTestBlueprint(
            sql="create table g(k, a as (1) virtual)",
            out="CREATE TABLE g(k, a AS (1));",
        )

    def test_explicit_asc_dropped(self):
        return DiffTestBlueprint(
            sql="select a from t order by a asc",
            out="SELECT a FROM t ORDER BY a;",
        )

    def test_ne_operator_canonicalised(self):
        return DiffTestBlueprint(
            sql="select * from t where a <> 1",
            out="SELECT * FROM t WHERE a != 1;",
        )

    def test_temporary_becomes_temp(self):
        return DiffTestBlueprint(
            sql="create temporary table x(a)",
            out="CREATE TEMP TABLE x(a);",
        )

    def test_insert_or_replace_becomes_replace(self):
        return DiffTestBlueprint(
            sql="insert or replace into t values(1)",
            out="REPLACE INTO t VALUES (1);",
        )
