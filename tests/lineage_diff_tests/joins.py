# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: joins trace origins through each joined source."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Joins(TestSuite):
    def test_inner_join_qualified(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE users (id INTEGER, name TEXT);\n"
                "CREATE TABLE orders (id INTEGER, user_id INTEGER);\n"
                "SELECT users.name, orders.id FROM users JOIN orders ON users.id = orders.user_id;\n"
            ),
            out="""\
            Lineage
              statement: 0
              status: complete
              target: users (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 1
              status: complete
              target: orders (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 2
              status: complete
              target: (none)
              columns:
                name <- users.name
                id <- orders.id
              relations:
                orders (table)
                users (table)
              physical_tables:
                orders
                users
              partial_reasons: (none)
""",
        )

    def test_two_table_relations(self):
        return DiffTestBlueprint(
            sql=(
                "CREATE TABLE a (x INTEGER);\n"
                "CREATE TABLE b (y INTEGER);\n"
                "SELECT a.x, b.y FROM a, b;\n"
            ),
            out="""\
            Lineage
              statement: 0
              status: complete
              target: a (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 1
              status: complete
              target: b (table)
              columns: (none)
              relations: (none)
              physical_tables: (none)
              partial_reasons: (none)
            Lineage
              statement: 2
              status: complete
              target: (none)
              columns:
                x <- a.x
                y <- b.y
              relations:
                a (table)
                b (table)
              physical_tables:
                a
                b
              partial_reasons: (none)
""",
        )
