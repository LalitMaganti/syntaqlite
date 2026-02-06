from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class PragmaFormat(TestSuite):
    def test_pragma_bare(self):
        return AstTestBlueprint(
            sql="pragma journal_mode",
            out="PRAGMA journal_mode",
        )

    def test_pragma_with_schema(self):
        return AstTestBlueprint(
            sql="pragma main.journal_mode",
            out="PRAGMA main.journal_mode",
        )

    def test_pragma_eq(self):
        return AstTestBlueprint(
            sql="pragma journal_mode = wal",
            out="PRAGMA journal_mode = wal",
        )

    def test_pragma_call(self):
        return AstTestBlueprint(
            sql="pragma table_info(t)",
            out="PRAGMA table_info(t)",
        )

    def test_pragma_negative_value(self):
        return AstTestBlueprint(
            sql="pragma cache_size = -2000",
            out="PRAGMA cache_size = -2000",
        )


class AnalyzeReindexFormat(TestSuite):
    def test_analyze_bare(self):
        return AstTestBlueprint(
            sql="analyze",
            out="ANALYZE",
        )

    def test_analyze_table(self):
        return AstTestBlueprint(
            sql="analyze t",
            out="ANALYZE t",
        )

    def test_analyze_with_schema(self):
        return AstTestBlueprint(
            sql="analyze main.t",
            out="ANALYZE main.t",
        )

    def test_reindex_bare(self):
        return AstTestBlueprint(
            sql="reindex",
            out="REINDEX",
        )

    def test_reindex_table(self):
        return AstTestBlueprint(
            sql="reindex t",
            out="REINDEX t",
        )


class AttachDetachFormat(TestSuite):
    def test_attach(self):
        return AstTestBlueprint(
            sql="attach 'file.db' as db2",
            out="ATTACH 'file.db' AS db2",
        )

    def test_detach(self):
        return AstTestBlueprint(
            sql="detach db2",
            out="DETACH db2",
        )


class VacuumFormat(TestSuite):
    def test_vacuum_bare(self):
        return AstTestBlueprint(
            sql="vacuum",
            out="VACUUM",
        )

    def test_vacuum_schema(self):
        return AstTestBlueprint(
            sql="vacuum main",
            out="VACUUM main",
        )

    def test_vacuum_into(self):
        return AstTestBlueprint(
            sql="vacuum into 'backup.db'",
            out="VACUUM INTO 'backup.db'",
        )


class ExplainFormat(TestSuite):
    def test_explain(self):
        return AstTestBlueprint(
            sql="explain select 1",
            out="""\
                EXPLAIN
                SELECT 1
            """,
        )

    def test_explain_query_plan(self):
        return AstTestBlueprint(
            sql="explain query plan select * from t",
            out="""\
                EXPLAIN QUERY PLAN
                SELECT * FROM t
            """,
        )


class CreateIndexFormat(TestSuite):
    def test_create_index(self):
        return AstTestBlueprint(
            sql="create index idx on t(x)",
            out="CREATE INDEX idx ON t (x)",
        )

    def test_create_unique_index(self):
        return AstTestBlueprint(
            sql="create unique index idx on t(x)",
            out="CREATE UNIQUE INDEX idx ON t (x)",
        )

    def test_create_index_if_not_exists(self):
        return AstTestBlueprint(
            sql="create index if not exists idx on t(x)",
            out="CREATE INDEX IF NOT EXISTS idx ON t (x)",
        )

    def test_create_index_with_schema(self):
        return AstTestBlueprint(
            sql="create index main.idx on t(x)",
            out="CREATE INDEX main.idx ON t (x)",
        )

    def test_create_index_multi_column(self):
        return AstTestBlueprint(
            sql="create index idx on t(x, y desc)",
            out="CREATE INDEX idx ON t (x, y DESC)",
        )

    def test_create_index_with_where(self):
        return AstTestBlueprint(
            sql="create index idx on t(x) where x > 0",
            out="CREATE INDEX idx ON t (x) WHERE x > 0",
        )


class CreateViewFormat(TestSuite):
    def test_create_view(self):
        return AstTestBlueprint(
            sql="create view v as select * from t",
            out="""\
                CREATE VIEW v AS
                SELECT * FROM t
            """,
        )

    def test_create_temp_view(self):
        return AstTestBlueprint(
            sql="create temp view v as select * from t",
            out="""\
                CREATE TEMP VIEW v AS
                SELECT * FROM t
            """,
        )

    def test_create_view_if_not_exists(self):
        return AstTestBlueprint(
            sql="create view if not exists v as select * from t",
            out="""\
                CREATE VIEW IF NOT EXISTS v AS
                SELECT * FROM t
            """,
        )

    def test_create_view_with_columns(self):
        return AstTestBlueprint(
            sql="create view v(a, b) as select x, y from t",
            out="""\
                CREATE VIEW v(a, b) AS
                SELECT x, y FROM t
            """,
        )
