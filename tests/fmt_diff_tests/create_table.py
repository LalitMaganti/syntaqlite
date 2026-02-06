from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class CreateTableFormat(TestSuite):
    def test_basic(self):
        return AstTestBlueprint(
            sql="create table t (a integer, b text)",
            out="CREATE TABLE t(a integer, b text)",
        )

    def test_single_column(self):
        return AstTestBlueprint(
            sql="create table t(a int)",
            out="CREATE TABLE t(a int)",
        )

    def test_no_type(self):
        return AstTestBlueprint(
            sql="create table t(a, b, c)",
            out="CREATE TABLE t(a, b, c)",
        )

    def test_temp(self):
        return AstTestBlueprint(
            sql="create temp table t(a int)",
            out="CREATE TEMP TABLE t(a int)",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="create table if not exists t(a int)",
            out="CREATE TABLE IF NOT EXISTS t(a int)",
        )

    def test_schema_prefix(self):
        return AstTestBlueprint(
            sql="create table main.t(a int)",
            out="CREATE TABLE main.t(a int)",
        )

    def test_as_select(self):
        return AstTestBlueprint(
            sql="create table t2 as select * from t1",
            out="""\
                CREATE TABLE t2 AS
                SELECT * FROM t1
            """,
        )

    def test_without_rowid(self):
        return AstTestBlueprint(
            sql="create table t(a int primary key) without rowid",
            out="CREATE TABLE t(a int PRIMARY KEY) WITHOUT ROWID",
        )

    def test_strict(self):
        return AstTestBlueprint(
            sql="create table t(a int) strict",
            out="CREATE TABLE t(a int) STRICT",
        )

    def test_without_rowid_strict(self):
        return AstTestBlueprint(
            sql="create table t(a int primary key) without rowid, strict",
            out="CREATE TABLE t(a int PRIMARY KEY) WITHOUT ROWID STRICT",
        )


class ColumnConstraintFormat(TestSuite):
    def test_primary_key(self):
        return AstTestBlueprint(
            sql="create table t(a int primary key)",
            out="CREATE TABLE t(a int PRIMARY KEY)",
        )

    def test_primary_key_autoincrement(self):
        return AstTestBlueprint(
            sql="create table t(a integer primary key autoincrement)",
            out="CREATE TABLE t(a integer PRIMARY KEY AUTOINCREMENT)",
        )

    def test_primary_key_desc(self):
        return AstTestBlueprint(
            sql="create table t(a int primary key desc)",
            out="CREATE TABLE t(a int PRIMARY KEY DESC)",
        )

    def test_not_null(self):
        return AstTestBlueprint(
            sql="create table t(a text not null)",
            out="CREATE TABLE t(a text NOT NULL)",
        )

    def test_unique(self):
        return AstTestBlueprint(
            sql="create table t(a text unique)",
            out="CREATE TABLE t(a text UNIQUE)",
        )

    def test_default_integer(self):
        return AstTestBlueprint(
            sql="create table t(a int default 42)",
            out="CREATE TABLE t(a int DEFAULT 42)",
        )

    def test_default_string(self):
        return AstTestBlueprint(
            sql="create table t(a text default 'hello')",
            out="CREATE TABLE t(a text DEFAULT 'hello')",
        )

    def test_check(self):
        return AstTestBlueprint(
            sql="create table t(a int check(a > 0))",
            out="CREATE TABLE t(a int CHECK(a > 0))",
        )

    def test_collate(self):
        return AstTestBlueprint(
            sql="create table t(a text collate nocase)",
            out="CREATE TABLE t(a text COLLATE nocase)",
        )

    def test_named_constraint(self):
        return AstTestBlueprint(
            sql="create table t(a int constraint nn not null)",
            out="CREATE TABLE t(a int CONSTRAINT nn NOT NULL)",
        )

    def test_generated_stored(self):
        return AstTestBlueprint(
            sql="create table t(a int, b int as (a * 2) stored)",
            out="CREATE TABLE t(a int, b int AS (a * 2) STORED)",
        )

    def test_generated_virtual(self):
        return AstTestBlueprint(
            sql="create table t(a int, b int as (a + 1))",
            out="CREATE TABLE t(a int, b int AS (a + 1))",
        )

    def test_multiple_constraints(self):
        return AstTestBlueprint(
            sql="create table t(a text not null unique)",
            out="CREATE TABLE t(a text NOT NULL UNIQUE)",
        )


class ForeignKeyFormat(TestSuite):
    def test_references_simple(self):
        return AstTestBlueprint(
            sql="create table t(a int references other(id))",
            out="CREATE TABLE t(a int REFERENCES other(id))",
        )

    def test_references_on_delete_cascade(self):
        return AstTestBlueprint(
            sql="create table t(a int references other(id) on delete cascade)",
            out="CREATE TABLE t(a int REFERENCES other(id) ON DELETE CASCADE)",
        )

    def test_references_on_update_set_null(self):
        return AstTestBlueprint(
            sql="create table t(a int references other(id) on update set null)",
            out="CREATE TABLE t(a int REFERENCES other(id) ON UPDATE SET NULL)",
        )

    def test_references_deferred(self):
        return AstTestBlueprint(
            sql="create table t(a int references other(id) deferrable initially deferred)",
            out="CREATE TABLE t(a int REFERENCES other(id) DEFERRABLE INITIALLY DEFERRED)",
        )


class TableConstraintFormat(TestSuite):
    def test_table_pk(self):
        return AstTestBlueprint(
            sql="create table t(a int, b int, primary key(a, b))",
            out="CREATE TABLE t(a int, b int, PRIMARY KEY(a, b))",
        )

    def test_named_table_pk(self):
        return AstTestBlueprint(
            sql="create table t(a int, constraint pk primary key(a))",
            out="CREATE TABLE t(a int, CONSTRAINT pk PRIMARY KEY(a))",
        )

    def test_table_unique(self):
        return AstTestBlueprint(
            sql="create table t(a int, b int, unique(a, b))",
            out="CREATE TABLE t(a int, b int, UNIQUE(a, b))",
        )

    def test_table_check(self):
        return AstTestBlueprint(
            sql="create table t(a int, b int, check(a > b))",
            out="CREATE TABLE t(a int, b int, CHECK(a > b))",
        )

    def test_table_fk(self):
        return AstTestBlueprint(
            sql="create table t(a int, foreign key(a) references other(id))",
            out="CREATE TABLE t(a int, FOREIGN KEY(a) REFERENCES other(id))",
        )

    def test_table_fk_with_actions(self):
        return AstTestBlueprint(
            sql="create table t(a int, foreign key(a) references other(id) on delete cascade on update set null)",
            out="""\
                CREATE TABLE t(
                  a int,
                  FOREIGN KEY(a) REFERENCES other(id) ON DELETE CASCADE ON UPDATE SET NULL
                )
            """,
        )
