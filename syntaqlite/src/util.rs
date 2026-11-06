// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Cross-cutting utilities for dialect configuration, compatibility, and rendering.

use std::io::{self, Write};

use syntaqlite_syntax::source::DocRange;

#[doc(inline)]
#[cfg(feature = "analysis")]
pub use crate::analysis::diagnostics::render::{DiagnosticRenderer, SourceContext};
#[doc(inline)]
pub use crate::sqlite::cflags::SqliteFlag;
#[doc(inline)]
pub use syntaqlite_syntax::util::{SqliteSyntaxFlag, SqliteSyntaxFlags, SqliteVersion};

/// Full set of `SQLite` compile-time compatibility flags.
///
/// Covers all 42 known flags using a 64-bit bitset indexed by [`SqliteFlag`]
/// discriminants. Parser flags (indices 0-21) share the same bit positions as
/// the C compact `SYNQ_CFLAG_IDX_*` values, so conversion to/from
/// [`SqliteSyntaxFlags`] requires no translation table.
///
/// Use this type with [`AnyDialect::with_cflags`](crate::any::AnyDialect::with_cflags)
/// to filter function availability based on compile-time `SQLite` configuration.
///
/// # Example
///
/// ```
/// use syntaqlite::util::{SqliteFlag, SqliteFlags};
///
/// let flags = SqliteFlags::default()
///     .with(SqliteFlag::EnableMathFunctions)
///     .with(SqliteFlag::EnableFts5);
///
/// assert!(flags.has(SqliteFlag::EnableMathFunctions));
/// assert!(flags.has(SqliteFlag::EnableFts5));
///
/// // Pass to a dialect to control function availability.
/// let dialect = syntaqlite::sqlite_dialect().erase()
///     .with_cflags(flags);
/// assert!(dialect.cflags().has(SqliteFlag::EnableMathFunctions));
/// ```
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct SqliteFlags(pub(crate) u64);

impl SqliteFlags {
    /// Returns `true` if compatibility flag `flag` is enabled.
    #[inline]
    pub fn has(&self, flag: SqliteFlag) -> bool {
        (self.0 >> (flag as u32)) & 1 != 0
    }

    /// Returns `true` if the flag at raw bit-index `idx` is enabled.
    ///
    /// Internal helper for generated availability rules where no
    /// [`SqliteFlag`] variant is available at the call site.
    #[inline]
    pub(crate) fn has_index(self, idx: u32) -> bool {
        idx < 64 && (self.0 >> idx) & 1 != 0
    }

    /// Return a copy of these flags with `flag` enabled.
    #[must_use]
    pub fn with(mut self, flag: SqliteFlag) -> Self {
        self.0 |= 1u64 << (flag as u32);
        self
    }

    /// Return a copy of these flags with `flag` disabled.
    #[must_use]
    pub fn without(mut self, flag: SqliteFlag) -> Self {
        self.0 &= !(1u64 << (flag as u32));
        self
    }
}

// ── Conversion between SqliteFlags and SqliteSyntaxFlags ─────────────────────
//
// SqliteFlag::as_syntax_flag() provides the explicit typed mapping for each
// parser flag; non-parser flags return None and are silently dropped.

impl From<SqliteFlags> for SqliteSyntaxFlags {
    /// Convert full Rust flags to C-compact parser flags.
    ///
    /// Only parser flags (indices 0–21) are preserved; non-parser flags are
    /// silently dropped since they have no C parser representation.
    fn from(flags: SqliteFlags) -> Self {
        let mut s = SqliteSyntaxFlags::default();
        for &flag in SqliteFlag::all() {
            if let Some(sf) = flag.as_syntax_flag()
                && flags.has(flag)
            {
                s = s.with(sf);
            }
        }
        s
    }
}

impl From<SqliteSyntaxFlags> for SqliteFlags {
    /// Convert C-compact parser flags to full Rust flags.
    fn from(s: SqliteSyntaxFlags) -> Self {
        let mut flags = SqliteFlags::default();
        for &flag in SqliteFlag::all() {
            if let Some(sf) = flag.as_syntax_flag()
                && s.has(sf)
            {
                flags = flags.with(flag);
            }
        }
        flags
    }
}

// ── Rustc-style source error rendering ───────────────────────────────────────

/// Parameters for rendering a rustc-style source error snippet.
///
/// `range` is treated as byte offsets into `source` — callers rendering
/// a span from a macro expansion buffer pass offsets relative to that
/// buffer (see `DiagnosticRenderer::render_diagnostic`).
pub(crate) struct SourceError<'a> {
    pub source: &'a str,
    pub file: &'a str,
    pub severity: &'a str,
    pub message: &'a str,
    pub range: DocRange,
    pub help: Option<&'a str>,
}

/// Render a rustc-style source error snippet to `out`.
///
/// General-purpose utility — callers supply raw strings and byte offsets,
/// with no dependency on diagnostic types.
///
/// ```text
/// error: syntax error near 'SELECT'
///  --> query.sql:1:15
///   |
/// 1 | SELECT id FROM usr WHERE id = 1
///   |               ^~~
///   = help: did you mean 'users'?
/// ```
///
/// # Errors
/// Returns `Err` if writing to `out` fails.
pub(crate) fn render_source_error(
    out: &mut impl Write,
    err: &SourceError<'_>,
    lines: &LineIndex,
) -> io::Result<()> {
    let start = err.range.start.as_usize();
    let end = err.range.end.as_usize();
    let (line, col, line_text) = lines.locate(err.source, start);
    let gutter_width = line.to_string().len();

    writeln!(out, "{}: {}", err.severity, err.message)?;
    writeln!(out, "{:>gutter_width$}--> {}:{line}:{col}", " ", err.file)?;
    writeln!(out, "{:>gutter_width$} |", " ")?;
    writeln!(out, "{line} | {line_text}")?;

    let underline_len = if end > start {
        let line_end = start + (line_text.len().saturating_sub(col - 1));
        (end.min(line_end) - start).max(1)
    } else {
        1
    };
    writeln!(
        out,
        "{:>gutter_width$} | {:padding$}^{}",
        " ",
        "",
        "~".repeat(underline_len.saturating_sub(1)),
        padding = col - 1,
    )?;

    if let Some(help) = err.help {
        writeln!(out, "{:>gutter_width$} = help: {help}", " ")?;
    }

    Ok(())
}

/// The byte offset where each line of a source starts, so a renderer can find
/// a line in O(log n) instead of rescanning the source for every diagnostic.
pub(crate) struct LineIndex {
    starts: Vec<usize>,
}

impl LineIndex {
    pub(crate) fn new(source: &str) -> Self {
        let mut starts = vec![0];
        starts.extend(
            source
                .bytes()
                .enumerate()
                .filter(|&(_, b)| b == b'\n')
                .map(|(i, _)| i + 1),
        );
        Self { starts }
    }

    /// The 1-based line and column of `offset`, counting columns in
    /// characters, and the text of that line without its newline.
    fn locate<'s>(&self, source: &'s str, offset: usize) -> (usize, usize, &'s str) {
        let offset = offset.min(source.len());
        let line = self.starts.partition_point(|&s| s <= offset);
        let line_start = self.starts[line - 1];
        let line_end = self.starts.get(line).map_or(source.len(), |&s| s - 1);
        let col = source
            .get(line_start..offset)
            .map_or(offset - line_start, |s| s.chars().count())
            + 1;
        (line, col, &source[line_start..line_end])
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn line_index_locates_offsets() {
        let src = "SELECT 1;\nSELECT 'é', x\n\nFROM t";
        let lines = LineIndex::new(src);
        assert_eq!(lines.locate(src, 0), (1, 1, "SELECT 1;"));
        assert_eq!(lines.locate(src, 9), (1, 10, "SELECT 1;"));
        let x = src.find('x').unwrap();
        assert_eq!(lines.locate(src, x), (2, 13, "SELECT 'é', x"));
        assert_eq!(lines.locate(src, x + 2), (3, 1, ""));
        assert_eq!(lines.locate(src, src.len()), (4, 7, "FROM t"));
    }

    /// Verify that parser-category flags all have discriminant < 22,
    /// and non-parser flags all have discriminant >= 22.
    #[test]
    fn cflag_index_invariant() {
        for &flag in SqliteFlag::all() {
            let idx = flag as u32;
            if flag.categories().contains(&"parser") {
                assert!(
                    idx < 22,
                    "parser flag {} has index {idx}, expected < 22",
                    flag.name()
                );
            } else {
                assert!(
                    idx >= 22,
                    "non-parser flag {} has index {idx}, expected >= 22",
                    flag.name()
                );
            }
        }
    }

    /// Verify that parser flags survive round-trip through `SqliteSyntaxFlags`.
    #[test]
    fn c_parser_flags_round_trip_through_syntax_flags() {
        for &flag in SqliteFlag::all() {
            if flag.as_syntax_flag().is_none() {
                continue;
            }
            let bit_index = flag as u32;
            let rust_flags = SqliteFlags(1u64 << bit_index);
            let syntax: SqliteSyntaxFlags = rust_flags.into();
            let back: SqliteFlags = syntax.into();
            assert!(
                back.has_index(bit_index),
                "C-parser flag {} (index {bit_index}) lost in SqliteFlags -> SqliteSyntaxFlags -> SqliteFlags round-trip",
                flag.name()
            );
        }
    }

    /// Verify that non-parser flags are dropped when converting to `SqliteSyntaxFlags`.
    #[test]
    fn rust_only_flags_dropped_in_syntax_flags() {
        for &flag in SqliteFlag::all() {
            if flag.as_syntax_flag().is_some() {
                continue;
            }
            let bit_index = flag as u32;
            let rust_flags = SqliteFlags(1u64 << bit_index);
            let syntax: SqliteSyntaxFlags = rust_flags.into();
            let back: SqliteFlags = syntax.into();
            assert!(
                !back.has_index(bit_index),
                "Rust-only flag {} (index {bit_index}) should be absent after round-trip through SqliteSyntaxFlags",
                flag.name()
            );
        }
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod cflag_tests {
    use super::*;

    #[test]
    fn parse_version_known() {
        assert_eq!(
            SqliteVersion::parse_with_latest("3.35.0"),
            Ok(SqliteVersion::V3_35)
        );
    }

    #[test]
    fn parse_version_latest() {
        assert_eq!(
            SqliteVersion::parse_with_latest("latest"),
            Ok(SqliteVersion::Latest)
        );
    }

    #[test]
    fn parse_version_unknown() {
        assert!(SqliteVersion::parse_with_latest("3.99.0").is_err());
    }

    #[test]
    fn parse_cflag_known() {
        assert_eq!(
            SqliteFlag::from_name("SQLITE_OMIT_WINDOWFUNC"),
            Some(SqliteFlag::OmitWindowfunc)
        );
    }

    #[test]
    fn parse_cflag_unknown() {
        assert!(SqliteFlag::from_name("SQLITE_OMIT_NONEXISTENT").is_none());
    }

    #[test]
    fn cflag_names_count() {
        let names: Vec<_> = SqliteFlag::all().iter().map(|f| f.name()).collect();
        assert_eq!(names.len(), SqliteFlag::all().len());
        assert!(names.contains(&"SQLITE_OMIT_WINDOWFUNC"));
        assert!(names.contains(&"SQLITE_ENABLE_FTS5"));
    }

    #[test]
    #[cfg(all(feature = "fmt", feature = "lsp"))]
    fn available_functions_latest_includes_builtins() {
        let host = crate::lsp::LspHost::new();
        let names = host.available_function_names();
        assert!(names.iter().any(|n| n == "abs"));
        assert!(names.iter().any(|n| n == "count"));
    }
}
