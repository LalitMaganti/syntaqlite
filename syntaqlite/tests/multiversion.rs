// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Oracle tests for version-gated tokenization and parsing.
//!
//! These tests verify that syntaqlite's tokenizer produces the same behavior as
//! real `SQLite` at version boundaries. The expected behavior was verified against
//! actual sqlite3 shells compiled from official amalgamations:
//!
//! - 3.37.2: `SELECT 1->2` → error near ">" (no -> operator)
//! - 3.38.0: `SELECT 1->2` → success (-> operator added)
//! - 3.45.3: `SELECT 1_000` → error "unrecognized token: `1_000`"
//! - 3.46.0: `SELECT 1_000` → success (digit separators added)
//! - 3.34.1: `INSERT ... RETURNING` → error near "RETURNING"
//! - 3.35.0: same query → success
//! - 3.34.1: `WITH t AS MATERIALIZED (...)` → error near "MATERIALIZED"
//! - 3.35.0: same query → success
//! - 3.24.0: `SELECT sum(1) OVER ()` → error near "("
//! - 3.25.0: same query → success
//! - 3.23.1: `ON CONFLICT(x) DO NOTHING` → error near "ON"
//! - 3.24.0: same query → success
//! - 3.24.0: `FILTER (WHERE ...)` → error near "("
//! - 3.25.0: same query (with OVER) → success

use syntaqlite::ParseOutcome;
use syntaqlite::parse::TokenType;
use syntaqlite::typed::{TypedParser, TypedTokenizer, grammar};
use syntaqlite::util::SqliteVersion;

/// Shorthand: convert a `TokenType` variant to its raw u32 value.
const fn tk(t: TokenType) -> u32 {
    t as u32
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Tokenize SQL with a specific `SQLite` version and return (`token_type`, text) pairs,
/// filtering out whitespace.
fn tokenize_at_version(sql: &str, version: SqliteVersion) -> Vec<(u32, String)> {
    let tok = TypedTokenizer::new(grammar().with_version(version));
    tok.tokenize(sql)
        .filter(|t| t.token_type() != TokenType::Space)
        .map(|t| (t.token_type() as u32, t.text().to_string()))
        .collect()
}

/// Tokenize SQL with latest version (default config).
fn tokenize_latest(sql: &str) -> Vec<(u32, String)> {
    tokenize_at_version(sql, SqliteVersion::Latest)
}

/// Parse SQL with a specific `SQLite` version and return whether it succeeded.
fn parses_ok_at_version(sql: &str, version: SqliteVersion) -> bool {
    let parser = TypedParser::new(grammar().with_version(version));
    let mut session = parser.parse(sql);
    matches!(session.next(), ParseOutcome::Ok(_))
}

/// Helper to encode a `SQLite` version like 3.38.0 as a `SqliteVersion` enum.
fn ver(major: u32, minor: u32, _patch: u32) -> SqliteVersion {
    SqliteVersion::from_int(major.cast_signed() * 1_000_000 + minor.cast_signed() * 1_000)
}

// ---------------------------------------------------------------------------
// TK_PTR reclassification (-> and ->> operators, added in 3.38.0)
//
// Verified against real sqlite3 shells:
//   3.37.2: `SELECT '{"a":1}' -> '$.a';` → Error: near ">": syntax error
//   3.38.0: `SELECT '{"a":1}' -> '$.a';` → 1
// ---------------------------------------------------------------------------

#[test]
fn ptr_operator_tokenizes_as_ptr_on_latest() {
    let tokens = tokenize_latest("1->2");
    let types: Vec<_> = tokens.iter().map(|(tt, _)| *tt).collect();
    assert_eq!(
        types,
        vec![
            tk(TokenType::Integer),
            tk(TokenType::Ptr),
            tk(TokenType::Integer)
        ]
    );
}

#[test]
fn ptr_operator_reclassified_to_minus_before_3_38() {
    // Before 3.38, -> should tokenize as TK_MINUS (length 1), then '>' is a
    // separate GT token, then '2' is an integer.
    let tokens = tokenize_at_version("1->2", ver(3, 37, 0));
    let types: Vec<_> = tokens.iter().map(|(tt, _)| *tt).collect();
    assert_eq!(
        types,
        vec![
            tk(TokenType::Integer),
            tk(TokenType::Minus),
            tk(TokenType::Gt),
            tk(TokenType::Integer)
        ],
        "Before 3.38, '->' should split into MINUS + GT"
    );
    // Verify the minus token is just '-' (length 1).
    assert_eq!(tokens[1].1, "-");
}

#[test]
fn ptr_operator_works_at_3_38() {
    let tokens = tokenize_at_version("1->2", ver(3, 38, 0));
    let types: Vec<_> = tokens.iter().map(|(tt, _)| *tt).collect();
    assert_eq!(
        types,
        vec![
            tk(TokenType::Integer),
            tk(TokenType::Ptr),
            tk(TokenType::Integer)
        ]
    );
}

#[test]
fn double_ptr_reclassified_before_3_38() {
    // ->> should also split into MINUS + >> before 3.38.
    let tokens = tokenize_at_version("1->>2", ver(3, 37, 0));
    let types: Vec<_> = tokens.iter().map(|(tt, _)| *tt).collect();
    assert_eq!(
        types,
        vec![
            tk(TokenType::Integer),
            tk(TokenType::Minus),
            tk(TokenType::Rshift),
            tk(TokenType::Integer)
        ],
        "Before 3.38, '->>' should split into MINUS + RSHIFT"
    );
    assert_eq!(tokens[1].1, "-");
}

#[test]
fn ptr_reclassification_parse_fails_before_3_38() {
    // Verified: real SQLite 3.37.2 returns "near '>': syntax error"
    assert!(
        !parses_ok_at_version("SELECT 1->2;", ver(3, 37, 0)),
        "SELECT 1->2 should fail to parse before 3.38"
    );
}

#[test]
fn ptr_reclassification_parse_succeeds_at_3_38() {
    assert!(
        parses_ok_at_version("SELECT 1->2;", ver(3, 38, 0)),
        "SELECT 1->2 should parse at 3.38+"
    );
}

// ---------------------------------------------------------------------------
// TK_QNUMBER reclassification (digit separators, added in 3.46.0)
//
// Verified against real sqlite3 shells:
//   3.45.3: `SELECT 1_000;` → "unrecognized token: 1_000"
//   3.46.0: `SELECT 1_000;` → 1000
// ---------------------------------------------------------------------------

#[test]
fn digit_separator_tokenizes_as_qnumber_on_latest() {
    let tokens = tokenize_latest("1_000");
    let types: Vec<_> = tokens.iter().map(|(tt, _)| *tt).collect();
    assert_eq!(types, vec![tk(TokenType::Qnumber)]);
    assert_eq!(tokens[0].1, "1_000");
}

#[test]
fn digit_separator_reclassified_to_integer_before_3_46() {
    // Before 3.46, 1_000 should truncate to just "1" (INTEGER).
    let tokens = tokenize_at_version("1_000", ver(3, 45, 0));
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Integer),
        "Should be INTEGER, not QNUMBER"
    );
    assert_eq!(
        tokens[0].1, "1",
        "Should truncate to '1' before the underscore"
    );
}

#[test]
fn digit_separator_float_reclassified_before_3_46() {
    // 1.5_0 should become FLOAT "1.5" before 3.46.
    let tokens = tokenize_at_version("1.5_0", ver(3, 45, 0));
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Float),
        "Should be FLOAT, not QNUMBER"
    );
    assert_eq!(
        tokens[0].1, "1.5",
        "Should truncate to '1.5' before the underscore"
    );
}

#[test]
fn digit_separator_works_at_3_46() {
    let tokens = tokenize_at_version("1_000", ver(3, 46, 0));
    assert_eq!(tokens[0].0, tk(TokenType::Qnumber));
    assert_eq!(tokens[0].1, "1_000");
}

// ---------------------------------------------------------------------------
// Baseline: version-independent tokens should be unaffected
// ---------------------------------------------------------------------------

#[test]
fn basic_tokens_unaffected_by_version() {
    // These should tokenize identically regardless of version.
    for version in [
        ver(3, 12, 0),
        ver(3, 37, 0),
        ver(3, 46, 0),
        SqliteVersion::Latest,
    ] {
        let tokens = tokenize_at_version("SELECT 1 + 2", version);
        let types: Vec<_> = tokens.iter().map(|(tt, _)| *tt).collect();
        assert_eq!(
            types,
            vec![
                tk(TokenType::Select),
                tk(TokenType::Integer),
                tk(TokenType::Plus),
                tk(TokenType::Integer)
            ],
            "Basic tokens should be stable at version {version:?}"
        );
    }
}

// ---------------------------------------------------------------------------
// Keyword version gating
//
// Verified against real sqlite3 shells compiled from official amalgamations:
//   3.34.1: `INSERT INTO t VALUES(1) RETURNING *;` → error near "RETURNING"
//   3.35.0: same query → success
//   3.34.1: `WITH t AS MATERIALIZED (SELECT 1) ...` → error near "MATERIALIZED"
//   3.35.0: same query → success
//   3.24.0: `SELECT sum(1) OVER ();` → error near "("
//   3.25.0: same query → success
//   3.23.1: `ON CONFLICT(x) DO NOTHING` → error near "ON"
//   3.24.0: same query → success
//   3.24.0: `FILTER (WHERE ...)` → error near "("
//   3.25.0: same query (with OVER) → success
//
// The tokenizer returns the keyword's fallback token (typically the same
// value the parser's %fallback would have produced) for versions older
// than the keyword's introduction. This prevents the parser from
// recognizing syntax that didn't exist at that version.
// ---------------------------------------------------------------------------

#[test]
fn returning_keyword_not_recognized_before_3_35() {
    // RETURNING was added as a keyword in 3.35.0.
    // Before that, it should NOT tokenize as TK_RETURNING.
    let tokens = tokenize_at_version("RETURNING", ver(3, 34, 0));
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Returning),
        "RETURNING should not be a keyword before 3.35"
    );
}

#[test]
fn returning_keyword_recognized_at_3_35() {
    let tokens = tokenize_at_version("RETURNING", ver(3, 35, 0));
    assert_eq!(tokens[0].0, tk(TokenType::Returning));
}

#[test]
fn materialized_keyword_not_recognized_before_3_35() {
    let tokens = tokenize_at_version("MATERIALIZED", ver(3, 34, 0));
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Materialized),
        "MATERIALIZED should not be a keyword before 3.35"
    );
}

#[test]
fn window_keyword_not_recognized_before_3_25() {
    // WINDOW was added in 3.25.0.
    let tokens = tokenize_at_version("WINDOW", ver(3, 24, 0));
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Window),
        "WINDOW should not be a keyword before 3.25"
    );
}

#[test]
fn over_keyword_not_recognized_before_3_25() {
    let tokens = tokenize_at_version("OVER", ver(3, 24, 0));
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Over),
        "OVER should not be a keyword before 3.25"
    );
}

#[test]
fn do_keyword_not_recognized_before_3_24() {
    // DO was added in 3.24.0 (upsert: ON CONFLICT DO).
    let tokens = tokenize_at_version("DO", ver(3, 23, 0));
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Do),
        "DO should not be a keyword before 3.24"
    );
}

#[test]
fn filter_keyword_not_recognized_before_3_25() {
    // FILTER was added in 3.25.0 (with window functions).
    let tokens = tokenize_at_version("FILTER", ver(3, 24, 0));
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Filter),
        "FILTER should not be a keyword before 3.25"
    );
}
