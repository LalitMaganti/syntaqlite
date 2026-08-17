# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""DROP, ALTER TABLE, and transaction control AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class DropTable(TestSuite):
    """DROP TABLE tests."""

    def test_drop_table(self):
        return DiffTestBlueprint(
            sql="DROP TABLE t",
            out="""\
            DropStmt
              object_type: TABLE
              if_exists: FALSE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
""",
        )

    def test_drop_table_if_exists(self):
        return DiffTestBlueprint(
            sql="DROP TABLE IF EXISTS t",
            out="""\
            DropStmt
              object_type: TABLE
              if_exists: TRUE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
""",
        )

    def test_drop_table_schema(self):
        return DiffTestBlueprint(
            sql="DROP TABLE main.t",
            out="""\
            DropStmt
              object_type: TABLE
              if_exists: FALSE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema:
                    IdentName
                      source: "main"
""",
        )


class DropOther(TestSuite):
    """DROP INDEX/VIEW/TRIGGER tests."""

    def test_drop_index(self):
        return DiffTestBlueprint(
            sql="DROP INDEX idx",
            out="""\
            DropStmt
              object_type: INDEX
              if_exists: FALSE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "idx"
                  schema: (none)
""",
        )

    def test_drop_view(self):
        return DiffTestBlueprint(
            sql="DROP VIEW v",
            out="""\
            DropStmt
              object_type: VIEW
              if_exists: FALSE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "v"
                  schema: (none)
""",
        )

    def test_drop_trigger(self):
        return DiffTestBlueprint(
            sql="DROP TRIGGER tr",
            out="""\
            DropStmt
              object_type: TRIGGER
              if_exists: FALSE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "tr"
                  schema: (none)
""",
        )

    def test_drop_index_if_exists(self):
        return DiffTestBlueprint(
            sql="DROP INDEX IF EXISTS idx",
            out="""\
            DropStmt
              object_type: INDEX
              if_exists: TRUE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "idx"
                  schema: (none)
""",
        )

    def test_drop_view_if_exists(self):
        return DiffTestBlueprint(
            sql="DROP VIEW IF EXISTS v",
            out="""\
            DropStmt
              object_type: VIEW
              if_exists: TRUE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "v"
                  schema: (none)
""",
        )

    def test_drop_trigger_if_exists(self):
        return DiffTestBlueprint(
            sql="DROP TRIGGER IF EXISTS trg",
            out="""\
            DropStmt
              object_type: TRIGGER
              if_exists: TRUE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "trg"
                  schema: (none)
""",
        )


class AlterTableRename(TestSuite):
    """ALTER TABLE RENAME tests."""

    def test_rename_table(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE t RENAME TO t2",
            out="""\
            AlterTableStmt
              op: RENAME_TABLE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              new_name:
                IdentName
                  source: "t2"
              old_name: (none)
              column: (none)
""",
        )

    def test_rename_table_with_schema(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE main.t RENAME TO t2",
            out="""\
            AlterTableStmt
              op: RENAME_TABLE
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema:
                    IdentName
                      source: "main"
              new_name:
                IdentName
                  source: "t2"
              old_name: (none)
              column: (none)
""",
        )

    def test_rename_column(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE t RENAME COLUMN c1 TO c2",
            out="""\
            AlterTableStmt
              op: RENAME_COLUMN
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              new_name:
                IdentName
                  source: "c2"
              old_name:
                IdentName
                  source: "c1"
              column: (none)
""",
        )

    def test_rename_column_no_keyword(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE t RENAME c1 TO c2",
            out="""\
            AlterTableStmt
              op: RENAME_COLUMN
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              new_name:
                IdentName
                  source: "c2"
              old_name:
                IdentName
                  source: "c1"
              column: (none)
""",
        )


class AlterTableDropAdd(TestSuite):
    """ALTER TABLE DROP/ADD COLUMN tests."""

    def test_drop_column(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE t DROP COLUMN c1",
            out="""\
            AlterTableStmt
              op: DROP_COLUMN
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              new_name: (none)
              old_name:
                IdentName
                  source: "c1"
              column: (none)
""",
        )

    def test_add_column(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE t ADD COLUMN c1",
            out="""\
            AlterTableStmt
              op: ADD_COLUMN
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              new_name: (none)
              old_name: (none)
              column:
                ColumnDef
                  column_name:
                    IdentName
                      source: "c1"
                  type_name: (none)
                  constraints: (none)
""",
        )

    def test_add_column_with_type_and_constraints(self):
        return DiffTestBlueprint(
            sql="ALTER TABLE t ADD COLUMN c1 INT NOT NULL DEFAULT 0",
            out="""\
            AlterTableStmt
              op: ADD_COLUMN
              target:
                QualifiedName
                  object_name:
                    IdentName
                      source: "t"
                  schema: (none)
              new_name: (none)
              old_name: (none)
              column:
                ColumnDef
                  column_name:
                    IdentName
                      source: "c1"
                  type_name: "INT"
                  constraints:
                    ColumnConstraintList [2 items]
                      ColumnConstraint
                        kind: NOT_NULL
                        constraint_name: (none)
                        onconf: DEFAULT
                        sort_order: ASC
                        is_autoincrement: FALSE
                        collation_name: (none)
                        generated_storage: VIRTUAL
                        deferrable: UNSET
                        initial_defer: UNSET
                        default_expr: (none)
                        check_expr: (none)
                        generated_expr: (none)
                        fk_clause: (none)
                      ColumnConstraint
                        kind: DEFAULT
                        constraint_name: (none)
                        onconf: DEFAULT
                        sort_order: ASC
                        is_autoincrement: FALSE
                        collation_name: (none)
                        generated_storage: VIRTUAL
                        deferrable: UNSET
                        initial_defer: UNSET
                        default_expr:
                          Literal
                            literal_type: INTEGER
                            source: "0"
                        check_expr: (none)
                        generated_expr: (none)
                        fk_clause: (none)
""",
        )


class TransactionControl(TestSuite):
    """BEGIN/COMMIT/ROLLBACK tests."""

    def test_begin(self):
        return DiffTestBlueprint(
            sql="BEGIN",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: DEFERRED
""",
        )

    def test_begin_deferred(self):
        return DiffTestBlueprint(
            sql="BEGIN DEFERRED",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: DEFERRED
""",
        )

    def test_begin_immediate(self):
        return DiffTestBlueprint(
            sql="BEGIN IMMEDIATE TRANSACTION",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: IMMEDIATE
""",
        )

    def test_begin_exclusive(self):
        return DiffTestBlueprint(
            sql="BEGIN EXCLUSIVE",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: EXCLUSIVE
""",
        )

    def test_commit(self):
        return DiffTestBlueprint(
            sql="COMMIT",
            out="""\
TransactionStmt
  op: COMMIT
  trans_type: DEFERRED
""",
        )

    def test_end(self):
        return DiffTestBlueprint(
            sql="END",
            out="""\
TransactionStmt
  op: COMMIT
  trans_type: DEFERRED
""",
        )

    def test_rollback(self):
        return DiffTestBlueprint(
            sql="ROLLBACK",
            out="""\
TransactionStmt
  op: ROLLBACK
  trans_type: DEFERRED
""",
        )


class SavepointControl(TestSuite):
    """SAVEPOINT/RELEASE/ROLLBACK TO tests."""

    def test_savepoint(self):
        return DiffTestBlueprint(
            sql="SAVEPOINT sp1",
            out="""\
            SavepointStmt
              op: SAVEPOINT
              savepoint_name:
                IdentName
                  source: "sp1"
""",
        )

    def test_release(self):
        return DiffTestBlueprint(
            sql="RELEASE sp1",
            out="""\
            SavepointStmt
              op: RELEASE
              savepoint_name:
                IdentName
                  source: "sp1"
""",
        )

    def test_release_savepoint(self):
        return DiffTestBlueprint(
            sql="RELEASE SAVEPOINT sp1",
            out="""\
            SavepointStmt
              op: RELEASE
              savepoint_name:
                IdentName
                  source: "sp1"
""",
        )

    def test_rollback_to(self):
        return DiffTestBlueprint(
            sql="ROLLBACK TO sp1",
            out="""\
            SavepointStmt
              op: ROLLBACK_TO
              savepoint_name:
                IdentName
                  source: "sp1"
""",
        )

    def test_rollback_to_savepoint(self):
        return DiffTestBlueprint(
            sql="ROLLBACK TO SAVEPOINT sp1",
            out="""\
            SavepointStmt
              op: ROLLBACK_TO
              savepoint_name:
                IdentName
                  source: "sp1"
""",
        )
