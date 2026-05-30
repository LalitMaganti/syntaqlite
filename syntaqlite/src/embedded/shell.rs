// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! sqlite3 shell-language SQL extraction.
//!
//! The sqlite3 CLI accepts a "shell" language that is a superset of pure SQL:
//! column-0 dot-commands such as `.read foo.sql`, column-0 `#` comments, and
//! `GO` / `/` statement terminators. These constructs are NOT valid in the SQL library
//! language, so the SQL parser correctly rejects them — but scripts written for
//! the sqlite3 CLI are ubiquitous, so we must handle them.
//!
//! This module mirrors [`extract_python`](super::extract_python), but with the
//! roles flipped: instead of finding SQL *inside* a host language, we find
//! non-SQL shell lines *around* SQL. Because the SQL fragments are contiguous
//! verbatim slices of the source with no interpolation, they carry NO holes and
//! their offsets map back to the host file via a simple base offset.

use super::{EmbeddedFragment, doc_range_from_usize};

/// Classification of a single line of a sqlite3 shell script.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum LineKind {
    /// A dot-command (e.g. `.read foo.sql`), recognized only at column 0 (no
    /// leading whitespace, per the sqlite CLI docs) and only when not inside a
    /// pending statement.
    DotCommand,
    /// A `#` whole-line comment at column 0 (sqlite docs require column 0).
    HashComment,
    /// A lone `GO` (case-insensitive) or `/` statement terminator.
    Terminator,
    /// A blank / whitespace-only line.
    Blank,
    /// Anything else — treated as SQL content.
    Sql,
}

/// Classify a single raw line (without its trailing newline).
///
/// `pending_sql` indicates whether we are currently inside an unterminated SQL
/// statement; a column-0 dot line mid-statement is SQL, not a dot-command.
fn classify_line(raw: &str, pending_sql: bool) -> LineKind {
    if raw.trim().is_empty() {
        return LineKind::Blank;
    }
    // `#` comments require column 0 (no leading whitespace), per sqlite docs.
    if raw.as_bytes().first() == Some(&b'#') {
        return LineKind::HashComment;
    }
    let trimmed = raw.trim();
    if trimmed == "/" || trimmed.eq_ignore_ascii_case("GO") {
        return LineKind::Terminator;
    }
    // Dot-commands require column 0 (no leading whitespace), per the sqlite CLI
    // docs, and are only recognized when we are not in the middle of a pending
    // SQL statement.
    if !pending_sql && raw.as_bytes().first() == Some(&b'.') {
        return LineKind::DotCommand;
    }
    LineKind::Sql
}

/// Update the "are we inside an unterminated SQL statement?" flag after seeing
/// a line classified as [`LineKind::Sql`].
///
/// This is a deliberately conservative, line-level heuristic (not a real SQL
/// statement tracker): a SQL line ends a statement when its last non-whitespace
/// character is `;`. Erring toward `pending_sql == true` only ever demotes a
/// would-be dot-command to SQL, never the reverse, so detection stays safe.
fn sql_line_leaves_pending(raw: &str) -> bool {
    !raw.trim_end().ends_with(';')
}

/// Detect whether `source` is a sqlite3 shell script rather than pure SQL.
///
/// **Experimental:** this function is part of the experimental embedded SQL API.
///
/// Scans lines in order. The first unambiguous shell marker — a column-0
/// dot-command (outside a pending SQL statement) or a column-0 `#` comment —
/// switches the whole file into shell mode. `GO` / `/` terminators are NOT
/// markers on their own (a stray `GO` in a pure-SQL file must stay a parse
/// error), so they never trigger detection.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::is_shell_script;
///
/// assert!(is_shell_script(".read foo.sql\nSELECT 1;"));
/// assert!(!is_shell_script("SELECT 1;\nSELECT 2;"));
/// ```
pub fn is_shell_script(source: &str) -> bool {
    let mut pending_sql = false;
    for raw in source.lines() {
        match classify_line(raw, pending_sql) {
            LineKind::DotCommand | LineKind::HashComment => return true,
            LineKind::Terminator => pending_sql = false,
            LineKind::Blank => {}
            LineKind::Sql => pending_sql = sql_line_leaves_pending(raw),
        }
    }
    false
}

/// Extract SQL fragments from a sqlite3 shell script.
///
/// **Experimental:** this function is part of the experimental embedded SQL API.
///
/// Walks the source line by line, accumulating contiguous runs of SQL (and
/// blank) lines into fragments. Column-0 dot-command lines, column-0 `#` comments, and
/// `GO` / `/` terminators sit *between* fragments as shell content and flush the
/// pending SQL run. Each emitted fragment is a verbatim slice of the source with
/// NO interpolation holes, so its offsets map back to host-file positions via a
/// pure base offset.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::extract_shell;
///
/// let source = ".read a.sql\nSELECT 1;\n.read b.sql\nSELECT 2;";
/// let fragments = extract_shell(source);
/// assert_eq!(fragments.len(), 2);
/// assert!(fragments[0].holes().is_empty());
/// assert!(fragments[0].sql_text().contains("SELECT 1;"));
/// ```
//
// TODO: This works at the line level, so a trailing bare `;` on a dot-command
// (e.g. `.read foo.sql;`, allowed in sqlite >= 3.52.0) is handled implicitly by
// dropping the whole dot-command line. A future real dot-command parser could
// model these precisely.
pub fn extract_shell(source: &str) -> Vec<EmbeddedFragment> {
    let mut fragments = Vec::new();
    let mut pending_sql = false;
    let mut run_start: Option<usize> = None;
    let mut run_end = 0usize;
    let mut cursor = 0usize;

    for raw in source.lines() {
        let line_start = cursor;
        let line_end = cursor + raw.len();
        cursor = next_line_offset(source, line_end);

        match classify_line(raw, pending_sql) {
            LineKind::Sql => {
                pending_sql = sql_line_leaves_pending(raw);
                run_start.get_or_insert(line_start);
                run_end = line_end;
            }
            LineKind::Blank => {
                // Blank lines extend an in-progress run but never start one.
                if run_start.is_some() {
                    run_end = line_end;
                }
            }
            LineKind::DotCommand | LineKind::HashComment | LineKind::Terminator => {
                pending_sql = false;
                flush_run(source, &mut fragments, run_start.take(), run_end);
            }
        }
    }
    flush_run(source, &mut fragments, run_start.take(), run_end);

    fragments
}

/// Emit a fragment for the SQL run `[start, end)` if it contains any
/// non-whitespace content.
fn flush_run(
    source: &str,
    fragments: &mut Vec<EmbeddedFragment>,
    start: Option<usize>,
    end: usize,
) {
    let Some(start) = start else { return };
    let text = &source[start..end];
    if text.trim().is_empty() {
        return;
    }
    fragments.push(EmbeddedFragment {
        sql_range: doc_range_from_usize(start, end),
        sql_text: text.to_string(),
        holes: Vec::new(),
    });
}

/// Given the byte offset just past a line's content (i.e. at its `\r\n` / `\n`
/// terminator or at end-of-source), return the byte offset of the next line's
/// first byte.
///
/// `str::lines()` yields lines without their terminator and strips a trailing
/// `\r` before the `\n`, so on CRLF (Windows) input `line_content_end` points at
/// the `\r`. We must skip both bytes to keep the cursor in sync; otherwise every
/// subsequent line's byte offsets — and thus the extracted fragment slices —
/// drift by the unconsumed terminator bytes.
fn next_line_offset(source: &str, line_content_end: usize) -> usize {
    let bytes = source.as_bytes();
    let mut offset = line_content_end;
    if offset < bytes.len() && bytes[offset] == b'\r' {
        offset += 1;
    }
    if offset < bytes.len() && bytes[offset] == b'\n' {
        offset += 1;
    }
    offset
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn classify_dot_command() {
        assert_eq!(classify_line(".read x.sql", false), LineKind::DotCommand);
    }

    #[test]
    fn classify_indented_dot_is_sql() {
        // Dot-commands require column 0; an indented `.read` is SQL, not a
        // dot-command (sqlite CLI docs require the `.` at the left margin).
        assert_eq!(classify_line("  .read x.sql", false), LineKind::Sql);
    }

    #[test]
    fn classify_dot_in_pending_statement_is_sql() {
        assert_eq!(classify_line(".foo", true), LineKind::Sql);
    }

    #[test]
    fn classify_hash_comment_requires_col0() {
        assert_eq!(classify_line("# a comment", false), LineKind::HashComment);
        // Indented `#` is not a shell comment marker.
        assert_eq!(classify_line("  # not col0", false), LineKind::Sql);
    }

    #[test]
    fn classify_terminators() {
        assert_eq!(classify_line("GO", false), LineKind::Terminator);
        assert_eq!(classify_line("go", false), LineKind::Terminator);
        assert_eq!(classify_line("  GO  ", false), LineKind::Terminator);
        assert_eq!(classify_line("/", false), LineKind::Terminator);
    }

    #[test]
    fn classify_sql_and_blank() {
        assert_eq!(classify_line("SELECT 1", false), LineKind::Sql);
        assert_eq!(classify_line("   ", false), LineKind::Blank);
        assert_eq!(classify_line("", false), LineKind::Blank);
    }

    #[test]
    fn detect_dot_command() {
        assert!(is_shell_script(".read a.sql\nSELECT 1;"));
    }

    #[test]
    fn no_detect_indented_dot_command() {
        // An indented `.read` is not a column-0 dot-command, so it does not
        // switch the file into shell mode — it stays pure SQL (and errors).
        assert!(!is_shell_script("  .read x.sql\nSELECT 1;"));
    }

    #[test]
    fn detect_col0_hash_comment() {
        assert!(is_shell_script("# a shell comment\nSELECT 1;"));
    }

    #[test]
    fn no_detect_indented_hash() {
        assert!(!is_shell_script("SELECT 1;\n  # not a marker"));
    }

    #[test]
    fn no_detect_stray_go() {
        assert!(!is_shell_script("SELECT 1;\nGO"));
    }

    #[test]
    fn no_detect_plain_sql() {
        assert!(!is_shell_script("SELECT 1;\nSELECT 2;"));
    }

    #[test]
    fn extract_between_dot_commands() {
        let source = ".read a.sql\nSELECT 1;\n.read b.sql\nSELECT 2;";
        let fragments = extract_shell(source);
        assert_eq!(fragments.len(), 2);

        for f in &fragments {
            assert!(f.holes().is_empty());
        }
        assert!(fragments[0].sql_text().contains("SELECT 1;"));
        assert!(fragments[1].sql_text().contains("SELECT 2;"));

        let first_select = source.find("SELECT 1").unwrap();
        assert_eq!(fragments[0].sql_range().start.as_usize(), first_select);
    }

    #[test]
    fn terminator_splits_fragments() {
        let source = "SELECT 1\nGO\nSELECT 2\n/";
        let fragments = extract_shell(source);
        assert_eq!(fragments.len(), 2);
        assert!(fragments[0].sql_text().contains("SELECT 1"));
        assert!(fragments[1].sql_text().contains("SELECT 2"));
    }

    #[test]
    fn fragment_text_is_verbatim_slice() {
        let source = ".read a.sql\nSELECT 1;";
        let fragments = extract_shell(source);
        assert_eq!(fragments.len(), 1);
        let f = &fragments[0];
        let start = f.sql_range().start.as_usize();
        let end = start + f.sql_text().len();
        assert_eq!(&source[start..end], f.sql_text());
    }

    #[test]
    fn dot_commands_only_yield_no_fragments() {
        let fragments = extract_shell(".read a.sql\n.read b.sql\n");
        assert!(fragments.is_empty());
    }

    #[test]
    fn extract_handles_crlf_line_endings() {
        // Windows scripts use `\r\n`; `str::lines()` strips the `\r`, so the
        // byte-offset accounting must skip both terminator bytes or fragment
        // slices drift (regression test for the CRLF cursor desync).
        let source = ".read a.sql\r\nSELECT 1;\r\n.read b.sql\r\nSELECT 2;";
        let fragments = extract_shell(source);
        assert_eq!(fragments.len(), 2);
        // Verbatim-slice invariant must hold, with no stray `\r` leaking in.
        for f in &fragments {
            let start = f.sql_range().start.as_usize();
            let end = start + f.sql_text().len();
            assert_eq!(&source[start..end], f.sql_text());
            assert!(
                !f.sql_text().contains('\r'),
                "fragment leaked a CR: {:?}",
                f.sql_text()
            );
        }
        assert_eq!(fragments[0].sql_text(), "SELECT 1;");
        assert_eq!(fragments[1].sql_text(), "SELECT 2;");
    }
}
