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
//! - [`extract_shell`](crate::embedded::extract_shell) — sqlite3 shell-language (dot-commands) extraction

pub(crate) mod offset_map;
mod python;
mod shell;
mod typescript;

#[doc(inline)]
pub use python::extract_python;
#[doc(inline)]
pub use shell::{extract_shell, is_shell_script};
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
use crate::analysis::engine::walker::SemanticVisitor;
use crate::dialect::AnyDialect;

use offset_map::OffsetMap;

// ── Host language seam ──────────────────────────────────────────────────

/// A host language whose source files embed SQL.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// Used by [`extract`] and [`detect`] to pick a language-specific extractor.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HostLanguage {
    /// sqlite3 CLI shell scripts (dot-commands, `GO` / `/` terminators).
    Shell,
    /// Python f-strings.
    Python,
    /// TypeScript / JavaScript template literals.
    Typescript,
}

/// Extract the embedded SQL fragments from `source` for a known host language.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// Dispatches to the language-specific extractor ([`extract_shell`],
/// [`extract_python`], [`extract_typescript`]).
pub fn extract(
    dialect: impl Into<AnyDialect>,
    lang: HostLanguage,
    source: &str,
) -> Vec<EmbeddedFragment> {
    match lang {
        HostLanguage::Shell => extract_shell(dialect, source),
        HostLanguage::Python => extract_python(source),
        HostLanguage::Typescript => extract_typescript(source),
    }
}

/// Auto-detect the host language of `source` from its content.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// `hint` is an optional out-of-band signal — a file extension or LSP
/// `languageId`. Today only sqlite3 shell scripts are content-detected (see
/// [`is_shell_script`]); the `hint`-based Python/TypeScript arms are a future
/// extension point, so those languages currently require an explicit
/// [`HostLanguage`].
pub fn detect(
    dialect: impl Into<AnyDialect>,
    source: &str,
    hint: Option<&str>,
) -> Option<HostLanguage> {
    let _ = hint;
    if is_shell_script(dialect, source) {
        return Some(HostLanguage::Shell);
    }
    None
}

/// Fragment `source` for an optional host language, returning a uniform list so
/// callers process standalone and embedded SQL through one path.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// With `Some(lang)` this is [`extract`]. With `None` the whole source is
/// returned as a single hole-free fragment, i.e. standalone SQL is just the
/// degenerate one-fragment case.
pub fn fragments(
    dialect: impl Into<AnyDialect>,
    source: &str,
    lang: Option<HostLanguage>,
) -> Vec<EmbeddedFragment> {
    match lang {
        Some(lang) => extract(dialect, lang, source),
        None => vec![EmbeddedFragment {
            sql_range: doc_range_from_usize(0, source.len()),
            sql_text: source.to_string(),
            holes: Vec::new(),
        }],
    }
}

/// Rebuild `source` with each fragment's SQL replaced by `format_sql`, leaving
/// the non-SQL host text between fragments untouched.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// This composes any in-place SQL rewrite (e.g. the formatter) over embedded
/// fragments without the `embedded` module depending on it. Fragments that
/// contain interpolation holes are left verbatim, since reconstructing them
/// around rewritten SQL is not yet supported.
///
/// # Errors
///
/// Propagates the first error returned by `format_sql`.
pub fn splice<E>(
    source: &str,
    fragments: &[EmbeddedFragment],
    mut format_sql: impl FnMut(&str) -> Result<String, E>,
) -> Result<String, E> {
    let mut out = String::with_capacity(source.len());
    let mut cursor = 0;
    for fragment in fragments {
        let start = fragment.sql_range().start.as_usize();
        let end = fragment.sql_range().end.as_usize();
        out.push_str(&source[cursor..start]);
        if fragment.holes().is_empty() {
            let formatted = format_sql(fragment.sql_text())?;
            // A whole-statement formatter appends a trailing newline; drop it
            // only when the host text already supplies the break after this
            // fragment (an interior fragment), so a trailing-newline-at-EOF
            // result is preserved for standalone SQL.
            if source[end..].starts_with('\n') {
                out.push_str(formatted.strip_suffix('\n').unwrap_or(&formatted));
            } else {
                out.push_str(&formatted);
            }
        } else {
            out.push_str(&source[start..end]);
        }
        cursor = end;
    }
    out.push_str(&source[cursor..]);
    Ok(out)
}

/// Mask `source` into a length-preserving, SQL-equivalent document: each
/// fragment's SQL is kept verbatim at its original offset, interpolation holes
/// become same-length underscores, and everything else is blanked to spaces
/// (newlines preserved so trailing line comments still terminate). A `;` is
/// inserted before any fragment that doesn't follow a terminated statement, so
/// the fragments parse as separate statements.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// Every byte keeps its position, so the result analyzes as one SQL document
/// whose offsets map 1:1 back to the host file — no per-fragment offset
/// bookkeeping, and cross-fragment navigation falls out for free. The underscore
/// hole placeholders are resolved unconditionally by the analyzer (a real schema
/// never names anything with only underscores), so interpolations never produce
/// spurious diagnostics.
pub fn mask(source: &str, fragments: &[EmbeddedFragment]) -> String {
    let bytes = source.as_bytes();
    // Blank everything to spaces but keep newlines, so line comments inside the
    // SQL still end where they did in the host file.
    let mut out: Vec<u8> = bytes
        .iter()
        .map(|&b| if b == b'\n' || b == b'\r' { b } else { b' ' })
        .collect();

    for fragment in fragments {
        let start = fragment.sql_range().start.as_usize();
        let end = fragment.sql_range().end.as_usize();
        out[start..end].copy_from_slice(&bytes[start..end]);
        for hole in fragment.holes() {
            let h = hole.host_range();
            out[h.start.as_usize()..h.end.as_usize()].fill(b'_');
        }
    }

    // Separate consecutive fragments that aren't already statement-terminated
    // (a `GO`-terminated shell run, or an f-string without a trailing `;`). The
    // `;` goes at the gap byte just before the next fragment — after any
    // preserved newline — so it can't land inside a trailing line comment.
    for pair in fragments.windows(2) {
        let prev = &pair[0];
        let next_start = pair[1].sql_range().start.as_usize();
        let prev_end = prev.sql_range().end.as_usize();
        let terminated = source[prev.sql_range().start.as_usize()..prev_end]
            .trim_end()
            .ends_with(';');
        if next_start > prev_end && !terminated {
            out[next_start - 1] = b';';
        }
    }

    // Only ASCII (space, `;`) is written over non-SQL bytes and SQL spans are
    // copied verbatim from valid UTF-8, so this is always valid UTF-8.
    String::from_utf8(out).unwrap_or_else(|_| source.to_string())
}

/// Produce a length-preserving SQL-equivalent document for analyzing `source`
/// as a single unit, or `None` when `source` is not an embedded host file.
///
/// **Experimental:** part of the experimental embedded SQL API.
///
/// Detects the host language (see [`detect`]), extracts its fragments, and
/// [`mask`]s them. The returned document's offsets match the host file exactly,
/// so the whole file can be analyzed in one pass with host-native offsets.
pub fn mask_for_analysis(
    dialect: impl Into<AnyDialect>,
    source: &str,
    hint: Option<&str>,
) -> Option<String> {
    let dialect = dialect.into();
    let lang = detect(dialect.clone(), source, hint)?;
    Some(mask(source, &extract(dialect, lang, source)))
}

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

    /// Whether parsing this fragment needs macro-fallback mode: its
    /// interpolation holes are [`HOLE_PLACEHOLDER`] macro-call tokens, so a
    /// fragment with holes must be parsed with fallback, while a hole-free one
    /// (shell, standalone SQL) must not be. Lets callers derive the parser
    /// configuration from the fragment instead of branching on host language.
    pub fn needs_macro_fallback(&self) -> bool {
        !self.holes.is_empty()
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
/// Holds the dialect, optional catalog context, and analysis config so they
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
    /// Create a new analyzer with an empty catalog and default analysis config.
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

    /// Override the default analysis config.
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
            let diags = self.analyze_fragment(fragment);
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
        let mut analyzer = self.make_analyzer(fragment);
        let mut capture = TokenCapture::default();
        let mut ctx = AnalysisContext::new(&mut self.catalog).with_config(self.config);
        let _ = analyzer.analyze_with_visitor(fragment.sql_text(), &mut ctx, &mut capture);
        let dialect = analyzer.dialect();
        let mut out: Vec<(DocOffset, DocLen, TokenCategory)> = Vec::new();
        for (offset, length, tt, flags) in capture.tokens {
            let cat = dialect.classify_token(tt, flags);
            if cat != TokenCategory::Other {
                out.push((offset, length, cat));
            }
        }
        for (offset, length) in capture.comments {
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

    /// Create an [`Analyzer`] for `fragment`, enabling `macro_fallback` only
    /// when the fragment has interpolation holes so that [`HOLE_PLACEHOLDER`]
    /// calls are treated as single identifier tokens. Hole-free fragments
    /// (shell, standalone SQL) parse exactly as plain SQL.
    fn make_analyzer(&self, fragment: &EmbeddedFragment) -> Analyzer {
        Analyzer::with_dialect(self.dialect.clone())
            .with_macro_fallback(fragment.needs_macro_fallback())
    }

    /// Parse and validate a single fragment.
    ///
    /// Returns diagnostics with SQL-text byte offsets (not yet mapped to host).
    fn analyze_fragment(&mut self, fragment: &EmbeddedFragment) -> Vec<Diagnostic> {
        let mut analyzer = self.make_analyzer(fragment);
        let mut ctx = AnalysisContext::new(&mut self.catalog).with_config(self.config);
        let model = analyzer.analyze(fragment.sql_text(), &mut ctx);
        model.diagnostics().cloned().collect()
    }
}

/// Pass that captures only tokens and comments, for
/// [`EmbeddedAnalyzer::fragment_semantic_tokens`].
#[derive(Default)]
struct TokenCapture {
    tokens: Vec<(DocOffset, DocLen, AnyTokenType, ParserTokenFlags)>,
    comments: Vec<(DocOffset, DocLen)>,
}

impl SemanticVisitor for TokenCapture {
    const WANTS_STATEMENT_CONTEXT: bool = true;

    fn on_parsed_statement(&mut self, stmt: &syntaqlite_syntax::any::AnyParsedStatement<'_>) {
        let base = stmt.statement_base();
        for tok in stmt.tokens() {
            let range = tok.stmt_range();
            self.tokens.push((
                range.start.to_doc(base),
                range.len().into(),
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
            let range = tok.stmt_range();
            self.tokens.push((
                range.start.to_doc(base),
                range.len().into(),
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

    // ── Shell language (sqlite3 CLI dot-commands) ────────────────────────
    //
    // Extraction and detection are unit-tested in `shell.rs`; this covers the
    // end-to-end path through the analyzer.

    /// A `.read` + `SELECT 1;` file run through the shell extractor produces no
    /// parse errors, while the same text analyzed as plain SQL does.
    #[test]
    fn shell_read_file_has_no_parse_errors() {
        let source = ".read foo.sql\nSELECT 1;";
        let dialect = crate::sqlite::dialect::dialect();

        let parse_errors: Vec<_> = analyzer()
            .analyze(&extract_shell(dialect.clone(), source))
            .into_iter()
            .filter(|d| d.message.is_parse_error())
            .collect();
        assert!(
            parse_errors.is_empty(),
            "shell .read line must not surface as a SQL parse error, got: {parse_errors:?}"
        );

        // The same source analyzed as plain SQL is a parse error.
        let mut plain = Analyzer::with_dialect(dialect.clone());
        let mut catalog = Catalog::new(dialect);
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = plain.analyze(source, &mut ctx);
        assert!(
            model.diagnostics().any(|d| d.message.is_parse_error()),
            "plain SQL analysis of a .read file should error",
        );
    }

    // ── Unified fragmentation + splicing ─────────────────────────────────

    #[test]
    fn fragments_none_is_whole_document() {
        let dialect = crate::sqlite::dialect::dialect();
        let src = "SELECT 1;\nSELECT 2;";
        let frags = fragments(dialect, src, None);
        assert_eq!(frags.len(), 1);
        assert_eq!(frags[0].sql_text(), src);
        assert_eq!(frags[0].sql_range().start.as_usize(), 0);
        assert!(frags[0].holes().is_empty());
        assert!(!frags[0].needs_macro_fallback());
    }

    #[test]
    fn fragments_shell_matches_extract() {
        let dialect = crate::sqlite::dialect::dialect();
        let src = ".read a.sql\nSELECT 1;";
        let via_fragments = fragments(dialect.clone(), src, Some(HostLanguage::Shell));
        let via_extract = extract(dialect, HostLanguage::Shell, src);
        assert_eq!(via_fragments.len(), via_extract.len());
        assert_eq!(via_fragments[0].sql_text(), via_extract[0].sql_text());
    }

    /// Splicing rewrites only the SQL, preserves the non-SQL host lines, and
    /// drops the rewriter's trailing newline at an interior fragment so the host
    /// line break is not duplicated.
    #[test]
    fn splice_rewrites_sql_and_preserves_shell_lines() {
        let dialect = crate::sqlite::dialect::dialect();
        let src = ".read a.sql\nselect 1\n";
        let frags = fragments(dialect, src, Some(HostLanguage::Shell));
        // Mimic a whole-statement formatter that appends a trailing newline.
        let out = splice(src, &frags, |sql| {
            Ok::<_, ()>(format!("{}\n", sql.to_uppercase()))
        })
        .unwrap();
        assert_eq!(out, ".read a.sql\nSELECT 1\n");
    }

    /// For standalone SQL (one whole-document fragment) the rewriter's trailing
    /// newline is kept, since no host text follows.
    #[test]
    fn splice_whole_document_keeps_trailing_newline() {
        let dialect = crate::sqlite::dialect::dialect();
        let src = "select 1\n";
        let frags = fragments(dialect, src, None);
        let out = splice(src, &frags, |sql| {
            Ok::<_, ()>(format!("{}\n", sql.trim().to_uppercase()))
        })
        .unwrap();
        assert_eq!(out, "SELECT 1\n");
    }

    // ── Masking + underscore placeholders ────────────────────────────────

    /// Masking a shell script blanks its non-SQL lines, turns `GO` into a `;`
    /// separator, keeps the SQL verbatim at the same offsets, and yields a doc
    /// that parses cleanly as one unit.
    #[test]
    fn mask_shell_blanks_non_sql_and_separates_runs() {
        let dialect = crate::sqlite::dialect::dialect();
        let src = ".read a.sql\nSELECT 1\nGO\nSELECT 2;";
        let frags = extract(dialect.clone(), HostLanguage::Shell, src);
        let masked = mask(src, &frags);

        assert_eq!(masked.len(), src.len(), "masking must preserve offsets");
        assert!(!masked.contains(".read"), "dot-command blanked");
        assert!(masked.contains("SELECT 1"));
        assert!(masked.contains("SELECT 2;"));

        let mut analyzer = Analyzer::with_dialect(dialect.clone());
        let mut catalog = Catalog::new(dialect);
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze(&masked, &mut ctx);
        assert!(
            !model.diagnostics().any(|d| d.message.is_parse_error()),
            "masked shell script must parse cleanly",
        );
    }

    /// Masking a Python f-string keeps the SQL and replaces each `{hole}` with
    /// same-length underscores, so byte offsets are preserved.
    #[test]
    fn mask_python_holes_become_underscores() {
        let src = r#"db.execute(f"SELECT {col} FROM users WHERE id = {uid}")"#;
        let masked = mask(src, &extract_python(src));
        assert_eq!(masked.len(), src.len());
        assert!(!masked.contains("db.execute"), "host code blanked");
        assert!(masked.contains("SELECT _____ FROM users WHERE id = _____"));
    }

    /// The underscore hole placeholders resolve unconditionally: a masked
    /// document referencing only placeholders produces no diagnostics (no
    /// spurious unknown table/column/function).
    #[test]
    fn underscore_placeholders_resolve_without_diagnostics() {
        let dialect = crate::sqlite::dialect::dialect();
        let sql = "SELECT ____(_____) FROM _____ WHERE ___ = 1;";
        let mut analyzer = Analyzer::with_dialect(dialect.clone());
        let mut catalog = Catalog::new(dialect);
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze(sql, &mut ctx);
        let diags: Vec<_> = model.diagnostics().collect();
        assert!(
            diags.is_empty(),
            "underscore placeholders must not be flagged, got: {diags:?}",
        );
    }
}
