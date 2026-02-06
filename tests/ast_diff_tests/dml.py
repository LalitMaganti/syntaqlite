# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""DML (INSERT/UPDATE/DELETE) AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class DeleteBasic(TestSuite):
    """Basic DELETE statement tests."""

    def test_simple_delete(self):
        return AstTestBlueprint(
            sql="DELETE FROM t",
            out="""\
DeleteStmt
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )

    def test_delete_with_where(self):
        return AstTestBlueprint(
            sql="DELETE FROM t WHERE x = 1",
            out="""\
DeleteStmt
  TableRef
    table_name: "t"
    schema: null
    alias: null
  BinaryExpr
    op: EQ
    ColumnRef
      column: "x"
      table: null
      schema: null
    Literal
      literal_type: INTEGER
      source: "1"
""",
        )

    def test_delete_with_schema(self):
        return AstTestBlueprint(
            sql="DELETE FROM main.t",
            out="""\
DeleteStmt
  TableRef
    table_name: "t"
    schema: "main"
    alias: null
""",
        )


class InsertBasic(TestSuite):
    """Basic INSERT statement tests."""

    def test_insert_values(self):
        return AstTestBlueprint(
            sql="INSERT INTO t VALUES (1, 2, 3)",
            out="""\
InsertStmt
  conflict_action: DEFAULT
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[3]
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
        Literal
          literal_type: INTEGER
          source: "3"
""",
        )

    def test_insert_with_columns(self):
        return AstTestBlueprint(
            sql="INSERT INTO t(a, b) VALUES (1, 2)",
            out="""\
InsertStmt
  conflict_action: DEFAULT
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ExprList[2]
    ColumnRef
      column: "a"
      table: null
      schema: null
    ColumnRef
      column: "b"
      table: null
      schema: null
  ValuesClause
    ValuesRowList[1]
      ExprList[2]
        Literal
          literal_type: INTEGER
          source: "1"
        Literal
          literal_type: INTEGER
          source: "2"
""",
        )

    def test_insert_from_select(self):
        return AstTestBlueprint(
            sql="INSERT INTO t SELECT * FROM s",
            out="""\
InsertStmt
  conflict_action: DEFAULT
  TableRef
    table_name: "t"
    schema: null
    alias: null
  SelectStmt
    flags: (none)
    ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
    TableRef
      table_name: "s"
      schema: null
      alias: null
""",
        )

    def test_insert_default_values(self):
        return AstTestBlueprint(
            sql="INSERT INTO t DEFAULT VALUES",
            out="""\
InsertStmt
  conflict_action: DEFAULT
  TableRef
    table_name: "t"
    schema: null
    alias: null
""",
        )


class InsertConflict(TestSuite):
    """INSERT with conflict resolution tests."""

    def test_insert_or_replace(self):
        return AstTestBlueprint(
            sql="INSERT OR REPLACE INTO t VALUES (1)",
            out="""\
InsertStmt
  conflict_action: REPLACE
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_replace_into(self):
        return AstTestBlueprint(
            sql="REPLACE INTO t VALUES (1)",
            out="""\
InsertStmt
  conflict_action: REPLACE
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_insert_or_rollback(self):
        return AstTestBlueprint(
            sql="INSERT OR ROLLBACK INTO t VALUES (1)",
            out="""\
InsertStmt
  conflict_action: ROLLBACK
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_insert_or_abort(self):
        return AstTestBlueprint(
            sql="INSERT OR ABORT INTO t VALUES (1)",
            out="""\
InsertStmt
  conflict_action: ABORT
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_insert_or_fail(self):
        return AstTestBlueprint(
            sql="INSERT OR FAIL INTO t VALUES (1)",
            out="""\
InsertStmt
  conflict_action: FAIL
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )

    def test_insert_or_ignore(self):
        return AstTestBlueprint(
            sql="INSERT OR IGNORE INTO t VALUES (1)",
            out="""\
InsertStmt
  conflict_action: IGNORE
  TableRef
    table_name: "t"
    schema: null
    alias: null
  ValuesClause
    ValuesRowList[1]
      ExprList[1]
        Literal
          literal_type: INTEGER
          source: "1"
""",
        )


class UpdateBasic(TestSuite):
    """Basic UPDATE statement tests."""

    def test_simple_update(self):
        return AstTestBlueprint(
            sql="UPDATE t SET x = 1",
            out="""\
UpdateStmt
  conflict_action: DEFAULT
  TableRef
    table_name: "t"
    schema: null
    alias: null
  SetClauseList[1]
    SetClause
      column: "x"
      Literal
        literal_type: INTEGER
        source: "1"
""",
        )

    def test_update_with_where(self):
        return AstTestBlueprint(
            sql="UPDATE t SET x = 1, y = 2 WHERE id = 3",
            out="""\
UpdateStmt
  conflict_action: DEFAULT
  TableRef
    table_name: "t"
    schema: null
    alias: null
  SetClauseList[2]
    SetClause
      column: "x"
      Literal
        literal_type: INTEGER
        source: "1"
    SetClause
      column: "y"
      Literal
        literal_type: INTEGER
        source: "2"
  BinaryExpr
    op: EQ
    ColumnRef
      column: "id"
      table: null
      schema: null
    Literal
      literal_type: INTEGER
      source: "3"
""",
        )

    def test_update_or_ignore(self):
        return AstTestBlueprint(
            sql="UPDATE OR IGNORE t SET x = 1",
            out="""\
UpdateStmt
  conflict_action: IGNORE
  TableRef
    table_name: "t"
    schema: null
    alias: null
  SetClauseList[1]
    SetClause
      column: "x"
      Literal
        literal_type: INTEGER
        source: "1"
""",
        )


class DmlWithCte(TestSuite):
    """DML statements with CTEs."""

    def test_insert_with_cte(self):
        return AstTestBlueprint(
            sql="WITH cte AS (SELECT 1) INSERT INTO t SELECT * FROM cte",
            out="""\
WithClause
  recursive: 0
  CteList[1]
    CteDefinition
      cte_name: "cte"
      materialized: 0
      SelectStmt
        flags: (none)
        ResultColumnList[1]
          ResultColumn
            flags: (none)
            alias: null
            Literal
              literal_type: INTEGER
              source: "1"
  InsertStmt
    conflict_action: DEFAULT
    TableRef
      table_name: "t"
      schema: null
      alias: null
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: STAR
          alias: null
      TableRef
        table_name: "cte"
        schema: null
        alias: null
""",
        )
