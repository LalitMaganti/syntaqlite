# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SELECT statement AST tests."""

from python.diff_tests.testing import AstTestBlueprint, TestSuite


class SelectBasic(TestSuite):
    """Basic SELECT statement tests."""

    def test_simple(self):
        return AstTestBlueprint(
            sql="SELECT id FROM users",
            out="""
                SelectStmt
                  columns:
                    ResultColumn
                      expr:
                        ColumnRef
                          name: "id"
                  from:
                    TableRef
                      name: "users"
            """,
        )
