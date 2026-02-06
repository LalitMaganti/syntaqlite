# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE VIRTUAL TABLE AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class CreateVirtualTable(TestSuite):
    """CREATE VIRTUAL TABLE tests."""

    def test_basic(self):
        return AstTestBlueprint(
            sql="CREATE VIRTUAL TABLE vt USING fts5(content)",
            out="""\
CreateVirtualTableStmt
  table_name: "vt"
  schema: null
  module_name: "fts5"
  if_not_exists: 0
  module_args: "content"
""",
        )

    def test_no_args(self):
        return AstTestBlueprint(
            sql="CREATE VIRTUAL TABLE vt USING mod",
            out="""\
CreateVirtualTableStmt
  table_name: "vt"
  schema: null
  module_name: "mod"
  if_not_exists: 0
  module_args: null
""",
        )

    def test_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE VIRTUAL TABLE IF NOT EXISTS vt USING fts5(content)",
            out="""\
CreateVirtualTableStmt
  table_name: "vt"
  schema: null
  module_name: "fts5"
  if_not_exists: 1
  module_args: "content"
""",
        )

    def test_schema_qualified(self):
        return AstTestBlueprint(
            sql="CREATE VIRTUAL TABLE main.vt USING fts5",
            out="""\
CreateVirtualTableStmt
  table_name: "vt"
  schema: "main"
  module_name: "fts5"
  if_not_exists: 0
  module_args: null
""",
        )

    def test_multiple_args(self):
        return AstTestBlueprint(
            sql="CREATE VIRTUAL TABLE vt USING fts5(content, detail=column)",
            out="""\
CreateVirtualTableStmt
  table_name: "vt"
  schema: null
  module_name: "fts5"
  if_not_exists: 0
  module_args: "content, detail=column"
""",
        )

    def test_schema_if_not_exists(self):
        return AstTestBlueprint(
            sql="CREATE VIRTUAL TABLE IF NOT EXISTS main.vt USING fts5(content)",
            out="""\
CreateVirtualTableStmt
  table_name: "vt"
  schema: "main"
  module_name: "fts5"
  if_not_exists: 1
  module_args: "content"
""",
        )
