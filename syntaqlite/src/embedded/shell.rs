// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! sqlite3 shell-language SQL extraction.
//!
//! The sqlite3 CLI accepts a "shell" language layered on top of SQL: column-0
//! dot-commands (`.read foo.sql`), column-0 `#` comments, and `GO` / `/`
//! statement terminators. None of these are valid in the SQL library language,
//! so the parser rejects them — but such scripts are ubiquitous, so we peel the
//! shell lines off and analyze only the SQL between them (issue #88).
//!
//! Fragments are verbatim, hole-free slices of the source, so their offsets map
//! back to the host file by a plain base offset (see [`EmbeddedFragment`]).

use std::borrow::Cow;
use syntaqlite_syntax::any::{AnyDialect as SyntaxDialect, AnyTokenizer, TokenCategory};

use super::{EmbeddedFragment, doc_range_from_usize};
use crate::dialect::AnyDialect;

/// Detect whether `source` is a sqlite3 shell script rather than plain SQL.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// True iff the file contains an unambiguous shell marker: a column-0
/// dot-command or column-0 `#` comment outside a pending SQL statement. `GO` /
/// `/` terminators are not markers on their own, so a stray `GO` in plain SQL
/// stays a parse error.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::is_shell_script;
///
/// let sqlite = syntaqlite::sqlite_dialect();
/// assert!(is_shell_script(sqlite.clone(), ".read foo.sql\nSELECT 1;"));
/// assert!(!is_shell_script(sqlite, "SELECT 1;\nSELECT 2;"));
/// ```
pub fn is_shell_script(dialect: impl Into<AnyDialect>, source: &str) -> bool {
    may_have_shell_lines(source) && scan(&dialect.into(), source).is_shell
}

/// Whether any line starts with `.` or `#`, the only lines that can make a
/// file a shell script. Lets ordinary SQL skip the line-by-line scan.
///
/// Reads the input once: each 64-byte block is checked with SIMD for a
/// newline followed by a marker.
fn may_have_shell_lines(source: &str) -> bool {
    let bytes = source.as_bytes();
    if matches!(bytes.first(), Some(b'.' | b'#')) {
        return true;
    }
    let mut i = 0;
    while let Some(block) = bytes[i..].first_chunk::<65>() {
        if newline_then_marker(block) {
            return true;
        }
        i += 64;
    }
    newline_then_marker_scalar(&bytes[i..])
}

fn newline_then_marker_scalar(bytes: &[u8]) -> bool {
    bytes
        .windows(2)
        .any(|w| w[0] == b'\n' && matches!(w[1], b'.' | b'#'))
}

/// Whether a newline in the first 64 bytes of `block` is followed by a marker.
#[cfg(target_arch = "x86_64")]
fn newline_then_marker(block: &[u8; 65]) -> bool {
    use std::arch::x86_64::{
        _mm_and_si128, _mm_cmpeq_epi8, _mm_loadu_si128, _mm_movemask_epi8, _mm_or_si128,
        _mm_set1_epi8, _mm_setzero_si128,
    };
    // SAFETY: SSE2 is part of the x86_64 baseline, and every 16-byte load
    // starts at or before offset 49 of the 65-byte block.
    unsafe {
        let newline = _mm_set1_epi8(b'\n'.cast_signed());
        let dot = _mm_set1_epi8(b'.'.cast_signed());
        let hash = _mm_set1_epi8(b'#'.cast_signed());
        let mut hits = _mm_setzero_si128();
        for k in (0..64).step_by(16) {
            let cur = _mm_loadu_si128(block.as_ptr().add(k).cast());
            let next = _mm_loadu_si128(block.as_ptr().add(k + 1).cast());
            let markers = _mm_or_si128(_mm_cmpeq_epi8(next, dot), _mm_cmpeq_epi8(next, hash));
            hits = _mm_or_si128(hits, _mm_and_si128(_mm_cmpeq_epi8(cur, newline), markers));
        }
        _mm_movemask_epi8(hits) != 0
    }
}

#[cfg(target_arch = "aarch64")]
fn newline_then_marker(block: &[u8; 65]) -> bool {
    use std::arch::aarch64::{vandq_u8, vceqq_u8, vdupq_n_u8, vld1q_u8, vmaxvq_u8, vorrq_u8};
    // SAFETY: NEON is part of the aarch64 baseline, and every 16-byte load
    // starts at or before offset 49 of the 65-byte block.
    unsafe {
        let newline = vdupq_n_u8(b'\n');
        let dot = vdupq_n_u8(b'.');
        let hash = vdupq_n_u8(b'#');
        let mut hits = vdupq_n_u8(0);
        for k in (0..64).step_by(16) {
            let cur = vld1q_u8(block.as_ptr().add(k));
            let next = vld1q_u8(block.as_ptr().add(k + 1));
            let markers = vorrq_u8(vceqq_u8(next, dot), vceqq_u8(next, hash));
            hits = vorrq_u8(hits, vandq_u8(vceqq_u8(cur, newline), markers));
        }
        vmaxvq_u8(hits) != 0
    }
}

#[cfg(all(target_arch = "wasm32", target_feature = "simd128"))]
fn newline_then_marker(block: &[u8; 65]) -> bool {
    use std::arch::wasm32::{u8x16_eq, u8x16_splat, v128_and, v128_any_true, v128_load, v128_or};
    let newline = u8x16_splat(b'\n');
    let dot = u8x16_splat(b'.');
    let hash = u8x16_splat(b'#');
    let mut hits = u8x16_splat(0);
    for k in (0..64).step_by(16) {
        // SAFETY: every 16-byte load starts at or before offset 49 of the
        // 65-byte block.
        let (cur, next) = unsafe {
            (
                v128_load(block.as_ptr().add(k).cast()),
                v128_load(block.as_ptr().add(k + 1).cast()),
            )
        };
        let markers = v128_or(u8x16_eq(next, dot), u8x16_eq(next, hash));
        hits = v128_or(hits, v128_and(u8x16_eq(cur, newline), markers));
    }
    v128_any_true(hits)
}

#[cfg(not(any(
    target_arch = "x86_64",
    target_arch = "aarch64",
    all(target_arch = "wasm32", target_feature = "simd128")
)))]
fn newline_then_marker(block: &[u8; 65]) -> bool {
    newline_then_marker_scalar(block)
}

/// Extract the SQL fragments of a sqlite3 shell script.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// Shell lines (dot-commands, column-0 `#` comments, `GO` / `/` terminators)
/// separate the fragments; each fragment is a verbatim, hole-free slice whose
/// offsets map straight back to the host file.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::extract_shell;
///
/// let source = ".read a.sql\nSELECT 1;\n.read b.sql\nSELECT 2;";
/// let fragments = extract_shell(syntaqlite::sqlite_dialect(), source);
/// assert_eq!(fragments.len(), 2);
/// assert!(fragments[0].sql_text().contains("SELECT 1;"));
/// ```
pub fn extract_shell(dialect: impl Into<AnyDialect>, source: &str) -> Vec<EmbeddedFragment<'_>> {
    scan(&dialect.into(), source).fragments
}

/// Result of a single shell-script pass.
struct ShellScan<'a> {
    /// Whether an unambiguous shell marker was seen (see [`is_shell_script`]).
    is_shell: bool,
    /// SQL runs found between the shell lines.
    fragments: Vec<EmbeddedFragment<'a>>,
}

/// Classification of one source line.
enum Line {
    /// Column-0 dot-command (`.read foo.sql`) outside a pending statement.
    DotCommand,
    /// Column-0 `#` whole-line comment outside a pending statement.
    HashComment,
    /// A lone `GO` (case-insensitive) or `/` statement terminator.
    Terminator,
    /// A blank / whitespace-only line.
    Blank,
    /// Anything else — SQL content.
    Sql,
}

/// Single pass shared by [`is_shell_script`] and [`extract_shell`].
///
/// Walks the file line by line, accumulating contiguous SQL into runs. A
/// column-0 `.` / `#` line, or a lone `GO` / `/`, ends the current run and is
/// dropped as shell content. A column-0 `.` / `#` is ambiguous — it may be a
/// shell line or part of a multi-line SQL statement — so [`run_complete`] treats
/// it as shell only when the SQL so far forms a complete (`;`-terminated)
/// statement.
fn scan<'a>(dialect: &AnyDialect, source: &'a str) -> ShellScan<'a> {
    let syntax = dialect.syntax_dialect();
    let tokenizer = AnyTokenizer::new(syntax.clone());

    let mut fragments = Vec::new();
    let mut is_shell = false;
    // Byte range of the SQL run being accumulated, if any.
    let mut run: Option<(usize, usize)> = None;

    let mut cursor = 0;
    for raw in source.lines() {
        let line_start = cursor;
        let line_end = cursor + raw.len();
        cursor = next_line_offset(source, line_end);

        // `run_complete` tokenizes, so only evaluate it for the column-0 `.` /
        // `#` lines whose classification actually depends on it.
        let pending = || run.is_some_and(|(s, e)| !run_complete(syntax, &tokenizer, &source[s..e]));
        match classify(raw, pending) {
            Line::Sql => {
                let start = run.map_or(line_start, |(s, _)| s);
                run = Some((start, line_end));
            }
            Line::Blank => {
                if let Some((s, _)) = run {
                    run = Some((s, line_end));
                }
            }
            Line::DotCommand | Line::HashComment => {
                is_shell = true;
                flush(source, &mut fragments, run.take());
            }
            Line::Terminator => flush(source, &mut fragments, run.take()),
        }
    }
    flush(source, &mut fragments, run.take());

    ShellScan {
        is_shell,
        fragments,
    }
}

/// Classify a raw line (without its trailing newline). `pending` reports whether
/// an unterminated SQL statement precedes this line; it is consulted only for
/// column-0 `.` / `#` lines, which are shell syntax solely outside a statement.
fn classify(raw: &str, pending: impl Fn() -> bool) -> Line {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return Line::Blank;
    }
    if trimmed == "/" || trimmed.eq_ignore_ascii_case("GO") {
        return Line::Terminator;
    }
    match raw.as_bytes().first() {
        Some(b'.') if !pending() => Line::DotCommand,
        Some(b'#') if !pending() => Line::HashComment,
        _ => Line::Sql,
    }
}

/// Whether `sql` forms a complete statement, i.e. its last meaningful token
/// (ignoring whitespace and comments) is a semicolon, or it is empty. Uses the
/// real tokenizer, so multi-line strings and block comments are handled
/// correctly. Empty/whitespace-only input counts as complete.
fn run_complete(dialect: &SyntaxDialect, tokenizer: &AnyTokenizer, sql: &str) -> bool {
    let mut last = None;
    for tok in tokenizer.tokenize(sql) {
        if dialect.token_category(tok.token_type()) == TokenCategory::Comment
            || tok.text().trim().is_empty()
        {
            continue;
        }
        last = Some(tok.text());
    }
    last.is_none_or(|t| t == ";")
}

/// Emit a fragment for the SQL run `[start, end)` if it has non-whitespace content.
fn flush<'a>(
    source: &'a str,
    fragments: &mut Vec<EmbeddedFragment<'a>>,
    run: Option<(usize, usize)>,
) {
    let Some((start, end)) = run else { return };
    let text = &source[start..end];
    if text.trim().is_empty() {
        return;
    }
    fragments.push(EmbeddedFragment {
        sql_range: doc_range_from_usize(start, end),
        sql_text: Cow::Borrowed(text),
        holes: Vec::new(),
    });
}

/// Byte offset of the next line's first byte, given the offset just past a
/// line's content. `str::lines()` strips a `\r` before `\n`, so on CRLF input
/// both terminator bytes must be skipped or every later offset drifts.
fn next_line_offset(source: &str, line_content_end: usize) -> usize {
    let bytes = source.as_bytes();
    let mut offset = line_content_end;
    if bytes.get(offset) == Some(&b'\r') {
        offset += 1;
    }
    if bytes.get(offset) == Some(&b'\n') {
        offset += 1;
    }
    offset
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::*;

    fn sqlite() -> AnyDialect {
        crate::sqlite::dialect::dialect().into()
    }

    fn is_shell(source: &str) -> bool {
        is_shell_script(sqlite(), source)
    }

    fn extract(source: &str) -> Vec<EmbeddedFragment<'_>> {
        extract_shell(sqlite(), source)
    }

    #[test]
    fn may_have_shell_lines_finds_markers_at_line_starts() {
        assert!(may_have_shell_lines(".read a.sql"));
        assert!(may_have_shell_lines("SELECT 1;\n# note"));
        assert!(may_have_shell_lines("SELECT 1;\r\n.mode csv"));
        // A newline ending one 64-byte block and a marker starting the next.
        let mut boundary = "x".repeat(63);
        boundary.push_str("\n.read b.sql");
        assert!(may_have_shell_lines(&boundary));
        assert!(!may_have_shell_lines("SELECT t.a FROM t;\nSELECT 1.5;"));
        assert!(!may_have_shell_lines(""));
    }

    #[test]
    fn may_have_shell_lines_handles_every_offset() {
        for len in [2, 64, 65, 66, 129, 200] {
            for pos in 0..len - 1 {
                for pair in ["\n.", "\n#", ".\n", "\nx"] {
                    let mut s = "x".repeat(len);
                    s.replace_range(pos..pos + 2, pair);
                    let expected = s.starts_with('.') || pair == "\n." || pair == "\n#";
                    assert_eq!(may_have_shell_lines(&s), expected, "{len} {pos} {pair:?}");
                }
            }
        }
    }

    #[test]
    fn detects_col0_dot_command() {
        assert!(is_shell(".read a.sql\nSELECT 1;"));
    }

    #[test]
    fn detects_col0_hash_comment() {
        assert!(is_shell("# a shell comment\nSELECT 1;"));
    }

    #[test]
    fn indented_marker_is_not_a_marker() {
        // Dot-commands and `#` comments require column 0.
        assert!(!is_shell("  .read x.sql\nSELECT 1;"));
        assert!(!is_shell("SELECT 1;\n  # not a marker"));
    }

    #[test]
    fn stray_terminator_does_not_trigger_shell() {
        // `GO` / `/` are honored only once a marker has put us in shell mode.
        assert!(!is_shell("SELECT 1;\nGO"));
        assert!(!is_shell("SELECT 1;\nSELECT 2;"));
    }

    #[test]
    fn dot_mid_statement_stays_sql() {
        // `SELECT` is pending, so the column-0 `.foo` is SQL, not a dot-command.
        assert!(!is_shell("SELECT\n.foo"));
    }

    #[test]
    fn dot_inside_multiline_string_stays_sql() {
        // The `.` line sits inside an unterminated string literal, so it is SQL,
        // not a dot-command.
        let frags = extract("SELECT '\n.read\n';");
        assert_eq!(frags.len(), 1);
        assert!(!is_shell("SELECT '\n.read\n';"));
    }

    #[test]
    fn one_fragment_per_sql_run() {
        let frags = extract(".read a.sql\nSELECT 1;\n.read b.sql\nSELECT 2;");
        assert_eq!(frags.len(), 2);
        assert!(frags.iter().all(|f| f.holes().is_empty()));
        assert_eq!(frags[0].sql_text(), "SELECT 1;");
        assert_eq!(frags[1].sql_text(), "SELECT 2;");
    }

    #[test]
    fn fragment_offset_maps_to_host() {
        let src = ".read a.sql\nSELECT 1;";
        let f = &extract(src)[0];
        let start = f.sql_range().start.as_usize();
        assert_eq!(start, src.find("SELECT").unwrap());
        assert_eq!(&src[start..start + f.sql_text().len()], f.sql_text());
    }

    #[test]
    fn terminator_splits_runs() {
        let frags = extract("SELECT 1\nGO\nSELECT 2\n/");
        assert_eq!(frags.len(), 2);
        assert!(frags[0].sql_text().contains("SELECT 1"));
        assert!(frags[1].sql_text().contains("SELECT 2"));
    }

    #[test]
    fn semicolon_then_inline_comment_completes_statement() {
        // `; -- done` is a complete statement, so the next dot-command is shell.
        let frags = extract(".read a.sql\nSELECT 1; -- done\n.read b.sql\nSELECT 2;");
        assert_eq!(frags.len(), 2);
        assert_eq!(frags[0].sql_text(), "SELECT 1; -- done");
    }

    #[test]
    fn dot_commands_only_yield_no_fragments() {
        assert!(extract(".read a.sql\n.read b.sql\n").is_empty());
    }

    #[test]
    fn crlf_offsets_do_not_drift() {
        let src = ".read a.sql\r\nSELECT 1;\r\n.read b.sql\r\nSELECT 2;";
        let frags = extract(src);
        assert_eq!(frags.len(), 2);
        for f in &frags {
            let start = f.sql_range().start.as_usize();
            assert_eq!(&src[start..start + f.sql_text().len()], f.sql_text());
        }
        assert_eq!(frags[0].sql_text(), "SELECT 1;");
        assert_eq!(frags[1].sql_text(), "SELECT 2;");
    }
}
