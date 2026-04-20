// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Embedded SQL extraction from host language sources.
//!
//! # Experimental
//!
//! This module is experimental and its API may change in future releases.
//!
//! Extracts SQL fragments from host language files, replaces interpolation holes
//! with macro-call placeholders (`HOLE_PLACEHOLDER`), runs validation via
//! [`Analyzer`](crate::Analyzer) with `macro_fallback` enabled,
//! and maps diagnostic offsets back to host-file positions. The parser records a
//! [`MacroRewrite`](crate::parse::MacroRewrite) for each hole, which is used to filter
//! diagnostics that would otherwise reference the placeholder.
//!
//! Language-specific extractors live in submodules:
//! - [`extract_python`](crate::embedded::extract_python) — Python f-string extraction
//! - [`extract_typescript`](crate::embedded::extract_typescript) — TypeScript/JavaScript template literal extraction

pub(crate) mod offset_map;
mod python;
mod typescript;

#[doc(inline)]
pub use python::extract_python;
#[doc(inline)]
pub use typescript::extract_typescript;

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::{AnyTokenType, TokenCategory};
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

use crate::analysis::AnalysisConfig;
use crate::analysis::AnalysisContext;
use crate::analysis::catalog::Catalog;
use crate::analysis::diagnostics::Diagnostic;
use crate::analysis::engine::Analyzer;
use crate::analysis::engine::walker::WalkPass;
use crate::dialect::AnyDialect;

use offset_map::OffsetMap;

// ── Shared types ────────────────────────────────────────────────────────

/// A SQL fragment extracted from a host language source file.
///
/// **Experimental:** this type is part of the experimental embedded SQL API.
///
/// Represents one contiguous SQL string found by a language-specific extractor
/// such as [`extract_python`] or [`extract_typescript`]. The fragment contains
/// the rewritten SQL text (with interpolation holes replaced by
/// [`HOLE_PLACEHOLDER`]) and metadata for mapping offsets back to the original
/// host file.
///
/// Use this when you need to inspect extraction results before passing them
/// to [`EmbeddedAnalyzer::analyze`] or
/// [`EmbeddedAnalyzer::semantic_tokens_encoded`].
#[derive(Debug)]
pub struct EmbeddedFragment {
    /// Byte range of the SQL content in the host file (excluding quotes).
    pub(crate) sql_range: DocRange,
    /// SQL text with holes replaced by placeholder identifiers.
    pub(crate) sql_text: String,
    /// Information about each interpolation hole.
    pub(crate) holes: Vec<Hole>,
}

impl EmbeddedFragment {
    /// Byte range of the SQL content in the host file (excluding quotes).
    pub fn sql_range(&self) -> DocRange {
        self.sql_range
    }
    /// SQL text with holes replaced by placeholder identifiers.
    pub fn sql_text(&self) -> &str {
        &self.sql_text
    }
    /// Information about each interpolation hole.
    pub fn holes(&self) -> &[Hole] {
        &self.holes
    }
}

/// An interpolation hole (e.g. `{expr}` in a Python f-string, `${expr}` in JS).
///
/// Holes are replaced with [`HOLE_PLACEHOLDER`] in `sql_text` and parsed as
/// macro calls via the parser's `macro_fallback` mode. The parser records a
/// [`MacroRewrite`](syntaqlite_syntax::any::MacroRewrite) for each one.
#[derive(Debug)]
pub struct Hole {
    /// Byte range of the hole expression in the host file.
    pub(crate) host_range: DocRange,
    /// Byte offset in `sql_text` where the placeholder sits.
    pub(crate) sql_offset: DocOffset,
}

impl Hole {
    /// Byte range of the hole expression in the host file.
    pub fn host_range(&self) -> DocRange {
        self.host_range
    }
    /// Byte offset in `sql_text` where the placeholder sits.
    pub fn sql_offset(&self) -> DocOffset {
        self.sql_offset
    }
}

/// Placeholder text inserted into `sql_text` for each interpolation hole.
///
/// Uses macro-call syntax so the parser's `macro_fallback` mode treats it as a
/// single identifier token and records a [`MacroRewrite`](syntaqlite_syntax::any::MacroRewrite).
pub const HOLE_PLACEHOLDER: &str = "__h__!()";

// ── Conversion helpers for extractors ──────────────────────────────────

/// Convert a `usize` byte offset from a host-language scanner into a
/// [`DocOffset`].  Host files addressable within `u32` are the supported
/// range; anything larger saturates.
pub(super) fn doc_offset_from_usize(n: usize) -> DocOffset {
    DocOffset::from_raw(u32::try_from(n).unwrap_or(u32::MAX))
}

/// Convert a `usize` byte length into a [`DocLen`] with the same saturation
/// rule as [`doc_offset_from_usize`].
pub(super) fn doc_len_from_usize(n: usize) -> DocLen {
    DocLen::from_raw(u32::try_from(n).unwrap_or(u32::MAX))
}

/// Construct a [`DocRange`] from a half-open `usize` byte range, as produced
/// by host-language scanners.
pub(super) fn doc_range_from_usize(start: usize, end: usize) -> DocRange {
    DocRange::from_offset_len(
        doc_offset_from_usize(start),
        doc_len_from_usize(end.saturating_sub(start)),
    )
}

// ── Shared scanner utilities ────────────────────────────────────────────

/// SQL keywords that identify a string as containing SQL.
const SQL_KEYWORDS: &[&str] = &[
    "SELECT",
    "INSERT",
    "UPDATE",
    "DELETE",
    "CREATE",
    "ALTER",
    "DROP",
    "WITH",
    "EXPLAIN",
    "PRAGMA",
    "ATTACH",
    "DETACH",
    "REINDEX",
    "VACUUM",
    "BEGIN",
    "COMMIT",
    "ROLLBACK",
    "SAVEPOINT",
    "RELEASE",
];

/// Check if the given text starts with a SQL keyword (case-insensitive).
fn starts_with_sql_keyword(text: &str) -> bool {
    let trimmed = text.trim_start();
    for kw in SQL_KEYWORDS {
        if trimmed.len() >= kw.len()
            && trimmed[..kw.len()].eq_ignore_ascii_case(kw)
            && (trimmed.len() == kw.len() || !trimmed.as_bytes()[kw.len()].is_ascii_alphanumeric())
        {
            return true;
        }
    }
    false
}

/// Skip a single-line string literal (`"..."` or `'...'`) with backslash escapes.
///
/// Shared by both Python (non-triple-quote case) and TypeScript/JavaScript
/// extractors. Terminates at the matching quote, a newline, or end of input.
fn skip_single_line_string(bytes: &[u8], pos: usize, end: usize) -> usize {
    let quote = bytes[pos];
    let mut j = pos + 1;
    while j < end {
        if bytes[j] == b'\\' {
            j += 2;
            continue;
        }
        if bytes[j] == quote {
            return j + 1;
        }
        if bytes[j] == b'\n' {
            return j;
        }
        j += 1;
    }
    j
}

// ── EmbeddedAnalyzer ────────────────────────────────────────────────────

/// Analyzer for embedded SQL in host-language source files.
///
/// **Experimental:** this type is part of the experimental embedded SQL API.
///
/// Holds the dialect, optional catalog context, and validation config so they
/// don't need to be threaded through every call. Use this when you want to
/// lint SQL embedded in Python, TypeScript, or other host languages.
///
/// The workflow is:
/// 1. Extract fragments with [`extract_python`] or [`extract_typescript`].
/// 2. Create an `EmbeddedAnalyzer`, optionally attaching a [`Catalog`] for
///    schema-aware validation.
/// 3. Call [`analyze`](Self::analyze) to get diagnostics mapped to host-file
///    positions, or [`semantic_tokens_encoded`](Self::semantic_tokens_encoded)
///    for syntax highlighting.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::{EmbeddedAnalyzer, extract_python};
///
/// let python_source = r#"db.execute(f"SELECT id, name FROM users WHERE age > {min_age}")"#;
/// let fragments = extract_python(python_source);
/// assert_eq!(fragments.len(), 1);
///
/// let mut analyzer = EmbeddedAnalyzer::new(syntaqlite::sqlite_dialect());
/// let diags = analyzer.analyze(&fragments);
/// // `diags` contains diagnostics with byte offsets into `python_source`.
/// ```
pub struct EmbeddedAnalyzer {
    dialect: AnyDialect,
    catalog: Catalog,
    config: AnalysisConfig,
}

impl EmbeddedAnalyzer {
    /// Create a new analyzer with an empty catalog and default validation config.
    pub fn new(dialect: impl Into<AnyDialect>) -> Self {
        let dialect = dialect.into();
        let catalog = Catalog::new(dialect.clone());
        Self {
            dialect,
            catalog,
            config: AnalysisConfig::default(),
        }
    }

    /// Attach a catalog context to enable relation/function validation.
    #[must_use]
    pub fn with_catalog(mut self, catalog: Catalog) -> Self {
        self.catalog = catalog;
        self
    }

    /// Override the default validation config.
    #[must_use]
    pub fn with_config(mut self, config: AnalysisConfig) -> Self {
        self.config = config;
        self
    }

    /// Validate all SQL fragments and return diagnostics mapped to host-file positions.
    ///
    /// Diagnostics whose spans fall inside a hole placeholder are automatically
    /// filtered out by the internal offset map returning `None`.
    pub fn analyze(&mut self, fragments: &[EmbeddedFragment]) -> Vec<Diagnostic> {
        let mut all_diags = Vec::new();

        for fragment in fragments {
            let diags = self.validate_fragment(fragment);
            let offset_map = OffsetMap::new(fragment);

            for mut d in diags {
                let Some(start) = offset_map.to_host(d.range.start) else {
                    continue;
                };
                let end = offset_map.to_host(d.range.end).unwrap_or(start);
                d.range = DocRange { start, end };
                all_diags.push(d);
            }
        }

        all_diags
    }

    /// Compute semantic tokens for a single embedded SQL fragment.
    ///
    /// Returns `(sql_offset, length, category)` tuples with byte offsets into
    /// `fragment.sql_text`. The caller is responsible for mapping these through
    /// an [`OffsetMap`] to host-file positions.
    pub(crate) fn fragment_semantic_tokens(
        &mut self,
        fragment: &EmbeddedFragment,
    ) -> Vec<(DocOffset, DocLen, TokenCategory)> {
        let mut analyzer = self.make_analyzer();
        let mut pass = TokenCapturePass::default();
        let mut ctx = AnalysisContext::new(&mut self.catalog).with_config(self.config);
        let _ = analyzer.analyze_with_pass(fragment.sql_text(), &mut ctx, &mut pass);
        let dialect = analyzer.dialect();
        let mut out: Vec<(DocOffset, DocLen, TokenCategory)> = Vec::new();
        for (offset, length, tt, flags) in pass.tokens {
            let cat = dialect.classify_token(tt, flags);
            if cat != TokenCategory::Other {
                out.push((offset, length, cat));
            }
        }
        for (offset, length) in pass.comments {
            out.push((offset, length, TokenCategory::Comment));
        }
        out.sort_by_key(|t| t.0);
        out
    }

    /// Produce LSP-encoded semantic tokens for a host-language source containing
    /// embedded SQL.
    ///
    /// Delta-encodes the result into the
    /// `[deltaLine, deltaStart, length, tokenType, modifiers]` 5-tuple format
    /// consumed by LSP `textDocument/semanticTokens` responses.
    ///
    /// # Panics
    /// Panics if a host token length does not fit in `u32` (practically impossible).
    pub fn semantic_tokens_encoded(
        &mut self,
        fragments: &[EmbeddedFragment],
        source: &str,
    ) -> Vec<u32> {
        let source_bytes = source.as_bytes();
        let source_end = DocOffset::from_raw(u32::try_from(source.len()).unwrap_or(u32::MAX));

        // Collect (host_offset, length, legend_idx) for all fragments.
        let mut all_tokens: Vec<(DocOffset, DocLen, u32)> = Vec::new();
        for fragment in fragments {
            let offset_map = OffsetMap::new(fragment);
            for (sql_offset, length, cat) in self.fragment_semantic_tokens(fragment) {
                if cat == TokenCategory::Other {
                    continue;
                }
                let legend_idx = cat as u32;
                let Some(host_offset) = offset_map.to_host(sql_offset) else {
                    continue;
                };
                let remaining: DocLen = if host_offset >= source_end {
                    DocLen::default()
                } else {
                    source_end - host_offset
                };
                let host_len = std::cmp::min(length, remaining);
                if host_len == DocLen::default() {
                    continue;
                }
                all_tokens.push((host_offset, host_len, legend_idx));
            }
        }

        // Sort by host offset before delta-encoding.
        all_tokens.sort_by_key(|t| t.0);

        // Delta-encode into LSP 5-tuple format.
        let mut result: Vec<u32> = Vec::with_capacity(all_tokens.len() * 5);
        let mut prev_line: u32 = 0;
        let mut prev_col: u32 = 0;
        let mut cur_line: u32 = 0;
        let mut cur_col: u32 = 0;
        let mut src_pos = DocOffset::default();
        let one = DocLen::from_raw(1);

        for (host_offset, host_len, legend_idx) in all_tokens {
            while src_pos < host_offset && src_pos < source_end {
                if source_bytes[src_pos.as_usize()] == b'\n' {
                    cur_line += 1;
                    cur_col = 0;
                } else {
                    cur_col += 1;
                }
                src_pos += one;
            }
            let delta_line = cur_line - prev_line;
            let delta_start = if delta_line == 0 {
                cur_col - prev_col
            } else {
                cur_col
            };
            result.push(delta_line);
            result.push(delta_start);
            result.push(host_len.as_u32());
            result.push(legend_idx);
            result.push(0); // modifiers
            prev_line = cur_line;
            prev_col = cur_col;
        }

        result
    }

    /// Create a [`Analyzer`] with `macro_fallback` enabled so that
    /// [`HOLE_PLACEHOLDER`] calls are treated as single identifier tokens.
    fn make_analyzer(&self) -> Analyzer {
        Analyzer::with_dialect(self.dialect.clone()).with_macro_fallback(true)
    }

    /// Parse and validate a single fragment.
    ///
    /// Returns diagnostics with SQL-text byte offsets (not yet mapped to host).
    fn validate_fragment(&mut self, fragment: &EmbeddedFragment) -> Vec<Diagnostic> {
        let mut analyzer = self.make_analyzer();
        let mut ctx = AnalysisContext::new(&mut self.catalog).with_config(self.config);
        let model = analyzer.analyze(fragment.sql_text(), &mut ctx);
        model.diagnostics().cloned().collect()
    }
}

/// Pass that captures only tokens and comments, for
/// [`EmbeddedAnalyzer::fragment_semantic_tokens`].
#[derive(Default)]
struct TokenCapturePass {
    tokens: Vec<(DocOffset, DocLen, AnyTokenType, ParserTokenFlags)>,
    comments: Vec<(DocOffset, DocLen)>,
}

impl WalkPass for TokenCapturePass {
    const WANTS_STATEMENT_CONTEXT: bool = true;

    fn on_parsed_statement(&mut self, stmt: &syntaqlite_syntax::any::AnyParsedStatement<'_>) {
        let base = stmt.statement_base();
        for tok in stmt.tokens() {
            self.tokens.push((
                tok.offset().to_doc(base),
                tok.length().into(),
                tok.token_type(),
                tok.flags(),
            ));
        }
        for c in stmt.comments() {
            self.comments
                .push((c.offset().to_doc(base), c.length().into()));
        }
    }

    fn on_parse_error(&mut self, err: &syntaqlite_syntax::any::AnyParseError<'_>) {
        let base = err.statement_base();
        for tok in err.tokens() {
            self.tokens.push((
                tok.offset().to_doc(base),
                tok.length().into(),
                tok.token_type(),
                tok.flags(),
            ));
        }
        for c in err.comments() {
            self.comments
                .push((c.offset().to_doc(base), c.length().into()));
        }
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::*;
    use crate::analysis::diagnostics::{DiagnosticMessage, Severity};
    use crate::embedded::{python::extract_python, typescript::extract_typescript};
    use syntaqlite_syntax::source::DocText;

    fn analyzer() -> EmbeddedAnalyzer {
        EmbeddedAnalyzer::new(crate::sqlite::dialect::dialect())
    }

    // ── Python syntax error tests ────────────────────────────────────

    #[test]
    fn python_valid_sql_no_errors() {
        let source = r#"db.execute(f"SELECT id, name FROM users WHERE id = {uid}")"#;
        let diags = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(diags.is_empty(), "expected no parse errors, got: {diags:?}");
    }

    #[test]
    fn python_syntax_error_missing_expr_list() {
        let source = r#"db.execute(f"SELECT FROM t")"#;
        let diags = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(!diags.is_empty(), "expected parse error for 'SELECT FROM'");
        assert!(diags.iter().all(|d| d.severity == Severity::Error));
    }

    #[test]
    fn python_syntax_error_misspelled_from() {
        let source = r#"db.execute(f"SELECT * FORM t")"#;
        let diags = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(!diags.is_empty(), "expected parse error for 'FORM'");
    }

    #[test]
    fn python_syntax_error_double_where() {
        let source = r#"db.execute(f"SELECT id FROM t WHERE x = 1 WHERE y = 2")"#;
        let diags = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(!diags.is_empty(), "expected parse error for double WHERE");
    }

    #[test]
    fn python_syntax_error_offset_in_host() {
        let source = r#"prefix = 1; db.execute(f"SELECT FROM t")"#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
        let parse_diags: Vec<_> = analyzer()
            .analyze(&fragments)
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect();
        assert!(!parse_diags.is_empty(), "expected parse error");
        let fstring_start = source.find("SELECT").unwrap();
        assert!(
            parse_diags[0].start().as_usize() >= fstring_start,
            "expected offset >= {fstring_start}, got {}",
            parse_diags[0].start(),
        );
    }

    #[test]
    fn python_multiple_fragments_only_second_errors() {
        let source = concat!("a = f\"SELECT id FROM t\"\n", "b = f\"SELECT FROM t\"\n",);
        let diags = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(!diags.is_empty(), "expected parse error in second fragment");
        let second_select = source.rfind("SELECT").unwrap();
        for d in &diags {
            assert!(
                d.start().as_usize() >= second_select,
                "error at offset {} is before second fragment start {second_select}",
                d.start(),
            );
        }
    }

    #[test]
    fn python_valid_with_hole_no_errors() {
        let source = r#"db.execute(f"INSERT INTO t (a, b) VALUES ({x}, {y})")"#;
        let diags = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(diags.is_empty(), "expected no parse errors, got: {diags:?}");
    }

    // ── TypeScript syntax error tests ────────────────────────────────

    #[test]
    fn typescript_valid_sql_no_errors() {
        let source = "db.prepare(`SELECT id, name FROM users WHERE id = ${uid}`).all();";
        let diags = analyzer()
            .analyze(&extract_typescript(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(diags.is_empty(), "expected no parse errors, got: {diags:?}");
    }

    #[test]
    fn typescript_syntax_error_missing_expr_list() {
        let source = "db.prepare(`SELECT FROM users`).all();";
        let diags = analyzer()
            .analyze(&extract_typescript(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(!diags.is_empty(), "expected parse error for 'SELECT FROM'");
        assert!(diags.iter().all(|d| d.severity == Severity::Error));
    }

    #[test]
    fn typescript_syntax_error_double_where() {
        let source = "db.prepare(`SELECT id FROM t WHERE x = 1 WHERE y = 2`).all();";
        let diags = analyzer()
            .analyze(&extract_typescript(source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect::<Vec<_>>();
        assert!(!diags.is_empty(), "expected parse error for double WHERE");
    }

    // ── Semantic diagnostics are included but separable ──────────────

    #[test]
    fn semantic_diagnostics_present_for_unknown_table() {
        let source = r#"db.execute(f"SELECT id FROM unknown_tbl")"#;
        let all = analyzer().analyze(&extract_python(source));
        let parse: Vec<_> = all.iter().filter(|d| d.message.is_parse_error()).collect();
        let semantic: Vec<_> = all.iter().filter(|d| !d.message.is_parse_error()).collect();
        assert!(parse.is_empty(), "no parse errors expected");
        assert!(
            !semantic.is_empty(),
            "expected semantic diagnostic for unknown table"
        );
    }

    #[test]
    fn python_syntax_error_offset_points_to_typo() {
        let source = r#"conn.execute(f"INSERT INTO orders (a, b) VALUS ({x}, {y})")"#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
        let parse_diags: Vec<_> = analyzer()
            .analyze(&fragments)
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect();
        assert!(!parse_diags.is_empty(), "expected parse error for VALUS");
        let valus_start = source.find("VALUS").unwrap();
        let valus_end = valus_start + "VALUS".len();
        assert_eq!(
            parse_diags[0].start().as_usize(),
            valus_start,
            "error start should point to VALUS (offset {valus_start}), got {}",
            parse_diags[0].start(),
        );
        assert_eq!(
            parse_diags[0].end().as_usize(),
            valus_end,
            "error end should span VALUS (offset {valus_end}), got {}",
            parse_diags[0].end(),
        );
    }

    #[test]
    fn python_builtin_function_not_flagged() {
        let source = r#"db.execute(f"INSERT INTO t (a) VALUES (datetime('now'))")"#;
        let unknown_fn: Vec<_> = analyzer()
            .analyze(&extract_python(source))
            .into_iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownFunction { .. }))
            .collect();
        assert!(
            unknown_fn.is_empty(),
            "datetime should not be flagged as unknown, got: {unknown_fn:?}",
        );
    }

    #[test]
    fn semantic_tokens_classify_function_callee() {
        let source = r#"db.execute(f"INSERT INTO t (a) VALUES (datetime('now'))")"#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
        let tokens = EmbeddedAnalyzer::new(crate::sqlite::dialect::dialect())
            .fragment_semantic_tokens(&fragments[0]);
        let datetime_tokens: Vec<_> = tokens
            .iter()
            .filter(|(off, len, _)| {
                let sql = DocText::new(fragments[0].sql_text());
                &sql[DocRange::from_offset_len(*off, *len)] == "datetime"
            })
            .collect();
        assert_eq!(
            datetime_tokens.len(),
            1,
            "expected exactly one 'datetime' token, got: {datetime_tokens:?}",
        );
        assert_eq!(
            datetime_tokens[0].2,
            TokenCategory::Function,
            "datetime should be classified as Function, got {:?}",
            datetime_tokens[0].2,
        );
    }

    // ── Bug regression: Python f-string with unknown table (user's report) ─────

    /// Exact scenario from the bug report: embedded Python f-string where
    /// `users` is not in the catalog.  The table should get one `UnknownTable`
    /// diagnostic, and the columns (id, name, email, age, name in ORDER BY)
    /// should NOT produce any `UnknownColumn` diagnostics.
    #[test]
    fn python_unknown_table_no_spurious_column_errors() {
        let source = concat!(
            "import sqlite3\n",
            "\n",
            "def get_active_users(conn, min_age):\n",
            "    cursor = conn.execute(\n",
            "        f\"SELECT id, name, email FROM users",
            " WHERE age >= {min_age} AND active = 1 ORDER BY name\"\n",
            "    )\n",
            "    return cursor.fetchall()\n",
        );

        let fragments = extract_python(source);
        assert_eq!(
            fragments.len(),
            1,
            "should extract exactly one SQL fragment"
        );

        let all = analyzer().analyze(&fragments);

        // UnknownTable for "users" is expected and correct.
        let table_diags: Vec<_> = all
            .iter()
            .filter(|d| {
                matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "users")
            })
            .collect();
        assert_eq!(
            table_diags.len(),
            1,
            "expected exactly one UnknownTable for 'users': {all:#?}"
        );

        // Column refs against an unknown table must NOT be flagged.
        let col_diags: Vec<_> = all
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_diags.is_empty(),
            "unknown table should suppress column errors, got: {col_diags:#?}"
        );
    }

    #[test]
    fn hole_diagnostics_filtered_out() {
        let source = r#"db.execute(f"SELECT {col} FROM {tbl}")"#;
        let all = analyzer().analyze(&extract_python(source));
        for diag in &all {
            let msg = format!("{}", diag.message);
            assert!(
                !msg.contains("__h__"),
                "hole placeholder leaked into diagnostics: {msg}",
            );
        }
    }
}
