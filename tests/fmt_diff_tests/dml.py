from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class DmlFormat(TestSuite):
    def test_delete(self):
        return AstTestBlueprint(
            sql="delete from t where x = 1",
            out="DELETE FROM t WHERE x = 1",
        )

    def test_delete_no_where(self):
        return AstTestBlueprint(
            sql="delete from t",
            out="DELETE FROM t",
        )

    def test_update(self):
        return AstTestBlueprint(
            sql="update t set x = 1",
            out="UPDATE t SET x = 1",
        )

    def test_update_where(self):
        return AstTestBlueprint(
            sql="update t set x = 1 where y = 2",
            out="UPDATE t SET x = 1 WHERE y = 2",
        )

    def test_update_or_rollback(self):
        return AstTestBlueprint(
            sql="update or rollback t set x = 1",
            out="UPDATE OR ROLLBACK t SET x = 1",
        )

    def test_update_or_abort(self):
        return AstTestBlueprint(
            sql="update or abort t set x = 1",
            out="UPDATE OR ABORT t SET x = 1",
        )

    def test_update_or_replace(self):
        return AstTestBlueprint(
            sql="update or replace t set x = 1",
            out="UPDATE OR REPLACE t SET x = 1",
        )

    def test_update_multiple_set(self):
        return AstTestBlueprint(
            sql="update t set x = 1, y = 2, z = 3",
            out="UPDATE t SET x = 1, y = 2, z = 3",
        )

    def test_insert_values(self):
        return AstTestBlueprint(
            sql="insert into t values (1, 2)",
            out="""\
                INSERT INTO t
                VALUES
                  (1, 2)
            """,
        )

    def test_insert_columns(self):
        return AstTestBlueprint(
            sql="insert into t (a, b) values (1, 2)",
            out="""\
                INSERT INTO t (a, b)
                VALUES
                  (1, 2)
            """,
        )

    def test_insert_or_replace(self):
        return AstTestBlueprint(
            sql="insert or replace into t values (1)",
            out="""\
                REPLACE INTO t
                VALUES
                  (1)
            """,
        )

    def test_replace(self):
        return AstTestBlueprint(
            sql="replace into t values (1)",
            out="""\
                REPLACE INTO t
                VALUES
                  (1)
            """,
        )

    def test_insert_or_ignore(self):
        return AstTestBlueprint(
            sql="insert or ignore into t values (1)",
            out="""\
                INSERT OR IGNORE INTO t
                VALUES
                  (1)
            """,
        )
