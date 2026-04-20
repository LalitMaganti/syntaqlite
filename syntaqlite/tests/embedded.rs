// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Integration tests for embedded SQL validation in Python f-strings and
//! TypeScript template literals.

#![cfg(feature = "experimental-embedded")]

use syntaqlite::embedded::{
    EmbeddedAnalyzer, HOLE_PLACEHOLDER, extract_python, extract_typescript,
};

fn analyzer() -> EmbeddedAnalyzer {
    EmbeddedAnalyzer::new(syntaqlite::sqlite_dialect())
}

// ── Extraction tests ────────────────────────────────────────────────────

#[test]
fn simple_fstring_with_valid_sql() {
    let source = r#"query = f"SELECT * FROM users WHERE id = {uid}""#;
    let fragments = extract_python(source);
    assert_eq!(fragments.len(), 1);
    assert_eq!(fragments[0].holes().len(), 1);
    assert!(fragments[0].sql_text().contains(HOLE_PLACEHOLDER));
}

#[test]
fn multiple_sql_fstrings_in_one_file() {
    let source = r#"
q1 = f"SELECT * FROM users WHERE id = {uid}"
msg = f"Hello {name}"
q2 = f"INSERT INTO logs (msg) VALUES ({log_msg})"
"#;
    let fragments = extract_python(source);
    assert_eq!(fragments.len(), 2); // q1 and q2, not msg
}

#[test]
fn non_sql_fstrings_skipped() {
    let source = r#"
greeting = f"Hello {name}, welcome!"
path = f"/api/{version}/users"
count = f"Total: {n} items"
"#;
    let fragments = extract_python(source);
    assert_eq!(fragments.len(), 0);
}

#[test]
fn escaped_braces() {
    let source = r#"q = f"SELECT '{{literal}}' FROM t""#;
    let fragments = extract_python(source);
    assert_eq!(fragments.len(), 1);
    assert_eq!(fragments[0].holes().len(), 0);
    // `{{` becomes `{` in the SQL text.
    assert!(fragments[0].sql_text().contains("{literal}"));
}

#[test]
fn multiline_fstring() {
    let source = "q = f\"\"\"\nSELECT *\nFROM users\nWHERE id = {uid}\n\"\"\"";
    let fragments = extract_python(source);
    assert_eq!(fragments.len(), 1);
    assert_eq!(fragments[0].holes().len(), 1);
    assert!(
        fragments[0]
            .sql_text()
            .contains("SELECT *\nFROM users\nWHERE id = ")
    );
}

#[test]
fn fstring_with_holes_in_multiple_positions() {
    let source = r#"q = f"SELECT {cols} FROM {table} WHERE {col} = {val}""#;
    let fragments = extract_python(source);
    assert_eq!(fragments.len(), 1);
    let f = &fragments[0];
    assert_eq!(f.holes().len(), 4);
    // All holes use the same constant placeholder.
    for hole in f.holes() {
        let start = hole.sql_offset().as_usize();
        let end = start + HOLE_PLACEHOLDER.len();
        assert_eq!(&f.sql_text()[start..end], HOLE_PLACEHOLDER);
    }
}

// ── Validation tests ────────────────────────────────────────────────────

#[test]
fn validate_simple_select_with_hole() {
    let source = r#"query = f"SELECT * FROM users WHERE id = {uid}""#;
    let fragments = extract_python(source);
    let diags = analyzer().validate(&fragments);

    // Hole placeholders must not leak into diagnostics.
    for d in &diags {
        let msg = d.message().to_string();
        assert!(
            !msg.contains("__h__"),
            "hole placeholder leaked into diagnostic: {msg}"
        );
    }
}

#[test]
fn validate_multiple_holes_no_placeholder_leaks() {
    let source = r#"q = f"SELECT {cols} FROM {table} WHERE {col} = {val}""#;
    let fragments = extract_python(source);
    let diags = analyzer().validate(&fragments);

    for d in &diags {
        let msg = d.message().to_string();
        assert!(!msg.contains("__h__"), "hole placeholder leaked: {msg}");
    }
}

#[test]
fn validate_with_known_schema() {
    // Create a source with DDL + query to test incremental validation.
    let source = r#"
setup = f"CREATE TABLE users (id INTEGER, name TEXT)"
query = f"SELECT id, name FROM users WHERE id = {uid}"
"#;
    let fragments = extract_python(source);

    // Each fragment is validated independently — the CREATE TABLE in one
    // f-string doesn't carry over to the SELECT in another. This is expected
    // for the prototype.
    let diags = analyzer().validate(&fragments);

    for d in &diags {
        let msg = d.message().to_string();
        assert!(!msg.contains("__h__"), "hole placeholder leaked: {msg}");
    }
}

#[test]
fn validate_offsets_mapped_to_host_file() {
    // "unknown table 'nonexistent'" diagnostic offset should point into the
    // host file, not the processed SQL text.
    let source = r#"q = f"SELECT * FROM nonexistent""#;
    let fragments = extract_python(source);
    let diags = analyzer().validate(&fragments);

    // Should have a diagnostic about 'nonexistent'.
    let table_diag = diags
        .iter()
        .find(|d| d.message().to_string().contains("nonexistent"));
    if let Some(d) = table_diag {
        // The word "nonexistent" starts at offset 20 in the host source
        // (after `q = f"SELECT * FROM `).
        let referenced = &source[d.start().as_usize()..d.end().as_usize()];
        assert_eq!(referenced, "nonexistent");
    }
}

// ── TypeScript extraction tests ──────────────────────────────────────

#[test]
fn ts_simple_template_literal_with_valid_sql() {
    let source = r"const q = `SELECT * FROM users WHERE id = ${uid}`;";
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 1);
    assert_eq!(fragments[0].holes().len(), 1);
    assert!(fragments[0].sql_text().contains(HOLE_PLACEHOLDER));
}

#[test]
fn ts_multiple_sql_template_literals_in_one_file() {
    let source = r"
const q1 = `SELECT * FROM users WHERE id = ${uid}`;
const msg = `Hello ${name}`;
const q2 = `INSERT INTO logs (msg) VALUES (${log_msg})`;
";
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 2); // q1 and q2, not msg
}

#[test]
fn ts_non_sql_templates_skipped() {
    let source = r"
const greeting = `Hello ${name}, welcome!`;
const path = `/api/${version}/users`;
const count = `Total: ${n} items`;
";
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 0);
}

#[test]
fn ts_multiline_template_literal() {
    let source = "const q = `\nSELECT *\nFROM users\nWHERE id = ${uid}\n`;";
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 1);
    assert_eq!(fragments[0].holes().len(), 1);
    assert!(
        fragments[0]
            .sql_text()
            .contains("SELECT *\nFROM users\nWHERE id = ")
    );
}

#[test]
fn ts_template_with_multiple_holes() {
    let source = r"const q = `SELECT ${cols} FROM ${table} WHERE ${col} = ${val}`;";
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 1);
    let f = &fragments[0];
    assert_eq!(f.holes().len(), 4);
    for hole in f.holes() {
        let start = hole.sql_offset().as_usize();
        let end = start + HOLE_PLACEHOLDER.len();
        assert_eq!(&f.sql_text()[start..end], HOLE_PLACEHOLDER);
    }
}

#[test]
fn ts_templates_inside_comments_skipped() {
    let source = r"
// const q = `SELECT * FROM users`;
/* const q = `SELECT * FROM users`; */
const x = 1;
";
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 0);
}

#[test]
fn ts_templates_inside_strings_skipped() {
    let source = r#"const s = "const q = `SELECT * FROM users`";"#;
    let fragments = extract_typescript(source);
    assert_eq!(fragments.len(), 0);
}

// ── TypeScript validation tests ──────────────────────────────────────

#[test]
fn ts_validate_simple_select_with_hole() {
    let source = r"const q = `SELECT * FROM users WHERE id = ${uid}`;";
    let fragments = extract_typescript(source);
    let diags = analyzer().validate(&fragments);

    for d in &diags {
        let msg = d.message().to_string();
        assert!(
            !msg.contains("__h__"),
            "hole placeholder leaked into diagnostic: {msg}"
        );
    }
}

#[test]
fn ts_validate_multiple_holes_no_placeholder_leaks() {
    let source = r"const q = `SELECT ${cols} FROM ${table} WHERE ${col} = ${val}`;";
    let fragments = extract_typescript(source);
    let diags = analyzer().validate(&fragments);

    for d in &diags {
        let msg = d.message().to_string();
        assert!(!msg.contains("__h__"), "hole placeholder leaked: {msg}");
    }
}

#[test]
fn ts_validate_offsets_mapped_to_host_file() {
    let source = r"const q = `SELECT * FROM nonexistent`;";
    let fragments = extract_typescript(source);
    let diags = analyzer().validate(&fragments);

    let table_diag = diags
        .iter()
        .find(|d| d.message().to_string().contains("nonexistent"));
    if let Some(d) = table_diag {
        let referenced = &source[d.start().as_usize()..d.end().as_usize()];
        assert_eq!(referenced, "nonexistent");
    }
}
