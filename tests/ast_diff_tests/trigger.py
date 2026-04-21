# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TRIGGER AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CreateTriggerBasic(TestSuite):
    """Basic CREATE TRIGGER tests."""

    def test_before_insert(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_after_delete(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr AFTER DELETE ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: AFTER
              event:
                TriggerEvent
                  event_type: DELETE
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_instead_of(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr INSTEAD OF INSERT ON v BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: INSTEAD_OF
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "v"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_default_timing(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr INSERT ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )


class CreateTriggerOptions(TestSuite):
    """CREATE TRIGGER with options (TEMP, IF NOT EXISTS, schema, UPDATE OF)."""

    def test_temp_trigger(self):
        return DiffTestBlueprint(
            sql="CREATE TEMP TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: TRUE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_if_not_exists(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER IF NOT EXISTS tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: TRUE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_schema_qualified(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER main.tr BEFORE INSERT ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: "main"
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_update_of_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE UPDATE OF col1, col2 ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: UPDATE
                  columns:
                    ExprList [2 items]
                      ColumnRef
                        column: "col1"
                        table: (none)
                        schema: (none)
                      ColumnRef
                        column: "col2"
                        table: (none)
                        schema: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_update_no_of(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE UPDATE ON t BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: UPDATE
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )


class CreateTriggerWhen(TestSuite):
    """CREATE TRIGGER with WHEN clause and FOR EACH ROW."""

    def test_when_clause(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t WHEN new.x > 0 BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr:
                BinaryExpr
                  op: GT
                  left:
                    ColumnRef
                      column: "x"
                      table: "new"
                      schema: (none)
                  right:
                    Literal
                      literal_type: INTEGER
                      source: "0"
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_for_each_row(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t FOR EACH ROW BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )


class CreateTriggerBody(TestSuite):
    """Trigger body commands."""

    def test_update_body(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN UPDATE t2 SET a = 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
                  UpdateStmt
                    with_ctes: (none)
                    with_recursive: FALSE
                    conflict_action: DEFAULT
                    table:
                      TableRef
                        table_name: "t2"
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

    def test_insert_body(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE DELETE ON t BEGIN INSERT INTO t2 VALUES (1); END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: DELETE
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
                  InsertStmt
                    with_ctes: (none)
                    with_recursive: FALSE
                    conflict_action: DEFAULT
                    table:
                      TableRef
                        table_name: "t2"
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

    def test_delete_body(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN DELETE FROM t2 WHERE x = 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
                  DeleteStmt
                    with_ctes: (none)
                    with_recursive: FALSE
                    table:
                      TableRef
                        table_name: "t2"
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

    def test_instead_of_update(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER t INSTEAD OF UPDATE ON v BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: INSTEAD_OF
              event:
                TriggerEvent
                  event_type: UPDATE
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "v"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_instead_of_delete(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER t INSTEAD OF DELETE ON v BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: INSTEAD_OF
              event:
                TriggerEvent
                  event_type: DELETE
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "v"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_after_update_of_columns(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER t AFTER UPDATE OF c1, c2 ON t1 BEGIN SELECT 1; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "t"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: AFTER
              event:
                TriggerEvent
                  event_type: UPDATE
                  columns:
                    ExprList [2 items]
                      ColumnRef
                        column: "c1"
                        table: (none)
                        schema: (none)
                      ColumnRef
                        column: "c2"
                        table: (none)
                        schema: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t1"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [1 items]
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
""",
        )

    def test_multiple_commands(self):
        return DiffTestBlueprint(
            sql="CREATE TRIGGER tr BEFORE INSERT ON t BEGIN SELECT 1; SELECT 2; END",
            out="""\
            CreateTriggerStmt
              trigger_name: "tr"
              schema: (none)
              is_temp: FALSE
              if_not_exists: FALSE
              timing: BEFORE
              event:
                TriggerEvent
                  event_type: INSERT
                  columns: (none)
              table:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              when_expr: (none)
              body:
                TriggerCmdList [2 items]
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
                              source: "2"
                    from_clause: (none)
                    where_clause: (none)
                    groupby: (none)
                    having: (none)
                    orderby: (none)
                    limit_clause: (none)
                    window_clause: (none)
""",
        )
