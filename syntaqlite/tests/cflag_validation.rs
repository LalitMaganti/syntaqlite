// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! End-to-end cflag validation tests.
//!
//! Verifies that semantic analysis (function catalog, parse behavior) correctly
//! respects compile-time flags set on the dialect:
//!
//! - Non-parser flags (indices 22–41) gate function availability.
//! - Parser flags (indices 0–21) affect keyword recognition during analysis.

#![cfg(all(feature = "sqlite", feature = "validation"))]

use syntaqlite::semantic::DiagnosticMessage;
use syntaqlite::util::{SqliteFlag, SqliteFlags};
use syntaqlite::{Catalog, SemanticAnalyzer, ValidationConfig, sqlite_dialect};

// ── Helpers ───────────────────────────────────────────────────────────────────

fn analyze_with_flags(sql: &str, flags: SqliteFlags) -> Vec<syntaqlite::Diagnostic> {
    let dialect = sqlite_dialect().with_cflags(flags);
    let mut analyzer = SemanticAnalyzer::with_dialect(dialect.clone());
    let catalog = Catalog::new(dialect);
    let config = ValidationConfig::default();
    let model = analyzer.analyze(sql, &catalog, &config);
    model.diagnostics().cloned().collect()
}

fn analyze_default(sql: &str) -> Vec<syntaqlite::Diagnostic> {
    analyze_with_flags(sql, SqliteFlags::default())
}

fn has_unknown_function(diags: &[syntaqlite::Diagnostic], name: &str) -> bool {
    diags
        .iter()
        .any(|d| matches!(d.message(), DiagnosticMessage::UnknownFunction { name: n } if n == name))
}

fn has_parse_error(diags: &[syntaqlite::Diagnostic]) -> bool {
    diags.iter().any(|d| d.message().is_parse_error())
}

// ── SQLITE_ENABLE_MATH_FUNCTIONS (cflag index 36) ────────────────────────────
//
// Math functions (sin, cos, asin, acos, atan, atan2, sqrt, pow, log, exp,
// ceil, floor, ln, log10, log2, pi, degrees, radians, mod, sign, trunc)
// require SQLITE_ENABLE_MATH_FUNCTIONS. Without the flag they are absent
// from the function catalog.

#[test]
fn sin_flagged_without_math_functions() {
    let diags = analyze_default("SELECT sin(1.0);");
    assert!(
        has_unknown_function(&diags, "sin"),
        "sin should be unknown without ENABLE_MATH_FUNCTIONS; diagnostics: {diags:?}"
    );
}

#[test]
fn sin_not_flagged_with_math_functions() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let diags = analyze_with_flags("SELECT sin(1.0);", flags);
    assert!(
        !has_unknown_function(&diags, "sin"),
        "sin should be known with ENABLE_MATH_FUNCTIONS; diagnostics: {diags:?}"
    );
}

#[test]
fn cos_flagged_without_math_functions() {
    let diags = analyze_default("SELECT cos(1.0);");
    assert!(
        has_unknown_function(&diags, "cos"),
        "cos should be unknown without ENABLE_MATH_FUNCTIONS"
    );
}

#[test]
fn cos_not_flagged_with_math_functions() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let diags = analyze_with_flags("SELECT cos(1.0);", flags);
    assert!(
        !has_unknown_function(&diags, "cos"),
        "cos should be known with ENABLE_MATH_FUNCTIONS"
    );
}

#[test]
fn sqrt_flagged_without_math_functions() {
    let diags = analyze_default("SELECT sqrt(4.0);");
    assert!(has_unknown_function(&diags, "sqrt"));
}

#[test]
fn sqrt_not_flagged_with_math_functions() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let diags = analyze_with_flags("SELECT sqrt(4.0);", flags);
    assert!(!has_unknown_function(&diags, "sqrt"));
}

#[test]
fn pi_flagged_without_math_functions() {
    let diags = analyze_default("SELECT pi();");
    assert!(has_unknown_function(&diags, "pi"));
}

#[test]
fn pi_not_flagged_with_math_functions() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let diags = analyze_with_flags("SELECT pi();", flags);
    assert!(!has_unknown_function(&diags, "pi"));
}

// ── SQLITE_OMIT_DATETIME_FUNCS (cflag index 23) ──────────────────────────────
//
// Datetime functions (date, time, datetime, julianday, strftime, etc.) are
// omitted when SQLITE_OMIT_DATETIME_FUNCS is set.

#[test]
fn date_available_without_omit_datetime() {
    let diags = analyze_default("SELECT date('now');");
    assert!(
        !has_unknown_function(&diags, "date"),
        "date() should be available by default; diagnostics: {diags:?}"
    );
}

#[test]
fn date_flagged_with_omit_datetime() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitDatetimeFuncs);
    let diags = analyze_with_flags("SELECT date('now');", flags);
    assert!(
        has_unknown_function(&diags, "date"),
        "date() should be unknown with OMIT_DATETIME_FUNCS; diagnostics: {diags:?}"
    );
}

#[test]
fn strftime_available_without_omit_datetime() {
    let diags = analyze_default("SELECT strftime('%Y', 'now');");
    assert!(!has_unknown_function(&diags, "strftime"));
}

#[test]
fn strftime_flagged_with_omit_datetime() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitDatetimeFuncs);
    let diags = analyze_with_flags("SELECT strftime('%Y', 'now');", flags);
    assert!(has_unknown_function(&diags, "strftime"));
}

// ── SQLITE_SOUNDEX (cflag index 41) ──────────────────────────────────────────
//
// soundex() is enabled only when SQLITE_SOUNDEX is set.

#[test]
fn soundex_flagged_without_soundex_flag() {
    let diags = analyze_default("SELECT soundex('hello');");
    assert!(
        has_unknown_function(&diags, "soundex"),
        "soundex() should be unknown without SQLITE_SOUNDEX"
    );
}

#[test]
fn soundex_not_flagged_with_soundex_flag() {
    let flags = SqliteFlags::default().with(SqliteFlag::Soundex);
    let diags = analyze_with_flags("SELECT soundex('hello');", flags);
    assert!(
        !has_unknown_function(&diags, "soundex"),
        "soundex() should be known with SQLITE_SOUNDEX"
    );
}

// ── SQLITE_OMIT_JSON (cflag index 25) ────────────────────────────────────────

#[test]
fn json_available_without_omit_json() {
    let diags = analyze_default("SELECT json('{\"a\":1}');");
    assert!(
        !has_unknown_function(&diags, "json"),
        "json() should be available by default"
    );
}

#[test]
fn json_flagged_with_omit_json() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitJson);
    let diags = analyze_with_flags("SELECT json('{\"a\":1}');", flags);
    assert!(
        has_unknown_function(&diags, "json"),
        "json() should be unknown with OMIT_JSON; diagnostics: {diags:?}"
    );
}

// ── Parser-level cflag: OMIT_CTE gates WITH keyword ──────────────────────────
//
// When OMIT_CTE is set, the WITH keyword is suppressed at the tokenizer level
// and CTE syntax fails to parse. The semantic analyzer should reflect this.

#[test]
fn cte_parses_ok_without_omit_cte() {
    let diags = analyze_default("WITH t AS (SELECT 1) SELECT * FROM t;");
    assert!(
        !has_parse_error(&diags),
        "CTE should parse without OMIT_CTE; diagnostics: {diags:?}"
    );
}

#[test]
fn cte_fails_parse_with_omit_cte() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitCte);
    let diags = analyze_with_flags("WITH t AS (SELECT 1) SELECT * FROM t;", flags);
    assert!(
        has_parse_error(&diags),
        "CTE should fail to parse with OMIT_CTE; diagnostics: {diags:?}"
    );
}

// ── Parser-level cflag: OMIT_WINDOWFUNC gates OVER keyword ───────────────────

#[test]
fn window_function_parses_ok_without_omit_windowfunc() {
    let diags = analyze_default("SELECT sum(x) OVER () FROM t;");
    assert!(
        !has_parse_error(&diags),
        "Window function should parse without OMIT_WINDOWFUNC; diagnostics: {diags:?}"
    );
}

#[test]
fn window_function_fails_parse_with_omit_windowfunc() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitWindowfunc);
    let diags = analyze_with_flags("SELECT sum(x) OVER () FROM t;", flags);
    assert!(
        has_parse_error(&diags),
        "Window function should fail to parse with OMIT_WINDOWFUNC; diagnostics: {diags:?}"
    );
}

// ── Parser-level cflag: OMIT_RETURNING ───────────────────────────────────────

#[test]
fn returning_parses_ok_without_omit_returning() {
    let diags = analyze_default("INSERT INTO t VALUES(1) RETURNING *;");
    assert!(
        !has_parse_error(&diags),
        "RETURNING should parse without OMIT_RETURNING"
    );
}

#[test]
fn returning_fails_parse_with_omit_returning() {
    let flags = SqliteFlags::default().with(SqliteFlag::OmitReturning);
    let diags = analyze_with_flags("INSERT INTO t VALUES(1) RETURNING *;", flags);
    assert!(
        has_parse_error(&diags),
        "RETURNING should fail to parse with OMIT_RETURNING"
    );
}

// ── Cross-cutting: multiple cflags ───────────────────────────────────────────

#[test]
fn multiple_math_functions_enabled_together() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let diags = analyze_with_flags("SELECT sin(1.0) + cos(1.0) + sqrt(4.0);", flags);
    assert!(!has_unknown_function(&diags, "sin"));
    assert!(!has_unknown_function(&diags, "cos"));
    assert!(!has_unknown_function(&diags, "sqrt"));
}

#[test]
fn unrelated_omit_flag_does_not_suppress_other_functions() {
    // OMIT_DATETIME_FUNCS should not affect abs() or count().
    let flags = SqliteFlags::default().with(SqliteFlag::OmitDatetimeFuncs);
    let diags = analyze_with_flags("SELECT abs(-1), count(*);", flags);
    assert!(!has_unknown_function(&diags, "abs"));
    assert!(!has_unknown_function(&diags, "count"));
}

#[test]
fn math_and_datetime_flags_independent() {
    // EnableMathFunctions should not affect datetime functions.
    let flags = SqliteFlags::default().with(SqliteFlag::EnableMathFunctions);
    let diags = analyze_with_flags("SELECT sin(1.0), date('now');", flags);
    // sin is available (math flag set), date is also available (not omitted)
    assert!(!has_unknown_function(&diags, "sin"));
    assert!(!has_unknown_function(&diags, "date"));
}
