// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Per-statement accessors. Each accessor lazily populates a slot in the
//! `PerStatementCache` so that the C pointers returned are stable between
//! `analyze()` calls.

use std::ffi::{CString, c_char};

use crate::semantic::lineage::RelationKind;
use crate::semantic::model::StatementModel;

use super::codes::{diagnostic_code_to_c, severity_to_c};
use super::{
    PerStatementCache, SyntaqliteColumnLineage, SyntaqliteColumnOrigin, SyntaqliteDefinedRelation,
    SyntaqliteDiagnostic, SyntaqlitePhysicalTableAccess, SyntaqliteRelationAccess,
    SyntaqliteUnexpandedView, SyntaqliteValidator,
};

fn ensure_source<'a>(cache: &'a mut PerStatementCache, stmt: &StatementModel) -> &'a CString {
    cache
        .source
        .get_or_insert_with(|| CString::new(stmt.source()).unwrap_or_default())
}

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
                start_offset: d.range().start.as_u32(),
                end_offset: d.range().end.as_u32(),
                kind_code: diagnostic_code_to_c(d.message()),
            });
        }
        (diags, msgs)
    })
}

fn ensure_column_lineage<'a>(
    cache: &'a mut PerStatementCache,
    stmt: &StatementModel,
) -> &'a (Vec<SyntaqliteColumnLineage>, Vec<CString>) {
    cache.column_lineage.get_or_insert_with(|| {
        let mut strings = Vec::new();
        let mut cols = Vec::new();
        if let Some(lineage) = stmt.lineage() {
            let inner = lineage.into_inner();
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
    // SAFETY: model lives in state and won't move while we hold &mut state.
    let stmt: &StatementModel = unsafe { &*std::ptr::from_ref::<StatementModel>(stmt) };
    Some((stmt, cache))
}

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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
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
    // SAFETY: caller guarantees `v` is valid.
    let Some((s, c)) = (unsafe { stmt_cache(v, idx) }) else {
        return std::ptr::null();
    };
    cached_slice(&ensure_unexpanded_views(c, s).0).0
}
