# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""DROP, ALTER TABLE, and transaction control AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class DropTable(TestSuite):
    """DROP TABLE tests."""

    def test_drop_table(self):
        return AstTestBlueprint(
            sql="DROP TABLE t",
            out="""\
DropStmt
  object_type: TABLE
  if_exists: 0
  target: QualifiedName
    object_name: "t"
    schema: null
""",
        )

    def test_drop_table_if_exists(self):
        return AstTestBlueprint(
            sql="DROP TABLE IF EXISTS t",
            out="""\
DropStmt
  object_type: TABLE
  if_exists: 1
  target: QualifiedName
    object_name: "t"
    schema: null
""",
        )

    def test_drop_table_schema(self):
        return AstTestBlueprint(
            sql="DROP TABLE main.t",
            out="""\
DropStmt
  object_type: TABLE
  if_exists: 0
  target: QualifiedName
    object_name: "t"
    schema: "main"
""",
        )


class DropOther(TestSuite):
    """DROP INDEX/VIEW/TRIGGER tests."""

    def test_drop_index(self):
        return AstTestBlueprint(
            sql="DROP INDEX idx",
            out="""\
DropStmt
  object_type: INDEX
  if_exists: 0
  target: QualifiedName
    object_name: "idx"
    schema: null
""",
        )

    def test_drop_view(self):
        return AstTestBlueprint(
            sql="DROP VIEW v",
            out="""\
DropStmt
  object_type: VIEW
  if_exists: 0
  target: QualifiedName
    object_name: "v"
    schema: null
""",
        )

    def test_drop_trigger(self):
        return AstTestBlueprint(
            sql="DROP TRIGGER tr",
            out="""\
DropStmt
  object_type: TRIGGER
  if_exists: 0
  target: QualifiedName
    object_name: "tr"
    schema: null
""",
        )


class AlterTableRename(TestSuite):
    """ALTER TABLE RENAME tests."""

    def test_rename_table(self):
        return AstTestBlueprint(
            sql="ALTER TABLE t RENAME TO t2",
            out="""\
AlterTableStmt
  op: RENAME_TABLE
  target: QualifiedName
    object_name: "t"
    schema: null
  new_name: "t2"
  old_name: null
""",
        )

    def test_rename_column(self):
        return AstTestBlueprint(
            sql="ALTER TABLE t RENAME COLUMN c1 TO c2",
            out="""\
AlterTableStmt
  op: RENAME_COLUMN
  target: QualifiedName
    object_name: "t"
    schema: null
  new_name: "c2"
  old_name: "c1"
""",
        )

    def test_rename_column_no_keyword(self):
        return AstTestBlueprint(
            sql="ALTER TABLE t RENAME c1 TO c2",
            out="""\
AlterTableStmt
  op: RENAME_COLUMN
  target: QualifiedName
    object_name: "t"
    schema: null
  new_name: "c2"
  old_name: "c1"
""",
        )


class AlterTableDropAdd(TestSuite):
    """ALTER TABLE DROP/ADD COLUMN tests."""

    def test_drop_column(self):
        return AstTestBlueprint(
            sql="ALTER TABLE t DROP COLUMN c1",
            out="""\
AlterTableStmt
  op: DROP_COLUMN
  target: QualifiedName
    object_name: "t"
    schema: null
  new_name: null
  old_name: "c1"
""",
        )

    def test_add_column(self):
        return AstTestBlueprint(
            sql="ALTER TABLE t ADD COLUMN c1",
            out="""\
AlterTableStmt
  op: ADD_COLUMN
  target: null
  new_name: null
  old_name: "c1"
""",
        )


class TransactionControl(TestSuite):
    """BEGIN/COMMIT/ROLLBACK tests."""

    def test_begin(self):
        return AstTestBlueprint(
            sql="BEGIN",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: DEFERRED
""",
        )

    def test_begin_deferred(self):
        return AstTestBlueprint(
            sql="BEGIN DEFERRED",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: DEFERRED
""",
        )

    def test_begin_immediate(self):
        return AstTestBlueprint(
            sql="BEGIN IMMEDIATE TRANSACTION",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: IMMEDIATE
""",
        )

    def test_begin_exclusive(self):
        return AstTestBlueprint(
            sql="BEGIN EXCLUSIVE",
            out="""\
TransactionStmt
  op: BEGIN
  trans_type: EXCLUSIVE
""",
        )

    def test_commit(self):
        return AstTestBlueprint(
            sql="COMMIT",
            out="""\
TransactionStmt
  op: COMMIT
  trans_type: DEFERRED
""",
        )

    def test_end(self):
        return AstTestBlueprint(
            sql="END",
            out="""\
TransactionStmt
  op: COMMIT
  trans_type: DEFERRED
""",
        )

    def test_rollback(self):
        return AstTestBlueprint(
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
        return AstTestBlueprint(
            sql="SAVEPOINT sp1",
            out="""\
SavepointStmt
  op: SAVEPOINT
  savepoint_name: "sp1"
""",
        )

    def test_release(self):
        return AstTestBlueprint(
            sql="RELEASE sp1",
            out="""\
SavepointStmt
  op: RELEASE
  savepoint_name: "sp1"
""",
        )

    def test_release_savepoint(self):
        return AstTestBlueprint(
            sql="RELEASE SAVEPOINT sp1",
            out="""\
SavepointStmt
  op: RELEASE
  savepoint_name: "sp1"
""",
        )

    def test_rollback_to(self):
        return AstTestBlueprint(
            sql="ROLLBACK TO sp1",
            out="""\
SavepointStmt
  op: ROLLBACK_TO
  savepoint_name: "sp1"
""",
        )

    def test_rollback_to_savepoint(self):
        return AstTestBlueprint(
            sql="ROLLBACK TO SAVEPOINT sp1",
            out="""\
SavepointStmt
  op: ROLLBACK_TO
  savepoint_name: "sp1"
""",
        )
