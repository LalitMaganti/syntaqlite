// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Configuration setters: analysis mode, check levels, strict schema,
//! suggestion threshold, module resolver.

use std::ffi::{CStr, CString, c_char};

use crate::semantic::{AnalysisMode, CheckConfig};

use super::SyntaqliteValidator;
use super::codes::check_level_from_c;

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
/// `"unknown-column"`, etc. — see [`CheckConfig::CATEGORY_NAMES`]
/// and [`CheckConfig::GROUP_NAMES`]).
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
        let checks = CheckConfig::default();
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

/// C function pointer type for module resolution.
///
/// Given a NUL-terminated module path (e.g. `"slices.flow"`), return the SQL
/// source as a NUL-terminated `malloc`-allocated string, or NULL if the module
/// is not found. The validator will `free()` the returned string.
pub type SyntaqliteModuleResolverFn = unsafe extern "C" fn(
    module_path: *const c_char,
    user_data: *mut std::ffi::c_void,
) -> *mut c_char;

struct CCallbackResolver {
    resolve_fn: SyntaqliteModuleResolverFn,
    user_data: *mut std::ffi::c_void,
}

// SAFETY: The C caller owns thread safety; validator is single-threaded by design.
unsafe impl Send for CCallbackResolver {}
// SAFETY: same as Send.
unsafe impl Sync for CCallbackResolver {}

impl crate::semantic::ModuleResolver for CCallbackResolver {
    fn resolve(&self, module_path: &str) -> Option<String> {
        let c_path = CString::new(module_path).ok()?;
        // SAFETY: `resolve_fn` is a valid C function pointer provided by the caller.
        let result = unsafe { (self.resolve_fn)(c_path.as_ptr(), self.user_data) };
        if result.is_null() {
            return None;
        }
        // SAFETY: the callback returned a malloc-allocated NUL-terminated string.
        let s = unsafe { CStr::from_ptr(result) }
            .to_str()
            .ok()
            .map(String::from);
        // SAFETY: `result` was malloc-allocated by the C callback.
        unsafe {
            unsafe extern "C" {
                fn free(ptr: *mut std::ffi::c_void);
            }
            free(result.cast());
        }
        s
    }
}

/// Set a module resolver callback on the validator. Pass NULL for `resolve_fn`
/// to clear the resolver.
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
