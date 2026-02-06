# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TRIGGER AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class CreateTriggerBasic(TestSuite):
    """Basic CREATE TRIGGER tests."""

    def test_before_insert(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_after_delete(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr AFTER DELETE ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: AFTER
  TriggerEvent
    event_type: DELETE
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_instead_of(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr INSTEAD OF INSERT ON v BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: INSTEAD_OF
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "v"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_default_timing(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )


class CreateTriggerOptions(TestSuite):
    """CREATE TRIGGER with options (TEMP, IF NOT EXISTS, schema, UPDATE OF)."""

    def test_temp_trigger(self):
        return AstTestBlueprint(
            sql="CREATE TEMP TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 1
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER IF NOT EXISTS tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 1
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_schema_qualified(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER main.tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: "main"
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_update_of_columns(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE UPDATE OF col1, col2 ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: UPDATE
    ExprList[2]
      ColumnRef
        column: "col1"
        table: null
        schema: null
      ColumnRef
        column: "col2"
        table: null
        schema: null
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_update_no_of(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE UPDATE ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: UPDATE
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )


class CreateTriggerWhen(TestSuite):
    """CREATE TRIGGER with WHEN clause and FOR EACH ROW."""

    def test_when_clause(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t WHEN new.x > 0 BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  BinaryExpr
    op: GT
    ColumnRef
      column: "x"
      table: "new"
      schema: null
    Literal
      literal_type: INTEGER
      source: "0"
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_for_each_row(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t FOR EACH ROW BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )


class CreateTriggerBody(TestSuite):
    """Trigger body commands."""

    def test_update_body(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN UPDATE t2 SET a = 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    UpdateStmt
      conflict_action: DEFAULT
      TableRef
        table_name: "t2"
        schema: null
        alias: null
      SetClauseList[1]
        SetClause
          column: "a"
          Literal
            literal_type: INTEGER
            source: "1"
""",
        )

    def test_insert_body(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE DELETE ON t BEGIN INSERT INTO t2 VALUES (1); END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: DELETE
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    InsertStmt
      conflict_action: DEFAULT
      TableRef
        table_name: "t2"
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

    def test_delete_body(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN DELETE FROM t2 WHERE x = 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[1]
    DeleteStmt
      TableRef
        table_name: "t2"
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

    def test_multiple_commands(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; SELECT 2; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: 0
  if_not_exists: 0
  timing: BEFORE
  TriggerEvent
    event_type: INSERT
  QualifiedName
    object_name: "t"
    schema: null
  TriggerCmdList[2]
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "1"
    SelectStmt
      flags: (none)
      ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          Literal
            literal_type: INTEGER
            source: "2"
""",
        )
