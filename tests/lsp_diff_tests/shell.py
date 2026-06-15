# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

# End-to-end LSP diagnostics for sqlite3 shell scripts (issue #88). The host
# auto-detects shell mode from a column-0 `.` / `#` marker and analyzes only the
# SQL between the shell lines, so dot-commands no longer surface as
# "syntax error near '.'". See https://sqlite.org/cli.html.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Shell(TestSuite):
    # A column-0 `.read` marks the file as shell; the dot line is dropped.
    def test_col0_dot_command_plus_sql_is_clean(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # Dot-commands require column 0; an indented `.read` is plain SQL and errors.
    def test_indented_dot_read_is_sql_error(self):
        return LspDiffTestBlueprint(
            sql="  .read foo.sql\nSELECT 1;",
            op="diagnostics",
            out="""\
            error 0:2..0:3: syntax error near '.'
""",
        )

    # A leading-dot line while a statement is pending stays SQL, and errors.
    def test_dot_mid_statement_is_sql_error(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT\n.foo",
            op="diagnostics",
            out="""\
            error 2:0..2:1: syntax error near '.'
""",
        )

    # In shell mode `GO` / `/` on their own line terminate the pending statement.
    def test_go_and_slash_terminators_split_statements(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1\nGO\nSELECT 2\n/\nSELECT 3;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # A statement ending `; -- comment` is complete, so the next column-0
    # dot-command is still recognized as shell syntax.
    def test_semicolon_before_inline_comment_allows_next_dot_command(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1; -- done\n.read b.sql\nSELECT 2;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # `GO` is a terminator only in shell mode; in plain SQL it is a parse error.
    def test_stray_go_in_pure_sql_is_error(self):
        return LspDiffTestBlueprint(
            sql="SELECT 1;\nGO",
            op="diagnostics",
            out="""\
            error 1:0..1:2: syntax error near 'GO'
""",
        )

    # A column-0 `#` whole-line comment is a shell marker; an indented `#` is not.
    def test_col0_hash_comment_is_marker(self):
        return LspDiffTestBlueprint(
            sql="# a shell comment\nSELECT 1;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    def test_indented_hash_is_sql_error(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\n  # not a marker",
            op="diagnostics",
            out="""\
            error 1:2..1:3: syntax error near '#'
""",
        )

    # issue #88 reproduction.
    def test_issue88_multiple_read_commands(self):
        return LspDiffTestBlueprint(
            sql=".read adapters/sqlite/scripts/includes/01-pragma.sql\n"
                ".read adapters/sqlite/scripts/includes/02-drops.sql\n"
                "SELECT 1;",
            op="diagnostics",
            out="""\
            (no diagnostics)
""",
        )

    # A genuine SQL error maps to the correct host line; the skipped `.read`
    # line must not shift the coordinates.
    def test_sql_error_maps_to_correct_line(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT 1;\nNOT VALID;",
            op="diagnostics",
            out="""\
            error 2:0..2:3: syntax error near 'NOT'
""",
        )

    # Semantic diagnostics still flow through shell mode, with correct columns.
    def test_unknown_table_is_warning(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\nSELECT * FROM nonexistent;",
            op="diagnostics",
            out="""\
            warning 1:14..1:25: unknown table 'nonexistent'
""",
        )

    # CRLF line endings must not desync the offset/line mapping.
    def test_crlf_error_line_mapping(self):
        return LspDiffTestBlueprint(
            sql=".read a.sql\r\nNOT VALID;\r\nSELECT 1;",
            op="diagnostics",
            out="""\
            error 1:0..1:3: syntax error near 'NOT'
""",
        )

    # Beyond diagnostics, the SQL fragments drive navigation: go-to-definition
    # resolves a reference to its DDL, mapped to host-file coordinates across the
    # skipped dot-command line.
    def test_goto_definition_within_fragment(self):
        return LspDiffTestBlueprint(
            sql=".read x.sql\nCREATE TABLE widgets (id INTEGER);\nSELECT id FROM <|>widgets;",
            op="definition",
            out="""\
            target: 1:13..1:20
""",
        )

    # Cross-fragment go-to-definition: a table defined in one run resolves from a
    # reference in a later run, even though a dot-command separates them. The
    # whole script is masked into one SQL document, so this just works.
    def test_goto_definition_across_fragments(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE widgets (id INTEGER);\n"
                ".read more.sql\n"
                "SELECT id FROM <|>widgets;",
            op="definition",
            out="""\
            target: 0:13..0:20
""",
        )
