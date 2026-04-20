// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! The main `analyze` entry point and lineage result population.

use std::ffi::{CString, c_char};

use crate::semantic::AnalysisContext;
use crate::semantic::lineage::RelationKind;
use crate::semantic::model::SemanticModel;

use super::codes::{diagnostic_code_to_c, severity_to_c};
use super::{
    PerStatementCache, SyntaqliteColumnLineage, SyntaqliteColumnOrigin, SyntaqliteDiagnostic,
    SyntaqlitePhysicalTableAccess, SyntaqliteRelationAccess, SyntaqliteUnexpandedView,
    SyntaqliteValidator, ValidatorState,
};

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

    let mut ctx =
        AnalysisContext::new(&mut state.user_catalog).with_config(state.validation_config);
    if let Some(r) = state.resolver.as_deref() {
        ctx = ctx.with_resolver(r);
    }
    let model = state.analyzer.analyze(src, &mut ctx);

    let all_diagnostics: Vec<_> = model.diagnostics().cloned().collect();

    state.last_source.clear();
    state.last_source.push_str(src);
    state.last_diagnostics.clear();
    state
        .last_diagnostics
        .extend(all_diagnostics.iter().cloned());

    state.rendered_messages.clear();
    state.c_diagnostics.clear();

    for d in &all_diagnostics {
        state
            .rendered_messages
            .push(CString::new(d.message().to_string()).unwrap_or_default());
    }

    for (d, msg) in all_diagnostics.iter().zip(state.rendered_messages.iter()) {
        state.c_diagnostics.push(SyntaqliteDiagnostic {
            severity: severity_to_c(d.severity()),
            message: msg.as_ptr(),
            start_offset: d.range().start.as_u32(),
            end_offset: d.range().end.as_u32(),
            kind_code: diagnostic_code_to_c(d.message()),
        });
    }

    populate_lineage(state, &model);

    let stmt_count = model.statements().len();
    state.per_statement_cache.clear();
    state
        .per_statement_cache
        .resize_with(stmt_count, PerStatementCache::default);
    state.last_model = Some(model);

    state.c_diagnostics.len() as u32
}

#[expect(clippy::too_many_lines)]
pub(super) fn populate_lineage(state: &mut ValidatorState, model: &SemanticModel) {
    state.lineage_strings.clear();
    state.c_column_lineage.clear();
    state.c_relations.clear();
    state.c_physical_tables.clear();
    state.c_unexpanded_views.clear();
    state.lineage_complete = false;

    if let Some(lineage_result) = model.lineage() {
        state.lineage_complete = lineage_result.is_complete();
        let columns = lineage_result.into_inner();

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
