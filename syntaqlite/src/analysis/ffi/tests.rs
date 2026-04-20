// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::ffi::{CStr, CString, c_char};

use super::SyntaqliteAnalyzer;
use super::analyze::*;
use super::catalog::*;
use super::codes::*;
use super::config::*;
use super::lifecycle::*;
use super::results::*;
use super::statement::*;
use super::types::*;

// All FFI calls here go through the `Validator` RAII guard: the inner
// `*mut SyntaqliteAnalyzer` is obtained from
// `syntaqlite_analyzer_create_sqlite` (non-null, valid for the guard's
// lifetime) and C strings passed to the FFI are kept alive across the
// call. Helper methods carry a single SAFETY comment on that basis.

/// RAII handle for a sqlite validator; destroys on drop.
struct Validator(*mut SyntaqliteAnalyzer);

impl Validator {
    fn new() -> Self {
        let v = syntaqlite_analyzer_create_sqlite();
        assert!(!v.is_null());
        Self(v)
    }

    fn as_ptr(&self) -> *mut SyntaqliteAnalyzer {
        self.0
    }

    /// Analyze SQL and return the diagnostic count.
    fn analyze(&self, sql: &str) -> u32 {
        // SAFETY: handle from `syntaqlite_analyzer_create_sqlite`; `sql` outlives the call.
        unsafe {
            syntaqlite_analyzer_analyze(
                self.0,
                sql.as_ptr().cast(),
                u32::try_from(sql.len()).unwrap(),
            )
        }
    }

    /// Aggregate diagnostic count.
    fn diag_count(&self) -> u32 {
        // SAFETY: handle from `syntaqlite_analyzer_create_sqlite`.
        unsafe { syntaqlite_analyzer_diagnostic_count(self.0) }
    }

    /// Raw pointer to the aggregate diagnostics buffer (may be null).
    fn diags_ptr(&self) -> *const SyntaqliteDiagnostic {
        // SAFETY: handle from `syntaqlite_analyzer_create_sqlite`.
        unsafe { syntaqlite_analyzer_diagnostics(self.0) }
    }

    /// Read the i-th diagnostic message as an owned Rust string; panics
    /// unless at least `i + 1` diagnostics are present.
    fn diag_msg(&self, i: usize) -> String {
        // SAFETY: buffer non-null (asserted) with count >= i + 1 by caller.
        unsafe {
            let ptr = syntaqlite_analyzer_diagnostics(self.0);
            assert!(!ptr.is_null());
            CStr::from_ptr((*ptr.add(i)).message)
                .to_str()
                .unwrap()
                .to_owned()
        }
    }

    /// Borrow the first diagnostic; panics if there are none.
    fn first_diag(&self) -> &SyntaqliteDiagnostic {
        // SAFETY: buffer non-null (asserted) with count >= 1 by caller.
        unsafe {
            let ptr = syntaqlite_analyzer_diagnostics(self.0);
            assert!(!ptr.is_null());
            &*ptr
        }
    }

    fn first_severity(&self) -> u32 {
        self.first_diag().severity
    }

    /// True if any aggregate diagnostic has the given kind code.
    fn has_code(&self, code: u32) -> bool {
        // SAFETY: handle from `syntaqlite_analyzer_create_sqlite`.
        unsafe {
            let n = syntaqlite_analyzer_diagnostic_count(self.0) as usize;
            let ptr = syntaqlite_analyzer_diagnostics(self.0);
            (0..n).any(|i| (*ptr.add(i)).kind_code == code)
        }
    }

    /// Render diagnostics as a Rust string; `file` is an optional label.
    fn render(&self, file: Option<&CStr>) -> String {
        // SAFETY: handle from `syntaqlite_analyzer_create_sqlite`; `file`'s CStr outlives the call.
        unsafe {
            let file_ptr = file.map_or(std::ptr::null(), CStr::as_ptr);
            let ptr = syntaqlite_analyzer_render_diagnostics(self.0, file_ptr);
            assert!(!ptr.is_null());
            CStr::from_ptr(ptr).to_str().unwrap().to_owned()
        }
    }

    fn set_check_level(&self, name: &str, level: u32) -> i32 {
        let c_name = CString::new(name).unwrap();
        // SAFETY: handle from `syntaqlite_analyzer_create_sqlite`; `c_name` outlives the call.
        unsafe { syntaqlite_analyzer_set_check_level(self.0, c_name.as_ptr(), level) }
    }

    /// Register one relation (table or view); empty `cols` means NULL
    /// columns (accepts any ref).
    fn add_relation(&self, name: &str, cols: &[&str], is_view: bool) {
        let c_name = CString::new(name).unwrap();
        let (_keep_cstrs, _keep_ptrs, cols_ptr, col_count) = cstring_vec(cols);
        let rel = SyntaqliteRelationDef {
            name: c_name.as_ptr(),
            columns: cols_ptr,
            column_count: col_count,
        };
        // SAFETY: FFI test — handle from `syntaqlite_analyzer_create_sqlite`;
        // all C strings and the `SyntaqliteRelationDef` outlive the call.
        unsafe {
            if is_view {
                syntaqlite_analyzer_add_views(self.0, &raw const rel, 1);
            } else {
                syntaqlite_analyzer_add_tables(self.0, &raw const rel, 1);
            }
        }
    }

    fn add_table(&self, name: &str, cols: &[&str]) {
        self.add_relation(name, cols, false);
    }
    fn add_view(&self, name: &str, cols: &[&str]) {
        self.add_relation(name, cols, true);
    }

    #[expect(clippy::cast_possible_truncation)]
    fn load_schema_ddl(&self, ddl: &str) -> u32 {
        // SAFETY: FFI test — handle from `syntaqlite_analyzer_create_sqlite`;
        // `ddl` outlives the call.
        unsafe {
            syntaqlite_analyzer_load_schema_ddl(self.0, ddl.as_ptr().cast(), ddl.len() as u32)
        }
    }

    fn add_function_overload(&self, name: &str, category: u32, arity_kind: u32, arity_value: u32) {
        let c_name = CString::new(name).unwrap();
        // SAFETY: FFI test — handle from `syntaqlite_analyzer_create_sqlite`;
        // `c_name` outlives the call.
        unsafe {
            syntaqlite_analyzer_add_function_overload(
                self.0,
                c_name.as_ptr(),
                category,
                arity_kind,
                arity_value,
            );
        }
    }

    fn add_scalar(&self, n: &str, ak: u32, av: u32) {
        self.add_function_overload(n, SYNTAQLITE_FUNCTION_SCALAR, ak, av);
    }
    fn add_aggregate(&self, n: &str, ak: u32, av: u32) {
        self.add_function_overload(n, SYNTAQLITE_FUNCTION_AGGREGATE, ak, av);
    }
    fn add_window(&self, n: &str, ak: u32, av: u32) {
        self.add_function_overload(n, SYNTAQLITE_FUNCTION_WINDOW, ak, av);
    }

    fn add_tfn(&self, name: &str, arity_kind: u32, arity_value: u32, cols: &[&str]) {
        let c_name = CString::new(name).unwrap();
        let (_keep_cstrs, _keep_ptrs, cols_ptr, col_count) = cstring_vec(cols);
        // SAFETY: FFI test — handle from `syntaqlite_analyzer_create_sqlite`;
        // all C strings outlive the call.
        unsafe {
            syntaqlite_analyzer_add_table_function(
                self.0,
                c_name.as_ptr(),
                arity_kind,
                arity_value,
                cols_ptr,
                col_count,
            );
        }
    }
}

/// Build an FFI-ready column array from `&[&str]`, returning the owning
/// `Vec<CString>` + pointer vector (must stay alive across the call)
/// plus the `(cols_ptr, col_count)` pair to pass to the FFI.
#[expect(clippy::cast_possible_truncation)]
fn cstring_vec(cols: &[&str]) -> (Vec<CString>, Vec<*const c_char>, *const *const c_char, u32) {
    let c_cols: Vec<CString> = cols.iter().map(|c| CString::new(*c).unwrap()).collect();
    let c_col_ptrs: Vec<*const c_char> = c_cols.iter().map(|c| c.as_ptr()).collect();
    let (cols_ptr, col_count) = if c_col_ptrs.is_empty() {
        (std::ptr::null(), 0u32)
    } else {
        (c_col_ptrs.as_ptr(), c_col_ptrs.len() as u32)
    };
    (c_cols, c_col_ptrs, cols_ptr, col_count)
}

/// Forward thin FFI wrappers onto `Validator` with centralised SAFETY.
/// Each entry expands to a method that calls the named FFI function;
/// the generated `unsafe` block gets a SAFETY comment explaining that
/// the handle comes from `syntaqlite_analyzer_create_sqlite` and is
/// kept alive for the guard's lifetime.
macro_rules! forward {
    (
        const $(,$const_name:ident -> $const_ret:ty = $const_fn:ident)* ;
        mut $(,$mut_name:ident($($mut_arg:ident: $mut_ty:ty),*) -> $mut_ret:ty = $mut_fn:ident)* ;
    ) => {
        impl Validator {
            $(
                fn $const_name(&self) -> $const_ret {
                    // SAFETY: `self.0` from `syntaqlite_analyzer_create_sqlite`;
                    // buffer lifetimes tied to the `Validator` guard.
                    unsafe { $const_fn(self.0.cast_const()) }
                }
            )*
            $(
                fn $mut_name(&self $(, $mut_arg: $mut_ty)*) -> $mut_ret {
                    // SAFETY: `self.0` from `syntaqlite_analyzer_create_sqlite`;
                    // buffer lifetimes tied to the `Validator` guard.
                    unsafe { $mut_fn(self.0 $(, $mut_arg)*) }
                }
            )*
        }
    };
}

forward! {
    const
        ,lineage_complete -> u32 = syntaqlite_analyzer_lineage_complete
        ,column_lineage_count -> u32 = syntaqlite_analyzer_column_lineage_count
        ,column_lineage -> *const SyntaqliteColumnLineage = syntaqlite_analyzer_column_lineage
        ,relation_count -> u32 = syntaqlite_analyzer_relation_count
        ,relations -> *const SyntaqliteRelationAccess = syntaqlite_analyzer_relations
        ,physical_table_count -> u32 = syntaqlite_analyzer_physical_table_count
        ,physical_tables -> *const SyntaqlitePhysicalTableAccess = syntaqlite_analyzer_physical_tables
    ;
    mut
        ,set_mode(mode: u32) -> () = syntaqlite_analyzer_set_mode
        ,set_strict_schema(enabled: u32) -> () = syntaqlite_analyzer_set_strict_schema
        ,set_suggestion_threshold(t: u32) -> () = syntaqlite_analyzer_set_suggestion_threshold
        ,reset_catalog() -> () = syntaqlite_analyzer_reset_catalog
        ,stmt_count() -> u32 = syntaqlite_analyzer_statement_count
        ,stmt_diag_count(i: u32) -> u32 = syntaqlite_analyzer_statement_diagnostic_count
        ,stmt_diags(i: u32) -> *const SyntaqliteDiagnostic = syntaqlite_analyzer_statement_diagnostics
        ,stmt_lineage_count(i: u32) -> u32 = syntaqlite_analyzer_statement_column_lineage_count
        ,stmt_lineage(i: u32) -> *const SyntaqliteColumnLineage = syntaqlite_analyzer_statement_column_lineage
        ,stmt_relation_count(i: u32) -> u32 = syntaqlite_analyzer_statement_relation_count
        ,stmt_relations(i: u32) -> *const SyntaqliteRelationAccess = syntaqlite_analyzer_statement_relations
        ,stmt_physical_table_count(i: u32) -> u32 = syntaqlite_analyzer_statement_physical_table_count
        ,stmt_physical_tables(i: u32) -> *const SyntaqlitePhysicalTableAccess = syntaqlite_analyzer_statement_physical_tables
        ,stmt_defined_count(i: u32) -> u32 = syntaqlite_analyzer_statement_defined_relation_count
        ,stmt_defined(i: u32) -> *const SyntaqliteDefinedRelation = syntaqlite_analyzer_statement_defined_relations
        ,stmt_source(i: u32) -> *const c_char = syntaqlite_analyzer_statement_source
        ,stmt_unexpanded_view_count(i: u32) -> u32 = syntaqlite_analyzer_statement_unexpanded_view_count
        ,stmt_unexpanded_views(i: u32) -> *const SyntaqliteUnexpandedView = syntaqlite_analyzer_statement_unexpanded_views
    ;
}

impl Drop for Validator {
    fn drop(&mut self) {
        if !self.0.is_null() {
            // SAFETY: FFI test — handle from `syntaqlite_analyzer_create_sqlite`;
            // destroyed exactly once at end of scope.
            unsafe { syntaqlite_analyzer_destroy(self.0) };
        }
    }
}

/// Read a nullable `*const c_char` as an owned `String`; empty if null.
fn c_str_val(ptr: *const c_char) -> String {
    if ptr.is_null() {
        String::new()
    } else {
        // SAFETY: FFI test — caller guarantees `ptr` is a valid
        // NUL-terminated C string owned by the validator state.
        unsafe { CStr::from_ptr(ptr) }
            .to_str()
            .unwrap_or("")
            .to_owned()
    }
}

/// Snapshot of the i-th `SyntaqliteColumnLineage` row at `ptr` as
/// `(name, origin.table, origin.column, index)`, with null C strings
/// mapped to empty Rust strings.
///
/// # Safety
///
/// `ptr` must point to at least `i + 1` valid rows owned by the
/// validator state.
unsafe fn col_at(ptr: *const SyntaqliteColumnLineage, i: usize) -> (String, String, String, u32) {
    // SAFETY: caller upholds the contract in the doc comment.
    unsafe {
        let c = &*ptr.add(i);
        (
            c_str_val(c.name),
            c_str_val(c.origin.table),
            c_str_val(c.origin.column),
            c.index,
        )
    }
}

// ── Lifecycle ─────────────────────────────────────────────────────────

#[test]
fn create_and_destroy() {
    let _v = Validator::new();
}

#[test]
fn null_destroy_is_noop() {
    // SAFETY: documented contract — destroying a null pointer is a no-op.
    unsafe { syntaqlite_analyzer_destroy(std::ptr::null_mut()) };
}

// ── Analysis: clean SQL ───────────────────────────────────────────────

#[test]
fn valid_sql_produces_no_diagnostics() {
    let v = Validator::new();
    assert_eq!(v.analyze("SELECT 1"), 0);
    assert_eq!(v.diag_count(), 0);
    assert!(v.diags_ptr().is_null());
}

// ── Analysis: unknown table ───────────────────────────────────────────

#[test]
fn unknown_table_produces_diagnostic() {
    let v = Validator::new();
    let sql = "SELECT id FROM no_such_table";
    assert!(v.analyze(sql) > 0, "expected at least one diagnostic");
    let msg = v.diag_msg(0);
    assert!(
        msg.contains("no_such_table"),
        "diag should mention table: {msg}"
    );
    let d = v.first_diag();
    assert_eq!(d.severity, SEVERITY_WARNING);
    assert!(d.start_offset < d.end_offset);
    assert!((d.end_offset as usize) <= sql.len());
}

// ── Catalog: add_tables ───────────────────────────────────────────────

#[test]
fn add_tables_resolves_unknown_table() {
    let v = Validator::new();
    assert!(v.analyze("SELECT id FROM users") > 0);
    v.add_table("users", &["id", "name"]);
    assert_eq!(
        v.analyze("SELECT id FROM users"),
        0,
        "table should be resolved after add_tables"
    );
}

#[test]
fn add_tables_with_null_columns_accepts_any_column() {
    let v = Validator::new();
    v.add_table("events", &[]);
    // Any column reference should be accepted (unknown-columns table).
    assert_eq!(v.analyze("SELECT anything, goes FROM events"), 0);
}

#[test]
fn add_tables_wrong_column_produces_diagnostic() {
    let v = Validator::new();
    v.add_table("users", &["id"]);
    let n = v.analyze("SELECT nonexistent FROM users");
    assert!(
        n > 0,
        "referencing a bad column should produce a diagnostic"
    );
    let msg = v.diag_msg(0);
    assert!(
        msg.contains("nonexistent"),
        "should mention the column: {msg}"
    );
}

// ── Catalog: reset ────────────────────────────────────────────────────

#[test]
fn reset_catalog_removes_tables() {
    let v = Validator::new();
    v.add_table("users", &[]);
    assert_eq!(v.analyze("SELECT 1 FROM users"), 0);
    v.reset_catalog();
    assert!(
        v.analyze("SELECT 1 FROM users") > 0,
        "table gone after reset"
    );
}

// ── Analysis mode ─────────────────────────────────────────────────────

#[test]
fn execute_mode_accumulates_ddl() {
    let v = Validator::new();
    v.set_mode(1); // Execute
    v.analyze("CREATE TABLE t(x)");
    // DDL from prior call is visible.
    assert_eq!(
        v.analyze("SELECT x FROM t"),
        0,
        "DDL should persist in execute mode"
    );
}

#[test]
fn document_mode_resets_ddl_between_calls() {
    let v = Validator::new();
    // Document mode is the default (mode=0).
    v.analyze("CREATE TABLE t(x)");
    assert!(
        v.analyze("SELECT x FROM t") > 0,
        "DDL should NOT persist in doc mode"
    );
}

// ── Reuse across calls ────────────────────────────────────────────────

#[test]
fn successive_analyze_calls_replace_diagnostics() {
    let v = Validator::new();
    assert!(v.analyze("SELECT 1 FROM bad_table") > 0);
    assert_eq!(v.analyze("SELECT 1"), 0);
    assert_eq!(v.diag_count(), 0);
    assert!(v.diags_ptr().is_null());
}

// ── Diagnostic rendering ──────────────────────────────────────────────

#[test]
fn render_diagnostics_with_file_label() {
    let v = Validator::new();
    assert!(v.analyze("SELECT 1 FROM bad") > 0);
    let file = CString::new("test.sql").unwrap();
    let r = v.render(Some(&file));
    assert!(r.contains("test.sql"), "file label: {r}");
    assert!(r.contains("bad"), "table name: {r}");
    assert!(
        r.contains("warning") || r.contains("error"),
        "severity: {r}"
    );
}

#[test]
fn render_diagnostics_with_null_file_uses_default() {
    let v = Validator::new();
    v.analyze("SELECT 1 FROM bad");
    let r = v.render(None);
    assert!(r.contains("<input>"), "should use default label: {r}");
}

#[test]
fn render_diagnostics_empty_when_no_errors() {
    let v = Validator::new();
    v.analyze("SELECT 1");
    let r = v.render(None);
    assert!(r.is_empty(), "should be empty for clean SQL: {r}");
}

#[test]
fn render_diagnostics_shows_multiple_issues() {
    let v = Validator::new();
    v.add_table("t", &["x"]);
    let n = v.analyze("SELECT bad1, bad2 FROM t");
    assert!(n >= 2, "expected at least 2 diagnostics, got {n}");
    let r = v.render(None);
    assert!(r.contains("bad1"), "should mention bad1: {r}");
    assert!(r.contains("bad2"), "should mention bad2: {r}");
}

#[test]
fn render_replaces_previous_render() {
    let v = Validator::new();
    v.analyze("SELECT 1 FROM alpha");
    let r1 = v.render(None);
    assert!(r1.contains("alpha"));
    // Previous render is replaced.
    v.analyze("SELECT 1 FROM beta");
    let r2 = v.render(None);
    assert!(r2.contains("beta"));
    assert!(!r2.contains("alpha"), "old render should be gone: {r2}");
}

// ── Strict schema (severity promotion) ─────────────────────────────

#[test]
fn no_schema_unknown_table_is_warning() {
    let v = Validator::new();
    // No tables added — empty catalog, strict_schema is false.
    v.analyze("SELECT 1 FROM bad_table");
    assert_eq!(v.first_severity(), SEVERITY_WARNING); // warning without schema
}

#[test]
fn with_schema_unknown_column_is_error() {
    let v = Validator::new();
    v.add_table("users", &["id"]);
    v.set_strict_schema(1);
    v.analyze("SELECT bogus FROM users");
    assert_eq!(v.first_severity(), SEVERITY_ERROR, "error with schema");
}

#[test]
fn with_schema_unknown_table_is_error() {
    let v = Validator::new();
    v.add_table("users", &[]);
    v.set_strict_schema(1);
    // Query a table that isn't registered.
    v.analyze("SELECT 1 FROM nonexistent");
    assert_eq!(v.first_severity(), SEVERITY_ERROR); // unknown table -> error under schema
}

#[test]
fn reset_catalog_reverts_to_warning_severity() {
    let v = Validator::new();
    v.add_table("t", &[]);
    v.set_strict_schema(1);
    v.analyze("SELECT 1 FROM gone");
    assert_eq!(v.first_severity(), SEVERITY_ERROR);
    v.reset_catalog();
    v.analyze("SELECT 1 FROM gone");
    assert_eq!(v.first_severity(), SEVERITY_WARNING); // revert to warning after reset
}

// ── string_destroy ────────────────────────────────────────────────────

#[test]
fn string_destroy_null_is_noop() {
    // SAFETY: documented contract — destroying a null pointer is a no-op.
    unsafe { syntaqlite_string_destroy(std::ptr::null_mut()) };
}

// ── Lineage ─────────────────────────────────────────────────────────

#[test]
fn lineage_simple_select() {
    let v = Validator::new();
    v.add_table("users", &["id", "name"]);
    v.analyze("SELECT id, name FROM users");

    assert_eq!(v.lineage_complete(), 1);
    assert_eq!(v.column_lineage_count(), 2);
    let cols = v.column_lineage();
    assert!(!cols.is_null());
    // SAFETY: FFI test — `cols` non-null with count = 2 above.
    let (c0, c1) = unsafe { (col_at(cols, 0), col_at(cols, 1)) };
    assert_eq!(c0, ("id".into(), "users".into(), "id".into(), 0));
    assert_eq!(c1, ("name".into(), "users".into(), "name".into(), 1));
}

#[test]
fn lineage_non_query_returns_empty() {
    let v = Validator::new();
    v.analyze("CREATE TABLE t(x)");

    assert_eq!(v.column_lineage_count(), 0);
    assert!(v.column_lineage().is_null());
    assert_eq!(v.relation_count(), 0);
    assert!(v.relations().is_null());
    assert_eq!(v.physical_table_count(), 0);
    assert!(v.physical_tables().is_null());
}

#[test]
fn lineage_expression_has_null_origin() {
    let v = Validator::new();
    v.add_table("users", &["id"]);
    v.analyze("SELECT id + 1 AS x FROM users");

    assert_eq!(v.column_lineage_count(), 1);
    let cols = v.column_lineage();
    // SAFETY: FFI test — `cols` non-null (count = 1); buffer owned by state.
    unsafe {
        let c0 = &*cols;
        assert_eq!(CStr::from_ptr(c0.name).to_str().unwrap(), "x");
        // An expression column has no origin table or column.
        assert!(c0.origin.table.is_null());
        assert!(c0.origin.column.is_null());
    }
}

#[test]
fn lineage_relations_and_tables() {
    let v = Validator::new();
    v.add_table("users", &["id", "name"]);
    v.add_table("posts", &["id", "user_id"]);
    v.analyze("SELECT u.id FROM users u JOIN posts p ON u.id = p.user_id");

    let rels = v.relation_count();
    assert!(rels >= 2, "expected >= 2 relations, got {rels}");
    assert!(!v.relations().is_null());

    let tbls = v.physical_table_count();
    assert!(tbls >= 2, "expected >= 2 tables, got {tbls}");
    assert!(!v.physical_tables().is_null());
}

#[test]
fn lineage_reset_on_next_analyze() {
    let v = Validator::new();
    v.add_table("users", &["id"]);

    // First: SELECT → lineage present.
    v.analyze("SELECT id FROM users");
    assert!(v.column_lineage_count() > 0);

    // Second: non-query → lineage cleared.
    v.analyze("CREATE TABLE t(x)");
    assert_eq!(v.column_lineage_count(), 0);
    assert!(v.column_lineage().is_null());
}

// ── add_views ───────────────────────────────────────────────────────

#[test]
fn add_views_registers_view() {
    let v = Validator::new();
    v.add_view("active_users", &["id"]);

    let n = v.analyze("SELECT id FROM active_users");
    assert_eq!(n, 0, "view should be resolved");
}

#[test]
fn add_views_lineage_is_partial() {
    let v = Validator::new();
    v.add_view("active_users", &["id"]);
    v.analyze("SELECT id FROM active_users");

    assert_eq!(v.lineage_complete(), 0, "view lineage should be partial");
    assert_eq!(
        v.stmt_unexpanded_view_count(0),
        1,
        "one unexpanded view expected"
    );
    let views = v.stmt_unexpanded_views(0);
    assert!(!views.is_null());
    // SAFETY: FFI test — `views` non-null (count = 1); buffer owned by state.
    let name = unsafe { c_str_val((*views).name) };
    assert_eq!(name, "active_users");
}

// ── load_schema_ddl ─────────────────────────────────────────────────

#[test]
fn load_schema_ddl_registers_tables_and_views() {
    let v = Validator::new();
    let ddl = "CREATE TABLE users(id, name); CREATE VIEW active AS SELECT id FROM users;";
    assert_eq!(v.load_schema_ddl(ddl), 0, "DDL should parse without errors");
    assert_eq!(
        v.analyze("SELECT id, name FROM users"),
        0,
        "table from DDL should resolve"
    );
    assert_eq!(
        v.analyze("SELECT id FROM active"),
        0,
        "view from DDL should resolve"
    );
}

#[test]
fn load_schema_ddl_reports_parse_errors() {
    let v = Validator::new();
    let ddl = "CREATE TABLE users(id); NOT VALID SQL;";
    assert!(v.load_schema_ddl(ddl) > 0, "should report parse errors");
    // Valid DDL before the error is still registered.
    assert_eq!(v.analyze("SELECT id FROM users"), 0);
}

// ── Per-statement API ────────────────────────────────────────────────

#[test]
fn statement_count_single() {
    let v = Validator::new();
    v.analyze("SELECT 1;");
    assert_eq!(v.stmt_count(), 1);
}

#[test]
fn statement_count_multiple() {
    let v = Validator::new();
    v.analyze("SELECT 1; SELECT 2; SELECT 3;");
    assert_eq!(v.stmt_count(), 3);
}

#[test]
fn statement_count_with_ddl() {
    let v = Validator::new();
    v.analyze("CREATE TABLE t (a INT); SELECT a FROM t;");
    assert_eq!(v.stmt_count(), 2);
}

#[test]
fn per_statement_diagnostics_only_on_bad_statement() {
    let v = Validator::new();
    v.add_table("users", &["id", "name"]);
    v.analyze("SELECT id FROM users; SELECT bogus FROM users;");

    assert_eq!(v.stmt_count(), 2);
    assert_eq!(v.stmt_diag_count(0), 0);
    assert_eq!(v.stmt_diag_count(1), 1);

    let diags = v.stmt_diags(1);
    assert!(!diags.is_null());
    // SAFETY: FFI test — `diags` non-null per assertion; buffer owned by state.
    let msg = unsafe { c_str_val((*diags).message) };
    assert!(msg.contains("bogus"), "expected 'bogus' in: {msg}");
}

#[test]
fn per_statement_diagnostics_parse_error() {
    let v = Validator::new();
    v.analyze("SELECT FROM; SELECT 1;");
    assert_eq!(v.stmt_count(), 2);
    assert!(v.stmt_diag_count(0) > 0);
    assert_eq!(v.stmt_diag_count(1), 0);
}

#[test]
fn per_statement_lineage_select() {
    let v = Validator::new();
    v.add_table("t", &["a", "b"]);
    v.analyze("SELECT a, b FROM t;");

    assert_eq!(v.stmt_count(), 1);
    assert_eq!(v.stmt_lineage_count(0), 2);
    let cols = v.stmt_lineage(0);
    assert!(!cols.is_null());
    // SAFETY: FFI test — `cols` non-null with count = 2 above.
    let (c0, c1) = unsafe { (col_at(cols, 0), col_at(cols, 1)) };
    assert_eq!(c0.0, "a");
    assert_eq!(c1.0, "b");
    assert_eq!(c0.1, "t");
    assert_eq!(c0.2, "a");
}

#[test]
fn per_statement_lineage_ddl_has_none() {
    let v = Validator::new();
    v.analyze("CREATE TABLE t (a INT); SELECT 1;");
    assert_eq!(v.stmt_count(), 2);
    assert_eq!(v.stmt_lineage_count(0), 0);
    assert_eq!(v.stmt_lineage_count(1), 1);
}

#[test]
fn per_statement_relations_accessed() {
    let v = Validator::new();
    v.add_table("users", &["id"]);
    v.add_table("orders", &["id"]);
    v.analyze("SELECT id FROM users; SELECT id FROM orders;");

    assert_eq!(v.stmt_count(), 2);
    assert_eq!(v.stmt_relation_count(0), 1);
    assert_eq!(v.stmt_relation_count(1), 1);
    // SAFETY: FFI test — both counts = 1 above guarantee valid element 0.
    unsafe {
        assert_eq!(c_str_val((*v.stmt_relations(0)).name), "users");
        assert_eq!(c_str_val((*v.stmt_relations(1)).name), "orders");
    }
}

#[test]
fn per_statement_defined_relations_create_table() {
    let v = Validator::new();
    v.analyze("CREATE TABLE foo (a INT); SELECT 1;");
    assert_eq!(v.stmt_count(), 2);
    assert_eq!(v.stmt_defined_count(0), 1);
    let defs = v.stmt_defined(0);
    assert!(!defs.is_null());
    // SAFETY: FFI test — `defs` non-null per assertion; buffer owned by state.
    unsafe {
        assert_eq!(c_str_val((*defs).name), "foo");
        assert_eq!((*defs).is_view, 0);
    }
    assert_eq!(v.stmt_defined_count(1), 0);
}

#[test]
fn per_statement_defined_relations_create_view() {
    let v = Validator::new();
    v.analyze("CREATE VIEW v AS SELECT 1; SELECT 1;");
    assert_eq!(v.stmt_count(), 2);
    assert_eq!(v.stmt_defined_count(0), 1);
    let defs = v.stmt_defined(0);
    // SAFETY: FFI test — count = 1 above guarantees a valid element 0.
    unsafe {
        assert_eq!(c_str_val((*defs).name), "v");
        assert_eq!((*defs).is_view, 1);
    }
}

#[test]
fn out_of_bounds_returns_zero() {
    let v = Validator::new();
    v.analyze("SELECT 1;");
    assert_eq!(v.stmt_diag_count(99), 0);
    assert!(v.stmt_diags(99).is_null());
    assert_eq!(v.stmt_lineage_count(99), 0);
    assert!(v.stmt_lineage(99).is_null());
    assert_eq!(v.stmt_relation_count(99), 0);
    assert!(v.stmt_relations(99).is_null());
    assert_eq!(v.stmt_physical_table_count(99), 0);
    assert!(v.stmt_physical_tables(99).is_null());
    assert_eq!(v.stmt_defined_count(99), 0);
    assert!(v.stmt_defined(99).is_null());
    assert!(v.stmt_source(99).is_null());
}

#[test]
fn per_statement_source_text() {
    let v = Validator::new();
    v.analyze("SELECT 1; SELECT 2;");
    assert_eq!(v.stmt_count(), 2);

    let (src0, src1) = (v.stmt_source(0), v.stmt_source(1));
    assert!(!src0.is_null() && !src1.is_null());
    let s0 = c_str_val(src0);
    let s1 = c_str_val(src1);
    assert!(s0.contains("SELECT 1"), "expected 'SELECT 1', got: {s0}");
    assert!(s1.contains("SELECT 2"), "expected 'SELECT 2', got: {s1}");
    // Out of bounds returns null.
    assert!(v.stmt_source(99).is_null());
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
    let v = Validator::new();
    // SAFETY: FFI test — handle from `Validator::new`; `null_resolver`
    // is an `extern "C" fn` with a `'static` lifetime.
    let set = |cb| unsafe {
        syntaqlite_analyzer_set_module_resolver(v.as_ptr(), cb, std::ptr::null_mut());
    };
    set(Some(null_resolver));
    v.analyze("SELECT 1;");
    assert_eq!(v.diag_count(), 0);
    set(None); // Clear the resolver — should not crash.
    v.analyze("SELECT 1;");
    assert_eq!(v.diag_count(), 0);
}

#[test]
fn aggregated_diagnostics_matches_per_statement_sum() {
    let v = Validator::new();
    v.add_table("t", &["a"]);
    let total = v.analyze("SELECT bogus FROM t; SELECT also_bad FROM t;");

    let per_stmt_total: u32 = (0..v.stmt_count()).map(|i| v.stmt_diag_count(i)).sum();
    assert_eq!(total, per_stmt_total);
}

// ── Check level / strict schema / suggestion threshold ───────────────

#[test]
fn set_check_level_allow_suppresses_unknown_table() {
    let v = Validator::new();
    let rc = v.set_check_level("unknown-table", SYNTAQLITE_CHECK_ALLOW);
    assert_eq!(rc, 0);
    let n = v.analyze("SELECT 1 FROM no_such");
    assert_eq!(n, 0, "allow should suppress the diagnostic");
}

#[test]
fn set_check_level_deny_raises_unknown_table_to_error() {
    let v = Validator::new();
    let rc = v.set_check_level("unknown-table", SYNTAQLITE_CHECK_DENY);
    assert_eq!(rc, 0);
    let n = v.analyze("SELECT 1 FROM no_such");
    assert!(n >= 1);
    assert_eq!(v.first_severity(), SEVERITY_ERROR);
}

#[test]
fn set_check_level_unknown_name_returns_error() {
    let v = Validator::new();
    let rc = v.set_check_level("not-a-real-category", SYNTAQLITE_CHECK_WARN);
    assert_eq!(rc, -1);
}

#[test]
fn set_check_level_schema_group_promotes_all_schema_checks() {
    let v = Validator::new();
    let rc = v.set_check_level("schema", SYNTAQLITE_CHECK_DENY);
    assert_eq!(rc, 0);
    let n = v.analyze("SELECT 1 FROM no_such");
    assert!(n >= 1);
    assert_eq!(v.first_severity(), SEVERITY_ERROR);
}

#[test]
fn set_strict_schema_raises_unknown_table_to_error() {
    let v = Validator::new();
    v.set_strict_schema(1);
    let n = v.analyze("SELECT 1 FROM no_such");
    assert!(n >= 1);
    assert_eq!(v.first_severity(), SEVERITY_ERROR);
}

#[test]
fn set_strict_schema_zero_reverts_to_warning() {
    let v = Validator::new();
    v.set_strict_schema(1);
    v.set_strict_schema(0);
    let n = v.analyze("SELECT 1 FROM no_such");
    assert!(n >= 1);
    assert_eq!(v.first_severity(), SEVERITY_WARNING);
}

#[test]
fn set_suggestion_threshold_zero_disables_help() {
    let v = Validator::new();
    v.add_table("users", &["id"]);
    v.set_suggestion_threshold(0);
    // "usr" is 1 edit from "users" — would normally trigger "did you mean".
    v.analyze("SELECT 1 FROM usr");
    let r = v.render(None);
    assert!(
        !r.contains("did you mean"),
        "threshold=0 should suppress suggestions; got: {r}"
    );
}

#[test]
fn user_config_survives_schema_load() {
    // Setting a custom suggestion threshold + allow-override for a
    // category before schema load must persist after add_tables.
    let v = Validator::new();
    v.set_suggestion_threshold(7);
    v.set_check_level("unknown-table", SYNTAQLITE_CHECK_ALLOW);
    v.add_table("users", &["id"]);
    let n = v.analyze("SELECT 1 FROM no_such");
    assert_eq!(n, 0, "allow set before schema load must persist");
}

#[test]
fn set_suggestion_threshold_permissive_emits_help() {
    let v = Validator::new();
    v.add_table("users", &["id"]);
    v.set_suggestion_threshold(5);
    v.analyze("SELECT 1 FROM usr");
    let r = v.render(None);
    let has_hint = r.contains("did you mean") && r.contains("users");
    assert!(
        has_hint,
        "threshold=5 should emit 'users' suggestion; got: {r}"
    );
}

// ── Diagnostic kind codes ────────────────────────────────────────────

#[test]
fn kind_code_parse_error() {
    let v = Validator::new();
    v.analyze("SELCT 1");
    assert!(
        v.has_code(DIAG_CODE_PARSE_ERROR),
        "expected ParseError kind_code"
    );
}

#[test]
fn kind_code_unknown_table() {
    let v = Validator::new();
    v.analyze("SELECT id FROM no_such_table");
    assert!(v.has_code(DIAG_CODE_UNKNOWN_TABLE));
}

#[test]
fn kind_code_unknown_column() {
    let v = Validator::new();
    v.add_table("t", &["a"]);
    v.analyze("SELECT bogus FROM t");
    assert!(v.has_code(DIAG_CODE_UNKNOWN_COLUMN));
}

#[test]
fn kind_code_unknown_function() {
    let v = Validator::new();
    v.analyze("SELECT no_such_fn(1)");
    assert!(v.has_code(DIAG_CODE_UNKNOWN_FUNCTION));
}

#[test]
fn kind_code_function_arity() {
    let v = Validator::new();
    // substr accepts 0/2/3 args; calling with 1 triggers WrongArity.
    v.analyze("SELECT substr('x')");
    assert!(
        v.has_code(DIAG_CODE_FUNCTION_ARITY),
        "expected FunctionArity kind_code"
    );
}

#[test]
fn kind_code_cte_column_count_mismatch() {
    let v = Validator::new();
    v.analyze("WITH cte(a) AS (SELECT 1, 2) SELECT a FROM cte");
    assert!(v.has_code(DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH));
}

#[test]
fn kind_code_per_statement_matches_aggregate() {
    let v = Validator::new();
    v.analyze("SELECT id FROM no_such_table");
    let ptr = v.stmt_diags(0);
    assert!(!ptr.is_null());
    // SAFETY: FFI test — `ptr` non-null per assertion; buffer owned by state.
    unsafe { assert_eq!((*ptr).kind_code, DIAG_CODE_UNKNOWN_TABLE) };
}

#[rustfmt::skip]
#[test]
fn diagnostic_code_to_c_covers_all_variants() {
    use super::super::diagnostics::DiagnosticMessage as M;
    let check = |msg: M, code: u32| assert_eq!(diagnostic_code_to_c(&msg), code);
    check(M::ParseError("x".into()), DIAG_CODE_PARSE_ERROR);
    check(M::UnknownTable { name: "t".into() }, DIAG_CODE_UNKNOWN_TABLE);
    check(M::UnknownColumn { column: "c".into(), table: None }, DIAG_CODE_UNKNOWN_COLUMN);
    check(M::UnknownFunction { name: "f".into() }, DIAG_CODE_UNKNOWN_FUNCTION);
    check(M::UnknownModule { name: "m".into() }, DIAG_CODE_UNKNOWN_MODULE);
    check(M::FunctionArity { name: "f".into(), expected: vec![1], got: 0 }, DIAG_CODE_FUNCTION_ARITY);
    check(M::CteColumnCountMismatch { name: "c".into(), declared: 1, actual: 2 }, DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH);
}

// ── Custom function / table-function registration ────────────────────

#[test]
fn add_function_overload_registers_scalar() {
    let v = Validator::new();
    v.add_scalar("my_udf", SYNTAQLITE_ARITY_EXACT, 1);
    let n = v.analyze("SELECT my_udf(1)");
    assert_eq!(n, 0, "custom scalar should accept as clean");
}

#[test]
fn add_function_overload_wrong_arity_emits_diagnostic() {
    let v = Validator::new();
    v.add_scalar("my_udf", SYNTAQLITE_ARITY_EXACT, 1);
    v.analyze("SELECT my_udf(1, 2)");
    let ptr = v.diags_ptr();
    assert!(!ptr.is_null());
    // SAFETY: FFI test — `ptr` non-null per assertion above; buffer owned
    // by validator state.
    unsafe { assert_eq!((*ptr).kind_code, DIAG_CODE_FUNCTION_ARITY) };
}

#[test]
fn add_function_overload_multiple_arities_build_overload_set() {
    let v = Validator::new();
    v.add_scalar("my_udf", SYNTAQLITE_ARITY_EXACT, 1);
    v.add_scalar("my_udf", SYNTAQLITE_ARITY_EXACT, 3);
    // Either arity 1 or 3 should be accepted; arity 2 should fail.
    assert_eq!(v.analyze("SELECT my_udf(1)"), 0);
    assert_eq!(v.analyze("SELECT my_udf(1, 2, 3)"), 0);
    let n = v.analyze("SELECT my_udf(1, 2)");
    assert!(n > 0);
}

#[test]
fn add_function_overload_at_least_accepts_variadic() {
    let v = Validator::new();
    v.add_scalar("vararg", SYNTAQLITE_ARITY_AT_LEAST, 2);
    assert_eq!(v.analyze("SELECT vararg(1, 2)"), 0);
    assert_eq!(v.analyze("SELECT vararg(1, 2, 3, 4)"), 0);
    // One arg is below the minimum.
    assert!(v.analyze("SELECT vararg(1)") > 0);
}

#[test]
fn add_function_overload_any_accepts_any_arity() {
    let v = Validator::new();
    v.add_scalar("anything", SYNTAQLITE_ARITY_ANY, 0);
    assert_eq!(v.analyze("SELECT anything()"), 0);
    assert_eq!(v.analyze("SELECT anything(1, 2, 3)"), 0);
}

#[test]
fn add_function_overload_aggregate_registers() {
    let v = Validator::new();
    v.add_aggregate("my_sum", SYNTAQLITE_ARITY_EXACT, 1);
    assert_eq!(v.analyze("SELECT my_sum(1)"), 0);
}

#[test]
fn add_function_overload_window_registers() {
    let v = Validator::new();
    v.add_window("my_rank", SYNTAQLITE_ARITY_EXACT, 0);
    assert_eq!(v.analyze("SELECT my_rank() OVER ()"), 0);
}

#[test]
fn add_function_overload_unknown_category_is_noop() {
    let v = Validator::new();
    // Bogus category ordinal — must not register anything or crash.
    v.add_function_overload("garbage", 99, SYNTAQLITE_ARITY_ANY, 0);
    let n = v.analyze("SELECT garbage()");
    assert!(n > 0, "unregistered garbage() should still be unknown");
}

#[test]
fn add_function_overload_persists_across_analyze_calls() {
    let v = Validator::new();
    v.add_scalar("my_udf", SYNTAQLITE_ARITY_EXACT, 1);
    assert_eq!(v.analyze("SELECT my_udf(1)"), 0);
    assert_eq!(v.analyze("SELECT my_udf(1) FROM (SELECT 1)"), 0);
}

#[test]
fn add_function_overload_reset_catalog_clears_registration() {
    let v = Validator::new();
    v.add_scalar("my_udf", SYNTAQLITE_ARITY_EXACT, 1);
    v.reset_catalog();
    let n = v.analyze("SELECT my_udf(1)");
    assert!(n > 0, "my_udf should be unknown after reset");
}

#[test]
fn add_table_function_registers_with_columns() {
    let v = Validator::new();
    v.add_tfn("json_each", SYNTAQLITE_ARITY_ANY, 0, &["key", "value"]);
    let n = v.analyze("SELECT key, value FROM json_each('[]')");
    assert_eq!(n, 0, "json_each should resolve with declared columns");
}

#[test]
fn add_table_function_without_columns_accepts_any_ref() {
    let v = Validator::new();
    v.add_tfn("some_tfn", SYNTAQLITE_ARITY_ANY, 0, &[]);
    let n = v.analyze("SELECT * FROM some_tfn(1, 2)");
    assert_eq!(n, 0, "tfn without columns should accept any query");
}

#[test]
fn add_table_function_null_cols_accepted() {
    let v = Validator::new();
    // Call the raw FFI with explicit NULL to cover that code path.
    let c_name = CString::new("raw_tfn").unwrap();
    // SAFETY: FFI test — handle from `Validator::new`; `c_name` outlives
    // the call; `output_columns` is intentionally NULL.
    unsafe {
        syntaqlite_analyzer_add_table_function(
            v.as_ptr(),
            c_name.as_ptr(),
            SYNTAQLITE_ARITY_ANY,
            0,
            std::ptr::null(),
            0,
        );
    }
    let n = v.analyze("SELECT * FROM raw_tfn()");
    assert_eq!(n, 0);
}

#[test]
fn add_table_function_reset_catalog_clears_registration() {
    let v = Validator::new();
    v.add_tfn("tfn", SYNTAQLITE_ARITY_ANY, 0, &["x"]);
    v.reset_catalog();
    let n = v.analyze("SELECT x FROM tfn()");
    assert!(n > 0, "tfn should be unknown after reset");
}
