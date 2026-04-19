// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::ffi::{CStr, CString, c_char};

use super::SyntaqliteValidator;
use super::analyze::*;
use super::catalog_ffi::*;
use super::codes::*;
use super::config::*;
use super::lifecycle::*;
use super::results::*;
use super::statement::*;
use super::types::*;

/// Helper: analyze SQL via FFI and return the diagnostic count.
unsafe fn analyze(v: *mut SyntaqliteValidator, sql: &str) -> u32 {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe {
        syntaqlite_validator_analyze(v, sql.as_ptr().cast(), u32::try_from(sql.len()).unwrap())
    }
}

/// Helper: read the i-th diagnostic message as a Rust string.
unsafe fn diag_msg(v: *const SyntaqliteValidator, i: usize) -> String {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe {
        let ptr = syntaqlite_validator_diagnostics(v);
        assert!(!ptr.is_null());
        let d = &*ptr.add(i);
        CStr::from_ptr(d.message).to_str().unwrap().to_owned()
    }
}

/// Helper: render diagnostics and return as a Rust string.
unsafe fn render(v: *mut SyntaqliteValidator, file: Option<&CStr>) -> String {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe {
        let file_ptr = file.map_or(std::ptr::null(), CStr::as_ptr);
        let ptr = syntaqlite_validator_render_diagnostics(v, file_ptr);
        assert!(!ptr.is_null());
        CStr::from_ptr(ptr).to_str().unwrap().to_owned()
    }
}

// ── Lifecycle ─────────────────────────────────────────────────────────

#[test]
fn create_and_destroy() {
    let v = syntaqlite_validator_create_sqlite();
    assert!(!v.is_null());
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn null_destroy_is_noop() {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(std::ptr::null_mut()) };
}

// ── Analysis: clean SQL ───────────────────────────────────────────────

#[test]
fn valid_sql_produces_no_diagnostics() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT 1") };
    assert_eq!(n, 0);
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    assert_eq!(unsafe { syntaqlite_validator_diagnostic_count(v) }, 0);
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    assert!(unsafe { syntaqlite_validator_diagnostics(v) }.is_null());
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Analysis: unknown table ───────────────────────────────────────────

#[test]
fn unknown_table_produces_diagnostic() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT id FROM no_such_table") };
    assert!(n > 0, "expected at least one diagnostic");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let msg = unsafe { diag_msg(v, 0) };
    assert!(
        msg.contains("no_such_table"),
        "diagnostic should mention the table: {msg}"
    );

    // Severity should be warning (default non-strict mode).
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let d = unsafe { &*syntaqlite_validator_diagnostics(v) };
    assert_eq!(d.severity, SEVERITY_WARNING);

    // Offsets should be within the source bounds.
    assert!(d.start_offset < d.end_offset);
    assert!((d.end_offset as usize) <= "SELECT id FROM no_such_table".len());

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Catalog: add_tables ───────────────────────────────────────────────

#[test]
fn add_tables_resolves_unknown_table() {
    let v = syntaqlite_validator_create_sqlite();

    // Before adding: diagnostic.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT id FROM users") };
    assert!(n > 0);

    // Register the table.
    let name = CString::new("users").unwrap();
    let col_id = CString::new("id").unwrap();
    let col_name = CString::new("name").unwrap();
    let cols: [*const c_char; 2] = [col_id.as_ptr(), col_name.as_ptr()];
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: cols.as_ptr(),
        column_count: 2,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };

    // After adding: clean.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT id FROM users") };
    assert_eq!(n, 0, "table should be resolved after add_tables");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn add_tables_with_null_columns_accepts_any_column() {
    let v = syntaqlite_validator_create_sqlite();

    let name = CString::new("events").unwrap();
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: std::ptr::null(),
        column_count: 0,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };

    // Any column reference should be accepted (unknown-columns table).
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT anything, goes FROM events") };
    assert_eq!(n, 0);

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn add_tables_wrong_column_produces_diagnostic() {
    let v = syntaqlite_validator_create_sqlite();

    let name = CString::new("users").unwrap();
    let col_id = CString::new("id").unwrap();
    let cols: [*const c_char; 1] = [col_id.as_ptr()];
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: cols.as_ptr(),
        column_count: 1,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT nonexistent FROM users") };
    assert!(
        n > 0,
        "referencing a bad column should produce a diagnostic"
    );

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let msg = unsafe { diag_msg(v, 0) };
    assert!(
        msg.contains("nonexistent"),
        "should mention the column: {msg}"
    );

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Catalog: reset ────────────────────────────────────────────────────

#[test]
fn reset_catalog_removes_tables() {
    let v = syntaqlite_validator_create_sqlite();

    let name = CString::new("users").unwrap();
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: std::ptr::null(),
        column_count: 0,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    assert_eq!(unsafe { analyze(v, "SELECT 1 FROM users") }, 0);

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_reset_catalog(v) };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT 1 FROM users") };
    assert!(n > 0, "table should be gone after reset");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Analysis mode ─────────────────────────────────────────────────────

#[test]
fn execute_mode_accumulates_ddl() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_set_mode(v, 1) }; // Execute

    // CREATE TABLE in one call...
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "CREATE TABLE t(x)") };

    // ...visible in the next call.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT x FROM t") };
    assert_eq!(n, 0, "DDL should persist in execute mode");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn document_mode_resets_ddl_between_calls() {
    let v = syntaqlite_validator_create_sqlite();
    // Document mode is the default (mode=0).

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "CREATE TABLE t(x)") };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT x FROM t") };
    assert!(n > 0, "DDL should NOT persist in document mode");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Reuse across calls ────────────────────────────────────────────────

#[test]
fn successive_analyze_calls_replace_diagnostics() {
    let v = syntaqlite_validator_create_sqlite();

    // First call: diagnostics.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n1 = unsafe { analyze(v, "SELECT 1 FROM bad_table") };
    assert!(n1 > 0);

    // Second call: clean.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n2 = unsafe { analyze(v, "SELECT 1") };
    assert_eq!(n2, 0);
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    assert_eq!(unsafe { syntaqlite_validator_diagnostic_count(v) }, 0);
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    assert!(unsafe { syntaqlite_validator_diagnostics(v) }.is_null());

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Diagnostic rendering ──────────────────────────────────────────────

#[test]
fn render_diagnostics_with_file_label() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT 1 FROM bad") };
    assert!(n > 0);

    let file = CString::new("test.sql").unwrap();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let rendered = unsafe { render(v, Some(&file)) };

    assert!(
        rendered.contains("test.sql"),
        "should contain file label: {rendered}"
    );
    assert!(
        rendered.contains("bad"),
        "should contain table name: {rendered}"
    );
    assert!(
        rendered.contains("warning") || rendered.contains("error"),
        "should contain severity: {rendered}"
    );
}

#[test]
fn render_diagnostics_with_null_file_uses_default() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM bad") };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let rendered = unsafe { render(v, None) };
    assert!(
        rendered.contains("<input>"),
        "should use default label: {rendered}"
    );

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn render_diagnostics_empty_when_no_errors() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1") };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let rendered = unsafe { render(v, None) };
    assert!(
        rendered.is_empty(),
        "should be empty for clean SQL: {rendered}"
    );

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn render_diagnostics_shows_multiple_issues() {
    let v = syntaqlite_validator_create_sqlite();

    let name = CString::new("t").unwrap();
    let col = CString::new("x").unwrap();
    let cols: [*const c_char; 1] = [col.as_ptr()];
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: cols.as_ptr(),
        column_count: 1,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };

    // Two bad columns in one query.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let n = unsafe { analyze(v, "SELECT bad1, bad2 FROM t") };
    assert!(n >= 2, "expected at least 2 diagnostics, got {n}");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let rendered = unsafe { render(v, None) };
    assert!(rendered.contains("bad1"), "should mention bad1: {rendered}");
    assert!(rendered.contains("bad2"), "should mention bad2: {rendered}");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn render_replaces_previous_render() {
    let v = syntaqlite_validator_create_sqlite();

    // Render with one error.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM alpha") };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let r1 = unsafe { render(v, None) };
    assert!(r1.contains("alpha"));

    // Render with a different error — previous render is replaced.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM beta") };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let r2 = unsafe { render(v, None) };
    assert!(r2.contains("beta"));
    assert!(!r2.contains("alpha"), "old render should be gone: {r2}");

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── Strict schema (severity promotion) ─────────────────────────────

#[test]
fn no_schema_unknown_table_is_warning() {
    let v = syntaqlite_validator_create_sqlite();
    // No tables added — empty catalog, strict_schema is false.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM bad_table") };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let d = unsafe { &*syntaqlite_validator_diagnostics(v) };
    assert_eq!(
        d.severity, SEVERITY_WARNING,
        "should be warning without schema"
    );
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn with_schema_unknown_column_is_error() {
    let v = syntaqlite_validator_create_sqlite();

    let name = CString::new("users").unwrap();
    let col = CString::new("id").unwrap();
    let cols: [*const c_char; 1] = [col.as_ptr()];
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: cols.as_ptr(),
        column_count: 1,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_set_strict_schema(v, 1) };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT bogus FROM users") };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let d = unsafe { &*syntaqlite_validator_diagnostics(v) };
    assert_eq!(d.severity, SEVERITY_ERROR, "should be error with schema");
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn with_schema_unknown_table_is_error() {
    let v = syntaqlite_validator_create_sqlite();

    let name = CString::new("users").unwrap();
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: std::ptr::null(),
        column_count: 0,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_set_strict_schema(v, 1) };

    // Query a different table that doesn't exist.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM nonexistent") };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let d = unsafe { &*syntaqlite_validator_diagnostics(v) };
    assert_eq!(
        d.severity, SEVERITY_ERROR,
        "unknown table should be error with schema"
    );
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

#[test]
fn reset_catalog_reverts_to_warning_severity() {
    let v = syntaqlite_validator_create_sqlite();

    // Add a table — activates strict mode.
    let name = CString::new("t").unwrap();
    let table = SyntaqliteRelationDef {
        name: name.as_ptr(),
        columns: std::ptr::null(),
        column_count: 0,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_set_strict_schema(v, 1) };

    // Verify it's error-level.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM gone") };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let d = unsafe { &*syntaqlite_validator_diagnostics(v) };
    assert_eq!(d.severity, SEVERITY_ERROR);

    // Reset catalog — should revert to warnings.
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_reset_catalog(v) };

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { analyze(v, "SELECT 1 FROM gone") };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    let d = unsafe { &*syntaqlite_validator_diagnostics(v) };
    assert_eq!(
        d.severity, SEVERITY_WARNING,
        "should revert to warning after reset"
    );

    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_destroy(v) };
}

// ── string_destroy ────────────────────────────────────────────────────

#[test]
fn string_destroy_null_is_noop() {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_string_destroy(std::ptr::null_mut()) };
}

// ── Lineage ─────────────────────────────────────────────────────────

/// Helper: register a table with known columns via FFI.
#[expect(clippy::cast_possible_truncation)]
unsafe fn add_table(v: *mut SyntaqliteValidator, name: &str, cols: &[&str]) {
    let c_name = CString::new(name).unwrap();
    let c_cols: Vec<CString> = cols.iter().map(|c| CString::new(*c).unwrap()).collect();
    let c_col_ptrs: Vec<*const c_char> = c_cols.iter().map(|c| c.as_ptr()).collect();
    let table = SyntaqliteRelationDef {
        name: c_name.as_ptr(),
        columns: c_col_ptrs.as_ptr(),
        column_count: c_col_ptrs.len() as u32,
    };
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe { syntaqlite_validator_add_tables(v, &raw const table, 1) };
}

#[test]
fn lineage_simple_select() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        add_table(v, "users", &["id", "name"]);
        analyze(v, "SELECT id, name FROM users");

        assert_eq!(syntaqlite_validator_lineage_complete(v), 1);
        assert_eq!(syntaqlite_validator_column_lineage_count(v), 2);

        let cols = syntaqlite_validator_column_lineage(v);
        assert!(!cols.is_null());

        // First column: id -> users.id
        let c0 = &*cols.add(0);
        assert_eq!(CStr::from_ptr(c0.name).to_str().unwrap(), "id");
        assert_eq!(c0.index, 0);
        assert!(!c0.origin.table.is_null());
        assert_eq!(CStr::from_ptr(c0.origin.table).to_str().unwrap(), "users");
        assert_eq!(CStr::from_ptr(c0.origin.column).to_str().unwrap(), "id");

        // Second column: name -> users.name
        let c1 = &*cols.add(1);
        assert_eq!(CStr::from_ptr(c1.name).to_str().unwrap(), "name");
        assert_eq!(c1.index, 1);
        assert_eq!(CStr::from_ptr(c1.origin.table).to_str().unwrap(), "users");
        assert_eq!(CStr::from_ptr(c1.origin.column).to_str().unwrap(), "name");

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn lineage_non_query_returns_empty() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        analyze(v, "CREATE TABLE t(x)");

        assert_eq!(syntaqlite_validator_column_lineage_count(v), 0);
        assert!(syntaqlite_validator_column_lineage(v).is_null());
        assert_eq!(syntaqlite_validator_relation_count(v), 0);
        assert!(syntaqlite_validator_relations(v).is_null());
        assert_eq!(syntaqlite_validator_physical_table_count(v), 0);
        assert!(syntaqlite_validator_physical_tables(v).is_null());

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn lineage_expression_has_null_origin() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        add_table(v, "users", &["id"]);
        analyze(v, "SELECT id + 1 AS x FROM users");

        assert_eq!(syntaqlite_validator_column_lineage_count(v), 1);
        let cols = syntaqlite_validator_column_lineage(v);
        let c0 = &*cols;
        assert_eq!(CStr::from_ptr(c0.name).to_str().unwrap(), "x");
        assert!(c0.origin.table.is_null());
        assert!(c0.origin.column.is_null());

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn lineage_relations_and_tables() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        add_table(v, "users", &["id", "name"]);
        add_table(v, "posts", &["id", "user_id"]);
        analyze(
            v,
            "SELECT u.id FROM users u JOIN posts p ON u.id = p.user_id",
        );

        // Relations
        let rel_count = syntaqlite_validator_relation_count(v);
        assert!(
            rel_count >= 2,
            "expected at least 2 relations, got {rel_count}"
        );
        let rels = syntaqlite_validator_relations(v);
        assert!(!rels.is_null());

        // Tables
        let tbl_count = syntaqlite_validator_physical_table_count(v);
        assert!(
            tbl_count >= 2,
            "expected at least 2 tables, got {tbl_count}"
        );
        let tbls = syntaqlite_validator_physical_tables(v);
        assert!(!tbls.is_null());

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn lineage_reset_on_next_analyze() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        add_table(v, "users", &["id"]);

        // First: SELECT → lineage present.
        analyze(v, "SELECT id FROM users");
        assert!(syntaqlite_validator_column_lineage_count(v) > 0);

        // Second: non-query → lineage cleared.
        analyze(v, "CREATE TABLE t(x)");
        assert_eq!(syntaqlite_validator_column_lineage_count(v), 0);
        assert!(syntaqlite_validator_column_lineage(v).is_null());

        syntaqlite_validator_destroy(v);
    }
}

// ── add_views ───────────────────────────────────────────────────────

#[test]
fn add_views_registers_view() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        let name = CString::new("active_users").unwrap();
        let col_id = CString::new("id").unwrap();
        let cols: [*const c_char; 1] = [col_id.as_ptr()];
        let view = SyntaqliteRelationDef {
            name: name.as_ptr(),
            columns: cols.as_ptr(),
            column_count: 1,
        };
        syntaqlite_validator_add_views(v, &raw const view, 1);

        let n = analyze(v, "SELECT id FROM active_users");
        assert_eq!(n, 0, "view should be resolved");

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_views_lineage_is_partial() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        let name = CString::new("active_users").unwrap();
        let col_id = CString::new("id").unwrap();
        let cols: [*const c_char; 1] = [col_id.as_ptr()];
        let view = SyntaqliteRelationDef {
            name: name.as_ptr(),
            columns: cols.as_ptr(),
            column_count: 1,
        };
        syntaqlite_validator_add_views(v, &raw const view, 1);

        analyze(v, "SELECT id FROM active_users");
        assert_eq!(
            syntaqlite_validator_lineage_complete(v),
            0,
            "view lineage should be partial"
        );

        let view_count = syntaqlite_validator_statement_unexpanded_view_count(v, 0);
        assert_eq!(view_count, 1, "one unexpanded view expected");
        let views = syntaqlite_validator_statement_unexpanded_views(v, 0);
        assert!(!views.is_null());
        let first = &*views;
        assert_eq!(CStr::from_ptr(first.name).to_str().unwrap(), "active_users",);

        syntaqlite_validator_destroy(v);
    }
}

// ── load_schema_ddl ─────────────────────────────────────────────────

#[test]
#[expect(clippy::cast_possible_truncation)]
fn load_schema_ddl_registers_tables_and_views() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        let ddl = "CREATE TABLE users(id, name); CREATE VIEW active AS SELECT id FROM users;";
        let errors = syntaqlite_validator_load_schema_ddl(v, ddl.as_ptr().cast(), ddl.len() as u32);
        assert_eq!(errors, 0, "DDL should parse without errors");

        // Table should be resolved.
        let n = analyze(v, "SELECT id, name FROM users");
        assert_eq!(n, 0, "table from DDL should be resolved");

        // View should be resolved.
        let n = analyze(v, "SELECT id FROM active");
        assert_eq!(n, 0, "view from DDL should be resolved");

        syntaqlite_validator_destroy(v);
    }
}

#[test]
#[expect(clippy::cast_possible_truncation)]
fn load_schema_ddl_reports_parse_errors() {
    let v = syntaqlite_validator_create_sqlite();
    // SAFETY: FFI test.
    unsafe {
        let ddl = "CREATE TABLE users(id); NOT VALID SQL;";
        let errors = syntaqlite_validator_load_schema_ddl(v, ddl.as_ptr().cast(), ddl.len() as u32);
        assert!(errors > 0, "should report parse errors");

        // The valid DDL before the error should still be registered.
        let n = analyze(v, "SELECT id FROM users");
        assert_eq!(n, 0, "valid DDL before error should still be registered");

        syntaqlite_validator_destroy(v);
    }
}

// ── Per-statement API ────────────────────────────────────────────────

// SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
unsafe fn c_str_val(ptr: *const c_char) -> String {
    if ptr.is_null() {
        String::new()
    } else {
        // SAFETY: caller guarantees `ptr` is a valid NUL-terminated C string.
        unsafe { CStr::from_ptr(ptr) }
            .to_str()
            .unwrap_or("")
            .to_owned()
    }
}

#[test]
fn statement_count_single() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT 1;");
        assert_eq!(syntaqlite_validator_statement_count(v), 1);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn statement_count_multiple() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT 1; SELECT 2; SELECT 3;");
        assert_eq!(syntaqlite_validator_statement_count(v), 3);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn statement_count_with_ddl() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "CREATE TABLE t (a INT); SELECT a FROM t;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_diagnostics_only_on_bad_statement() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "users", &["id", "name"]);
        analyze(v, "SELECT id FROM users; SELECT bogus FROM users;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);
        assert_eq!(syntaqlite_validator_statement_diagnostic_count(v, 0), 0);
        assert_eq!(syntaqlite_validator_statement_diagnostic_count(v, 1), 1);

        let diags = syntaqlite_validator_statement_diagnostics(v, 1);
        assert!(!diags.is_null());
        let msg = c_str_val((*diags).message);
        assert!(msg.contains("bogus"), "expected 'bogus' in: {msg}");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_diagnostics_parse_error() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT FROM; SELECT 1;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);
        assert!(syntaqlite_validator_statement_diagnostic_count(v, 0) > 0);
        assert_eq!(syntaqlite_validator_statement_diagnostic_count(v, 1), 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_lineage_select() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "t", &["a", "b"]);
        analyze(v, "SELECT a, b FROM t;");
        assert_eq!(syntaqlite_validator_statement_count(v), 1);
        assert_eq!(syntaqlite_validator_statement_column_lineage_count(v, 0), 2);

        let cols = syntaqlite_validator_statement_column_lineage(v, 0);
        assert!(!cols.is_null());
        assert_eq!(c_str_val((*cols).name), "a");
        assert_eq!(c_str_val((*cols.add(1)).name), "b");
        assert_eq!(c_str_val((*cols).origin.table), "t");
        assert_eq!(c_str_val((*cols).origin.column), "a");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_lineage_ddl_has_none() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "CREATE TABLE t (a INT); SELECT 1;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);
        assert_eq!(syntaqlite_validator_statement_column_lineage_count(v, 0), 0);
        assert_eq!(syntaqlite_validator_statement_column_lineage_count(v, 1), 1);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_relations_accessed() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "users", &["id"]);
        add_table(v, "orders", &["id"]);
        analyze(v, "SELECT id FROM users; SELECT id FROM orders;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);

        assert_eq!(syntaqlite_validator_statement_relation_count(v, 0), 1);
        let rels0 = syntaqlite_validator_statement_relations(v, 0);
        assert_eq!(c_str_val((*rels0).name), "users");

        assert_eq!(syntaqlite_validator_statement_relation_count(v, 1), 1);
        let rels1 = syntaqlite_validator_statement_relations(v, 1);
        assert_eq!(c_str_val((*rels1).name), "orders");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_defined_relations_create_table() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "CREATE TABLE foo (a INT); SELECT 1;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);
        assert_eq!(
            syntaqlite_validator_statement_defined_relation_count(v, 0),
            1
        );
        let defs = syntaqlite_validator_statement_defined_relations(v, 0);
        assert!(!defs.is_null());
        assert_eq!(c_str_val((*defs).name), "foo");
        assert_eq!((*defs).is_view, 0);
        assert_eq!(
            syntaqlite_validator_statement_defined_relation_count(v, 1),
            0
        );
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_defined_relations_create_view() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "CREATE VIEW v AS SELECT 1; SELECT 1;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);
        assert_eq!(
            syntaqlite_validator_statement_defined_relation_count(v, 0),
            1
        );
        let defs = syntaqlite_validator_statement_defined_relations(v, 0);
        assert_eq!(c_str_val((*defs).name), "v");
        assert_eq!((*defs).is_view, 1);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn out_of_bounds_returns_zero() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT 1;");
        assert_eq!(syntaqlite_validator_statement_diagnostic_count(v, 99), 0);
        assert!(syntaqlite_validator_statement_diagnostics(v, 99).is_null());
        assert_eq!(
            syntaqlite_validator_statement_column_lineage_count(v, 99),
            0
        );
        assert!(syntaqlite_validator_statement_column_lineage(v, 99).is_null());
        assert_eq!(syntaqlite_validator_statement_relation_count(v, 99), 0);
        assert!(syntaqlite_validator_statement_relations(v, 99).is_null());
        assert_eq!(
            syntaqlite_validator_statement_physical_table_count(v, 99),
            0
        );
        assert!(syntaqlite_validator_statement_physical_tables(v, 99).is_null());
        assert_eq!(
            syntaqlite_validator_statement_defined_relation_count(v, 99),
            0
        );
        assert!(syntaqlite_validator_statement_defined_relations(v, 99).is_null());
        assert!(syntaqlite_validator_statement_source(v, 99).is_null());
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn per_statement_source_text() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT 1; SELECT 2;");
        assert_eq!(syntaqlite_validator_statement_count(v), 2);

        let src0 = syntaqlite_validator_statement_source(v, 0);
        assert!(!src0.is_null());
        let s0 = CStr::from_ptr(src0).to_str().unwrap();
        assert!(s0.contains("SELECT 1"), "expected 'SELECT 1', got: {s0}");

        let src1 = syntaqlite_validator_statement_source(v, 1);
        assert!(!src1.is_null());
        let s1 = CStr::from_ptr(src1).to_str().unwrap();
        assert!(s1.contains("SELECT 2"), "expected 'SELECT 2', got: {s1}");

        // Out of bounds returns null.
        assert!(syntaqlite_validator_statement_source(v, 99).is_null());

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_module_resolver_callback_no_crash() {
    // A resolver that always returns NULL (module not found).
    unsafe extern "C" fn null_resolver(
        _module_path: *const c_char,
        _user_data: *mut std::ffi::c_void,
    ) -> *mut c_char {
        std::ptr::null_mut()
    }

    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();

        // Set a resolver callback — should not crash.
        syntaqlite_validator_set_module_resolver(v, Some(null_resolver), std::ptr::null_mut());
        analyze(v, "SELECT 1;");
        assert_eq!(syntaqlite_validator_diagnostic_count(v), 0);

        // Clear the resolver — should not crash.
        syntaqlite_validator_set_module_resolver(v, None, std::ptr::null_mut());
        analyze(v, "SELECT 1;");
        assert_eq!(syntaqlite_validator_diagnostic_count(v), 0);

        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn aggregated_diagnostics_matches_per_statement_sum() {
    // SAFETY: FFI test — all pointers are valid and calls follow documented API contracts.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "t", &["a"]);
        let total = analyze(v, "SELECT bogus FROM t; SELECT also_bad FROM t;");
        let stmt_count = syntaqlite_validator_statement_count(v);
        let mut per_stmt_total = 0u32;
        for i in 0..stmt_count {
            per_stmt_total += syntaqlite_validator_statement_diagnostic_count(v, i);
        }
        assert_eq!(total, per_stmt_total);
        syntaqlite_validator_destroy(v);
    }
}

// ── Check level / strict schema / suggestion threshold ───────────────

/// Helper: severity of the first diagnostic as the public u32 code.
unsafe fn first_severity(v: *const SyntaqliteValidator) -> u32 {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe {
        let ptr = syntaqlite_validator_diagnostics(v);
        assert!(!ptr.is_null());
        (*ptr).severity
    }
}

#[test]
fn set_check_level_allow_suppresses_unknown_table() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        let name = CString::new("unknown-table").unwrap();
        let rc = syntaqlite_validator_set_check_level(v, name.as_ptr(), SYNTAQLITE_CHECK_ALLOW);
        assert_eq!(rc, 0);
        let n = analyze(v, "SELECT 1 FROM no_such");
        assert_eq!(n, 0, "allow should suppress the diagnostic");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_check_level_deny_raises_unknown_table_to_error() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        let name = CString::new("unknown-table").unwrap();
        let rc = syntaqlite_validator_set_check_level(v, name.as_ptr(), SYNTAQLITE_CHECK_DENY);
        assert_eq!(rc, 0);
        let n = analyze(v, "SELECT 1 FROM no_such");
        assert!(n >= 1);
        assert_eq!(first_severity(v), SEVERITY_ERROR);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_check_level_unknown_name_returns_error() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        let name = CString::new("not-a-real-category").unwrap();
        let rc = syntaqlite_validator_set_check_level(v, name.as_ptr(), SYNTAQLITE_CHECK_WARN);
        assert_eq!(rc, -1);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_check_level_schema_group_promotes_all_schema_checks() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        let name = CString::new("schema").unwrap();
        let rc = syntaqlite_validator_set_check_level(v, name.as_ptr(), SYNTAQLITE_CHECK_DENY);
        assert_eq!(rc, 0);
        let n = analyze(v, "SELECT 1 FROM no_such");
        assert!(n >= 1);
        assert_eq!(first_severity(v), SEVERITY_ERROR);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_strict_schema_raises_unknown_table_to_error() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        syntaqlite_validator_set_strict_schema(v, 1);
        let n = analyze(v, "SELECT 1 FROM no_such");
        assert!(n >= 1);
        assert_eq!(first_severity(v), SEVERITY_ERROR);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_strict_schema_zero_reverts_to_warning() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        syntaqlite_validator_set_strict_schema(v, 1);
        syntaqlite_validator_set_strict_schema(v, 0);
        let n = analyze(v, "SELECT 1 FROM no_such");
        assert!(n >= 1);
        assert_eq!(first_severity(v), SEVERITY_WARNING);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_suggestion_threshold_zero_disables_help() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "users", &["id"]);
        syntaqlite_validator_set_suggestion_threshold(v, 0);
        // "usr" is 1 edit from "users" — would normally trigger "did you mean".
        analyze(v, "SELECT 1 FROM usr");
        let rendered = render(v, None);
        assert!(
            !rendered.contains("did you mean"),
            "threshold=0 should suppress suggestions; got: {rendered}"
        );
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn user_config_survives_schema_load() {
    // Setting a custom suggestion threshold + allow-override for a
    // category before schema load must persist after add_tables.
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        syntaqlite_validator_set_suggestion_threshold(v, 7);
        let cat = CString::new("unknown-table").unwrap();
        syntaqlite_validator_set_check_level(v, cat.as_ptr(), SYNTAQLITE_CHECK_ALLOW);
        add_table(v, "users", &["id"]);
        let n = analyze(v, "SELECT 1 FROM no_such");
        assert_eq!(n, 0, "allow set before schema load must persist");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn set_suggestion_threshold_permissive_emits_help() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "users", &["id"]);
        syntaqlite_validator_set_suggestion_threshold(v, 5);
        analyze(v, "SELECT 1 FROM usr");
        let rendered = render(v, None);
        assert!(
            rendered.contains("did you mean") && rendered.contains("users"),
            "threshold=5 should emit a 'users' suggestion; got: {rendered}"
        );
        syntaqlite_validator_destroy(v);
    }
}

// ── Diagnostic kind codes ────────────────────────────────────────────

/// Helper: find the first diagnostic with the given `kind_code`, panicking
/// if none is present.
unsafe fn find_code(v: *const SyntaqliteValidator, code: u32) -> bool {
    // SAFETY: FFI test — pointer obtained from `syntaqlite_validator_create_sqlite`.
    unsafe {
        let n = syntaqlite_validator_diagnostic_count(v) as usize;
        let ptr = syntaqlite_validator_diagnostics(v);
        (0..n).any(|i| (*ptr.add(i)).kind_code == code)
    }
}

#[test]
fn kind_code_parse_error() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELCT 1");
        assert!(
            find_code(v, DIAG_CODE_PARSE_ERROR),
            "expected ParseError kind_code",
        );
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn kind_code_unknown_table() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT id FROM no_such_table");
        assert!(find_code(v, DIAG_CODE_UNKNOWN_TABLE));
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn kind_code_unknown_column() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_table(v, "t", &["a"]);
        analyze(v, "SELECT bogus FROM t");
        assert!(find_code(v, DIAG_CODE_UNKNOWN_COLUMN));
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn kind_code_unknown_function() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT no_such_fn(1)");
        assert!(find_code(v, DIAG_CODE_UNKNOWN_FUNCTION));
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn kind_code_function_arity() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        // substr accepts 0/2/3 args; calling with 1 triggers WrongArity.
        analyze(v, "SELECT substr('x')");
        assert!(
            find_code(v, DIAG_CODE_FUNCTION_ARITY),
            "expected FunctionArity kind_code",
        );
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn kind_code_cte_column_count_mismatch() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "WITH cte(a) AS (SELECT 1, 2) SELECT a FROM cte");
        assert!(find_code(v, DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH));
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn kind_code_per_statement_matches_aggregate() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        analyze(v, "SELECT id FROM no_such_table");
        let ptr = syntaqlite_validator_statement_diagnostics(v, 0);
        assert!(!ptr.is_null());
        assert_eq!((*ptr).kind_code, DIAG_CODE_UNKNOWN_TABLE);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn diagnostic_code_to_c_covers_all_variants() {
    use super::super::diagnostics::DiagnosticMessage;
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::ParseError("x".into())),
        DIAG_CODE_PARSE_ERROR,
    );
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::UnknownTable { name: "t".into() }),
        DIAG_CODE_UNKNOWN_TABLE,
    );
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::UnknownColumn {
            column: "c".into(),
            table: None,
        }),
        DIAG_CODE_UNKNOWN_COLUMN,
    );
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::UnknownFunction { name: "f".into() }),
        DIAG_CODE_UNKNOWN_FUNCTION,
    );
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::UnknownModule { name: "m".into() }),
        DIAG_CODE_UNKNOWN_MODULE,
    );
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::FunctionArity {
            name: "f".into(),
            expected: vec![1],
            got: 0,
        }),
        DIAG_CODE_FUNCTION_ARITY,
    );
    assert_eq!(
        diagnostic_code_to_c(&DiagnosticMessage::CteColumnCountMismatch {
            name: "c".into(),
            declared: 1,
            actual: 2,
        }),
        DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH,
    );
}

// ── Custom function / table-function registration ────────────────────

unsafe fn add_scalar(v: *mut SyntaqliteValidator, name: &str, arity_kind: u32, arity_value: u32) {
    // SAFETY: FFI test.
    unsafe {
        let c_name = CString::new(name).unwrap();
        syntaqlite_validator_add_function_overload(
            v,
            c_name.as_ptr(),
            SYNTAQLITE_FUNCTION_SCALAR,
            arity_kind,
            arity_value,
        );
    }
}

unsafe fn add_aggregate(
    v: *mut SyntaqliteValidator,
    name: &str,
    arity_kind: u32,
    arity_value: u32,
) {
    // SAFETY: FFI test.
    unsafe {
        let c_name = CString::new(name).unwrap();
        syntaqlite_validator_add_function_overload(
            v,
            c_name.as_ptr(),
            SYNTAQLITE_FUNCTION_AGGREGATE,
            arity_kind,
            arity_value,
        );
    }
}

unsafe fn add_window(v: *mut SyntaqliteValidator, name: &str, arity_kind: u32, arity_value: u32) {
    // SAFETY: FFI test.
    unsafe {
        let c_name = CString::new(name).unwrap();
        syntaqlite_validator_add_function_overload(
            v,
            c_name.as_ptr(),
            SYNTAQLITE_FUNCTION_WINDOW,
            arity_kind,
            arity_value,
        );
    }
}

unsafe fn add_tfn(
    v: *mut SyntaqliteValidator,
    name: &str,
    arity_kind: u32,
    arity_value: u32,
    cols: &[&str],
) {
    // SAFETY: FFI test.
    unsafe {
        let c_name = CString::new(name).unwrap();
        let c_cols: Vec<CString> = cols.iter().map(|c| CString::new(*c).unwrap()).collect();
        let c_col_ptrs: Vec<*const c_char> = c_cols.iter().map(|c| c.as_ptr()).collect();
        let (cols_ptr, col_count) = if c_col_ptrs.is_empty() {
            (std::ptr::null(), 0u32)
        } else {
            (
                c_col_ptrs.as_ptr(),
                u32::try_from(c_col_ptrs.len()).expect("test column count fits in u32"),
            )
        };
        syntaqlite_validator_add_table_function(
            v,
            c_name.as_ptr(),
            arity_kind,
            arity_value,
            cols_ptr,
            col_count,
        );
    }
}

#[test]
fn add_function_overload_registers_scalar() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "my_udf", SYNTAQLITE_ARITY_EXACT, 1);
        let n = analyze(v, "SELECT my_udf(1)");
        assert_eq!(n, 0, "custom scalar should validate clean");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_wrong_arity_emits_diagnostic() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "my_udf", SYNTAQLITE_ARITY_EXACT, 1);
        analyze(v, "SELECT my_udf(1, 2)");
        let ptr = syntaqlite_validator_diagnostics(v);
        assert!(!ptr.is_null());
        assert_eq!((*ptr).kind_code, DIAG_CODE_FUNCTION_ARITY);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_multiple_arities_build_overload_set() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "my_udf", SYNTAQLITE_ARITY_EXACT, 1);
        add_scalar(v, "my_udf", SYNTAQLITE_ARITY_EXACT, 3);
        // Either arity 1 or 3 should be accepted; arity 2 should fail.
        assert_eq!(analyze(v, "SELECT my_udf(1)"), 0);
        assert_eq!(analyze(v, "SELECT my_udf(1, 2, 3)"), 0);
        let n = analyze(v, "SELECT my_udf(1, 2)");
        assert!(n > 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_at_least_accepts_variadic() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "vararg", SYNTAQLITE_ARITY_AT_LEAST, 2);
        assert_eq!(analyze(v, "SELECT vararg(1, 2)"), 0);
        assert_eq!(analyze(v, "SELECT vararg(1, 2, 3, 4)"), 0);
        // One arg is below the minimum.
        assert!(analyze(v, "SELECT vararg(1)") > 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_any_accepts_any_arity() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "anything", SYNTAQLITE_ARITY_ANY, 0);
        assert_eq!(analyze(v, "SELECT anything()"), 0);
        assert_eq!(analyze(v, "SELECT anything(1, 2, 3)"), 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_aggregate_registers() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_aggregate(v, "my_sum", SYNTAQLITE_ARITY_EXACT, 1);
        assert_eq!(analyze(v, "SELECT my_sum(1)"), 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_window_registers() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_window(v, "my_rank", SYNTAQLITE_ARITY_EXACT, 0);
        assert_eq!(analyze(v, "SELECT my_rank() OVER ()"), 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_unknown_category_is_noop() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        let c_name = CString::new("garbage").unwrap();
        // Bogus category ordinal — must not register anything or crash.
        syntaqlite_validator_add_function_overload(v, c_name.as_ptr(), 99, SYNTAQLITE_ARITY_ANY, 0);
        let n = analyze(v, "SELECT garbage()");
        assert!(n > 0, "unregistered garbage() should still be unknown");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_persists_across_analyze_calls() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "my_udf", SYNTAQLITE_ARITY_EXACT, 1);
        assert_eq!(analyze(v, "SELECT my_udf(1)"), 0);
        assert_eq!(analyze(v, "SELECT my_udf(1) FROM (SELECT 1)"), 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_function_overload_reset_catalog_clears_registration() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_scalar(v, "my_udf", SYNTAQLITE_ARITY_EXACT, 1);
        syntaqlite_validator_reset_catalog(v);
        let n = analyze(v, "SELECT my_udf(1)");
        assert!(n > 0, "my_udf should be unknown after reset");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_table_function_registers_with_columns() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_tfn(v, "json_each", SYNTAQLITE_ARITY_ANY, 0, &["key", "value"]);
        let n = analyze(v, "SELECT key, value FROM json_each('[]')");
        assert_eq!(n, 0, "json_each should resolve with declared columns");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_table_function_without_columns_accepts_any_ref() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_tfn(v, "some_tfn", SYNTAQLITE_ARITY_ANY, 0, &[]);
        let n = analyze(v, "SELECT * FROM some_tfn(1, 2)");
        assert_eq!(n, 0, "tfn without columns should accept any query");
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_table_function_null_cols_accepted() {
    // SAFETY: FFI test — pass NULL directly for output_columns.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        let c_name = CString::new("raw_tfn").unwrap();
        syntaqlite_validator_add_table_function(
            v,
            c_name.as_ptr(),
            SYNTAQLITE_ARITY_ANY,
            0,
            std::ptr::null(),
            0,
        );
        let n = analyze(v, "SELECT * FROM raw_tfn()");
        assert_eq!(n, 0);
        syntaqlite_validator_destroy(v);
    }
}

#[test]
fn add_table_function_reset_catalog_clears_registration() {
    // SAFETY: FFI test.
    unsafe {
        let v = syntaqlite_validator_create_sqlite();
        add_tfn(v, "tfn", SYNTAQLITE_ARITY_ANY, 0, &["x"]);
        syntaqlite_validator_reset_catalog(v);
        let n = analyze(v, "SELECT x FROM tfn()");
        assert!(n > 0, "tfn should be unknown after reset");
        syntaqlite_validator_destroy(v);
    }
}
