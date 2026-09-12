# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE VIRTUAL TABLE AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class CreateVirtualTable(TestSuite):
    """CREATE VIRTUAL TABLE tests."""

    def test_basic(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE vt USING fts5(content)",
            out="""\
                CreateVirtualTableStmt
                  table_name: "vt"
                  schema: (none)
                  module_name: "fts5"
                  if_not_exists: FALSE
                  has_module_args: TRUE
                  module_args: "content"
            """,
        )

    def test_no_args(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE vt USING mod",
            out="""\
                CreateVirtualTableStmt
                  table_name: "vt"
                  schema: (none)
                  module_name: "mod"
                  if_not_exists: FALSE
                  has_module_args: FALSE
                  module_args: (none)
            """,
        )

    def test_if_not_exists(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE IF NOT EXISTS vt USING fts5(content)",
            out="""\
                CreateVirtualTableStmt
                  table_name: "vt"
                  schema: (none)
                  module_name: "fts5"
                  if_not_exists: TRUE
                  has_module_args: TRUE
                  module_args: "content"
            """,
        )

    def test_schema_qualified(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE main.vt USING fts5",
            out="""\
                CreateVirtualTableStmt
                  table_name: "vt"
                  schema: "main"
                  module_name: "fts5"
                  if_not_exists: FALSE
                  has_module_args: FALSE
                  module_args: (none)
            """,
        )

    def test_multiple_args(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE vt USING fts5(content, detail=column)",
            out="""\
                CreateVirtualTableStmt
                  table_name: "vt"
                  schema: (none)
                  module_name: "fts5"
                  if_not_exists: FALSE
                  has_module_args: TRUE
                  module_args: "content, detail=column"
            """,
        )

    def test_schema_if_not_exists(self):
        return DiffTestBlueprint(
            sql="CREATE VIRTUAL TABLE IF NOT EXISTS main.vt USING fts5(content)",
            out="""\
                CreateVirtualTableStmt
                  table_name: "vt"
                  schema: "main"
                  module_name: "fts5"
                  if_not_exists: TRUE
                  has_module_args: TRUE
                  module_args: "content"
            """,
        )
