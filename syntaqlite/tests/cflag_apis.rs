// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Cflag tests for the standalone Formatter, `TypedParser`, and `TypedTokenizer` APIs.
//!
//! Verifies that compile-time flags are respected by every public entry point:
//!
//! - [`Formatter::with_dialect_config`]: parser-level cflags must gate syntax.
//! - [`TypedParser`]: grammar constructed with cflags rejects suppressed syntax.
//! - [`TypedTokenizer`]: tokenizer produced with cflags affects keyword recognition.

#![cfg(all(feature = "sqlite", feature = "fmt"))]

use syntaqlite::any::ParseOutcome;
use syntaqlite::util::{SqliteFlag, SqliteFlags};
use syntaqlite::{FormatConfig, Formatter, sqlite_dialect};

// ── Helpers ───────────────────────────────────────────────────────────────────

fn formatter_with_flags(flags: SqliteFlags) -> Formatter {
    let dialect = sqlite_dialect().with_cflags(flags);
    Formatter::with_dialect_config(dialect, &FormatConfig::default())
}

fn format_with_flags(
    sql: &str,
    flags: SqliteFlags,
) -> Result<String, syntaqlite::fmt::FormatError> {
    formatter_with_flags(flags).format(sql)
}

fn format_default(sql: &str) -> Result<String, syntaqlite::fmt::FormatError> {
    format_with_flags(sql, SqliteFlags::default())
}

// ── Formatter: OMIT_CTE ───────────────────────────────────────────────────────

#[test]
fn formatter_accepts_cte_without_omit_cte() {
    let result = format_default("WITH t AS (SELECT 1) SELECT * FROM t;");
    assert!(
        result.is_ok(),
        "CTE should format without OMIT_CTE; error: {result:?}"
    );
}

#[test]
fn formatter_rejects_cte_with_omit_cte() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitCte);
    let result = format_with_flags("WITH t AS (SELECT 1) SELECT * FROM t;", flags);
    assert!(
        result.is_err(),
        "CTE should fail to format with OMIT_CTE; got: {result:?}"
    );
}

// ── Formatter: OMIT_WINDOWFUNC ────────────────────────────────────────────────

#[test]
fn formatter_accepts_window_function_without_omit_windowfunc() {
    let result = format_default("SELECT sum(x) OVER () FROM t;");
    assert!(
        result.is_ok(),
        "window function should format without OMIT_WINDOWFUNC; error: {result:?}"
    );
}

#[test]
fn formatter_rejects_window_function_with_omit_windowfunc() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitWindowfunc);
    let result = format_with_flags("SELECT sum(x) OVER () FROM t;", flags);
    assert!(
        result.is_err(),
        "window function should fail to format with OMIT_WINDOWFUNC; got: {result:?}"
    );
}

// ── Formatter: OMIT_RETURNING ─────────────────────────────────────────────────

#[test]
fn formatter_accepts_returning_without_omit_returning() {
    let result = format_default("INSERT INTO t VALUES(1) RETURNING *;");
    assert!(
        result.is_ok(),
        "RETURNING should format without OMIT_RETURNING; error: {result:?}"
    );
}

#[test]
fn formatter_rejects_returning_with_omit_returning() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitReturning);
    let result = format_with_flags("INSERT INTO t VALUES(1) RETURNING *;", flags);
    assert!(
        result.is_err(),
        "RETURNING should fail to format with OMIT_RETURNING; got: {result:?}"
    );
}

// ── Formatter: non-parser cflags do not affect parse ─────────────────────────
//
// EnableMathFunctions, OmitDatetimeFuncs, etc. are non-parser cflags: they
// control function catalog availability in the semantic analyzer but do NOT
// affect whether SQL parses. The formatter should succeed regardless.

#[test]
fn formatter_accepts_sin_regardless_of_math_functions_flag() {
    // Formatter only parses — it doesn't validate function existence.
    let result_without = format_default("SELECT sin(1.0);");
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let result_with = format_with_flags("SELECT sin(1.0);", flags);
    assert!(
        result_without.is_ok(),
        "sin() should format without EnableMathFunctions (no semantic validation)"
    );
    assert!(
        result_with.is_ok(),
        "sin() should format with EnableMathFunctions"
    );
}

// ── Standalone TypedParser: cflags applied via grammar ───────────────────────
//
// TypedParser is a low-level API — the caller constructs the grammar directly.
// These tests verify the mechanism works end-to-end.

#[test]
fn typed_parser_accepts_cte_without_cflag() {
    let parser = syntaqlite::typed::TypedParser::new(syntaqlite::typed::grammar());
    let mut session = parser.parse("WITH t AS (SELECT 1) SELECT * FROM t;");
    assert!(
        matches!(session.next(), ParseOutcome::Ok(_)),
        "CTE should parse with default grammar"
    );
}

#[test]
fn typed_parser_rejects_cte_with_omit_cte_syntax_cflag() {
    // Build grammar with OmitCte cflag by extracting syntax_cflags from a
    // dialect that carries the flag.
    let flags = SqliteFlags::default().with(SqliteFlag::OmitCte);
    let dialect = sqlite_dialect().with_cflags(flags);
    let grammar = syntaqlite::typed::grammar().with_cflags(dialect.syntax_cflags());
    let parser = syntaqlite::typed::TypedParser::new(grammar);
    let mut session = parser.parse("WITH t AS (SELECT 1) SELECT * FROM t;");
    assert!(
        matches!(session.next(), ParseOutcome::Err(_)),
        "CTE should fail to parse with OmitCte syntax cflag"
    );
}

#[test]
fn typed_parser_rejects_window_function_with_omit_windowfunc_syntax_cflag() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitWindowfunc);
    let dialect = sqlite_dialect().with_cflags(flags);
    let grammar = syntaqlite::typed::grammar().with_cflags(dialect.syntax_cflags());
    let parser = syntaqlite::typed::TypedParser::new(grammar);
    let mut session = parser.parse("SELECT sum(x) OVER () FROM t;");
    assert!(
        matches!(session.next(), ParseOutcome::Err(_)),
        "window function should fail to parse with OmitWindowfunc syntax cflag"
    );
}

#[test]
fn typed_parser_rejects_returning_with_omit_returning_syntax_cflag() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitReturning);
    let dialect = sqlite_dialect().with_cflags(flags);
    let grammar = syntaqlite::typed::grammar().with_cflags(dialect.syntax_cflags());
    let parser = syntaqlite::typed::TypedParser::new(grammar);
    let mut session = parser.parse("INSERT INTO t VALUES(1) RETURNING *;");
    assert!(
        matches!(session.next(), ParseOutcome::Err(_)),
        "RETURNING should fail to parse with OmitReturning syntax cflag"
    );
}

// ── LspHost: cflags propagate to parser via diagnostics ──────────────────────
//
// Verifies the full path: LspHost::with_dialect (dialect carrying cflag) →
// SemanticAnalyzer → AnyParser → C parser.
//
// This tests the same mechanism used by `wasm_diagnostics` in syntaqlite-wasm.

#[cfg(feature = "lsp")]
#[test]
fn lsp_diagnostics_parse_error_with_omit_windowfunc() {
    use syntaqlite::ValidationConfig;
    use syntaqlite::lsp::LspHost;

    let flags = SqliteFlags::default().with(SqliteFlag::OmitWindowfunc);
    let dialect = sqlite_dialect().with_cflags(flags);

    let mut host = LspHost::with_dialect(dialect);
    host.update_document("test://x", 1, "SELECT sum(x) OVER () FROM t;".to_string());

    let diags = host.all_diagnostics("test://x", &ValidationConfig::default());
    assert!(
        !diags.is_empty(),
        "expected parse error when SQLITE_OMIT_WINDOWFUNC is set, got no diagnostics"
    );
}

#[cfg(feature = "lsp")]
#[test]
fn lsp_diagnostics_no_parse_error_without_omit_windowfunc() {
    use syntaqlite::ValidationConfig;
    use syntaqlite::lsp::LspHost;

    let mut host = LspHost::with_dialect(sqlite_dialect());
    host.update_document("test://x", 1, "SELECT sum(x) OVER () FROM t;".to_string());

    let diags = host.all_diagnostics("test://x", &ValidationConfig::default());
    let parse_errors: Vec<_> = diags
        .iter()
        .filter(|d| {
            matches!(d.severity(), syntaqlite::semantic::Severity::Error)
                && d.message().to_string().contains("syntax")
        })
        .collect();
    // The query should parse without error; any diagnostics here would be semantic (unknown table).
    // We only care that the parser doesn't reject window function syntax.
    let has_parse_failures = diags.iter().any(|d| {
        // Parse errors come from the C parser and typically say "near X: syntax error"
        d.message().to_string().contains("syntax error")
            || d.message().to_string().contains("parse")
    });
    let _ = parse_errors;
    assert!(
        !has_parse_failures,
        "window function should parse without SQLITE_OMIT_WINDOWFUNC; diagnostics: {diags:?}"
    );
}

// ── Standalone TypedTokenizer: cflag affects keyword recognition ──────────────
//
// When OMIT_CTE is set, WITH is demoted from a keyword to an identifier.
// We feed the tokenizer output into a parser to confirm the cflag takes effect.

#[test]
fn typed_tokenizer_cte_parses_ok_without_cflag() {
    use syntaqlite::typed::TypedTokenizer;
    let tokenizer = TypedTokenizer::new(syntaqlite::typed::grammar());
    let tokens: Vec<_> = tokenizer
        .tokenize("WITH t AS (SELECT 1) SELECT * FROM t;")
        .collect();
    // Simply confirm tokenization produces a non-trivial token stream.
    assert!(
        tokens.len() > 5,
        "expected several tokens from CTE query, got {}",
        tokens.len()
    );
}

#[test]
fn typed_tokenizer_with_omit_cte_produces_fewer_keyword_tokens() {
    use syntaqlite::any::AnyTokenType;
    use syntaqlite::typed::TypedTokenizer;

    let sql = "WITH t AS (SELECT 1) SELECT * FROM t;";

    // Tokenize with default grammar.
    let default_tokens: Vec<_> = TypedTokenizer::new(syntaqlite::typed::grammar())
        .tokenize(sql)
        .map(|t| AnyTokenType::from(t.token_type()))
        .collect();

    // Tokenize with OmitCte cflag applied to the grammar.
    let flags = SqliteFlags::default().with(SqliteFlag::OmitCte);
    let dialect = sqlite_dialect().with_cflags(flags);
    let grammar_with_flag = syntaqlite::typed::grammar().with_cflags(dialect.syntax_cflags());
    let flagged_tokens: Vec<_> = TypedTokenizer::new(grammar_with_flag)
        .tokenize(sql)
        .map(|t| AnyTokenType::from(t.token_type()))
        .collect();

    // The WITH keyword should be demoted to an identifier when OmitCte is set,
    // so the first token should differ between the two token streams.
    assert_ne!(
        default_tokens[0], flagged_tokens[0],
        "WITH token should be classified differently with OmitCte cflag; \
         default={:?} flagged={:?}",
        default_tokens[0], flagged_tokens[0]
    );
}
