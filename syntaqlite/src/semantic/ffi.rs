// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C FFI layer for the semantic validator.
//!
//! Exposes [`SemanticAnalyzer`] and [`Catalog`] to C via opaque handle +
//! accessor functions, following the same pattern as the parser FFI in
//! `syntaqlite-syntax`.

use std::ffi::{CStr, CString, c_char};

use crate::dialect::AnyDialect;

use super::ValidationConfig;
use super::analyzer::SemanticAnalyzer;
use super::catalog::{Catalog, CatalogLayer};
use super::diagnostics::{Diagnostic, Severity};

/// Opaque validator handle exposed to C.
///
/// Owns a `SemanticAnalyzer`, a user `Catalog` (for persistent schema), and
/// the most recent diagnostics + rendered messages.
struct ValidatorState {
    analyzer: SemanticAnalyzer,
    user_catalog: Catalog,
    dialect: AnyDialect,
    /// Diagnostics from the most recent `analyze()` call.
    diagnostics: Vec<Diagnostic>,
    /// Rendered message strings, kept alive for the C pointers.
    rendered_messages: Vec<CString>,
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

// ── Exported C functions ────────────────────────────────────────────────────

/// Create a validator for the built-in `SQLite` dialect.
#[cfg(feature = "sqlite")]
#[unsafe(no_mangle)]
pub extern "C" fn syntaqlite_validator_create_sqlite() -> *mut SyntaqliteValidator {
    let dialect: AnyDialect = crate::sqlite_dialect().into();
    let analyzer = SemanticAnalyzer::with_dialect(dialect.clone());
    let user_catalog = Catalog::new(dialect.clone());

    let state = Box::new(ValidatorState {
        analyzer,
        user_catalog,
        dialect,
        diagnostics: Vec::new(),
        rendered_messages: Vec::new(),
    });
    Box::into_raw(state).cast::<SyntaqliteValidator>()
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

    let config = ValidationConfig::default();
    let model = state.analyzer.analyze(src, &state.user_catalog, &config);

    state.diagnostics = model.diagnostics().to_vec();
    state.rendered_messages = state
        .diagnostics
        .iter()
        .map(|d| CString::new(d.message.to_string()).unwrap_or_default())
        .collect();

    state.diagnostics.len() as u32
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
}

/// Add a table to the database layer of the catalog.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `table_name` must be a valid NUL-terminated C string.
/// - `column_names` may be NULL. If non-NULL, must point to `column_count`
///   valid NUL-terminated C string pointers.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_add_table(
    v: *mut SyntaqliteValidator,
    table_name: *const c_char,
    column_names: *const *const c_char,
    column_count: u32,
) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    // SAFETY: caller guarantees `table_name` is a valid NUL-terminated C string.
    let name = unsafe { CStr::from_ptr(table_name) }
        .to_str()
        .unwrap_or("")
        .to_owned();

    let columns = if column_names.is_null() {
        None
    } else {
        let cols: Vec<String> = (0..column_count as usize)
            .map(|i| {
                // SAFETY: caller guarantees `column_names[i]` is valid.
                unsafe { CStr::from_ptr(*column_names.add(i)) }
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
        .insert_relation(name, columns);
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
    v.state().diagnostics.len() as u32
}

/// Severity of the i-th diagnostic.
///
/// # Safety
///
/// `v` must be valid, `index` must be < `diagnostic_count`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_diagnostic_severity(
    v: *const SyntaqliteValidator,
    index: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if (index as usize) < state.diagnostics.len() {
        severity_to_c(state.diagnostics[index as usize].severity)
    } else {
        SEVERITY_ERROR
    }
}

/// Human-readable message for the i-th diagnostic.
///
/// # Safety
///
/// `v` must be valid, `index` must be < `diagnostic_count`.
/// The returned pointer is valid until the next `analyze()` or `destroy()`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_diagnostic_message(
    v: *const SyntaqliteValidator,
    index: u32,
) -> *const c_char {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if (index as usize) < state.rendered_messages.len() {
        state.rendered_messages[index as usize].as_ptr()
    } else {
        c"(out of bounds)".as_ptr()
    }
}

/// Byte offset of the start of the i-th diagnostic's source range.
///
/// # Safety
///
/// `v` must be valid, `index` must be < `diagnostic_count`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_diagnostic_start_offset(
    v: *const SyntaqliteValidator,
    index: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if (index as usize) < state.diagnostics.len() {
        state.diagnostics[index as usize].start_offset as u32
    } else {
        0
    }
}

/// Byte offset of the end of the i-th diagnostic's source range.
///
/// # Safety
///
/// `v` must be valid, `index` must be < `diagnostic_count`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_diagnostic_end_offset(
    v: *const SyntaqliteValidator,
    index: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &*v };
    let state = v.state();
    if (index as usize) < state.diagnostics.len() {
        state.diagnostics[index as usize].end_offset as u32
    } else {
        0
    }
}
