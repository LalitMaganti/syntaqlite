// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Per-call analysis context.

use super::catalog::Catalog;
use super::config::AnalysisConfig;
use super::resolver::ModuleResolver;

/// Per-call context passed to [`Analyzer::analyze`]. Bundles the
/// catalog (mutated in place as DDL accumulates and imports are recorded),
/// the validation config, and an optional module resolver.
///
/// Construct via [`AnalysisContext::new`] with a `&mut Catalog`, then chain
/// [`with_config`](Self::with_config) and
/// [`with_resolver`](Self::with_resolver) as needed.
///
/// [`Analyzer::analyze`]: super::Analyzer::analyze
///
/// # Example
///
/// ```
/// # use syntaqlite::{Catalog, Analyzer, AnalysisConfig};
/// # use syntaqlite::analysis::AnalysisContext;
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// let mut analyzer = Analyzer::new();
///
/// let mut ctx = AnalysisContext::new(&mut catalog);
/// let model = analyzer.analyze("SELECT 1", &mut ctx);
/// assert!(!model.has_diagnostics());
/// ```
pub struct AnalysisContext<'a> {
    pub(crate) catalog: &'a mut Catalog,
    pub(crate) config: AnalysisConfig,
    pub(crate) resolver: Option<&'a dyn ModuleResolver>,
}

impl<'a> AnalysisContext<'a> {
    /// Create a context that mutates `catalog`, with default config and no
    /// module resolver.
    pub fn new(catalog: &'a mut Catalog) -> Self {
        AnalysisContext {
            catalog,
            config: AnalysisConfig::default(),
            resolver: None,
        }
    }

    /// Set the validation config.
    #[must_use]
    pub fn with_config(mut self, config: AnalysisConfig) -> Self {
        self.config = config;
        self
    }

    /// Attach a module resolver. Invoked by the analyzer when it encounters an
    /// `INCLUDE PERFETTO MODULE` (or equivalent) statement.
    #[must_use]
    pub fn with_resolver(mut self, resolver: &'a dyn ModuleResolver) -> Self {
        self.resolver = Some(resolver);
        self
    }

    /// The catalog being mutated by this analysis.
    pub fn catalog(&self) -> &Catalog {
        self.catalog
    }

    /// The catalog being mutated by this analysis (mutable).
    pub fn catalog_mut(&mut self) -> &mut Catalog {
        self.catalog
    }

    /// The validation config.
    pub fn config(&self) -> &AnalysisConfig {
        &self.config
    }

    /// The module resolver, if any.
    pub fn resolver(&self) -> Option<&dyn ModuleResolver> {
        self.resolver
    }
}
