// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C FFI layer for the semantic validator.
//!
//! Exposes [`SemanticAnalyzer`] and [`Catalog`] to C via opaque handle +
//! accessor functions, following the same pattern as the parser FFI in
//! `syntaqlite-syntax`.

use std::ffi::{CStr, CString, c_char};

use crate::dialect::AnyDialect;

use super::analyzer::SemanticAnalyzer;
use super::catalog::{Catalog, CatalogLayer};
use super::diagnostics::Severity;
use super::lineage::RelationKind;
use super::render::DiagnosticRenderer;
use super::{AnalysisMode, ValidationConfig};

// ── C-compatible structs ────────────────────────────────────────────────────

/// Mirrors `SyntaqliteDiagnostic` from the C header.
#[repr(C)]
pub struct SyntaqliteDiagnostic {
    pub severity: u32,
    pub message: *const c_char,
    pub start_offset: u32,
    pub end_offset: u32,
}

/// Mirrors `SyntaqliteRelationDef` from the C header.
#[repr(C)]
pub struct SyntaqliteRelationDef {
    pub name: *const c_char,
    pub columns: *const *const c_char,
    pub column_count: u32,
}

/// Origin of a result column — which table.column it traces back to.
#[repr(C)]
pub struct SyntaqliteColumnOrigin {
    pub table: *const c_char,
    pub column: *const c_char,
}

/// Lineage information for a single result column.
#[repr(C)]
pub struct SyntaqliteColumnLineage {
    pub name: *const c_char,
    pub index: u32,
    pub origin: SyntaqliteColumnOrigin,
}

/// A catalog relation (table or view) referenced in a FROM clause.
#[repr(C)]
pub struct SyntaqliteRelationAccess {
    pub name: *const c_char,
    pub kind: u32,
}

/// A physical table accessed by the query.
#[repr(C)]
pub struct SyntaqlitePhysicalTableAccess {
    pub name: *const c_char,
}

/// A relation defined by a DDL statement.
#[repr(C)]
pub struct SyntaqliteDefinedRelation {
    pub name: *const c_char,
    pub is_view: u32,
}

/// A view whose body was not available for expansion during lineage
/// resolution.
#[repr(C)]
pub struct SyntaqliteUnexpandedView {
    pub name: *const c_char,
}

/// Lazily-cached C-compatible data for a single statement.
/// Each field is populated on first access by the corresponding accessor
/// function and remains valid until the next `analyze()` call.
#[derive(Default)]
struct PerStatementCache {
    source: Option<CString>,
    diagnostics: Option<(Vec<SyntaqliteDiagnostic>, Vec<CString>)>,
    column_lineage: Option<(Vec<SyntaqliteColumnLineage>, Vec<CString>)>,
    relations: Option<(Vec<SyntaqliteRelationAccess>, Vec<CString>)>,
    physical_tables: Option<(Vec<SyntaqlitePhysicalTableAccess>, Vec<CString>)>,
    defined_relations: Option<(Vec<SyntaqliteDefinedRelation>, Vec<CString>)>,
    unexpanded_views: Option<(Vec<SyntaqliteUnexpandedView>, Vec<CString>)>,
}

/// Opaque validator handle exposed to C.
///
/// Owns a `SemanticAnalyzer`, a user `Catalog` (for persistent schema), and
/// the most recent diagnostics + rendered messages.
struct ValidatorState {
    analyzer: SemanticAnalyzer,
    user_catalog: Catalog,
    dialect: AnyDialect,
    /// Validation config — strict mode is enabled when schema tables are added.
    validation_config: ValidationConfig,
    /// C-compatible diagnostics from the most recent `analyze()` call.
    c_diagnostics: Vec<SyntaqliteDiagnostic>,
    /// Rendered message strings, kept alive for the C pointers.
    rendered_messages: Vec<CString>,
    /// Source from the last `analyze()` call, retained for rendering.
    last_source: String,
    /// Diagnostics from the last `analyze()` call, retained for rendering.
    last_diagnostics: Vec<super::diagnostics::Diagnostic>,
    /// Rendered diagnostic output, kept alive for C pointer.
    rendered_output: CString,
    /// Whether the last lineage result was complete.
    lineage_complete: bool,
    /// C-compatible column lineage from the most recent `analyze()` call.
    c_column_lineage: Vec<SyntaqliteColumnLineage>,
    /// C-compatible relation access from the most recent `analyze()` call.
    c_relations: Vec<SyntaqliteRelationAccess>,
    /// C-compatible table access from the most recent `analyze()` call.
    c_physical_tables: Vec<SyntaqlitePhysicalTableAccess>,
    /// C-compatible unexpanded views from the most recent `analyze()` call.
    c_unexpanded_views: Vec<SyntaqliteUnexpandedView>,
    /// Rendered lineage strings, kept alive for the C pointers.
    lineage_strings: Vec<CString>,
    /// The model from the most recent `analyze()` call.
    last_model: Option<SemanticModel>,
    /// Lazily-cached per-statement C data.
    per_statement_cache: Vec<PerStatementCache>,
}

/// Opaque C handle — the pointer target of `SyntaqliteValidator*`.
///
/// This is a zero-variant enum so that Rust cannot construct it directly;
/// all access goes through raw pointer casts to `ValidatorState`.
pub enum SyntaqliteValidator {}

impl SyntaqliteValidator {
    fn state(&self) -> &ValidatorState {
        // SAFETY: `self` was created from a `Box<ValidatorState>` via
        // `Box::into_raw` cast in `syntaqlite_validator_create_sqlite`.
        unsafe { &*std::ptr::from_ref::<Self>(self).cast::<ValidatorState>() }
    }

    fn state_mut(&mut self) -> &mut ValidatorState {
        // SAFETY: `self` was created from a `Box<ValidatorState>` via
        // `Box::into_raw` cast in `syntaqlite_validator_create_sqlite`.
        unsafe { &mut *std::ptr::from_mut::<Self>(self).cast::<ValidatorState>() }
    }
}

// ── Severity mapping ────────────────────────────────────────────────────────

const SEVERITY_ERROR: u32 = 0;
const SEVERITY_WARNING: u32 = 1;
const SEVERITY_INFO: u32 = 2;
const SEVERITY_HINT: u32 = 3;

fn severity_to_c(s: Severity) -> u32 {
    match s {
        Severity::Error => SEVERITY_ERROR,
        Severity::Warning => SEVERITY_WARNING,
        Severity::Info => SEVERITY_INFO,
        Severity::Hint => SEVERITY_HINT,
    }
}

// ── Check-level mapping ─────────────────────────────────────────────────────

/// C-ABI codes for [`super::CheckLevel`]. Must match `SyntaqliteCheckLevel`
/// in `syntaqlite/include/syntaqlite/validation.h`.
pub(crate) const SYNTAQLITE_CHECK_ALLOW: u32 = 0;
pub(crate) const SYNTAQLITE_CHECK_WARN: u32 = 1;
pub(crate) const SYNTAQLITE_CHECK_DENY: u32 = 2;

fn check_level_from_c(level: u32) -> Option<super::CheckLevel> {
    match level {
        SYNTAQLITE_CHECK_ALLOW => Some(super::CheckLevel::Allow),
        SYNTAQLITE_CHECK_WARN => Some(super::CheckLevel::Warn),
        SYNTAQLITE_CHECK_DENY => Some(super::CheckLevel::Deny),
        _ => None,
    }
}

// ── Lineage helper ───────────────────────────────────────────────────────────

use super::model::SemanticModel;

/// Populate lineage C structs from the analysis model.
#[expect(clippy::too_many_lines)]
fn populate_lineage(state: &mut ValidatorState, model: &SemanticModel) {
    state.lineage_strings.clear();
    state.c_column_lineage.clear();
    state.c_relations.clear();
    state.c_physical_tables.clear();
    state.c_unexpanded_views.clear();
    state.lineage_complete = false;

    if let Some(lineage_result) = model.lineage() {
        state.lineage_complete = lineage_result.is_complete();
        let columns = lineage_result.into_inner();

        // First pass: render all strings so CString pointers are stable.
        for col in columns {
            state
                .lineage_strings
                .push(CString::new(col.name.as_str()).unwrap_or_default());
            if let Some(ref origin) = col.origin {
                state
                    .lineage_strings
                    .push(CString::new(origin.table.as_str()).unwrap_or_default());
                state
                    .lineage_strings
                    .push(CString::new(origin.column.as_str()).unwrap_or_default());
            }
        }
    }

    // Second pass: build C structs with stable pointers (after all pushes).
    if let Some(lineage_result) = model.lineage() {
        let columns = lineage_result.into_inner();
        let mut str_idx = 0;
        for col in columns {
            let name_ptr = state.lineage_strings[str_idx].as_ptr();
            str_idx += 1;
            let origin = if col.origin.is_some() {
                let table_ptr = state.lineage_strings[str_idx].as_ptr();
                str_idx += 1;
                let column_ptr = state.lineage_strings[str_idx].as_ptr();
                str_idx += 1;
                SyntaqliteColumnOrigin {
                    table: table_ptr,
                    column: column_ptr,
                }
            } else {
                SyntaqliteColumnOrigin {
                    table: std::ptr::null(),
                    column: std::ptr::null(),
                }
            };
            state.c_column_lineage.push(SyntaqliteColumnLineage {
                name: name_ptr,
                index: col.index,
                origin,
            });
        }
    }

    // Aggregate relations_accessed and physical_tables_accessed across all statements.
    {
        let base = state.lineage_strings.len();
        let mut rel_idx = 0;
        for stmt in model.statements() {
            if let Some(rels_result) = stmt.relations_accessed() {
                for r in rels_result.into_inner() {
                    state
                        .lineage_strings
                        .push(CString::new(r.name.as_str()).unwrap_or_default());
                    rel_idx += 1;
                }
            }
        }
        let rel_count = rel_idx;
        rel_idx = 0;
        for stmt in model.statements() {
            if let Some(rels_result) = stmt.relations_accessed() {
                for r in rels_result.into_inner() {
                    state.c_relations.push(SyntaqliteRelationAccess {
                        name: state.lineage_strings[base + rel_idx].as_ptr(),
                        kind: match r.kind {
                            RelationKind::Table => 0,
                            RelationKind::View => 1,
                        },
                    });
                    rel_idx += 1;
                }
            }
        }
        debug_assert_eq!(rel_idx, rel_count);
    }

    {
        let base = state.lineage_strings.len();
        let mut tbl_count = 0;
        for stmt in model.statements() {
            if let Some(tbls_result) = stmt.physical_tables_accessed() {
                for t in tbls_result.into_inner() {
                    state
                        .lineage_strings
                        .push(CString::new(t.name.as_str()).unwrap_or_default());
                    tbl_count += 1;
                }
            }
        }
        for i in 0..tbl_count {
            state.c_physical_tables.push(SyntaqlitePhysicalTableAccess {
                name: state.lineage_strings[base + i].as_ptr(),
            });
        }
    }

    {
        let base = state.lineage_strings.len();
        let mut view_count = 0;
        for stmt in model.statements() {
            for view in stmt.unexpanded_views() {
                state
                    .lineage_strings
                    .push(CString::new(view.as_str()).unwrap_or_default());
                view_count += 1;
            }
        }
        for i in 0..view_count {
            state.c_unexpanded_views.push(SyntaqliteUnexpandedView {
                name: state.lineage_strings[base + i].as_ptr(),
            });
        }
    }
}

// ── Exported C functions ────────────────────────────────────────────────────

/// Create a validator from a dialect handle.
fn create_validator(dialect: AnyDialect) -> *mut SyntaqliteValidator {
    let analyzer = SemanticAnalyzer::with_dialect(dialect.clone());
    let user_catalog = Catalog::new(dialect.clone());

    let state = Box::new(ValidatorState {
        analyzer,
        user_catalog,
        dialect,
        validation_config: ValidationConfig::default(),
        c_diagnostics: Vec::new(),
        rendered_messages: Vec::new(),
        last_source: String::new(),
        last_diagnostics: Vec::new(),
        rendered_output: CString::default(),
        lineage_complete: false,
        c_column_lineage: Vec::new(),
        c_relations: Vec::new(),
        c_physical_tables: Vec::new(),
        c_unexpanded_views: Vec::new(),
        lineage_strings: Vec::new(),
        last_model: None,
        per_statement_cache: Vec::new(),
    });
    Box::into_raw(state).cast::<SyntaqliteValidator>()
}

/// Create a validator for any dialect.
///
/// # Safety
///
/// `dialect.tmpl` must point to a valid `SyntaqliteDialectTemplate`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_create_with_dialect(
    dialect: crate::dialect::ffi::CDialect,
) -> *mut SyntaqliteValidator {
    // SAFETY: caller guarantees `dialect.tmpl` is valid.
    let any = unsafe { AnyDialect::from_c_dialect(dialect) };
    create_validator(any)
}

/// Create a validator for the built-in `SQLite` dialect.
#[cfg(feature = "sqlite")]
#[unsafe(no_mangle)]
pub extern "C" fn syntaqlite_validator_create_sqlite() -> *mut SyntaqliteValidator {
    let dialect: AnyDialect = crate::sqlite_dialect().into();
    create_validator(dialect)
}

/// Free a validator. No-op if `v` is NULL.
///
/// # Safety
///
/// `v` must be NULL or a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_destroy(v: *mut SyntaqliteValidator) {
    if !v.is_null() {
        // SAFETY: `v` was created by `Box::into_raw` in `create_sqlite`.
        drop(unsafe { Box::from_raw(v.cast::<ValidatorState>()) });
    }
}

/// Set the analysis mode.
///
/// - `SYNTAQLITE_MODE_DOCUMENT` (0): DDL resets between `analyze()` calls.
/// - `SYNTAQLITE_MODE_EXECUTE` (1): DDL accumulates across `analyze()` calls.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_set_mode(v: *mut SyntaqliteValidator, mode: u32) {
    // SAFETY: caller guarantees `v` is a valid pointer from `syntaqlite_validator_create_sqlite`.
    let v = unsafe { &mut *v };
    let state = v.state_mut();
    state.analyzer.set_mode(match mode {
        1 => AnalysisMode::Execute,
        _ => AnalysisMode::Document,
    });
}

/// Set the severity level for a check category (`"unknown-table"`,
/// `"unknown-column"`, etc. — see [`super::CheckConfig::CATEGORY_NAMES`]
/// and [`super::CheckConfig::GROUP_NAMES`]).
///
/// `level` is one of `SYNTAQLITE_CHECK_ALLOW` (0), `SYNTAQLITE_CHECK_WARN`
/// (1), `SYNTAQLITE_CHECK_DENY` (2).
///
/// Returns `0` on success, `-1` if `name` is not a recognised category or
/// `level` is out of range.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_*`.
/// - `name` must be a NUL-terminated UTF-8 string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_set_check_level(
    v: *mut SyntaqliteValidator,
    name: *const c_char,
    level: u32,
) -> i32 {
    if v.is_null() || name.is_null() {
        return -1;
    }
    let Some(cl) = check_level_from_c(level) else {
        return -1;
    };
    // SAFETY: caller guarantees `name` is a NUL-terminated UTF-8 string.
    let Ok(name_str) = unsafe { CStr::from_ptr(name) }.to_str() else {
        return -1;
    };
    // SAFETY: caller guarantees `v` is a valid pointer.
    let state = unsafe { &mut *v }.state_mut();
    let checks = state.validation_config.checks();
    match checks.set_by_name(name_str, cl) {
        Ok(new_checks) => {
            state.validation_config = state.validation_config.with_checks(new_checks);
            0
        }
        Err(_) => -1,
    }
}

/// Toggle strict-schema mode. When `enabled` is non-zero, all schema
/// checks (unknown-table/column/function, function-arity) are promoted
/// to errors. When zero, checks revert to the default mix (warnings).
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_set_strict_schema(
    v: *mut SyntaqliteValidator,
    enabled: u32,
) {
    if v.is_null() {
        return;
    }
    // SAFETY: caller guarantees `v` is a valid pointer.
    let state = unsafe { &mut *v }.state_mut();
    if enabled != 0 {
        state.validation_config = state.validation_config.with_strict_schema();
    } else {
        let checks = super::CheckConfig::default();
        state.validation_config = state.validation_config.with_checks(checks);
    }
}

/// Set the maximum Levenshtein distance for "did you mean?" suggestions.
/// Pass `0` to disable suggestions entirely. The default is `2`.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_set_suggestion_threshold(
    v: *mut SyntaqliteValidator,
    threshold: u32,
) {
    if v.is_null() {
        return;
    }
    // SAFETY: caller guarantees `v` is a valid pointer.
    let state = unsafe { &mut *v }.state_mut();
    state.validation_config = state
        .validation_config
        .with_suggestion_threshold(threshold as usize);
}

// ── Module resolver callback ────────────────────────────────────────────────

/// C function pointer type for module resolution.
///
/// Given a NUL-terminated module path (e.g. `"slices.flow"`), return the SQL
/// source as a NUL-terminated `malloc`-allocated string, or NULL if the module
/// is not found. The validator will `free()` the returned string.
pub type SyntaqliteModuleResolverFn = unsafe extern "C" fn(
    module_path: *const c_char,
    user_data: *mut std::ffi::c_void,
) -> *mut c_char;

/// Wraps a C callback pair into a Rust `ModuleResolver`.
struct CCallbackResolver {
    resolve_fn: SyntaqliteModuleResolverFn,
    user_data: *mut std::ffi::c_void,
}

// SAFETY: The C caller is responsible for ensuring thread safety of the
// callback and user_data. The validator is single-threaded by design.
unsafe impl Send for CCallbackResolver {}
// SAFETY: Same as above — single-threaded validator, C caller owns thread safety.
unsafe impl Sync for CCallbackResolver {}

impl super::ModuleResolver for CCallbackResolver {
    fn resolve(&self, module_path: &str) -> Option<String> {
        let c_path = CString::new(module_path).ok()?;
        // SAFETY: `resolve_fn` is a valid C function pointer provided by the caller
        // of `syntaqlite_validator_set_module_resolver`. `c_path` is a valid
        // NUL-terminated string and `user_data` is caller-managed.
        let result = unsafe { (self.resolve_fn)(c_path.as_ptr(), self.user_data) };
        if result.is_null() {
            return None;
        }
        // SAFETY: the C callback returned a malloc-allocated NUL-terminated string.
        let s = unsafe { CStr::from_ptr(result) }
            .to_str()
            .ok()
            .map(String::from);
        // SAFETY: `result` was returned by the C callback as a malloc-allocated
        // string, so it is safe to pass to `free`.
        unsafe {
            unsafe extern "C" {
                fn free(ptr: *mut std::ffi::c_void);
            }
            free(result.cast());
        }
        s
    }
}

/// Set a module resolver callback on the validator. When the analyzer
/// encounters an import statement, it calls `resolve_fn` to obtain the
/// module's SQL source. Pass NULL for `resolve_fn` to clear the resolver.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_*`.
/// - `resolve_fn` (if non-null) and `user_data` must remain valid for the
///   lifetime of the validator or until the resolver is replaced/cleared.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_set_module_resolver(
    v: *mut SyntaqliteValidator,
    resolve_fn: Option<SyntaqliteModuleResolverFn>,
    user_data: *mut std::ffi::c_void,
) {
    // SAFETY: caller guarantees `v` is a valid pointer from `syntaqlite_validator_create_*`.
    let v = unsafe { &mut *v };
    let state = v.state_mut();
    match resolve_fn {
        Some(f) => {
            let resolver = CCallbackResolver {
                resolve_fn: f,
                user_data,
            };
            state.analyzer.set_module_resolver(Some(Box::new(resolver)));
        }
        None => {
            state.analyzer.set_module_resolver(None);
        }
    }
}

/// Analyze a SQL source string. Returns the number of diagnostics.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `source` must point to `len` bytes of valid UTF-8.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_analyze(
    v: *mut SyntaqliteValidator,
    source: *const c_char,
    len: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    // SAFETY: caller guarantees `source` points to `len` bytes of valid UTF-8.
    let src = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(source.cast(), len as usize))
    };

    let model = state
        .analyzer
        .analyze(src, &state.user_catalog, &state.validation_config);

    // Collect diagnostics across all statements.
    let all_diagnostics: Vec<_> = model.diagnostics().cloned().collect();

    // Retain source + diagnostics for diagnostic rendering.
    state.last_source.clear();
    state.last_source.push_str(src);
    state.last_diagnostics.clear();
    state
        .last_diagnostics
        .extend(all_diagnostics.iter().cloned());

    // Reuse existing Vec capacity — clear + push avoids reallocating
    // on steady-state calls.
    state.rendered_messages.clear();
    state.c_diagnostics.clear();

    // First pass: render messages (must be done before building
    // SyntaqliteDiagnostic so the CString pointers are stable).
    for d in &all_diagnostics {
        state
            .rendered_messages
            .push(CString::new(d.message().to_string()).unwrap_or_default());
    }

    // Second pass: build C structs pointing into rendered_messages.
    for (d, msg) in all_diagnostics.iter().zip(state.rendered_messages.iter()) {
        state.c_diagnostics.push(SyntaqliteDiagnostic {
            severity: severity_to_c(d.severity()),
            message: msg.as_ptr(),
            start_offset: d.start_offset() as u32,
            end_offset: d.end_offset() as u32,
        });
    }

    populate_lineage(state, &model);

    // Store model and reset per-statement cache for lazy access.
    let stmt_count = model.statements().len();
    state.per_statement_cache.clear();
    state
        .per_statement_cache
        .resize_with(stmt_count, PerStatementCache::default);
    state.last_model = Some(model);

    state.c_diagnostics.len() as u32
}

/// Clear accumulated DDL from the catalog.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_reset_catalog(v: *mut SyntaqliteValidator) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();
    state.user_catalog = Catalog::new(state.dialect.clone());
    state.validation_config = ValidationConfig::default();
}

/// Add tables to the database layer of the catalog.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `tables` must point to `count` valid `SyntaqliteRelationDef` entries.
/// - Each `name` must be a valid NUL-terminated C string.
/// - Each `columns` may be NULL. If non-NULL, must point to `column_count`
///   valid NUL-terminated C string pointers.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_add_tables(
    v: *mut SyntaqliteValidator,
    tables: *const SyntaqliteRelationDef,
    count: u32,
) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    for i in 0..count as usize {
        // SAFETY: caller guarantees `tables[i]` is valid.
        let def = unsafe { &*tables.add(i) };

        // SAFETY: caller guarantees `name` is a valid NUL-terminated C string.
        let name = unsafe { CStr::from_ptr(def.name) }
            .to_str()
            .unwrap_or("")
            .to_owned();

        let columns = if def.columns.is_null() {
            None
        } else {
            let cols: Vec<String> = (0..def.column_count as usize)
                .map(|j| {
                    // SAFETY: caller guarantees `columns[j]` is valid.
                    unsafe { CStr::from_ptr(*def.columns.add(j)) }
                        .to_str()
                        .unwrap_or("")
                        .to_owned()
                })
                .collect();
            Some(cols)
        };

        state
            .user_catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table(name, columns, false);
    }
}

/// Add views to the database layer of the catalog.
///
/// Uses the same `SyntaqliteRelationDef` struct as `add_tables` — `name` is
/// the view name, `columns` are the view's output columns.
///
/// # Safety
///
/// Same requirements as `syntaqlite_validator_add_tables`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_add_views(
    v: *mut SyntaqliteValidator,
    views: *const SyntaqliteRelationDef,
    count: u32,
) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    for i in 0..count as usize {
        // SAFETY: caller guarantees `views[i]` is valid.
        let def = unsafe { &*views.add(i) };

        // SAFETY: caller guarantees `name` is a valid NUL-terminated C string.
        let name = unsafe { CStr::from_ptr(def.name) }
            .to_str()
            .unwrap_or("")
            .to_owned();

        let columns = if def.columns.is_null() {
            None
        } else {
            let cols: Vec<String> = (0..def.column_count as usize)
                .map(|j| {
                    // SAFETY: caller guarantees `columns[j]` is valid.
                    unsafe { CStr::from_ptr(*def.columns.add(j)) }
                        .to_str()
                        .unwrap_or("")
                        .to_owned()
                })
                .collect();
            Some(cols)
        };

        state
            .user_catalog
            .layer_mut(CatalogLayer::Database)
            .insert_view(name, columns);
    }
}

/// Load schema from DDL statements (CREATE TABLE, CREATE VIEW, etc.).
///
/// Parses `source` as SQL and accumulates all DDL into the database layer
/// of the catalog. This is equivalent to calling `add_tables` / `add_views`
/// but lets you provide the schema as SQL text.
///
/// Returns the number of parse errors encountered (0 on success).
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `source` must point to `len` bytes of valid UTF-8.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_load_schema_ddl(
    v: *mut SyntaqliteValidator,
    source: *const c_char,
    len: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    // SAFETY: caller guarantees `source` points to `len` bytes of valid UTF-8.
    let src = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(source.cast(), len as usize))
    };

    let (catalog, errors) = Catalog::from_ddl(state.dialect.clone(), &[(src, None)]);
    state.user_catalog.copy_schema_layers_from(&catalog);
    errors.len() as u32
}

/// Number of diagnostics from the last `analyze()` call.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_diagnostic_count(
    v: *const SyntaqliteValidator,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_diagnostics.len() as u32
}

/// Pointer to the diagnostic array from the last `analyze()` call.
/// Returns NULL when diagnostic count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_diagnostics(
    v: *const SyntaqliteValidator,
) -> *const SyntaqliteDiagnostic {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if state.c_diagnostics.is_empty() {
        std::ptr::null()
    } else {
        state.c_diagnostics.as_ptr()
    }
}

// ── Diagnostic rendering ──────────────────────────────────────────────────

/// Render all diagnostics from the last `analyze()` call as a rustc-style
/// human-readable string.
///
/// `file` is a NUL-terminated label shown in the `-->` line (e.g. "query.sql").
/// If `file` is NULL, the label `"<input>"` is used.
///
/// Returns a NUL-terminated UTF-8 string. The pointer is valid until the
/// next `analyze()`, `render_diagnostics()`, or `destroy()` call.
/// Returns an empty string when there are no diagnostics.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `file` must be NULL or a valid NUL-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_render_diagnostics(
    v: *mut SyntaqliteValidator,
    file: *const c_char,
) -> *const c_char {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    if state.last_diagnostics.is_empty() {
        state.rendered_output = CString::default();
        return state.rendered_output.as_ptr();
    }

    let file_label = if file.is_null() {
        "<input>"
    } else {
        // SAFETY: caller guarantees `file` is a valid NUL-terminated C string.
        unsafe { CStr::from_ptr(file) }
            .to_str()
            .unwrap_or("<input>")
    };

    let renderer = DiagnosticRenderer::new(&state.last_source, file_label);
    let mut buf = Vec::new();
    // Ignore write errors — Vec<u8> writes are infallible.
    let _ = renderer.render_diagnostics(&state.last_diagnostics, &mut buf);

    state.rendered_output = CString::new(buf).unwrap_or_default();
    state.rendered_output.as_ptr()
}

// ── Lineage access ────────────────────────────────────────────────────────

/// Whether lineage was fully resolved (1) or partially resolved (0).
/// Returns 0 if the last analyzed statement was not a query.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_lineage_complete(
    v: *const SyntaqliteValidator,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    u32::from(v.state().lineage_complete)
}

/// Number of result columns with lineage information.
/// Returns 0 if the last analyzed statement was not a query.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_column_lineage_count(
    v: *const SyntaqliteValidator,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_column_lineage.len() as u32
}

/// Pointer to the column lineage array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_column_lineage(
    v: *const SyntaqliteValidator,
) -> *const SyntaqliteColumnLineage {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if state.c_column_lineage.is_empty() {
        std::ptr::null()
    } else {
        state.c_column_lineage.as_ptr()
    }
}

/// Number of relations (tables/views) directly referenced in FROM clauses.
/// Returns 0 if the last analyzed statement was not a query.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_relation_count(v: *const SyntaqliteValidator) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_relations.len() as u32
}

/// Pointer to the relation access array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_relations(
    v: *const SyntaqliteValidator,
) -> *const SyntaqliteRelationAccess {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if state.c_relations.is_empty() {
        std::ptr::null()
    } else {
        state.c_relations.as_ptr()
    }
}

/// Number of physical tables accessed (after resolving CTEs/views).
/// Returns 0 if the last analyzed statement was not a query.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_physical_table_count(
    v: *const SyntaqliteValidator,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_physical_tables.len() as u32
}

/// Pointer to the table access array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_physical_tables(
    v: *const SyntaqliteValidator,
) -> *const SyntaqlitePhysicalTableAccess {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if state.c_physical_tables.is_empty() {
        std::ptr::null()
    } else {
        state.c_physical_tables.as_ptr()
    }
}

/// Number of views whose bodies were not available for expansion during
/// lineage resolution across all statements. A non-zero count means at
/// least one statement had a Partial lineage result.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_unexpanded_view_count(
    v: *const SyntaqliteValidator,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_unexpanded_views.len() as u32
}

/// Pointer to the unexpanded views array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_unexpanded_views(
    v: *const SyntaqliteValidator,
) -> *const SyntaqliteUnexpandedView {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if state.c_unexpanded_views.is_empty() {
        std::ptr::null()
    } else {
        state.c_unexpanded_views.as_ptr()
    }
}

// ── Per-statement lazy accessors ──────────────────────────────────────────
//
// Each accessor lazily populates its cache in PerStatementCache on first
// call, reading from last_model.statements()[idx]. The cache (and model)
// are cleared on the next analyze() call.

use super::model::StatementModel;

/// Build C source string from a `StatementModel`, caching into the slot.
fn ensure_source<'a>(cache: &'a mut PerStatementCache, stmt: &StatementModel) -> &'a CString {
    cache
        .source
        .get_or_insert_with(|| CString::new(stmt.source()).unwrap_or_default())
}

/// Build C diagnostics from a `StatementModel`, caching into the slot.
#[expect(clippy::cast_possible_truncation)]
fn ensure_diagnostics<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqliteDiagnostic>, Vec<CString>) {
    cache.diagnostics.get_or_insert_with(|| {
        let mut msgs = Vec::new();
        let mut diags = Vec::new();
        for d in stmt.diagnostics() {
            msgs.push(CString::new(d.message().to_string()).unwrap_or_default());
        }
        for (d, msg) in stmt.diagnostics().iter().zip(msgs.iter()) {
            diags.push(SyntaqliteDiagnostic {
                severity: severity_to_c(d.severity()),
                message: msg.as_ptr(),
                start_offset: d.start_offset() as u32,
                end_offset: d.end_offset() as u32,
            });
        }
        (diags, msgs)
    })
}

/// Build C column lineage from a `StatementModel`.
fn ensure_column_lineage<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqliteColumnLineage>, Vec<CString>) {
    cache.column_lineage.get_or_insert_with(|| {
        let mut strings = Vec::new();
        let mut cols = Vec::new();
        if let Some(lineage) = stmt.lineage() {
            let inner = lineage.into_inner();
            // First pass: collect all CStrings.
            let has_origin: Vec<bool> = inner
                .iter()
                .map(|col| {
                    strings.push(CString::new(col.name.as_str()).unwrap_or_default());
                    if let Some(ref origin) = col.origin {
                        strings.push(CString::new(origin.table.as_str()).unwrap_or_default());
                        strings.push(CString::new(origin.column.as_str()).unwrap_or_default());
                        true
                    } else {
                        false
                    }
                })
                .collect();
            // Second pass: build C structs referencing the collected strings.
            let mut si = 0;
            for (col, &has_orig) in inner.iter().zip(has_origin.iter()) {
                let name_ptr = strings[si].as_ptr();
                si += 1;
                let origin = if has_orig {
                    let tp = strings[si].as_ptr();
                    si += 1;
                    let cp = strings[si].as_ptr();
                    si += 1;
                    SyntaqliteColumnOrigin {
                        table: tp,
                        column: cp,
                    }
                } else {
                    SyntaqliteColumnOrigin {
                        table: std::ptr::null(),
                        column: std::ptr::null(),
                    }
                };
                cols.push(SyntaqliteColumnLineage {
                    name: name_ptr,
                    index: col.index,
                    origin,
                });
            }
        }
        (cols, strings)
    })
}

/// Build C relation access from a `StatementModel`.
fn ensure_relations<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqliteRelationAccess>, Vec<CString>) {
    cache.relations.get_or_insert_with(|| {
        let mut strings = Vec::new();
        let mut rels = Vec::new();
        if let Some(result) = stmt.relations_accessed() {
            let inner = result.into_inner();
            for r in inner {
                strings.push(CString::new(r.name.as_str()).unwrap_or_default());
            }
            for (s, r) in strings.iter().zip(inner.iter()) {
                rels.push(SyntaqliteRelationAccess {
                    name: s.as_ptr(),
                    kind: match r.kind {
                        RelationKind::Table => 0,
                        RelationKind::View => 1,
                    },
                });
            }
        }
        (rels, strings)
    })
}

/// Build C table access from a `StatementModel`.
fn ensure_physical_tables<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqlitePhysicalTableAccess>, Vec<CString>) {
    cache.physical_tables.get_or_insert_with(|| {
        let mut strings = Vec::new();
        let mut tbls = Vec::new();
        if let Some(result) = stmt.physical_tables_accessed() {
            for t in result.into_inner() {
                strings.push(CString::new(t.name.as_str()).unwrap_or_default());
            }
            for s in &strings {
                tbls.push(SyntaqlitePhysicalTableAccess { name: s.as_ptr() });
            }
        }
        (tbls, strings)
    })
}

/// Build C defined relations from a `StatementModel`.
fn ensure_defined_relations<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqliteDefinedRelation>, Vec<CString>) {
    cache.defined_relations.get_or_insert_with(|| {
        let mut strings = Vec::new();
        let mut defs = Vec::new();
        let defined = stmt.defined_relations();
        for dr in defined {
            strings.push(CString::new(dr.name.as_str()).unwrap_or_default());
        }
        for (s, dr) in strings.iter().zip(defined.iter()) {
            defs.push(SyntaqliteDefinedRelation {
                name: s.as_ptr(),
                is_view: u32::from(dr.is_view),
            });
        }
        (defs, strings)
    })
}

/// Build C unexpanded views from a `StatementModel`.
fn ensure_unexpanded_views<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqliteUnexpandedView>, Vec<CString>) {
    cache.unexpanded_views.get_or_insert_with(|| {
        let strings: Vec<CString> = stmt
            .unexpanded_views()
            .iter()
            .map(|v| CString::new(v.as_str()).unwrap_or_default())
            .collect();
        let views = strings
            .iter()
            .map(|s| SyntaqliteUnexpandedView { name: s.as_ptr() })
            .collect();
        (views, strings)
    })
}

/// Look up the statement + cache for a given index, or return `None` if
/// out of bounds or no model is available.
///
/// # Safety
/// `v` must be a valid pointer from `syntaqlite_validator_create_*`.
unsafe fn stmt_cache<'a>(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> Option<(&'a StatementModel, &'a mut PerStatementCache)> {
    // SAFETY: caller guarantees `v` is a valid pointer from `syntaqlite_validator_create_*`.
    let state = unsafe { &mut *v }.state_mut();
    let i = idx as usize;
    let model = state.last_model.as_ref()?;
    let stmt = model.statements().get(i)?;
    let cache = &mut state.per_statement_cache[i];
    // SAFETY: Reborrow stmt with a longer lifetime — model lives in state and won't
    // move while we hold &mut state.
    let stmt: &StatementModel = unsafe { &*std::ptr::from_ref::<StatementModel>(stmt) };
    Some((stmt, cache))
}

/// Return (ptr, count) for a lazily-cached vec, or (null, 0) on miss.
#[expect(clippy::cast_possible_truncation)]
fn cached_slice<T>(v: &[T]) -> (*const T, u32) {
    if v.is_empty() {
        (std::ptr::null(), 0)
    } else {
        (v.as_ptr(), v.len() as u32)
    }
}

/// Source text for statement `idx`. Returns NULL when out of bounds.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_source(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const c_char {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    ensure_source(c, s).as_ptr()
}

#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_validator_statement_count(v: *mut SyntaqliteValidator) -> u32 {
    // SAFETY: caller guarantees `v` is a valid pointer from `syntaqlite_validator_create_*`.
    let state = unsafe { &mut *v }.state_mut();
    state
        .last_model
        .as_ref()
        .map_or(0, |m| m.statements().len() as u32)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_diagnostic_count(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return 0;
    };
    cached_slice(&ensure_diagnostics(c, s).0).1
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_diagnostics(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const SyntaqliteDiagnostic {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_diagnostics(c, s).0).0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_column_lineage_count(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return 0;
    };
    cached_slice(&ensure_column_lineage(c, s).0).1
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_column_lineage(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const SyntaqliteColumnLineage {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_column_lineage(c, s).0).0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_relation_count(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return 0;
    };
    cached_slice(&ensure_relations(c, s).0).1
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_relations(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const SyntaqliteRelationAccess {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_relations(c, s).0).0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_physical_table_count(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return 0;
    };
    cached_slice(&ensure_physical_tables(c, s).0).1
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_physical_tables(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const SyntaqlitePhysicalTableAccess {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_physical_tables(c, s).0).0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_defined_relation_count(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return 0;
    };
    cached_slice(&ensure_defined_relations(c, s).0).1
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_defined_relations(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const SyntaqliteDefinedRelation {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_defined_relations(c, s).0).0
}

/// Number of views referenced in statement `idx` whose bodies were not
/// available for expansion. A non-zero count means lineage is Partial.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_unexpanded_view_count(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return 0;
    };
    cached_slice(&ensure_unexpanded_views(c, s).0).1
}

/// Unexpanded views for statement `idx`. NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_validator_create_*`.
/// Returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_statement_unexpanded_views(
    v: *mut SyntaqliteValidator,
    idx: u32,
) -> *const SyntaqliteUnexpandedView {
    // SAFETY: caller guarantees `v` is valid; `stmt_cache` documents its safety requirements.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_unexpanded_views(c, s).0).0
}

/// Free a string returned by `syntaqlite_string_*` functions.
/// No-op if `s` is NULL.
///
/// # Safety
///
/// `s` must be NULL or a pointer returned by a `syntaqlite_*` function that
/// documents ownership transfer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_string_destroy(s: *mut c_char) {
    if !s.is_null() {
        // SAFETY: `s` was allocated by `CString::into_raw` in a `syntaqlite_*` function.
        drop(unsafe { CString::from_raw(s) });
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::*;

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
            let errors =
                syntaqlite_validator_load_schema_ddl(v, ddl.as_ptr().cast(), ddl.len() as u32);
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
            let errors =
                syntaqlite_validator_load_schema_ddl(v, ddl.as_ptr().cast(), ddl.len() as u32);
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
}
