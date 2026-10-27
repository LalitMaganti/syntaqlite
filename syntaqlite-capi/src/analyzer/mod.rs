// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C FFI layer for the semantic validator.
//!
//! Exposes [`Analyzer`](super::engine::Analyzer) and
//! [`Catalog`](super::catalog::Catalog) to C via opaque handle + accessor
//! functions, following the same pattern as the parser FFI in
//! `syntaqlite-syntax`.

use std::ffi::CString;

use syntaqlite::analysis::{
    Analysis, AnalysisConfig, Analyzer, Catalog, Diagnostic, ModuleResolver,
};
use syntaqlite::any::AnyDialect;

use types::{
    SyntaqliteColumnLineage, SyntaqliteColumnOrigin, SyntaqliteDefinedRelation,
    SyntaqliteDiagnostic, SyntaqlitePhysicalTableAccess, SyntaqliteRelationAccess,
    SyntaqliteUnexpandedView,
};

mod analyze;
mod catalog;
mod codes;
mod config;
mod lifecycle;
mod results;
mod statement;
mod types;

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests;

#[derive(Default)]
pub(super) struct PerStatementCache {
    pub(super) source: Option<CString>,
    pub(super) diagnostics: Option<(Vec<SyntaqliteDiagnostic>, Vec<CString>)>,
    pub(super) column_lineage: Option<(Vec<SyntaqliteColumnLineage>, Vec<CString>)>,
    pub(super) relations: Option<(Vec<SyntaqliteRelationAccess>, Vec<CString>)>,
    pub(super) physical_tables: Option<(Vec<SyntaqlitePhysicalTableAccess>, Vec<CString>)>,
    pub(super) defined_relations: Option<(Vec<SyntaqliteDefinedRelation>, Vec<CString>)>,
    pub(super) unexpanded_views: Option<(Vec<SyntaqliteUnexpandedView>, Vec<CString>)>,
}

pub(super) struct AnalyzerState {
    pub(super) analyzer: Analyzer,
    pub(super) user_catalog: Catalog,
    pub(super) dialect: AnyDialect,
    pub(super) analysis_config: AnalysisConfig,
    pub(super) resolver: Option<Box<dyn ModuleResolver>>,
    pub(super) c_diagnostics: Vec<SyntaqliteDiagnostic>,
    pub(super) rendered_messages: Vec<CString>,
    pub(super) last_source: String,
    pub(super) last_diagnostics: Vec<Diagnostic>,
    pub(super) rendered_output: CString,
    pub(super) lineage_complete: bool,
    pub(super) c_column_lineage: Vec<SyntaqliteColumnLineage>,
    pub(super) c_relations: Vec<SyntaqliteRelationAccess>,
    pub(super) c_physical_tables: Vec<SyntaqlitePhysicalTableAccess>,
    pub(super) c_unexpanded_views: Vec<SyntaqliteUnexpandedView>,
    pub(super) lineage_strings: Vec<CString>,
    pub(super) last_model: Option<Analysis>,
    pub(super) per_statement_cache: Vec<PerStatementCache>,
}

/// Opaque C handle — the pointer target of `SyntaqliteAnalyzer*`.
///
/// Never constructed or referenced in Rust; handle pointers are cast
/// directly to `AnalyzerState`.
#[repr(C)]
pub struct SyntaqliteAnalyzer {
    _private: [u8; 0],
}

impl SyntaqliteAnalyzer {
    /// # Safety
    ///
    /// `v` must be a live pointer from a `syntaqlite_analyzer_create_*`
    /// function with no mutable borrow of its state alive for `'a`.
    pub(super) unsafe fn state<'a>(v: *const Self) -> &'a AnalyzerState {
        // SAFETY: `v` was created from a `Box<AnalyzerState>` via
        // `Box::into_raw` cast in a `syntaqlite_analyzer_create_*` function.
        unsafe { &*v.cast::<AnalyzerState>() }
    }

    /// # Safety
    ///
    /// `v` must be a live pointer from a `syntaqlite_analyzer_create_*`
    /// function with no other borrow of its state alive for `'a`.
    pub(super) unsafe fn state_mut<'a>(v: *mut Self) -> &'a mut AnalyzerState {
        // SAFETY: same as above.
        unsafe { &mut *v.cast::<AnalyzerState>() }
    }
}
