// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Catalog population: `add_tables`, `add_views`, `load_schema_ddl`,
//! `add_function_overload`, `add_table_function`, `reset_catalog`.

use std::ffi::{CStr, c_char};

use crate::semantic::ValidationConfig;
use crate::semantic::catalog::{Catalog, CatalogLayer};

use super::codes::{arity_spec_from_c, function_category_from_c};
use super::{SyntaqliteRelationDef, SyntaqliteValidator};

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

    let (catalog, errors) = Catalog::from_ddl(state.dialect.clone(), &[src]);
    state.user_catalog.copy_schema_layers_from(&catalog);
    errors.len() as u32
}

/// Register a scalar / aggregate / window function overload in the database
/// layer. Repeat calls with the same `name` build up an overload set.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `name` must be a valid NUL-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_add_function_overload(
    v: *mut SyntaqliteValidator,
    name: *const c_char,
    category: u32,
    arity_kind: u32,
    arity_value: u32,
) {
    let Some(category) = function_category_from_c(category) else {
        return;
    };
    let Some(arity) = arity_spec_from_c(arity_kind, arity_value) else {
        return;
    };

    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    // SAFETY: caller guarantees `name` is a valid NUL-terminated C string.
    let name = unsafe { CStr::from_ptr(name) }
        .to_str()
        .unwrap_or("")
        .to_owned();

    state
        .user_catalog
        .layer_mut(CatalogLayer::Database)
        .insert_function_overload(name, category, arity);
}

/// Register a table-valued function (usable in `FROM` clauses).
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_validator_create_sqlite`.
/// - `name` must be a valid NUL-terminated C string.
/// - `output_columns` may be NULL. If non-NULL, must point to
///   `output_column_count` valid NUL-terminated C string pointers.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_validator_add_table_function(
    v: *mut SyntaqliteValidator,
    name: *const c_char,
    arity_kind: u32,
    arity_value: u32,
    output_columns: *const *const c_char,
    output_column_count: u32,
) {
    let Some(arity) = arity_spec_from_c(arity_kind, arity_value) else {
        return;
    };

    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    // SAFETY: caller guarantees `name` is a valid NUL-terminated C string.
    let name = unsafe { CStr::from_ptr(name) }
        .to_str()
        .unwrap_or("")
        .to_owned();

    let cols: Vec<String> = if output_columns.is_null() {
        Vec::new()
    } else {
        (0..output_column_count as usize)
            .map(|i| {
                // SAFETY: caller guarantees `output_columns[i]` is valid.
                unsafe { CStr::from_ptr(*output_columns.add(i)) }
                    .to_str()
                    .unwrap_or("")
                    .to_owned()
            })
            .collect()
    };

    state
        .user_catalog
        .layer_mut(CatalogLayer::Database)
        .insert_table_function(name, arity, cols);
}
