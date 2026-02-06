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
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_after_delete(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr AFTER DELETE ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: AFTER
  event: TriggerEvent
    event_type: DELETE
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_instead_of(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr INSTEAD OF INSERT ON v BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: INSTEAD_OF
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "v"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_default_timing(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
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
  is_temp: TRUE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER IF NOT EXISTS tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: TRUE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_schema_qualified(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER main.tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: "main"
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_update_of_columns(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE UPDATE OF col1, col2 ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: UPDATE
    columns: ExprList[2]
      ColumnRef
        column: "col1"
        table: null
        schema: null
      ColumnRef
        column: "col2"
        table: null
        schema: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_update_no_of(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE UPDATE ON t BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: UPDATE
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: BinaryExpr
    op: GT
    left: ColumnRef
      column: "x"
      table: "new"
      schema: null
    right: Literal
      literal_type: INTEGER
      source: "0"
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )

    def test_for_each_row(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t FOR EACH ROW BEGIN SELECT 1; END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
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
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    UpdateStmt
      conflict_action: DEFAULT
      table: TableRef
        table_name: "t2"
        schema: null
        alias: null
      setlist: SetClauseList[1]
        SetClause
          column: "a"
          columns: null
          value: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
""",
        )

    def test_insert_body(self):
        return AstTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE DELETE ON t BEGIN INSERT INTO t2 VALUES (1); END",
            out="""\
CreateTriggerStmt
  trigger_name: "tr"
  schema: null
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: DELETE
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    InsertStmt
      conflict_action: DEFAULT
      table: TableRef
        table_name: "t2"
        schema: null
        alias: null
      columns: null
      source: ValuesClause
        rows: ValuesRowList[1]
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
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[1]
    DeleteStmt
      table: TableRef
        table_name: "t2"
        schema: null
        alias: null
      where: BinaryExpr
        op: EQ
        left: ColumnRef
          column: "x"
          table: null
          schema: null
        right: Literal
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
  is_temp: FALSE
  if_not_exists: FALSE
  timing: BEFORE
  event: TriggerEvent
    event_type: INSERT
    columns: null
  table: QualifiedName
    object_name: "t"
    schema: null
  when_expr: null
  body: TriggerCmdList[2]
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
    SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "2"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
""",
        )
