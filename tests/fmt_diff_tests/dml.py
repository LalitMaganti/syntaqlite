# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class DmlFormat(TestSuite):
    def test_delete(self):
        return DiffTestBlueprint(
            sql="delete from t where x = 1",
            out="DELETE FROM t WHERE x = 1;",
        )

    def test_delete_no_where(self):
        return DiffTestBlueprint(
            sql="delete from t",
            out="DELETE FROM t;",
        )

    def test_update(self):
        return DiffTestBlueprint(
            sql="update t set x = 1",
            out="UPDATE t SET x = 1;",
        )

    def test_update_where(self):
        return DiffTestBlueprint(
            sql="update t set x = 1 where y = 2",
            out="UPDATE t SET x = 1 WHERE y = 2;",
        )

    def test_update_or_rollback(self):
        return DiffTestBlueprint(
            sql="update or rollback t set x = 1",
            out="UPDATE OR ROLLBACK t SET x = 1;",
        )

    def test_update_or_abort(self):
        return DiffTestBlueprint(
            sql="update or abort t set x = 1",
            out="UPDATE OR ABORT t SET x = 1;",
        )

    def test_update_or_replace(self):
        return DiffTestBlueprint(
            sql="update or replace t set x = 1",
            out="UPDATE OR REPLACE t SET x = 1;",
        )

    def test_update_multiple_set(self):
        return DiffTestBlueprint(
            sql="update t set x = 1, y = 2, z = 3",
            out="UPDATE t SET x = 1, y = 2, z = 3;",
        )

    def test_insert_values(self):
        return DiffTestBlueprint(
            sql="insert into t values (1, 2)",
            out="INSERT INTO t VALUES (1, 2);",
        )

    def test_insert_columns(self):
        return DiffTestBlueprint(
            sql="insert into t (a, b) values (1, 2)",
            out="INSERT INTO t(a, b) VALUES (1, 2);",
        )

    def test_insert_space_before_values(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t(a, b) VALUES(1, 2)",
            out="INSERT INTO t(a, b) VALUES (1, 2);",
        )

    def test_insert_or_replace(self):
        return DiffTestBlueprint(
            sql="insert or replace into t values (1)",
            out="REPLACE INTO t VALUES (1);",
        )

    def test_replace(self):
        return DiffTestBlueprint(
            sql="replace into t values (1)",
            out="REPLACE INTO t VALUES (1);",
        )

    def test_insert_or_ignore(self):
        return DiffTestBlueprint(
            sql="insert or ignore into t values (1)",
            out="INSERT OR IGNORE INTO t VALUES (1);",
        )

    def test_insert_default_values(self):
        return DiffTestBlueprint(
            sql="insert into t default values",
            out="INSERT INTO t DEFAULT VALUES;",
        )


    def test_update_multi_column_set_stays_inline(self):
        """Multi-column SET target should not wrap when outer group breaks."""
        return DiffTestBlueprint(
            sql="update t set (a, b, c) = (1, 2, 3) where some_really_long_condition = 1 and another_long_condition = 2",
            out="""\
                UPDATE t
                SET
                  (a, b, c) = (1, 2, 3)
                WHERE
                  some_really_long_condition = 1
                  AND another_long_condition = 2;
            """,
        )


class ReturningFormat(TestSuite):
    def test_delete_returning_star(self):
        return DiffTestBlueprint(
            sql="delete from t returning *",
            out="DELETE FROM t RETURNING *;",
        )

    def test_delete_where_returning(self):
        return DiffTestBlueprint(
            sql="delete from t where id = 1 returning id, name",
            out="DELETE FROM t WHERE id = 1 RETURNING id, name;",
        )

    def test_update_returning(self):
        return DiffTestBlueprint(
            sql="update t set x = 1 returning *",
            out="UPDATE t SET x = 1 RETURNING *;",
        )

    def test_update_where_returning(self):
        return DiffTestBlueprint(
            sql="update t set x = 1 where id = 1 returning id, x",
            out="UPDATE t SET x = 1 WHERE id = 1 RETURNING id, x;",
        )

    def test_insert_returning(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) returning id",
            out="INSERT INTO t VALUES (1) RETURNING id;",
        )

    def test_insert_default_values_returning(self):
        return DiffTestBlueprint(
            sql="insert into t default values returning *",
            out="INSERT INTO t DEFAULT VALUES RETURNING *;",
        )

    def test_insert_returning_multiline(self):
        return DiffTestBlueprint(
            sql="""\
                INSERT INTO inventory(sku, warehouse, qty, price)
                VALUES ('ABC-123', 'WH-EAST', 50, 19.99)
                ON CONFLICT (sku, warehouse) DO UPDATE
                SET qty = inventory.qty + excluded.qty
                ON CONFLICT (sku) WHERE warehouse IS NULL DO NOTHING
                RETURNING sku, qty AS new_qty, TYPEOF(price) AS price_type
            """,
            out="""\
                INSERT INTO inventory(sku, warehouse, qty, price)
                VALUES ('ABC-123', 'WH-EAST', 50, 19.99)
                ON CONFLICT (sku, warehouse) DO UPDATE
                SET
                  qty = inventory.qty + excluded.qty
                ON CONFLICT (sku) WHERE warehouse IS NULL DO NOTHING
                RETURNING sku, qty AS new_qty, TYPEOF(price) AS price_type;
            """,
        )

    def test_insert_returning_wraps_when_long(self):
        return DiffTestBlueprint(
            sql="""\
                INSERT INTO inventory(sku, warehouse, qty, price)
                VALUES ('ABC-123', 'WH-EAST', 50, 19.99)
                RETURNING sku AS product_sku, warehouse AS warehouse_name, qty AS current_quantity, price AS unit_price
            """,
            out="""\
                INSERT INTO inventory(sku, warehouse, qty, price)
                VALUES ('ABC-123', 'WH-EAST', 50, 19.99)
                RETURNING
                  sku AS product_sku,
                  warehouse AS warehouse_name,
                  qty AS current_quantity,
                  price AS unit_price;
            """,
        )


class UpsertFormat(TestSuite):
    def test_on_conflict_do_nothing(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) on conflict do nothing",
            out="INSERT INTO t VALUES (1) ON CONFLICT DO NOTHING;",
        )

    def test_on_conflict_do_update(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) on conflict do update set x = 1",
            out="INSERT INTO t VALUES (1) ON CONFLICT DO UPDATE SET x = 1;",
        )

    def test_on_conflict_column_do_nothing(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) on conflict(id) do nothing",
            out="INSERT INTO t VALUES (1) ON CONFLICT (id) DO NOTHING;",
        )

    def test_on_conflict_column_do_update_where(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) on conflict(id) do update set x = 1 where x != 1",
            out="INSERT INTO t VALUES (1) ON CONFLICT (id) DO UPDATE SET x = 1 WHERE x != 1;",
        )

    def test_on_conflict_do_nothing_returning(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) on conflict do nothing returning *",
            out="INSERT INTO t VALUES (1) ON CONFLICT DO NOTHING RETURNING *;",
        )

    def test_multi_on_conflict_order_preserved(self):
        return DiffTestBlueprint(
            sql="insert into t values (1) on conflict(a, b) do update set x = 1 on conflict(a) do nothing",
            out="INSERT INTO t\nVALUES (1)\nON CONFLICT (a, b) DO UPDATE\nSET\n  x = 1\nON CONFLICT (a) DO NOTHING;",
        )


class IndexedByFormat(TestSuite):
    def test_update_indexed_by(self):
        return DiffTestBlueprint(
            sql="update t indexed by idx_foo set x = 1 where y = 2",
            out="UPDATE t INDEXED BY idx_foo SET x = 1 WHERE y = 2;",
        )

    def test_delete_indexed_by(self):
        return DiffTestBlueprint(
            sql="delete from t indexed by idx_foo where y = 2",
            out="DELETE FROM t INDEXED BY idx_foo WHERE y = 2;",
        )

    def test_update_not_indexed(self):
        return DiffTestBlueprint(
            sql="update t not indexed set x = 1",
            out="UPDATE t NOT INDEXED SET x = 1;",
        )

    def test_delete_not_indexed(self):
        return DiffTestBlueprint(
            sql="delete from t not indexed where y = 2",
            out="DELETE FROM t NOT INDEXED WHERE y = 2;",
        )


class DmlWithCte(TestSuite):
    """WITH-prefixed DML statements (DELETE / UPDATE / INSERT)."""

    def test_with_delete(self):
        return DiffTestBlueprint(
            sql="with stale as (select id from s) delete from s where id in (select id from stale)",
            out="WITH stale AS (SELECT id FROM s)\nDELETE FROM s\nWHERE\n  id IN (SELECT id FROM stale);",
        )

    def test_with_update(self):
        return DiffTestBlueprint(
            sql="with x as (select 1) update t set a = 1",
            out="WITH x AS (SELECT 1)\nUPDATE t\nSET\n  a = 1;",
        )

    def test_with_insert(self):
        return DiffTestBlueprint(
            sql="with cte as (select 1) insert into t select * from cte",
            out="WITH cte AS (SELECT 1)\nINSERT INTO t\nSELECT * FROM cte;",
        )

    def test_with_recursive_insert(self):
        return DiffTestBlueprint(
            sql="with recursive c(n) as (select 1) insert into t(x) select n from c",
            out="WITH RECURSIVE c(n) AS (SELECT 1)\nINSERT INTO t(x)\nSELECT n FROM c;",
        )

    def test_with_delete_returning(self):
        return DiffTestBlueprint(
            sql="with x as (select id from s) delete from s where id in (select id from x) returning id",
            out="WITH x AS (SELECT id FROM s)\nDELETE FROM s\nWHERE\n  id IN (SELECT id FROM x)\nRETURNING id;",
        )

    def test_with_delete_long_cte_breaks(self):
        """Long CTE body forces the WITH prefix onto its own block."""
        return DiffTestBlueprint(
            sql=(
                "with stale as (select id from sessions "
                "where last_active < DATETIME('now', '-30 days')) "
                "delete from sessions where id in (select id from stale) "
                "returning id, user_id"
            ),
            out=(
                "WITH\n"
                "  stale AS (\n"
                "    SELECT id FROM sessions WHERE last_active < DATETIME('now', '-30 days')\n"
                "  )\n"
                "DELETE FROM sessions\n"
                "WHERE\n"
                "  id IN (SELECT id FROM stale)\n"
                "RETURNING id, user_id;"
            ),
        )