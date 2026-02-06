from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class DropFormat(TestSuite):
    def test_drop_table(self):
        return AstTestBlueprint(
            sql="drop table t",
            out="DROP TABLE t",
        )

    def test_drop_table_if_exists(self):
        return AstTestBlueprint(
            sql="drop table if exists t",
            out="DROP TABLE IF EXISTS t",
        )

    def test_drop_table_schema(self):
        return AstTestBlueprint(
            sql="drop table main.t",
            out="DROP TABLE main.t",
        )

    def test_drop_index(self):
        return AstTestBlueprint(
            sql="drop index idx",
            out="DROP INDEX idx",
        )

    def test_drop_view(self):
        return AstTestBlueprint(
            sql="drop view v",
            out="DROP VIEW v",
        )

    def test_drop_trigger(self):
        return AstTestBlueprint(
            sql="drop trigger tr",
            out="DROP TRIGGER tr",
        )


class AlterTableFormat(TestSuite):
    def test_rename_table(self):
        return AstTestBlueprint(
            sql="alter table t rename to t2",
            out="ALTER TABLE t RENAME TO t2",
        )

    def test_rename_column(self):
        return AstTestBlueprint(
            sql="alter table t rename column c1 to c2",
            out="ALTER TABLE t RENAME COLUMN c1 TO c2",
        )

    def test_drop_column(self):
        return AstTestBlueprint(
            sql="alter table t drop column c1",
            out="ALTER TABLE t DROP COLUMN c1",
        )

    def test_add_column(self):
        return AstTestBlueprint(
            sql="alter table t add column c1",
            out="ALTER TABLE ADD COLUMN c1",
        )


class TransactionFormat(TestSuite):
    def test_begin(self):
        return AstTestBlueprint(
            sql="begin",
            out="BEGIN",
        )

    def test_begin_immediate(self):
        return AstTestBlueprint(
            sql="begin immediate",
            out="BEGIN IMMEDIATE",
        )

    def test_begin_exclusive(self):
        return AstTestBlueprint(
            sql="begin exclusive",
            out="BEGIN EXCLUSIVE",
        )

    def test_commit(self):
        return AstTestBlueprint(
            sql="commit",
            out="COMMIT",
        )

    def test_end(self):
        return AstTestBlueprint(
            sql="end",
            out="COMMIT",
        )

    def test_rollback(self):
        return AstTestBlueprint(
            sql="rollback",
            out="ROLLBACK",
        )


class SavepointFormat(TestSuite):
    def test_savepoint(self):
        return AstTestBlueprint(
            sql="savepoint sp1",
            out="SAVEPOINT sp1",
        )

    def test_release(self):
        return AstTestBlueprint(
            sql="release sp1",
            out="RELEASE SAVEPOINT sp1",
        )

    def test_release_savepoint(self):
        return AstTestBlueprint(
            sql="release savepoint sp1",
            out="RELEASE SAVEPOINT sp1",
        )

    def test_rollback_to(self):
        return AstTestBlueprint(
            sql="rollback to sp1",
            out="ROLLBACK TO SAVEPOINT sp1",
        )

    def test_rollback_to_savepoint(self):
        return AstTestBlueprint(
            sql="rollback to savepoint sp1",
            out="ROLLBACK TO SAVEPOINT sp1",
        )
