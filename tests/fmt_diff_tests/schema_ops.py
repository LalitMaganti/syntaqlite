# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class DropFormat(TestSuite):
    def test_drop_table(self):
        return DiffTestBlueprint(
            sql="drop table t",
            out="DROP TABLE t;",
        )

    def test_drop_table_if_exists(self):
        return DiffTestBlueprint(
            sql="drop table if exists t",
            out="DROP TABLE IF EXISTS t;",
        )

    def test_drop_table_schema(self):
        return DiffTestBlueprint(
            sql="drop table main.t",
            out="DROP TABLE main.t;",
        )

    def test_drop_index(self):
        return DiffTestBlueprint(
            sql="drop index idx",
            out="DROP INDEX idx;",
        )

    def test_drop_view(self):
        return DiffTestBlueprint(
            sql="drop view v",
            out="DROP VIEW v;",
        )

    def test_drop_trigger(self):
        return DiffTestBlueprint(
            sql="drop trigger tr",
            out="DROP TRIGGER tr;",
        )


class AlterTableFormat(TestSuite):
    def test_rename_table(self):
        return DiffTestBlueprint(
            sql="alter table t rename to t2",
            out="ALTER TABLE t RENAME TO t2;",
        )

    def test_rename_column(self):
        return DiffTestBlueprint(
            sql="alter table t rename column c1 to c2",
            out="ALTER TABLE t RENAME COLUMN c1 TO c2;",
        )

    def test_drop_column(self):
        return DiffTestBlueprint(
            sql="alter table t drop column c1",
            out="ALTER TABLE t DROP COLUMN c1;",
        )

    def test_add_column(self):
        return DiffTestBlueprint(
            sql="alter table t add column c1",
            out="ALTER TABLE t ADD COLUMN c1;",
        )

    def test_add_column_no_keyword(self):
        return DiffTestBlueprint(
            sql="alter table t add c1",
            out="ALTER TABLE t ADD COLUMN c1;",
        )

    def test_add_column_with_type(self):
        return DiffTestBlueprint(
            sql="alter table t add column c1 varchar(10)",
            out="ALTER TABLE t ADD COLUMN c1 varchar(10);",
        )

    def test_add_column_not_null_default(self):
        return DiffTestBlueprint(
            sql="alter table t add column c1 text not null default ''",
            out="ALTER TABLE t ADD COLUMN c1 text NOT NULL DEFAULT '';",
        )

    def test_add_column_named_constraint(self):
        return DiffTestBlueprint(
            sql="alter table t add column c1 int constraint nn not null",
            out="ALTER TABLE t ADD COLUMN c1 int CONSTRAINT nn NOT NULL;",
        )

    def test_add_column_collate_check_references(self):
        return DiffTestBlueprint(
            sql=(
                "alter table t add column c1 text collate nocase "
                "check(c1 <> '') references u(x) on delete cascade"
            ),
            out="""\
ALTER TABLE t ADD COLUMN c1 text
  COLLATE nocase
  CHECK(c1 != '')
  REFERENCES u(x) ON DELETE CASCADE;""",
        )

    def test_add_column_generated_stored(self):
        return DiffTestBlueprint(
            sql="alter table t add column c1 as (a+b) stored",
            out="ALTER TABLE t ADD COLUMN c1 AS (a + b) STORED;",
        )


class TransactionFormat(TestSuite):
    def test_begin(self):
        return DiffTestBlueprint(
            sql="begin",
            out="BEGIN;",
        )

    def test_begin_immediate(self):
        return DiffTestBlueprint(
            sql="begin immediate",
            out="BEGIN IMMEDIATE;",
        )

    def test_begin_exclusive(self):
        return DiffTestBlueprint(
            sql="begin exclusive",
            out="BEGIN EXCLUSIVE;",
        )

    def test_commit(self):
        return DiffTestBlueprint(
            sql="commit",
            out="COMMIT;",
        )

    def test_end(self):
        return DiffTestBlueprint(
            sql="end",
            out="COMMIT;",
        )

    def test_rollback(self):
        return DiffTestBlueprint(
            sql="rollback",
            out="ROLLBACK;",
        )

    def test_begin_transaction_unnamed(self):
        return DiffTestBlueprint(
            sql="begin transaction",
            out="BEGIN;",
        )

    def test_begin_transaction_named(self):
        return DiffTestBlueprint(
            sql="begin transaction foo",
            out="BEGIN TRANSACTION foo;",
        )

    def test_begin_immediate_transaction_named(self):
        return DiffTestBlueprint(
            sql="begin immediate transaction foo",
            out="BEGIN IMMEDIATE TRANSACTION foo;",
        )

    def test_commit_transaction_named(self):
        return DiffTestBlueprint(
            sql="commit transaction foo",
            out="COMMIT TRANSACTION foo;",
        )

    def test_rollback_transaction_named(self):
        return DiffTestBlueprint(
            sql="rollback transaction foo",
            out="ROLLBACK TRANSACTION foo;",
        )


class SavepointFormat(TestSuite):
    def test_savepoint(self):
        return DiffTestBlueprint(
            sql="savepoint sp1",
            out="SAVEPOINT sp1;",
        )

    def test_release(self):
        return DiffTestBlueprint(
            sql="release sp1",
            out="RELEASE SAVEPOINT sp1;",
        )

    def test_release_savepoint(self):
        return DiffTestBlueprint(
            sql="release savepoint sp1",
            out="RELEASE SAVEPOINT sp1;",
        )

    def test_rollback_to(self):
        return DiffTestBlueprint(
            sql="rollback to sp1",
            out="ROLLBACK TO SAVEPOINT sp1;",
        )

    def test_rollback_to_savepoint(self):
        return DiffTestBlueprint(
            sql="rollback to savepoint sp1",
            out="ROLLBACK TO SAVEPOINT sp1;",
        )

    def test_rollback_transaction_named_to_savepoint(self):
        return DiffTestBlueprint(
            sql="rollback transaction foo to savepoint sp1",
            out="ROLLBACK TRANSACTION foo TO SAVEPOINT sp1;",
        )