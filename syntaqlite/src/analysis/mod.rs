// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Analysis and validation.
//!
//! Validates SQL against a known database schema — resolving table, column,
//! and function references and producing structured [`Diagnostic`] values with
//! byte-offset spans and "did you mean?" suggestions.
//!
//! The most commonly used types ([`Analyzer`], [`Catalog`],
//! [`CatalogLayer`], [`Diagnostic`], [`Severity`], [`AnalysisConfig`]) are
//! re-exported at the crate root. This module also provides:
//!
//! - [`Analysis`] — the result of a single analysis pass.
//! - [`DiagnosticMessage`] — structured message variants for pattern matching.
//! - [`Help`] — "did you mean?" suggestion attached to a diagnostic.
//! - [`AnalysisMode`] — document vs. execute mode for DDL accumulation.
//! - [`CatalogLayerContents`] — the data stored in a single catalog layer.
//! - [`AritySpec`], [`FunctionCategory`] — function metadata for catalog
//!   registration.
//!
//! # Example
//!
//! ```
//! use syntaqlite::analysis::{
//!     AnalysisContext, Analyzer, Catalog, CatalogLayer,
//! };
//!
//! let mut analyzer = Analyzer::new();
//! let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
//! catalog.layer_mut(CatalogLayer::Database)
//!     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
//!
//! let mut ctx = AnalysisContext::new(&mut catalog);
//! let model = analyzer.analyze("SELECT id, nme FROM users", &mut ctx);
//!
//! // "nme" is close to "name" — expect a diagnostic with a suggestion.
//! assert!(model.has_diagnostics());
//! ```

#[cfg(feature = "analysis")]
pub(crate) mod catalog;
#[cfg(feature = "analysis")]
pub(crate) mod completion;
#[cfg(feature = "analysis")]
pub(crate) mod config;
#[cfg(feature = "analysis")]
pub(crate) mod context;
#[cfg(feature = "analysis")]
pub(crate) mod ddl;
pub(crate) mod diagnostics;
#[cfg(feature = "analysis")]
pub(crate) mod engine;
#[cfg(feature = "analysis")]
#[expect(unreachable_pub)]
pub(crate) mod ffi;
#[cfg(feature = "analysis")]
mod lineage;
#[cfg(feature = "analysis")]
pub(crate) mod model;
#[cfg(feature = "analysis")]
pub(crate) mod resolver;

// ── Public re-exports ─────────────────────────────────────────────────────────

#[doc(inline)]
#[cfg(feature = "analysis")]
pub use catalog::{AritySpec, Catalog, CatalogLayer, CatalogLayerContents, FunctionCategory};
#[doc(inline)]
#[cfg(feature = "analysis")]
pub use config::{AnalysisConfig, AnalysisMode, CheckConfig, CheckLevel};
#[doc(inline)]
#[cfg(feature = "analysis")]
pub use context::AnalysisContext;
#[doc(inline)]
pub use diagnostics::{Diagnostic, DiagnosticMessage, Help, Severity};
#[doc(inline)]
#[cfg(feature = "analysis")]
pub use engine::Analyzer;
#[doc(inline)]
#[cfg(feature = "analysis")]
pub use lineage::{
    ColumnLineage, ColumnOrigin, LineageResult, PhysicalTableAccess, RelationAccess, RelationKind,
};
#[doc(inline)]
#[cfg(feature = "analysis")]
pub use model::{Analysis, DefinedRelation, StatementAnalysis};
#[doc(inline)]
#[cfg(feature = "analysis")]
pub use resolver::{DirectoryModuleResolver, ModuleResolver};
