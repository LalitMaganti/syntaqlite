// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C FFI layer for the semantic validator.
//!
//! Exposes [`SemanticAnalyzer`](super::analyzer::SemanticAnalyzer) and
//! [`Catalog`](super::catalog::Catalog) to C via opaque handle + accessor
//! functions, following the same pattern as the parser FFI in
//! `syntaqlite-syntax`.

use std::ffi::CString;

use crate::dialect::AnyDialect;

use super::ValidationConfig;
use super::analyzer::SemanticAnalyzer;
use super::catalog::Catalog;
use super::model::SemanticModel;

use types::{
    SyntaqliteColumnLineage, SyntaqliteColumnOrigin, SyntaqliteDefinedRelation,
    SyntaqliteDiagnostic, SyntaqlitePhysicalTableAccess, SyntaqliteRelationAccess,
    SyntaqliteRelationDef, SyntaqliteUnexpandedView,
};

mod analyze;
mod catalog_ffi;
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

pub(super) struct ValidatorState {
    pub(super) analyzer: SemanticAnalyzer,
    pub(super) user_catalog: Catalog,
    pub(super) dialect: AnyDialect,
    pub(super) validation_config: ValidationConfig,
    pub(super) resolver: Option<Box<dyn super::ModuleResolver>>,
    pub(super) c_diagnostics: Vec<SyntaqliteDiagnostic>,
    pub(super) rendered_messages: Vec<CString>,
    pub(super) last_source: String,
    pub(super) last_diagnostics: Vec<super::diagnostics::Diagnostic>,
    pub(super) rendered_output: CString,
    pub(super) lineage_complete: bool,
    pub(super) c_column_lineage: Vec<SyntaqliteColumnLineage>,
    pub(super) c_relations: Vec<SyntaqliteRelationAccess>,
    pub(super) c_physical_tables: Vec<SyntaqlitePhysicalTableAccess>,
    pub(super) c_unexpanded_views: Vec<SyntaqliteUnexpandedView>,
    pub(super) lineage_strings: Vec<CString>,
    pub(super) last_model: Option<SemanticModel>,
    pub(super) per_statement_cache: Vec<PerStatementCache>,
}

/// Opaque C handle — the pointer target of `SyntaqliteValidator*`.
///
/// This is a zero-variant enum so that Rust cannot construct it directly;
/// all access goes through raw pointer casts to `ValidatorState`.
pub enum SyntaqliteValidator {}

impl SyntaqliteValidator {
    pub(super) fn state(&self) -> &ValidatorState {
        // SAFETY: `self` was created from a `Box<ValidatorState>` via
        // `Box::into_raw` cast in `syntaqlite_validator_create_sqlite`.
        unsafe { &*std::ptr::from_ref::<Self>(self).cast::<ValidatorState>() }
    }

    pub(super) fn state_mut(&mut self) -> &mut ValidatorState {
        // SAFETY: `self` was created from a `Box<ValidatorState>` via
        // `Box::into_raw` cast in `syntaqlite_validator_create_sqlite`.
        unsafe { &mut *std::ptr::from_mut::<Self>(self).cast::<ValidatorState>() }
    }
}
