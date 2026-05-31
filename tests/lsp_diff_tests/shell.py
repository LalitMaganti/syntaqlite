# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Shell(TestSuite):
    # R1.1: a column-0 `.read` is the unambiguous shell marker; the dot line is
    # skipped and there is no spurious `syntax error near '.'`.
    def test_col0_dot_command_plus_sql_is_shell_clean(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # R1.1: an indented `.read` is NOT a dot-command, so the file is pure SQL and
    # the indented dot line is a SQL syntax error.
    def test_indented_dot_read_is_not_marker_pure_sql_errors(self):
        return LspDiffTestBlueprint(
            sql="  .read foo.sql\nSELECT 1;",
            op="diagnostics",
            out="""\
            error 0:2..0:3: syntax error near '.'
""",
        )

    # R1.3: a leading-dot line while a SQL statement is pending is NOT a
    # dot-command; it stays SQL and, being invalid, errors.
    def test_leading_dot_mid_statement_is_sql_not_dot_command(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT\n.foo",
            op="diagnostics",
            out="""\
            error 2:0..2:1: syntax error near '.'
""",
        )

    # R2.1: in shell mode a lone `GO` terminates the pending statement, so the
    # two SELECTs do not collapse into one invalid statement.
    def test_go_terminator_splits_statements_in_shell_mode(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1\nGO\nSELECT 2;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # A SQL statement ending with `; -- comment` is complete; the following
    # column-0 dot-command must still be recognized as shell syntax.
    def test_semicolon_before_inline_comment_allows_next_dot_command(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1; -- done\n.read b.sql\nSELECT 2;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # R2.1: in shell mode a lone `/` is a statement terminator equivalent to GO.
    def test_slash_terminator_splits_statements_in_shell_mode(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1\n/\nSELECT 2;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # R2.3: trailing/leading whitespace around a terminator is ignored; `  GO  `
    # still terminates.
    def test_go_terminator_with_surrounding_whitespace(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1\n  GO  \nSELECT 2;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # R2.4: GO is a terminator ONLY in shell mode. With no shell marker the file
    # is pure SQL and the bare `GO` is a syntax error.
    def test_stray_go_in_pure_sql_remains_parse_error(self):
        return LspDiffTestBlueprint(
            sql="SELECT 1;\nGO",
            op="diagnostics",
            out="""\
            error 1:0..1:2: syntax error near 'GO'
""",
        )

    # R3.1: a column-0 `#` whole-line comment is an unambiguous shell marker; the
    # `#` line is skipped.
    def test_col0_hash_comment_is_shell_marker_skipped(self):
        return LspDiffTestBlueprint(
            sql="# a shell comment\nSELECT 1;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # R3.1: an indented `#` is NOT a shell comment; it reaches the SQL core and
    # errors.
    def test_indented_hash_in_shell_mode_is_sql_error(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\n  # not a marker",
            op="diagnostics",
            out="""\
            error 1:2..1:3: syntax error near '#'
""",
        )

    # issue #88 reproduction: multiple column-0 `.read` dot-commands followed by
    # SQL must NOT be flagged `syntax error near '.'`.
    def test_issue88_three_read_dot_commands_no_syntax_error(self):
        return LspDiffTestBlueprint(
            sql=".read adapters/sqlite/scripts/includes/01-pragma.sql\n"
                ".read adapters/sqlite/scripts/includes/02-drops.sql\n"
                "SELECT 1;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # A genuine SQL error inside a shell script maps to the correct host
    # line/column; the skipped `.read` line must not shift the SQL coordinates.
    def test_genuine_sql_error_in_shell_maps_to_correct_line(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1;\nNOT VALID;",
            op="diagnostics",
            out="""\
            error 2:0..2:3: syntax error near 'NOT'
""",
        )

    # Semantic diagnostics still flow through in shell mode; an unknown table is
    # a WARNING with the correct column mapping.
    def test_unknown_table_in_shell_is_warning_not_error(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT * FROM nonexistent;",
            op="diagnostics",
            out="""\
            warning 1:14..1:25: unknown table 'nonexistent'
""",
        )

    # CRLF line endings must not desync byte offsets / line mapping; the error on
    # the second line maps correctly.
    def test_crlf_shell_script_error_line_mapping(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\r\nNOT VALID;\r\nSELECT 1;",
            op="diagnostics",
            out="""\
            error 1:0..1:3: syntax error near 'NOT'
""",
        )
