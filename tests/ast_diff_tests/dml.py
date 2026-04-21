# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""DML (INSERT/UPDATE/DELETE) AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class DeleteBasic(TestSuite):
    """Basic DELETE statement tests."""

    def test_simple_delete(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_delete_with_where(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t WHERE x = 1",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "x"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "1"
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_delete_with_schema(self):
        return DiffTestBlueprint(
            sql="DELETE FROM main.t",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: "main"
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_delete_indexed_by(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t INDEXED BY idx_foo WHERE x = 1",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: INDEXED
              index_name: "idx_foo"
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "x"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "1"
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_delete_not_indexed(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t NOT INDEXED WHERE x = 1",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: NOT_INDEXED
              index_name: (none)
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "x"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "1"
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )


class UpdateIndexedBy(TestSuite):
    """INDEXED BY / NOT INDEXED on UPDATE."""

    def test_update_indexed_by(self):
        return DiffTestBlueprint(
            sql="UPDATE t INDEXED BY idx_foo SET x = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: INDEXED
              index_name: "idx_foo"
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_not_indexed(self):
        return DiffTestBlueprint(
            sql="UPDATE t NOT INDEXED SET x = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: NOT_INDEXED
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )


class InsertBasic(TestSuite):
    """Basic INSERT statement tests."""

    def test_insert_values(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1, 2, 3)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [3 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
                        Literal
                          literal_type: INTEGER
                          source: "2"
                        Literal
                          literal_type: INTEGER
                          source: "3"
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_with_columns(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t(a, b) VALUES (1, 2)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns:
                ExprList [2 items]
                  ColumnRef
                    column: "a"
                    table: (none)
                    schema: (none)
                  ColumnRef
                    column: "b"
                    table: (none)
                    schema: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
                        Literal
                          literal_type: INTEGER
                          source: "2"
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_from_select(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t SELECT * FROM s",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: STAR
                        alias: (none)
                        expr: (none)
                  from_clause:
                    TableRef
                      table_name: "s"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_default_values(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t DEFAULT VALUES",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source: (none)
              upsert: (none)
              returning: (none)
""",
        )


class InsertConflict(TestSuite):
    """INSERT with conflict resolution tests."""

    def test_insert_or_replace(self):
        return DiffTestBlueprint(
            sql="INSERT OR REPLACE INTO t VALUES (1)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: REPLACE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert: (none)
              returning: (none)
""",
        )

    def test_replace_into(self):
        return DiffTestBlueprint(
            sql="REPLACE INTO t VALUES (1)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: REPLACE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_or_rollback(self):
        return DiffTestBlueprint(
            sql="INSERT OR ROLLBACK INTO t VALUES (1)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: ROLLBACK
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_or_abort(self):
        return DiffTestBlueprint(
            sql="INSERT OR ABORT INTO t VALUES (1)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: ABORT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_or_fail(self):
        return DiffTestBlueprint(
            sql="INSERT OR FAIL INTO t VALUES (1)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: FAIL
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert: (none)
              returning: (none)
""",
        )

    def test_insert_or_ignore(self):
        return DiffTestBlueprint(
            sql="INSERT OR IGNORE INTO t VALUES (1)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: IGNORE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert: (none)
              returning: (none)
""",
        )


class UpdateBasic(TestSuite):
    """Basic UPDATE statement tests."""

    def test_simple_update(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET x = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_with_where(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET x = 1, y = 2 WHERE id = 3",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [2 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
                  SetClause
                    column: "y"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "2"
              from_clause: (none)
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "id"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "3"
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_or_ignore(self):
        return DiffTestBlueprint(
            sql="UPDATE OR IGNORE t SET x = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: IGNORE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )


class UpdateFrom(TestSuite):
    """UPDATE with FROM clause tests."""

    def test_update_with_from(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET a = o.a FROM other o WHERE t.id = o.id",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "a"
                    columns: (none)
                    value:
                      ColumnRef
                        column: "a"
                        table: "o"
                        schema: (none)
              from_clause:
                TableRef
                  table_name: "other"
                  schema: (none)
                  has_parens: FALSE
                  alias:
                    IdentName
                      source: "o"
                  args: (none)
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "id"
                      table: "t"
                      schema: (none)
                  right:
                    ColumnRef
                      column: "id"
                      table: "o"
                      schema: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )


class UpdateConflict(TestSuite):
    """UPDATE with conflict resolution tests."""

    def test_update_or_rollback(self):
        return DiffTestBlueprint(
            sql="UPDATE OR ROLLBACK t SET a = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: ROLLBACK
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "a"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_or_abort(self):
        return DiffTestBlueprint(
            sql="UPDATE OR ABORT t SET a = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: ABORT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "a"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_or_fail(self):
        return DiffTestBlueprint(
            sql="UPDATE OR FAIL t SET a = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: FAIL
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "a"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_or_replace(self):
        return DiffTestBlueprint(
            sql="UPDATE OR REPLACE t SET a = 1",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: REPLACE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "a"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )


class UpdateSetClauseMultiColumn(TestSuite):
    """UPDATE with multi-column SET clause tests."""

    def test_set_clause_multi_column(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET (a, b) = (1, 2)",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: (none)
                    columns:
                      ExprList [2 items]
                        ColumnRef
                          column: "a"
                          table: (none)
                          schema: (none)
                        ColumnRef
                          column: "b"
                          table: (none)
                          schema: (none)
                    value:
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
                        Literal
                          literal_type: INTEGER
                          source: "2"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )


class InsertMultipleRows(TestSuite):
    """INSERT with multiple value rows tests."""

    def test_insert_multiple_value_rows(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1, 2), (3, 4), (5, 6)",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [3 items]
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
                        Literal
                          literal_type: INTEGER
                          source: "2"
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "3"
                        Literal
                          literal_type: INTEGER
                          source: "4"
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "5"
                        Literal
                          literal_type: INTEGER
                          source: "6"
              upsert: (none)
              returning: (none)
""",
        )


class DmlWithCte(TestSuite):
    """DML statements with CTEs."""

    def test_insert_with_cte(self):
        return DiffTestBlueprint(
            sql="WITH cte AS (SELECT 1) INSERT INTO t SELECT * FROM cte",
            out="""\
            InsertStmt
              with_ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "cte"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      SelectStmt
                        flags: (none)
                        columns:
                          ResultColumnList [1 items]
                            ResultColumn
                              flags: (none)
                              alias: (none)
                              expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                        from_clause: (none)
                        where_clause: (none)
                        groupby: (none)
                        having: (none)
                        orderby: (none)
                        limit_clause: (none)
                        window_clause: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: STAR
                        alias: (none)
                        expr: (none)
                  from_clause:
                    TableRef
                      table_name: "cte"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
              upsert: (none)
              returning: (none)
""",
        )

    def test_delete_with_cte(self):
        return DiffTestBlueprint(
            sql="WITH stale AS (SELECT id FROM s) DELETE FROM s WHERE id IN (SELECT id FROM stale)",
            out="""\
            DeleteStmt
              with_ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "stale"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      SelectStmt
                        flags: (none)
                        columns:
                          ResultColumnList [1 items]
                            ResultColumn
                              flags: (none)
                              alias: (none)
                              expr:
                                ColumnRef
                                  column: "id"
                                  table: (none)
                                  schema: (none)
                        from_clause:
                          TableRef
                            table_name: "s"
                            schema: (none)
                            has_parens: FALSE
                            alias: (none)
                            args: (none)
                        where_clause: (none)
                        groupby: (none)
                        having: (none)
                        orderby: (none)
                        limit_clause: (none)
                        window_clause: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "s"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              where_clause:
                InExpr
                  negated: FALSE
                  bare_source: FALSE
                  operand:
                    ColumnRef
                      column: "id"
                      table: (none)
                      schema: (none)
                  source:
                    SelectStmt
                      flags: (none)
                      columns:
                        ResultColumnList [1 items]
                          ResultColumn
                            flags: (none)
                            alias: (none)
                            expr:
                              ColumnRef
                                column: "id"
                                table: (none)
                                schema: (none)
                      from_clause:
                        TableRef
                          table_name: "stale"
                          schema: (none)
                          has_parens: FALSE
                          alias: (none)
                          args: (none)
                      where_clause: (none)
                      groupby: (none)
                      having: (none)
                      orderby: (none)
                      limit_clause: (none)
                      window_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_update_with_cte(self):
        return DiffTestBlueprint(
            sql="WITH x AS (SELECT 1) UPDATE t SET a = 1",
            out="""\
            UpdateStmt
              with_ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "x"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      SelectStmt
                        flags: (none)
                        columns:
                          ResultColumnList [1 items]
                            ResultColumn
                              flags: (none)
                              alias: (none)
                              expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                        from_clause: (none)
                        where_clause: (none)
                        groupby: (none)
                        having: (none)
                        orderby: (none)
                        limit_clause: (none)
                        window_clause: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "a"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning: (none)
""",
        )

    def test_insert_with_recursive(self):
        return DiffTestBlueprint(
            sql="WITH RECURSIVE c(n) AS (SELECT 1) INSERT INTO t(x) SELECT n FROM c",
            out="""\
            InsertStmt
              with_ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "c"
                    materialized: DEFAULT
                    columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "n"
                          table: (none)
                          schema: (none)
                    select:
                      SelectStmt
                        flags: (none)
                        columns:
                          ResultColumnList [1 items]
                            ResultColumn
                              flags: (none)
                              alias: (none)
                              expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                        from_clause: (none)
                        where_clause: (none)
                        groupby: (none)
                        having: (none)
                        orderby: (none)
                        limit_clause: (none)
                        window_clause: (none)
              with_recursive: TRUE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns:
                ExprList [1 items]
                  ColumnRef
                    column: "x"
                    table: (none)
                    schema: (none)
              source:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: (none)
                        alias: (none)
                        expr:
                          ColumnRef
                            column: "n"
                            table: (none)
                            schema: (none)
                  from_clause:
                    TableRef
                      table_name: "c"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
              upsert: (none)
              returning: (none)
""",
        )


class ReturningClause(TestSuite):
    """RETURNING clause tests."""

    def test_delete_returning(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t WHERE id = 1 RETURNING *",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "id"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "1"
              orderby: (none)
              limit_clause: (none)
              returning:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
""",
        )

    def test_delete_returning_columns(self):
        return DiffTestBlueprint(
            sql="DELETE FROM t RETURNING id, name",
            out="""\
            DeleteStmt
              with_ctes: (none)
              with_recursive: FALSE
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning:
                ResultColumnList [2 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "name"
                        table: (none)
                        schema: (none)
""",
        )

    def test_update_returning(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET x = 1 RETURNING *",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause: (none)
              orderby: (none)
              limit_clause: (none)
              returning:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
""",
        )

    def test_update_where_returning(self):
        return DiffTestBlueprint(
            sql="UPDATE t SET x = 1 WHERE id = 1 RETURNING id, x",
            out="""\
            UpdateStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              index_hint: DEFAULT
              index_name: (none)
              setlist:
                SetClauseList [1 items]
                  SetClause
                    column: "x"
                    columns: (none)
                    value:
                      Literal
                        literal_type: INTEGER
                        source: "1"
              from_clause: (none)
              where_clause:
                BinaryExpr
                  op: EQ
                  left:
                    ColumnRef
                      column: "id"
                      table: (none)
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "1"
              orderby: (none)
              limit_clause: (none)
              returning:
                ResultColumnList [2 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
""",
        )

    def test_insert_returning(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1, 2) RETURNING id",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
                        Literal
                          literal_type: INTEGER
                          source: "2"
              upsert: (none)
              returning:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
""",
        )

    def test_insert_default_values_returning(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t DEFAULT VALUES RETURNING *",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source: (none)
              upsert: (none)
              returning:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
""",
        )


class UpsertClause(TestSuite):
    """INSERT ... ON CONFLICT (UPSERT) tests."""

    def test_on_conflict_do_nothing(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT DO NOTHING",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns: (none)
                    target_where: (none)
                    action: NOTHING
                    setlist: (none)
                    update_where: (none)
              returning: (none)
""",
        )

    def test_on_conflict_do_update(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT DO UPDATE SET x = 1",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns: (none)
                    target_where: (none)
                    action: UPDATE
                    setlist:
                      SetClauseList [1 items]
                        SetClause
                          column: "x"
                          columns: (none)
                          value:
                            Literal
                              literal_type: INTEGER
                              source: "1"
                    update_where: (none)
              returning: (none)
""",
        )

    def test_on_conflict_column_do_nothing(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT(id) DO NOTHING",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    target_where: (none)
                    action: NOTHING
                    setlist: (none)
                    update_where: (none)
              returning: (none)
""",
        )

    def test_on_conflict_column_do_update(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t(id, x) VALUES (1, 2) ON CONFLICT(id) DO UPDATE SET x = excluded.x",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns:
                ExprList [2 items]
                  ColumnRef
                    column: "id"
                    table: (none)
                    schema: (none)
                  ColumnRef
                    column: "x"
                    table: (none)
                    schema: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [2 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
                        Literal
                          literal_type: INTEGER
                          source: "2"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    target_where: (none)
                    action: UPDATE
                    setlist:
                      SetClauseList [1 items]
                        SetClause
                          column: "x"
                          columns: (none)
                          value:
                            ColumnRef
                              column: "x"
                              table: "excluded"
                              schema: (none)
                    update_where: (none)
              returning: (none)
""",
        )

    def test_on_conflict_column_where_do_update(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT(id) WHERE id > 0 DO UPDATE SET x = 1 WHERE x != 1",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    target_where:
                      BinaryExpr
                        op: GT
                        left:
                          ColumnRef
                            column: "id"
                            table: (none)
                            schema: (none)
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "0"
                    action: UPDATE
                    setlist:
                      SetClauseList [1 items]
                        SetClause
                          column: "x"
                          columns: (none)
                          value:
                            Literal
                              literal_type: INTEGER
                              source: "1"
                    update_where:
                      BinaryExpr
                        op: NE
                        left:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                        right:
                          Literal
                            literal_type: INTEGER
                            source: "1"
              returning: (none)
""",
        )

    def test_on_conflict_do_nothing_returning(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT DO NOTHING RETURNING *",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns: (none)
                    target_where: (none)
                    action: NOTHING
                    setlist: (none)
                    update_where: (none)
              returning:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
""",
        )

    def test_on_conflict_do_update_returning(self):
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT(id) DO UPDATE SET x = 1 RETURNING id, x",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [1 items]
                  UpsertClause
                    columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "id"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    target_where: (none)
                    action: UPDATE
                    setlist:
                      SetClauseList [1 items]
                        SetClause
                          column: "x"
                          columns: (none)
                          value:
                            Literal
                              literal_type: INTEGER
                              source: "1"
                    update_where: (none)
              returning:
                ResultColumnList [2 items]
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
                  ResultColumn
                    flags: (none)
                    alias: (none)
                    expr:
                      ColumnRef
                        column: "x"
                        table: (none)
                        schema: (none)
""",
        )

    def test_multi_upsert_order_preserved(self):
        """Multiple ON CONFLICT clauses must preserve source order."""
        return DiffTestBlueprint(
            sql="INSERT INTO t VALUES (1) ON CONFLICT(a) DO UPDATE SET x = 1 ON CONFLICT(b) DO NOTHING",
            out="""\
            InsertStmt
              with_ctes: (none)
              with_recursive: FALSE
              conflict_action: DEFAULT
              table:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
              columns: (none)
              source:
                ValuesClause
                  rows:
                    ValuesRowList [1 items]
                      ExprList [1 items]
                        Literal
                          literal_type: INTEGER
                          source: "1"
              upsert:
                UpsertClauseList [2 items]
                  UpsertClause
                    columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "a"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    target_where: (none)
                    action: UPDATE
                    setlist:
                      SetClauseList [1 items]
                        SetClause
                          column: "x"
                          columns: (none)
                          value:
                            Literal
                              literal_type: INTEGER
                              source: "1"
                    update_where: (none)
                  UpsertClause
                    columns:
                      OrderByList [1 items]
                        OrderingTerm
                          expr:
                            ColumnRef
                              column: "b"
                              table: (none)
                              schema: (none)
                          sort_order: ASC
                          nulls_order: NONE
                    target_where: (none)
                    action: NOTHING
                    setlist: (none)
                    update_where: (none)
              returning: (none)
""",
        )
