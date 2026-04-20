# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Diagnostics(TestSuite):
    def test_select_after_create_table_as_select_is_clean(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE orders AS SELECT 1 AS order_id;\n"
                "SELECT o.order_id FROM orders o;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    def test_unknown_table_warns(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM nonexistent;",
            op="diagnostics",
            out="""\
            warning 0:14..0:25: unknown table 'nonexistent'
""",
        )

    def test_forward_reference_warns(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM t;\nCREATE TABLE t (id INTEGER);",
            op="diagnostics",
            out="""\
            warning 0:14..0:15: unknown table 't'
""",
        )

    def test_syntax_error_on_bare_select(self):
        return LspDiffTestBlueprint(
            sql="SELECT ",
            op="diagnostics",
            out="""\
            error 0:6..0:7: incomplete SQL statement
""",
        )

    def test_syntax_error_on_incomplete_from(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM",
            op="diagnostics",
            out="""\
            error 0:12..0:13: incomplete SQL statement
""",
        )

    def test_invalid_sql(self):
        return LspDiffTestBlueprint(
            sql="NOT VALID SQL;",
            op="diagnostics",
            out="""\
            error 0:0..0:3: syntax error near 'NOT'
""",
        )

    def test_multiple_syntax_errors_all_reported(self):
        return LspDiffTestBlueprint(
            sql="include ;\ninclude ;\nSELECT 1;",
            op="diagnostics",
            out="""\
            error 0:0..0:7: syntax error near 'include'
            error 1:0..1:7: syntax error near 'include'
""",
        )

    def test_syntax_errors_do_not_suppress_later_statements(self):
        return LspDiffTestBlueprint(
            sql="NOT VALID;\nSELECT 1;",
            op="diagnostics",
            out="""\
            error 0:0..0:3: syntax error near 'NOT'
""",
        )

    def test_syntax_error_after_valid_statement_is_reported(self):
        return LspDiffTestBlueprint(
            sql="SELECT 1;\nNOT VALID;",
            op="diagnostics",
            out="""\
            error 1:0..1:3: syntax error near 'NOT'
""",
        )

    def test_validation_continues_past_errors(self):
        return LspDiffTestBlueprint(
            sql="SELECT ;\nSELECT ;\nSELECT * FROM no_such_table;",
            op="diagnostics",
            out="""\
            error 0:7..0:8: syntax error near ';'
            error 1:7..1:8: syntax error near ';'
            warning 2:14..2:27: unknown table 'no_such_table'
""",
        )

    def test_syntax_error_for_create_table_as_missing_select(self):
        return LspDiffTestBlueprint(
            sql="create table orders as;",
            op="diagnostics",
            out="""\
            error 0:22..0:23: syntax error near ';'
""",
        )
