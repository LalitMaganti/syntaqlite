// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Oracle tests for cflag-gated keyword suppression and parsing.
//!
//! These tests verify that compile-time flags (cflags) correctly gate keywords
//! at the tokenizer level:
//!
//! - **OMIT-polarity** (polarity=0): keyword is suppressed (falls back to ID)
//!   when the OMIT flag IS set.
//! - **ENABLE-polarity** (polarity=1): keyword is only recognized when the
//!   ENABLE flag IS set; without it, falls back to ID.
//!
//! Each OMIT group tests:
//!   1. Keyword falls back to ID when the OMIT flag is set.
//!   2. Keyword is recognized normally without the flag.
//!   3. A representative SQL statement fails to parse with the flag.
//!   4. The same statement parses successfully without the flag.
//!
//! ENABLE groups additionally test version interaction (the keyword must
//! also meet its version requirement).
//!
//! The `saw_subquery` tests verify the `OMIT_SUBQUERY` detection mechanism,
//! which uses a parser flag rather than keyword suppression.

use syntaqlite::ParseOutcome;
use syntaqlite::parse::TokenType;
use syntaqlite::typed::{TypedParser, TypedTokenizer, grammar};
use syntaqlite::util::{SqliteSyntaxFlag, SqliteSyntaxFlags, SqliteVersion};

const fn tk(t: TokenType) -> u32 {
    t as u32
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Tokenize SQL with cflags set (latest version).
fn tokenize_with_cflags(sql: &str, flags: &[SqliteSyntaxFlag]) -> Vec<(u32, String)> {
    let mut syntax_flags = SqliteSyntaxFlags::default();
    for &f in flags {
        syntax_flags = syntax_flags.with(f);
    }
    let tok = TypedTokenizer::new(grammar().with_cflags(syntax_flags));
    tok.tokenize(sql)
        .filter(|t| t.token_type() != TokenType::Space)
        .map(|t| (t.token_type() as u32, t.text().to_string()))
        .collect()
}

/// Tokenize SQL with default config (no cflags, latest version).
fn tokenize_default(sql: &str) -> Vec<(u32, String)> {
    tokenize_with_cflags(sql, &[])
}

/// Tokenize SQL with a specific version and cflags set.
fn tokenize_at_version_cflags(
    sql: &str,
    version: SqliteVersion,
    flags: &[SqliteSyntaxFlag],
) -> Vec<(u32, String)> {
    let mut syntax_flags = SqliteSyntaxFlags::default();
    for &f in flags {
        syntax_flags = syntax_flags.with(f);
    }
    let tok = TypedTokenizer::new(grammar().with_version(version).with_cflags(syntax_flags));
    tok.tokenize(sql)
        .filter(|t| t.token_type() != TokenType::Space)
        .map(|t| (t.token_type() as u32, t.text().to_string()))
        .collect()
}

/// Parse SQL with cflags set (latest version) and return whether it succeeded.
fn parses_ok_with_cflags(sql: &str, flags: &[SqliteSyntaxFlag]) -> bool {
    let mut syntax_flags = SqliteSyntaxFlags::default();
    for &f in flags {
        syntax_flags = syntax_flags.with(f);
    }
    let parser = TypedParser::new(grammar().with_cflags(syntax_flags));
    let mut session = parser.parse(sql);
    matches!(session.next(), ParseOutcome::Ok(_))
}

/// Parse SQL with default config (no cflags, latest version).
fn parses_ok_default(sql: &str) -> bool {
    parses_ok_with_cflags(sql, &[])
}

/// Helper to encode a `SQLite` version like 3.47.0 as a `SqliteVersion` enum.
fn ver(major: u32, minor: u32, _patch: u32) -> SqliteVersion {
    SqliteVersion::from_int(major.cast_signed() * 1_000_000 + minor.cast_signed() * 1_000)
}

// ===========================================================================
// ENABLE-polarity cflag tests
// ===========================================================================

// ---------------------------------------------------------------------------
// ENABLE_ORDERED_SET_AGGREGATES — gates: WITHIN
//
// WITHIN is an ENABLE-polarity keyword (polarity=1): it is only recognized
// when the SQLITE_ENABLE_ORDERED_SET_AGGREGATES flag IS set. Without the
// flag, WITHIN falls back to ID.
//
// Verified against SQLite 3.47.0+ compiled with/without the flag.
// ---------------------------------------------------------------------------

#[test]
fn within_keyword_not_recognized_without_cflag() {
    // Without the ENABLE flag, WITHIN should NOT be recognized as TK_WITHIN.
    let tokens = tokenize_with_cflags("WITHIN", &[]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Within),
        "WITHIN should not be a keyword without ENABLE_ORDERED_SET_AGGREGATES"
    );
}

#[test]
fn within_keyword_recognized_with_cflag() {
    // With the ENABLE flag set, WITHIN should be recognized.
    let tokens = tokenize_with_cflags("WITHIN", &[SqliteSyntaxFlag::EnableOrderedSetAggregates]);
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Within),
        "WITHIN should be a keyword when ENABLE_ORDERED_SET_AGGREGATES is set"
    );
}

#[test]
fn within_keyword_not_recognized_before_3_47() {
    // Even with the cflag, WITHIN was not available before 3.47.
    let tokens = tokenize_at_version_cflags(
        "WITHIN",
        ver(3, 46, 0),
        &[SqliteSyntaxFlag::EnableOrderedSetAggregates],
    );
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Within),
        "WITHIN should not be a keyword before 3.47 even with cflag"
    );
}

#[test]
fn within_group_parses_with_cflag() {
    assert!(
        parses_ok_with_cflags(
            "SELECT percentile(0.5) WITHIN GROUP (ORDER BY salary) FROM t;",
            &[SqliteSyntaxFlag::EnableOrderedSetAggregates],
        ),
        "WITHIN GROUP syntax should parse when cflag is set"
    );
}

#[test]
fn within_group_fails_without_cflag() {
    // Without the cflag, WITHIN falls back to ID and the syntax fails.
    assert!(
        !parses_ok_default("SELECT percentile(0.5) WITHIN GROUP (ORDER BY salary) FROM t;",),
        "WITHIN GROUP syntax should fail without cflag"
    );
}

// ===========================================================================
// OMIT-polarity cflag tests
//
// OMIT cflags suppress keywords at the tokenizer level, causing the keyword
// to fall back to ID. This prevents the parser from recognizing the gated
// syntax.
// ===========================================================================

// ---------------------------------------------------------------------------
// OMIT_WINDOWFUNC — gates: OVER, FILTER, WINDOW, CURRENT, EXCLUDE,
//                          FOLLOWING, GROUPS, OTHERS, PARTITION,
//                          PRECEDING, RANGE, TIES, UNBOUNDED
// ---------------------------------------------------------------------------

#[test]
fn omit_windowfunc_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("OVER", &[SqliteSyntaxFlag::OmitWindowfunc]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Over),
        "OVER should fall back to ID with OMIT_WINDOWFUNC"
    );
}

#[test]
fn omit_windowfunc_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "SELECT sum(x) OVER () FROM t;",
            &[SqliteSyntaxFlag::OmitWindowfunc],
        ),
        "Window function syntax should fail with OMIT_WINDOWFUNC"
    );
}

#[test]
fn omit_windowfunc_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("SELECT sum(x) OVER () FROM t;"),
        "Window function syntax should parse without OMIT_WINDOWFUNC"
    );
}

// ---------------------------------------------------------------------------
// OMIT_CTE — gates: WITH, MATERIALIZED, RECURSIVE
// ---------------------------------------------------------------------------

#[test]
fn omit_cte_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("WITH", &[SqliteSyntaxFlag::OmitCte]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::With),
        "WITH should fall back to ID with OMIT_CTE"
    );
}

#[test]
fn omit_cte_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "WITH t AS (SELECT 1) SELECT * FROM t;",
            &[SqliteSyntaxFlag::OmitCte],
        ),
        "CTE syntax should fail with OMIT_CTE"
    );
}

#[test]
fn omit_cte_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("WITH t AS (SELECT 1) SELECT * FROM t;"),
        "CTE syntax should parse without OMIT_CTE"
    );
}

// ---------------------------------------------------------------------------
// OMIT_RETURNING — gates: RETURNING
// ---------------------------------------------------------------------------

#[test]
fn omit_returning_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("RETURNING", &[SqliteSyntaxFlag::OmitReturning]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Returning),
        "RETURNING should fall back to ID with OMIT_RETURNING"
    );
}

#[test]
fn omit_returning_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "INSERT INTO t VALUES(1) RETURNING *;",
            &[SqliteSyntaxFlag::OmitReturning],
        ),
        "RETURNING syntax should fail with OMIT_RETURNING"
    );
}

#[test]
fn omit_returning_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("INSERT INTO t VALUES(1) RETURNING *;"),
        "RETURNING syntax should parse without OMIT_RETURNING"
    );
}

// ---------------------------------------------------------------------------
// OMIT_COMPOUND_SELECT — gates: UNION, EXCEPT, INTERSECT
// ---------------------------------------------------------------------------

#[test]
fn omit_compound_select_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("UNION", &[SqliteSyntaxFlag::OmitCompoundSelect]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Union),
        "UNION should fall back to ID with OMIT_COMPOUND_SELECT"
    );
}

#[test]
fn omit_compound_select_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "SELECT 1 UNION SELECT 2;",
            &[SqliteSyntaxFlag::OmitCompoundSelect],
        ),
        "UNION syntax should fail with OMIT_COMPOUND_SELECT"
    );
}

#[test]
fn omit_compound_select_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("SELECT 1 UNION SELECT 2;"),
        "UNION syntax should parse without OMIT_COMPOUND_SELECT"
    );
}

// ---------------------------------------------------------------------------
// OMIT_VIEW — gates: VIEW
// ---------------------------------------------------------------------------

#[test]
fn omit_view_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("VIEW", &[SqliteSyntaxFlag::OmitView]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::View),
        "VIEW should fall back to ID with OMIT_VIEW"
    );
}

#[test]
fn omit_view_parse_fails() {
    assert!(
        !parses_ok_with_cflags("CREATE VIEW v AS SELECT 1;", &[SqliteSyntaxFlag::OmitView]),
        "CREATE VIEW syntax should fail with OMIT_VIEW"
    );
}

#[test]
fn omit_view_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("CREATE VIEW v AS SELECT 1;"),
        "CREATE VIEW syntax should parse without OMIT_VIEW"
    );
}

// ---------------------------------------------------------------------------
// OMIT_TRIGGER — gates: TRIGGER, AFTER, BEFORE, EACH, FOR, INSTEAD,
//                        RAISE, ROW
// ---------------------------------------------------------------------------

#[test]
fn omit_trigger_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("TRIGGER", &[SqliteSyntaxFlag::OmitTrigger]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Trigger),
        "TRIGGER should fall back to ID with OMIT_TRIGGER"
    );
}

#[test]
fn omit_trigger_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "CREATE TRIGGER t AFTER INSERT ON x BEGIN SELECT 1; END;",
            &[SqliteSyntaxFlag::OmitTrigger],
        ),
        "CREATE TRIGGER syntax should fail with OMIT_TRIGGER"
    );
}

#[test]
fn omit_trigger_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("CREATE TRIGGER t AFTER INSERT ON x BEGIN SELECT 1; END;",),
        "CREATE TRIGGER syntax should parse without OMIT_TRIGGER"
    );
}

// ---------------------------------------------------------------------------
// OMIT_FOREIGN_KEY — gates: ACTION, CASCADE, DEFERRABLE, FOREIGN,
//                           INITIALLY, REFERENCES, RESTRICT
// ---------------------------------------------------------------------------

#[test]
fn omit_foreign_key_keywords_fall_back_to_id() {
    let cflags = &[SqliteSyntaxFlag::OmitForeignKey];
    for (sql, expected_tt, name) in [
        ("FOREIGN", tk(TokenType::Foreign), "FOREIGN"),
        ("REFERENCES", tk(TokenType::References), "REFERENCES"),
        ("CASCADE", tk(TokenType::Cascade), "CASCADE"),
        ("RESTRICT", tk(TokenType::Restrict), "RESTRICT"),
        ("DEFERRABLE", tk(TokenType::Deferrable), "DEFERRABLE"),
        ("INITIALLY", tk(TokenType::Initially), "INITIALLY"),
        ("ACTION", tk(TokenType::Action), "ACTION"),
    ] {
        let tokens = tokenize_with_cflags(sql, cflags);
        assert_ne!(
            tokens[0].0, expected_tt,
            "{name} should fall back to ID with OMIT_FOREIGN_KEY"
        );
    }
}

#[test]
fn omit_foreign_key_keywords_recognized_without_flag() {
    for (sql, expected_tt, name) in [
        ("FOREIGN", tk(TokenType::Foreign), "FOREIGN"),
        ("REFERENCES", tk(TokenType::References), "REFERENCES"),
    ] {
        let tokens = tokenize_default(sql);
        assert_eq!(
            tokens[0].0, expected_tt,
            "{name} should be recognized without OMIT_FOREIGN_KEY"
        );
    }
}

#[test]
fn omit_foreign_key_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "CREATE TABLE t(x INTEGER REFERENCES other(id));",
            &[SqliteSyntaxFlag::OmitForeignKey],
        ),
        "REFERENCES syntax should fail with OMIT_FOREIGN_KEY"
    );
}

#[test]
fn omit_foreign_key_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("CREATE TABLE t(x INTEGER REFERENCES other(id));"),
        "REFERENCES syntax should parse without OMIT_FOREIGN_KEY"
    );
}

// ---------------------------------------------------------------------------
// OMIT_GENERATED_COLUMNS — gates: ALWAYS
// ---------------------------------------------------------------------------

#[test]
fn omit_generated_columns_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("ALWAYS", &[SqliteSyntaxFlag::OmitGeneratedColumns]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Always),
        "ALWAYS should fall back to ID with OMIT_GENERATED_COLUMNS"
    );
}

#[test]
fn omit_generated_columns_keyword_recognized_without_flag() {
    let tokens = tokenize_default("ALWAYS");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Always),
        "ALWAYS should be recognized without OMIT_GENERATED_COLUMNS"
    );
}

// ---------------------------------------------------------------------------
// OMIT_EXPLAIN — gates: EXPLAIN, PLAN, QUERY
// ---------------------------------------------------------------------------

#[test]
fn omit_explain_keywords_fall_back_to_id() {
    let cflags = &[SqliteSyntaxFlag::OmitExplain];
    for (sql, expected_tt, name) in [
        ("EXPLAIN", tk(TokenType::Explain), "EXPLAIN"),
        ("QUERY", tk(TokenType::Query), "QUERY"),
    ] {
        let tokens = tokenize_with_cflags(sql, cflags);
        assert_ne!(
            tokens[0].0, expected_tt,
            "{name} should fall back to ID with OMIT_EXPLAIN"
        );
    }
}

#[test]
fn omit_explain_keywords_recognized_without_flag() {
    let tokens = tokenize_default("EXPLAIN");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Explain),
        "EXPLAIN should be recognized without OMIT_EXPLAIN"
    );
}

#[test]
fn omit_explain_parse_fails() {
    assert!(
        !parses_ok_with_cflags("EXPLAIN SELECT 1;", &[SqliteSyntaxFlag::OmitExplain]),
        "EXPLAIN syntax should fail with OMIT_EXPLAIN"
    );
}

#[test]
fn omit_explain_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("EXPLAIN SELECT 1;"),
        "EXPLAIN syntax should parse without OMIT_EXPLAIN"
    );
}

#[test]
fn omit_explain_query_plan_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "EXPLAIN QUERY PLAN SELECT 1;",
            &[SqliteSyntaxFlag::OmitExplain]
        ),
        "EXPLAIN QUERY PLAN should fail with OMIT_EXPLAIN"
    );
}

#[test]
fn omit_explain_query_plan_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("EXPLAIN QUERY PLAN SELECT 1;"),
        "EXPLAIN QUERY PLAN should parse without OMIT_EXPLAIN"
    );
}

// ---------------------------------------------------------------------------
// OMIT_ATTACH — gates: ATTACH, DATABASE, DETACH
// ---------------------------------------------------------------------------

#[test]
fn omit_attach_keywords_fall_back_to_id() {
    let cflags = &[SqliteSyntaxFlag::OmitAttach];
    for (sql, expected_tt, name) in [
        ("ATTACH", tk(TokenType::Attach), "ATTACH"),
        ("DATABASE", tk(TokenType::Database), "DATABASE"),
        ("DETACH", tk(TokenType::Detach), "DETACH"),
    ] {
        let tokens = tokenize_with_cflags(sql, cflags);
        assert_ne!(
            tokens[0].0, expected_tt,
            "{name} should fall back to ID with OMIT_ATTACH"
        );
    }
}

#[test]
fn omit_attach_keywords_recognized_without_flag() {
    for (sql, expected_tt, name) in [
        ("ATTACH", tk(TokenType::Attach), "ATTACH"),
        ("DETACH", tk(TokenType::Detach), "DETACH"),
        ("DATABASE", tk(TokenType::Database), "DATABASE"),
    ] {
        let tokens = tokenize_default(sql);
        assert_eq!(
            tokens[0].0, expected_tt,
            "{name} should be recognized without OMIT_ATTACH"
        );
    }
}

#[test]
fn omit_attach_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "ATTACH DATABASE ':memory:' AS db2;",
            &[SqliteSyntaxFlag::OmitAttach],
        ),
        "ATTACH DATABASE syntax should fail with OMIT_ATTACH"
    );
}

#[test]
fn omit_attach_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("ATTACH DATABASE ':memory:' AS db2;"),
        "ATTACH DATABASE syntax should parse without OMIT_ATTACH"
    );
}

#[test]
fn omit_detach_parse_fails() {
    assert!(
        !parses_ok_with_cflags("DETACH DATABASE db2;", &[SqliteSyntaxFlag::OmitAttach]),
        "DETACH DATABASE syntax should fail with OMIT_ATTACH"
    );
}

#[test]
fn omit_detach_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("DETACH DATABASE db2;"),
        "DETACH DATABASE syntax should parse without OMIT_ATTACH"
    );
}

// ---------------------------------------------------------------------------
// OMIT_AUTOINCREMENT — gates: AUTOINCREMENT
// ---------------------------------------------------------------------------

#[test]
fn omit_autoincrement_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("AUTOINCREMENT", &[SqliteSyntaxFlag::OmitAutoincrement]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Autoincr),
        "AUTOINCREMENT should fall back to ID with OMIT_AUTOINCREMENT"
    );
}

#[test]
fn omit_autoincrement_keyword_recognized_without_flag() {
    let tokens = tokenize_default("AUTOINCREMENT");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Autoincr),
        "AUTOINCREMENT should be recognized without OMIT_AUTOINCREMENT"
    );
}

#[test]
fn omit_autoincrement_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "CREATE TABLE t(id INTEGER PRIMARY KEY AUTOINCREMENT);",
            &[SqliteSyntaxFlag::OmitAutoincrement],
        ),
        "AUTOINCREMENT syntax should fail with OMIT_AUTOINCREMENT"
    );
}

#[test]
fn omit_autoincrement_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("CREATE TABLE t(id INTEGER PRIMARY KEY AUTOINCREMENT);"),
        "AUTOINCREMENT syntax should parse without OMIT_AUTOINCREMENT"
    );
}

// ---------------------------------------------------------------------------
// OMIT_CAST — gates: CAST
// ---------------------------------------------------------------------------

#[test]
fn omit_cast_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("CAST", &[SqliteSyntaxFlag::OmitCast]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Cast),
        "CAST should fall back to ID with OMIT_CAST"
    );
}

#[test]
fn omit_cast_keyword_recognized_without_flag() {
    let tokens = tokenize_default("CAST");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Cast),
        "CAST should be recognized without OMIT_CAST"
    );
}

#[test]
fn omit_cast_parse_fails() {
    assert!(
        !parses_ok_with_cflags("SELECT CAST(1 AS TEXT);", &[SqliteSyntaxFlag::OmitCast],),
        "CAST syntax should fail with OMIT_CAST"
    );
}

#[test]
fn omit_cast_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("SELECT CAST(1 AS TEXT);"),
        "CAST syntax should parse without OMIT_CAST"
    );
}

// ---------------------------------------------------------------------------
// OMIT_PRAGMA — gates: PRAGMA
// ---------------------------------------------------------------------------

#[test]
fn omit_pragma_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("PRAGMA", &[SqliteSyntaxFlag::OmitPragma]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Pragma),
        "PRAGMA should fall back to ID with OMIT_PRAGMA"
    );
}

#[test]
fn omit_pragma_keyword_recognized_without_flag() {
    let tokens = tokenize_default("PRAGMA");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Pragma),
        "PRAGMA should be recognized without OMIT_PRAGMA"
    );
}

#[test]
fn omit_pragma_parse_fails() {
    assert!(
        !parses_ok_with_cflags("PRAGMA table_info('t');", &[SqliteSyntaxFlag::OmitPragma]),
        "PRAGMA syntax should fail with OMIT_PRAGMA"
    );
}

#[test]
fn omit_pragma_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("PRAGMA table_info('t');"),
        "PRAGMA syntax should parse without OMIT_PRAGMA"
    );
}

// ---------------------------------------------------------------------------
// OMIT_REINDEX — gates: REINDEX
// ---------------------------------------------------------------------------

#[test]
fn omit_reindex_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("REINDEX", &[SqliteSyntaxFlag::OmitReindex]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Reindex),
        "REINDEX should fall back to ID with OMIT_REINDEX"
    );
}

#[test]
fn omit_reindex_keyword_recognized_without_flag() {
    let tokens = tokenize_default("REINDEX");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Reindex),
        "REINDEX should be recognized without OMIT_REINDEX"
    );
}

#[test]
fn omit_reindex_parse_fails() {
    assert!(
        !parses_ok_with_cflags("REINDEX;", &[SqliteSyntaxFlag::OmitReindex]),
        "REINDEX syntax should fail with OMIT_REINDEX"
    );
}

#[test]
fn omit_reindex_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("REINDEX;"),
        "REINDEX syntax should parse without OMIT_REINDEX"
    );
}

// ---------------------------------------------------------------------------
// OMIT_VIRTUALTABLE — gates: VIRTUAL
// ---------------------------------------------------------------------------

#[test]
fn omit_virtualtable_keyword_falls_back_to_id() {
    let tokens = tokenize_with_cflags("VIRTUAL", &[SqliteSyntaxFlag::OmitVirtualtable]);
    assert_ne!(
        tokens[0].0,
        tk(TokenType::Virtual),
        "VIRTUAL should fall back to ID with OMIT_VIRTUALTABLE"
    );
}

#[test]
fn omit_virtualtable_keyword_recognized_without_flag() {
    let tokens = tokenize_default("VIRTUAL");
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Virtual),
        "VIRTUAL should be recognized without OMIT_VIRTUALTABLE"
    );
}

#[test]
fn omit_virtualtable_parse_fails() {
    assert!(
        !parses_ok_with_cflags(
            "CREATE VIRTUAL TABLE t USING fts5(content);",
            &[SqliteSyntaxFlag::OmitVirtualtable],
        ),
        "CREATE VIRTUAL TABLE syntax should fail with OMIT_VIRTUALTABLE"
    );
}

#[test]
fn omit_virtualtable_parse_succeeds_without_flag() {
    assert!(
        parses_ok_default("CREATE VIRTUAL TABLE t USING fts5(content);"),
        "CREATE VIRTUAL TABLE syntax should parse without OMIT_VIRTUALTABLE"
    );
}

// ===========================================================================
// Cross-cutting cflag tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Multiple cflags combined
// ---------------------------------------------------------------------------

#[test]
fn multiple_omit_flags_suppress_independently() {
    let cflags = &[SqliteSyntaxFlag::OmitExplain, SqliteSyntaxFlag::OmitAttach];

    // EXPLAIN suppressed
    let tokens = tokenize_with_cflags("EXPLAIN", cflags);
    assert_ne!(tokens[0].0, tk(TokenType::Explain));

    // ATTACH suppressed
    let tokens = tokenize_with_cflags("ATTACH", cflags);
    assert_ne!(tokens[0].0, tk(TokenType::Attach));

    // SELECT still works (not gated by either flag)
    assert!(
        parses_ok_with_cflags("SELECT 1;", cflags),
        "SELECT should still parse with unrelated OMIT flags"
    );
}

#[test]
fn omit_flag_does_not_affect_unrelated_keywords() {
    // OMIT_CAST should not affect PRAGMA
    let tokens = tokenize_with_cflags("PRAGMA", &[SqliteSyntaxFlag::OmitCast]);
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Pragma),
        "PRAGMA should not be affected by OMIT_CAST"
    );

    // OMIT_PRAGMA should not affect CAST
    let tokens = tokenize_with_cflags("CAST", &[SqliteSyntaxFlag::OmitPragma]);
    assert_eq!(
        tokens[0].0,
        tk(TokenType::Cast),
        "CAST should not be affected by OMIT_PRAGMA"
    );
}

// ===========================================================================
// ENABLE_UPDATE_DELETE_LIMIT — parser-level cflag check
//
// ORDER BY / LIMIT on DELETE and UPDATE require the
// SQLITE_ENABLE_UPDATE_DELETE_LIMIT cflag. The grammar actions check
// the cflag at parse time and raise a syntax error if it's not set.
// ===========================================================================

#[test]
fn delete_with_order_by_limit_fails_without_cflag() {
    assert!(
        !parses_ok_default("DELETE FROM t ORDER BY id LIMIT 5;"),
        "DELETE with ORDER BY/LIMIT should fail without ENABLE_UPDATE_DELETE_LIMIT"
    );
}

#[test]
fn delete_with_order_by_limit_succeeds_with_cflag() {
    assert!(
        parses_ok_with_cflags(
            "DELETE FROM t ORDER BY id LIMIT 5;",
            &[SqliteSyntaxFlag::EnableUpdateDeleteLimit],
        ),
        "DELETE with ORDER BY/LIMIT should parse when ENABLE_UPDATE_DELETE_LIMIT is set"
    );
}

#[test]
fn update_with_limit_fails_without_cflag() {
    assert!(
        !parses_ok_default("UPDATE t SET a = 1 ORDER BY id LIMIT 3;"),
        "UPDATE with ORDER BY/LIMIT should fail without ENABLE_UPDATE_DELETE_LIMIT"
    );
}

#[test]
fn update_with_limit_succeeds_with_cflag() {
    assert!(
        parses_ok_with_cflags(
            "UPDATE t SET a = 1 ORDER BY id LIMIT 3;",
            &[SqliteSyntaxFlag::EnableUpdateDeleteLimit],
        ),
        "UPDATE with ORDER BY/LIMIT should parse when ENABLE_UPDATE_DELETE_LIMIT is set"
    );
}

#[test]
fn plain_delete_succeeds_without_cflag() {
    assert!(
        parses_ok_default("DELETE FROM t WHERE x = 1;"),
        "Plain DELETE should parse without ENABLE_UPDATE_DELETE_LIMIT"
    );
}

#[test]
fn plain_update_succeeds_without_cflag() {
    assert!(
        parses_ok_default("UPDATE t SET a = 1 WHERE x = 2;"),
        "Plain UPDATE should parse without ENABLE_UPDATE_DELETE_LIMIT"
    );
}
