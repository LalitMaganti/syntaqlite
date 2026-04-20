// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Validator lifecycle: create, destroy, string cleanup.

use std::ffi::{CString, c_char};

use crate::dialect::AnyDialect;
use crate::semantic::ValidationConfig;
use crate::semantic::analyzer::SemanticAnalyzer;
use crate::semantic::catalog::Catalog;

use super::{SyntaqliteValidator, ValidatorState};

pub(super) fn create_validator(dialect: AnyDialect) -> *mut SyntaqliteValidator {
    let analyzer = SemanticAnalyzer::with_dialect(dialect.clone());
    let user_catalog = Catalog::new(dialect.clone());

    let state = Box::new(ValidatorState {
        analyzer,
        user_catalog,
        dialect,
        validation_config: ValidationConfig::default(),
        resolver: None,
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
        // SAFETY: `v` was created by `Box::into_raw` in `create_validator`.
        drop(unsafe { Box::from_raw(v.cast::<ValidatorState>()) });
    }
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
