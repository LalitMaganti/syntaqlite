// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Catalog population: `add_tables`, `add_views`, `load_schema_ddl`,
//! `add_function_overload`, `add_table_function`, `reset_catalog`.

use std::ffi::{CStr, c_char};

use crate::analysis::AnalysisConfig;
use crate::analysis::catalog::{Catalog, CatalogLayer};

use super::SyntaqliteAnalyzer;
use super::codes::{arity_spec_from_c, column_kind_from_c, function_category_from_c};
use super::types::{SyntaqliteColumnDef, SyntaqliteTableDef, SyntaqliteViewDef};
use crate::analysis::catalog::{Column, TableDef, ViewDef};

/// Clear accumulated DDL from the catalog.
///
/// # Safety
///
/// `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_reset_catalog(v: *mut SyntaqliteAnalyzer) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();
    state.user_catalog = Catalog::new(state.dialect.clone());
    state.analysis_config = AnalysisConfig::default();
}

/// Add tables to the database layer of the catalog. A table with an
/// out-of-range column kind is not added.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// - `tables` must point to `count` valid `SyntaqliteTableDef` entries.
/// - Each `name` must be a valid NUL-terminated C string.
/// - Each `columns` may be NULL. If non-NULL, must point to `column_count`
///   valid `SyntaqliteColumnDef` entries whose names are valid NUL-terminated
///   C strings.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_add_tables(
    v: *mut SyntaqliteAnalyzer,
    tables: *const SyntaqliteTableDef,
    count: u32,
) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let layer = v.state_mut().user_catalog.layer_mut(CatalogLayer::Database);
    for i in 0..count as usize {
        // SAFETY: caller guarantees `tables[i]` is valid.
        let def = unsafe { &*tables.add(i) };
        // SAFETY: caller guarantees `name` and `columns` are valid.
        let Some((name, columns)) =
            (unsafe { relation_from_c(def.name, def.columns, def.column_count) })
        else {
            continue;
        };
        let table = TableDef::new(name);
        layer.insert_table(match columns {
            Some(columns) => table.columns(columns),
            None => table,
        });
    }
}

/// Add views to the database layer of the catalog. A view with an
/// out-of-range column kind is not added.
///
/// # Safety
///
/// Same requirements as `syntaqlite_analyzer_add_tables`, with `views`
/// pointing to `count` valid `SyntaqliteViewDef` entries.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_add_views(
    v: *mut SyntaqliteAnalyzer,
    views: *const SyntaqliteViewDef,
    count: u32,
) {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let layer = v.state_mut().user_catalog.layer_mut(CatalogLayer::Database);
    for i in 0..count as usize {
        // SAFETY: caller guarantees `views[i]` is valid.
        let def = unsafe { &*views.add(i) };
        // SAFETY: caller guarantees `name` and `columns` are valid.
        let Some((name, columns)) =
            (unsafe { relation_from_c(def.name, def.columns, def.column_count) })
        else {
            continue;
        };
        let view = ViewDef::new(name);
        layer.insert_view(match columns {
            Some(columns) => view.columns(columns),
            None => view,
        });
    }
}

/// A relation's name and columns, which are `None` when `columns` is NULL.
/// `None` overall when a column has an out-of-range kind.
///
/// # Safety
///
/// `name` must be a valid NUL-terminated C string. `columns` may be NULL;
/// if non-NULL it must point to `count` valid `SyntaqliteColumnDef` entries
/// whose names are valid NUL-terminated C strings.
unsafe fn relation_from_c(
    name: *const c_char,
    columns: *const SyntaqliteColumnDef,
    count: u32,
) -> Option<(String, Option<Vec<Column>>)> {
    // SAFETY: caller guarantees `name` is a valid NUL-terminated C string.
    let name = unsafe { CStr::from_ptr(name) }
        .to_str()
        .unwrap_or("")
        .to_owned();
    if columns.is_null() {
        return Some((name, None));
    }
    let columns = (0..count as usize)
        .map(|j| {
            // SAFETY: caller guarantees `columns[j]` is valid.
            let col = unsafe { &*columns.add(j) };
            // SAFETY: caller guarantees the column name is a valid NUL-terminated C string.
            let col_name = unsafe { CStr::from_ptr(col.name) }.to_str().unwrap_or("");
            Some(Column::new(col_name, column_kind_from_c(col.kind)?))
        })
        .collect::<Option<Vec<_>>>()?;
    Some((name, Some(columns)))
}

/// Load schema from DDL statements (CREATE TABLE, CREATE VIEW, etc.).
///
/// Returns the number of parse errors encountered (0 on success).
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// - `source` must be NULL (treated as empty) or point to `len` bytes of
///   valid UTF-8.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_analyzer_load_schema_ddl(
    v: *mut SyntaqliteAnalyzer,
    source: *const c_char,
    len: u32,
) -> u32 {
    // SAFETY: caller guarantees `v` is valid.
    let v = unsafe { &mut *v };
    let state = v.state_mut();

    // SAFETY: caller guarantees `source` is NULL or points to `len` bytes of
    // valid UTF-8.
    let src = unsafe { crate::dialect::ffi::source_str(source, len) };

    let (catalog, errors) = Catalog::from_ddl(state.dialect.clone(), &[src]);
    state
        .user_catalog
        .layer_mut(CatalogLayer::Database)
        .merge_from(catalog.layer(CatalogLayer::Database));
    errors.len() as u32
}

/// Register a scalar / aggregate / window function overload in the database
/// layer. Repeat calls with the same `name` build up an overload set.
///
/// # Safety
///
/// - `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// - `name` must be a valid NUL-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_add_function_overload(
    v: *mut SyntaqliteAnalyzer,
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
/// - `v` must be a valid pointer from `syntaqlite_analyzer_create_sqlite`.
/// - `name` must be a valid NUL-terminated C string.
/// - `output_columns` may be NULL. If non-NULL, must point to
///   `output_column_count` valid NUL-terminated C string pointers.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_analyzer_add_table_function(
    v: *mut SyntaqliteAnalyzer,
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
