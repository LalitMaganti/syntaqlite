from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class VariableFormat(TestSuite):
    def test_positional(self):
        return AstTestBlueprint(
            sql="SELECT ?1",
            out="SELECT ?1",
        )

    def test_named_colon(self):
        return AstTestBlueprint(
            sql="SELECT :name",
            out="SELECT :name",
        )

    def test_named_at(self):
        return AstTestBlueprint(
            sql="SELECT @var",
            out="SELECT @var",
        )

    def test_named_dollar(self):
        return AstTestBlueprint(
            sql="SELECT $param",
            out="SELECT $param",
        )


class CollateFormat(TestSuite):
    def test_collate(self):
        return AstTestBlueprint(
            sql="select x collate nocase from t",
            out="SELECT x COLLATE nocase FROM t",
        )


class ValuesFormat(TestSuite):
    def test_single_row(self):
        return AstTestBlueprint(
            sql="values (1, 2, 3)",
            out="""\
                VALUES
                  (1, 2, 3)
            """,
        )

    def test_multiple_rows(self):
        return AstTestBlueprint(
            sql="values (1, 2), (3, 4)",
            out="""\
                VALUES
                  (1, 2),
                  (3, 4)
            """,
        )


class CteFormat(TestSuite):
    def test_basic_cte(self):
        return AstTestBlueprint(
            sql="with cte as (select 1) select * from cte",
            out="""\
                WITH cte AS (SELECT 1)
                SELECT * FROM cte
            """,
        )

    def test_recursive_cte(self):
        return AstTestBlueprint(
            sql="with recursive cte as (select 1) select * from cte",
            out="""\
                WITH RECURSIVE cte AS (SELECT 1)
                SELECT * FROM cte
            """,
        )

    def test_cte_with_columns(self):
        return AstTestBlueprint(
            sql="with cte(a, b) as (select 1, 2) select * from cte",
            out="""\
                WITH cte(a, b) AS (SELECT 1, 2)
                SELECT * FROM cte
            """,
        )

    def test_materialized_cte(self):
        return AstTestBlueprint(
            sql="with cte as materialized (select 1) select * from cte",
            out="""\
                WITH cte AS MATERIALIZED (SELECT 1)
                SELECT * FROM cte
            """,
        )

    def test_not_materialized_cte(self):
        return AstTestBlueprint(
            sql="with cte as not materialized (select 1) select * from cte",
            out="""\
                WITH cte AS NOT MATERIALIZED (SELECT 1)
                SELECT * FROM cte
            """,
        )


class JoinFormat(TestSuite):
    def test_inner_join(self):
        return AstTestBlueprint(
            sql="select * from a join b on a.id = b.id",
            out="""\
                SELECT *
                FROM
                  a
                  JOIN b
                  ON a.id = b.id
            """,
        )

    def test_left_join(self):
        return AstTestBlueprint(
            sql="select * from a left join b on a.id = b.id",
            out="""\
                SELECT *
                FROM
                  a
                  LEFT JOIN b
                  ON a.id = b.id
            """,
        )

    def test_cross_join(self):
        return AstTestBlueprint(
            sql="select * from a cross join b",
            out="""\
                SELECT *
                FROM
                  a
                  CROSS JOIN b
            """,
        )

    def test_join_using(self):
        return AstTestBlueprint(
            sql="select * from a join b using (id)",
            out="""\
                SELECT *
                FROM
                  a
                  JOIN b USING (id)
            """,
        )

    def test_comma_join(self):
        return AstTestBlueprint(
            sql="select * from a, b",
            out="SELECT * FROM a, b",
        )


class SubqueryFormat(TestSuite):
    def test_subquery_table_source(self):
        return AstTestBlueprint(
            sql="select * from (select 1) as t",
            out="SELECT * FROM (SELECT 1) AS t",
        )

    def test_scalar_subquery(self):
        return AstTestBlueprint(
            sql="select (select 1)",
            out="SELECT (SELECT 1)",
        )

    def test_in_subquery(self):
        return AstTestBlueprint(
            sql="select a from t where x in (select id from t2)",
            out="SELECT a FROM t WHERE x IN ((SELECT id FROM t2))",
        )

    def test_not_in_subquery(self):
        return AstTestBlueprint(
            sql="select a from t where x not in (select id from t2)",
            out="SELECT a FROM t WHERE x NOT IN ((SELECT id FROM t2))",
        )


class RaiseFormat(TestSuite):
    def test_raise_ignore(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(IGNORE)",
            out="SELECT RAISE(IGNORE)",
        )

    def test_raise_rollback(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(ROLLBACK, 'error message')",
            out="SELECT RAISE(ROLLBACK, 'error message')",
        )

    def test_raise_abort(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(ABORT, 'constraint failed')",
            out="SELECT RAISE(ABORT, 'constraint failed')",
        )

    def test_raise_fail(self):
        return AstTestBlueprint(
            sql="SELECT RAISE(FAIL, 'error')",
            out="SELECT RAISE(FAIL, 'error')",
        )


class AggregateFunctionFormat(TestSuite):
    def test_count_star(self):
        return AstTestBlueprint(
            sql="select count(*) from t",
            out="SELECT count(*) FROM t",
        )

    def test_sum_distinct(self):
        return AstTestBlueprint(
            sql="select sum(distinct x) from t",
            out="SELECT sum(DISTINCT x) FROM t",
        )


class TriggerFormat(TestSuite):
    def test_basic_trigger(self):
        return AstTestBlueprint(
            sql="create trigger tr before insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_after_delete_trigger(self):
        return AstTestBlueprint(
            sql="create trigger tr after delete on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr AFTER DELETE ON t
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_instead_of_trigger(self):
        return AstTestBlueprint(
            sql="create trigger tr instead of insert on v begin select 1; end",
            out="""\
                CREATE TRIGGER tr INSTEAD OF INSERT ON v
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_temp_trigger(self):
        return AstTestBlueprint(
            sql="create temp trigger tr before insert on t begin select 1; end",
            out="""\
                CREATE TEMP TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_if_not_exists_trigger(self):
        return AstTestBlueprint(
            sql="create trigger if not exists tr before insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER IF NOT EXISTS tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_schema_qualified_trigger(self):
        return AstTestBlueprint(
            sql="create trigger main.tr before insert on t begin select 1; end",
            out="""\
                CREATE TRIGGER main.tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_update_of_trigger(self):
        return AstTestBlueprint(
            sql="create trigger tr before update of col1, col2 on t begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE UPDATE OF col1, col2 ON t
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_when_clause_trigger(self):
        return AstTestBlueprint(
            sql="create trigger tr before insert on t when new.x > 0 begin select 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                WHEN new.x > 0
                BEGIN
                  SELECT 1;
                END
            """,
        )

    def test_multiple_commands_trigger(self):
        return AstTestBlueprint(
            sql="create trigger tr before insert on t begin select 1; select 2; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  SELECT 1;
                  SELECT 2;
                END
            """,
        )

    def test_trigger_with_dml(self):
        return AstTestBlueprint(
            sql="create trigger tr before insert on t begin update t2 set a = 1; end",
            out="""\
                CREATE TRIGGER tr BEFORE INSERT ON t
                BEGIN
                  UPDATE t2 SET a = 1;
                END
            """,
        )


class VirtualTableFormat(TestSuite):
    def test_basic_virtual_table(self):
        return AstTestBlueprint(
            sql="create virtual table vt using fts5(content)",
            out="CREATE VIRTUAL TABLE vt USING fts5(content)",
        )

    def test_no_args(self):
        return AstTestBlueprint(
            sql="create virtual table vt using mod",
            out="CREATE VIRTUAL TABLE vt USING mod",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="create virtual table if not exists vt using fts5(content)",
            out="CREATE VIRTUAL TABLE IF NOT EXISTS vt USING fts5(content)",
        )

    def test_schema_qualified(self):
        return AstTestBlueprint(
            sql="create virtual table main.vt using fts5",
            out="CREATE VIRTUAL TABLE main.vt USING fts5",
        )
