// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Global result accessors exposing diagnostics, rendered output, and
//! aggregated lineage/relations/tables/unexpanded views from the last
//! `analyze()` call.

use std::ffi::{CStr, CString, c_char};

use crate::analysis::diagnostics::render::DiagnosticRenderer;

use super::{
    SyntaqliteAnalyzer, SyntaqliteColumnLineage, SyntaqliteDiagnostic,
    SyntaqlitePhysicalTableAccess, SyntaqliteRelationAccess, SyntaqliteUnexpandedView,
};

/// Number of diagnostics from the last `analyze()` call.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_analyzer_diagnostic_count(v: *const SyntaqliteAnalyzer) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_diagnostics.len() as u32
}

/// Pointer to the diagnostic array from the last `analyze()` call.
/// Returns NULL when diagnostic count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_diagnostics(
    v: *const SyntaqliteAnalyzer,
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
/// - `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// - `file` must be NULL or a valid NUL-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_render_diagnostics(
    v: *mut SyntaqliteAnalyzer,
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
    let _ = renderer.render_diagnostics(&state.last_diagnostics, &mut buf);

    state.rendered_output = CString::new(buf).unwrap_or_default();
    state.rendered_output.as_ptr()
}

/// Whether lineage was fully resolved (1) or partially resolved (0).
/// Returns 0 if the last analyzed statement was not a query.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_lineage_complete(v: *const SyntaqliteAnalyzer) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    u32::from(v.state().lineage_complete)
}

/// Number of result columns with lineage information.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_analyzer_column_lineage_count(
    v: *const SyntaqliteAnalyzer,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_column_lineage.len() as u32
}

/// Pointer to the column lineage array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_column_lineage(
    v: *const SyntaqliteAnalyzer,
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

/// Number of relations directly referenced in FROM clauses.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_analyzer_relation_count(v: *const SyntaqliteAnalyzer) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_relations.len() as u32
}

/// Pointer to the relation access array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_relations(
    v: *const SyntaqliteAnalyzer,
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
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_analyzer_physical_table_count(
    v: *const SyntaqliteAnalyzer,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_physical_tables.len() as u32
}

/// Pointer to the physical table array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_physical_tables(
    v: *const SyntaqliteAnalyzer,
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

/// Number of views whose bodies were not available for expansion across
/// all statements.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_analyzer_unexpanded_view_count(
    v: *const SyntaqliteAnalyzer,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    v.state().c_unexpanded_views.len() as u32
}

/// Pointer to the unexpanded views array. Returns NULL when count is 0.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_unexpanded_views(
    v: *const SyntaqliteAnalyzer,
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
