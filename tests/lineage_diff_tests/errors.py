# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage tests: error records (parse + validation failures)."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Errors(TestSuite):
    def test_parse_error_emits_error_record(self):
        return DiffTestBlueprint(
            sql="SELECT FROM;\n",
            out="""\
            Error
              statement: 0
              stage: parse
              message: syntax error near 'FROM'
""",
        )
