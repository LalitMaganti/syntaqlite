// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Integration tests exercising the formatter with various SQL patterns.

use syntaqlite::fmt::KeywordCase;
use syntaqlite::util::{SqliteFlag, SqliteFlags};
use syntaqlite::{FormatConfig, Formatter, sqlite_dialect};

fn format_sql(sql: &str) -> String {
    format_sql_with(sql, &FormatConfig::default())
}

fn format_sql_with(sql: &str, config: &FormatConfig) -> String {
    let mut f = Formatter::with_config(config);
    let result = f.format(sql).expect("format failed");
    result
        .trim_end_matches('\n')
        .trim_end_matches(';')
        .to_string()
}

fn format_sql_with_flags(sql: &str, config: &FormatConfig, flags: SqliteFlags) -> String {
    let dialect = sqlite_dialect().with_cflags(flags);
    let mut f = Formatter::with_dialect_config(dialect, config);
    let result = f.format(sql).expect("format failed");
    result
        .trim_end_matches('\n')
        .trim_end_matches(';')
        .to_string()
}

// -- Idempotent round-trip: well-formatted SQL survives formatting unchanged --

#[test]
fn format_idempotent() {
    let cases = [
        // Basic SELECT variations
        "SELECT 42",
        "SELECT a FROM t",
        "SELECT a, b, c FROM t",
        "SELECT a FROM t WHERE x = 1",
        "SELECT DISTINCT a FROM t",
        "SELECT * FROM t",
        "SELECT a AS x FROM t",
        // Expressions
        "SELECT 1 + 2",
        "SELECT -1",
        "SELECT a FROM t WHERE x = 1 AND y = 2",
        // Table references
        "SELECT a FROM t AS u",
        // Clauses
        "SELECT a, COUNT(*) FROM t GROUP BY a",
        "SELECT a FROM t ORDER BY a",
        "SELECT a FROM t LIMIT 10",
        // Other statement types
        "DELETE FROM t WHERE x = 1",
        "UPDATE t SET a = 1 WHERE x = 2",
        "CREATE TABLE t(a INTEGER, b TEXT)",
        "DROP TABLE t",
        "DROP TABLE IF EXISTS t",
    ];
    for sql in &cases {
        assert_eq!(format_sql(sql), *sql, "round-trip failed for: {sql}");
    }
}

// -- Line breaking --

#[test]
fn long_select_breaks() {
    let config = FormatConfig::default().with_line_width(20);
    let result = format_sql_with(
        "SELECT column_one, column_two FROM very_long_table",
        &config,
    );
    assert_eq!(
        result,
        "SELECT\n  column_one,\n  column_two\nFROM very_long_table"
    );
}

// -- Formatting transformations (input != output) --

#[test]
fn delete_with_order_by_limit() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableUpdateDeleteLimit);
    assert_eq!(
        format_sql_with_flags(
            "delete from t where x > 0 order by id limit 10 offset 5",
            &FormatConfig::default(),
            flags,
        ),
        "DELETE FROM t WHERE x > 0 ORDER BY id LIMIT 10 OFFSET 5"
    );
}

#[test]
fn update_with_order_by_limit() {
    let flags = SqliteFlags::default().with(SqliteFlag::EnableUpdateDeleteLimit);
    assert_eq!(
        format_sql_with_flags(
            "update t set a = 1 where x > 0 order by id limit 5",
            &FormatConfig::default(),
            flags,
        ),
        "UPDATE t SET a = 1 WHERE x > 0 ORDER BY id LIMIT 5"
    );
}

// -- Line breaking for INSERT --

#[test]
fn insert_breaks_when_narrow() {
    let config = FormatConfig::default().with_line_width(20);
    let result = format_sql_with("INSERT INTO t(a, b) VALUES(1, 2)", &config);
    assert_eq!(result, "INSERT INTO t(a, b)\nVALUES (1, 2)");
}

// -- Large VALUES --

#[test]
fn insert_many_values_flat() {
    let config = FormatConfig::default().with_line_width(40);
    let result = format_sql_with(
        "INSERT INTO t(a, b) VALUES(1, 2), (3, 4), (5, 6), (7, 8)",
        &config,
    );
    assert_eq!(
        result,
        "INSERT INTO t(a, b)\nVALUES (1, 2), (3, 4), (5, 6), (7, 8)"
    );
}

#[test]
fn insert_many_values_breaks() {
    let config = FormatConfig::default().with_line_width(30);
    let result = format_sql_with(
        "INSERT INTO t(a, b) VALUES(1, 2), (3, 4), (5, 6), (7, 8)",
        &config,
    );
    assert_eq!(
        result,
        "INSERT INTO t(a, b)\nVALUES\n  (1, 2),\n  (3, 4),\n  (5, 6),\n  (7, 8)"
    );
}

// -- Comments --

#[test]
fn comment_leading_before_column() {
    let config = FormatConfig::default().with_line_width(20);
    assert_eq!(
        format_sql_with("SELECT\n  -- comment\n  a\nFROM t", &config),
        "SELECT\n  -- comment\n  a\nFROM t"
    );
}

#[test]
fn comment_between_columns() {
    let config = FormatConfig::default().with_line_width(20);
    assert_eq!(
        format_sql_with("SELECT\n  a,\n  -- about b\n  b\nFROM t", &config),
        "SELECT\n  a,\n  -- about b\n  b\nFROM t"
    );
}

// -- Multi-statement comments --

#[test]
fn debug_multi_stmt_comments() {
    // Log exactly which comments each statement sees from the parser.
    use syntaqlite::parse::ParserConfig;
    use syntaqlite::{ParseOutcome, Parser};
    for source in [
        "SELECT 1;\n-- between\nSELECT 2",
        "SELECT 1; -- after first\nSELECT 2",
    ] {
        eprintln!("\n=== source: {source:?} ===");
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse(source);
        let mut stmt_num = 0;
        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(s) => s,
                ParseOutcome::Err(e) => panic!("parse error: {e:?}"),
            };
            let comments: Vec<_> = stmt.comments().collect();
            let stmt_text = stmt.text();
            eprintln!("stmt {stmt_num}: source={stmt_text:?}");
            eprintln!("  {} comment(s):", comments.len());
            for c in &comments {
                let range = syntaqlite::source::StmtRange::from_offset_len(c.offset(), c.length());
                let text = &stmt_text[range];
                eprintln!(
                    "    offset={} kind={:?} text={text:?}",
                    c.offset(),
                    c.kind()
                );
            }
            stmt_num += 1;
        }
    }
}

// -- Keyword casing --

#[test]
fn keyword_case_lower() {
    let config = FormatConfig::default().with_keyword_case(KeywordCase::Lower);
    assert_eq!(
        format_sql_with("SELECT a FROM t", &config),
        "select a from t"
    );
}

#[test]
fn keyword_case_upper() {
    let config = FormatConfig::default().with_keyword_case(KeywordCase::Upper);
    assert_eq!(
        format_sql_with("select a from t", &config),
        "SELECT a FROM t"
    );
}
