// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Result types for a single analysis pass.

use super::diagnostics::Diagnostic;
use super::lineage::{
    ColumnLineage, LineageResult, PhysicalTableAccess, QueryLineage, RelationAccess,
};

/// A relation defined by a DDL statement (e.g. `CREATE TABLE`, `CREATE VIEW`).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DefinedRelation {
    /// The relation name as it appears in the DDL statement.
    pub name: String,
    /// Whether this is a view (`true`) or a table (`false`).
    pub is_view: bool,
}

/// Per-statement analysis result.
///
/// Each statement in the analyzed source produces its own `StatementAnalysis`
/// containing diagnostics, lineage, and defined relations for that statement.
/// Access these via [`Analysis::statements`].
pub struct StatementAnalysis {
    source: String,
    diagnostics: Vec<Diagnostic>,
    lineage: Option<QueryLineage>,
    defined_relations: Vec<DefinedRelation>,
}

impl StatementAnalysis {
    /// Create a new per-statement model.
    pub(crate) fn new(
        source: String,
        diagnostics: Vec<Diagnostic>,
        lineage: Option<QueryLineage>,
        defined_relations: Vec<DefinedRelation>,
    ) -> Self {
        Self {
            source,
            diagnostics,
            lineage,
            defined_relations,
        }
    }

    /// The SQL source text for this statement.
    pub fn source(&self) -> &str {
        &self.source
    }

    /// All diagnostics produced for this statement.
    pub fn diagnostics(&self) -> &[Diagnostic] {
        &self.diagnostics
    }

    /// Per-column lineage for this statement's query body.
    ///
    /// Returns `None` if the statement is not a query or DDL-with-SELECT.
    /// Returns `Some(Complete(...))` when all columns are fully resolved.
    /// Returns `Some(Partial(...))` when some view bodies are unavailable.
    pub fn lineage(&self) -> Option<LineageResult<&[ColumnLineage]>> {
        self.lineage.as_ref().map(|ql| {
            if ql.complete {
                LineageResult::Complete(ql.columns.as_slice())
            } else {
                LineageResult::Partial(ql.columns.as_slice())
            }
        })
    }

    /// Relations (tables, views) directly referenced in FROM for this statement.
    ///
    /// Returns `None` if this statement did not contain a query body.
    pub fn relations_accessed(&self) -> Option<LineageResult<&[RelationAccess]>> {
        self.lineage.as_ref().map(|ql| {
            if ql.complete {
                LineageResult::Complete(ql.relations.as_slice())
            } else {
                LineageResult::Partial(ql.relations.as_slice())
            }
        })
    }

    /// Physical tables accessed by this statement (after resolving CTEs,
    /// subqueries, views).
    ///
    /// Returns `None` if this statement did not contain a query body.
    pub fn physical_tables_accessed(&self) -> Option<LineageResult<&[PhysicalTableAccess]>> {
        self.lineage.as_ref().map(|ql| {
            if ql.complete {
                LineageResult::Complete(ql.physical_tables.as_slice())
            } else {
                LineageResult::Partial(ql.physical_tables.as_slice())
            }
        })
    }

    /// Relations defined by this DDL statement (e.g. `CREATE TABLE`, `CREATE VIEW`).
    pub fn defined_relations(&self) -> &[DefinedRelation] {
        &self.defined_relations
    }

    /// Canonical names of views referenced in this statement whose bodies
    /// could not be expanded (no DDL available). Empty when all sources were
    /// fully resolved or when the statement is not a query.
    pub fn unexpanded_views(&self) -> &[String] {
        self.lineage
            .as_ref()
            .map_or(&[], |ql| ql.unexpanded_views.as_slice())
    }
}

// ── Analysis ─────────────────────────────────────────────────────────────

/// Result of a single analysis pass.
///
/// Owns the source text and per-statement analysis results (diagnostics,
/// lineage, defined relations). Produced by
/// [`Analyzer::analyze`](super::engine::Analyzer::analyze).
///
/// # Example
///
/// ```
/// # use syntaqlite::{AnalysisContext, Analyzer, Catalog};
/// # use syntaqlite::analysis::{CatalogLayer, Severity};
/// let mut analyzer = Analyzer::new();
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
///
/// let mut ctx = AnalysisContext::new(&mut catalog);
/// let model = analyzer.analyze("SELECT emial FROM users;", &mut ctx);
///
/// // Iterate diagnostics to find the warning about "emial".
/// for diag in model.diagnostics() {
///     assert_eq!(diag.severity(), Severity::Warning);
///     let msg = diag.message().to_string();
///     assert!(msg.contains("emial"));
/// }
/// ```
pub struct Analysis {
    pub(crate) source: String,
    pub(crate) statements: Vec<StatementAnalysis>,
}

impl Analysis {
    /// The source text that was analyzed.
    pub fn source(&self) -> &str {
        &self.source
    }

    /// All per-statement analysis results.
    pub fn statements(&self) -> &[StatementAnalysis] {
        &self.statements
    }

    /// All diagnostics produced by the analysis pass (parse errors + semantic
    /// issues), aggregated across all statements.
    pub fn diagnostics(&self) -> impl Iterator<Item = &Diagnostic> {
        self.statements.iter().flat_map(|s| s.diagnostics.iter())
    }

    /// The total number of diagnostics across all statements.
    pub fn diagnostic_count(&self) -> usize {
        self.statements.iter().map(|s| s.diagnostics.len()).sum()
    }

    /// Whether any statement produced diagnostics.
    pub fn has_diagnostics(&self) -> bool {
        self.statements.iter().any(|s| !s.diagnostics.is_empty())
    }

    /// Per-column lineage for the **last** query or DDL-with-SELECT in the
    /// analyzed source.
    ///
    /// Works for `SELECT` statements as well as DDL with inner queries
    /// (`CREATE TABLE/VIEW/FUNCTION ... AS SELECT`).
    /// Returns `None` if no statement contained a query body.
    /// Returns `Some(Complete(...))` when all columns are fully resolved.
    /// Returns `Some(Partial(...))` when some view bodies are unavailable.
    pub fn lineage(&self) -> Option<LineageResult<&[ColumnLineage]>> {
        self.statements
            .iter()
            .rev()
            .find_map(StatementAnalysis::lineage)
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::super::AnalysisContext;
    use super::super::catalog::{Catalog, CatalogLayer};
    use super::super::engine::Analyzer;

    fn sqlite_catalog() -> Catalog {
        Catalog::new(crate::sqlite::dialect::dialect())
    }

    #[test]
    fn statements_returns_one_per_statement() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT 1; SELECT 2; SELECT 3;", &mut ctx);
        assert_eq!(model.statements().len(), 3);
    }

    #[test]
    fn diagnostics_isolated_per_statement() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into()]),
            false,
        );
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT id FROM users; SELECT * FROM missing;", &mut ctx);
        assert_eq!(model.statements().len(), 2);
        assert!(model.statements()[0].diagnostics().is_empty());
        assert!(!model.statements()[1].diagnostics().is_empty());
        assert_eq!(model.diagnostic_count(), 1);
    }

    #[test]
    fn lineage_delegates_to_last_statement() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table("a", Some(vec!["x".into()]), false);
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table("b", Some(vec!["y".into()]), false);
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT x FROM a; SELECT y FROM b;", &mut ctx);

        let cols0 = model.statements()[0].lineage().unwrap().into_inner();
        assert_eq!(cols0[0].origin.as_ref().unwrap().table, "a");

        let cols1 = model.statements()[1].lineage().unwrap().into_inner();
        assert_eq!(cols1[0].origin.as_ref().unwrap().table, "b");

        let last_cols = model.lineage().unwrap().into_inner();
        assert_eq!(last_cols[0].origin.as_ref().unwrap().table, "b");
    }

    #[test]
    fn defined_relations_for_create_table() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("CREATE TABLE users (id INTEGER, name TEXT);", &mut ctx);
        let defs = model.statements()[0].defined_relations();
        assert_eq!(defs.len(), 1);
        assert_eq!(defs[0].name, "users");
        assert!(!defs[0].is_view);
    }

    #[test]
    fn defined_relations_for_create_view() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("CREATE VIEW v AS SELECT 1;", &mut ctx);
        let defs = model.statements()[0].defined_relations();
        assert_eq!(defs.len(), 1);
        assert_eq!(defs[0].name, "v");
        assert!(defs[0].is_view);
    }

    #[test]
    fn defined_relations_empty_for_select() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT 1;", &mut ctx);
        assert!(model.statements()[0].defined_relations().is_empty());
    }

    #[test]
    fn parse_error_produces_statement_model_with_diagnostic() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT;", &mut ctx);
        assert!(model.has_diagnostics());
        assert!(
            model
                .statements()
                .iter()
                .any(|s| !s.diagnostics().is_empty())
        );
    }

    #[test]
    fn clean_source_has_no_diagnostics() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT 1;", &mut ctx);
        assert!(!model.has_diagnostics());
        assert_eq!(model.diagnostic_count(), 0);
    }

    #[test]
    fn unexpanded_view_surfaced_on_view_access() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_view("active_users", Some(vec!["id".into(), "name".into()]));

        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT id FROM active_users", &mut ctx);

        let unresolved = model.statements().last().unwrap().unexpanded_views();
        assert_eq!(unresolved, ["active_users".to_string()].as_slice());
    }

    #[test]
    fn table_access_has_no_unexpanded_views() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into()]),
            false,
        );
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("SELECT id FROM users", &mut ctx);
        assert!(
            model
                .statements()
                .last()
                .unwrap()
                .unexpanded_views()
                .is_empty()
        );
    }

    #[test]
    fn non_select_has_no_lineage() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze("CREATE TABLE t(x)", &mut ctx);
        assert!(model.lineage().is_none());
    }

    #[test]
    fn recursive_cte_does_not_stack_overflow() {
        let mut analyzer = Analyzer::new();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = analyzer.analyze(
            "WITH RECURSIVE cte(id) AS (
                SELECT id FROM users
                UNION ALL
                SELECT id FROM cte
            ) SELECT id FROM cte",
            &mut ctx,
        );
        let lineage = model.lineage().expect("should be a query");
        let cols = lineage.into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(cols[0].name, "id");
    }
}
