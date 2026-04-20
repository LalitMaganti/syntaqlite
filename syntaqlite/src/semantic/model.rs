// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Result types for a single semantic analysis pass.

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
/// Each statement in the analyzed source produces its own `StatementModel`
/// containing diagnostics, lineage, and defined relations for that statement.
/// Access these via [`SemanticModel::statements`].
pub struct StatementModel {
    source: String,
    diagnostics: Vec<Diagnostic>,
    lineage: Option<QueryLineage>,
    defined_relations: Vec<DefinedRelation>,
}

impl StatementModel {
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

// ── SemanticModel ─────────────────────────────────────────────────────────────

/// Result of a single analysis pass.
///
/// Owns the source text and per-statement analysis results (diagnostics,
/// lineage, defined relations). Produced by
/// [`SemanticAnalyzer::analyze`](super::analyzer::SemanticAnalyzer::analyze).
///
/// For incremental events (symbol resolutions, definition sites, tokens,
/// comments) use
/// [`SemanticAnalyzer::analyze_with_observer`](super::analyzer::SemanticAnalyzer::analyze_with_observer)
/// and supply an [`AnalysisObserver`](super::observer::AnalysisObserver).
///
/// # Example
///
/// ```
/// # use syntaqlite::{
/// #     SemanticAnalyzer, Catalog, ValidationConfig,
/// # };
/// # use syntaqlite::semantic::{CatalogLayer, Severity};
/// let mut analyzer = SemanticAnalyzer::new();
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
///
/// let model = analyzer.analyze(
///     "SELECT emial FROM users;",
///     &catalog,
///     &ValidationConfig::default(),
/// );
///
/// // Iterate diagnostics to find the warning about "emial".
/// for diag in model.diagnostics() {
///     assert_eq!(diag.severity(), Severity::Warning);
///     let msg = diag.message().to_string();
///     assert!(msg.contains("emial"));
/// }
/// ```
pub struct SemanticModel {
    pub(crate) source: String,
    pub(crate) statements: Vec<StatementModel>,
}

impl SemanticModel {
    /// The source text that was analyzed.
    pub fn source(&self) -> &str {
        &self.source
    }

    /// All per-statement analysis results.
    pub fn statements(&self) -> &[StatementModel] {
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
            .find_map(StatementModel::lineage)
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::super::ValidationConfig;
    use super::super::analyzer::SemanticAnalyzer;
    use super::super::catalog::{Catalog, CatalogLayer};

    fn lenient() -> ValidationConfig {
        ValidationConfig::default()
    }

    fn sqlite_catalog() -> Catalog {
        Catalog::new(crate::sqlite::dialect::dialect())
    }

    #[test]
    fn statements_returns_one_per_statement() {
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT 1; SELECT 2; SELECT 3;", &catalog, &lenient());
        assert_eq!(model.statements().len(), 3);
    }

    #[test]
    fn diagnostics_isolated_per_statement() {
        let mut analyzer = SemanticAnalyzer::new();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into()]),
            false,
        );
        let model = analyzer.analyze(
            "SELECT id FROM users; SELECT * FROM missing;",
            &catalog,
            &lenient(),
        );
        assert_eq!(model.statements().len(), 2);
        assert!(model.statements()[0].diagnostics().is_empty());
        assert!(!model.statements()[1].diagnostics().is_empty());
        assert_eq!(model.diagnostic_count(), 1);
    }

    #[test]
    fn lineage_delegates_to_last_statement() {
        let mut analyzer = SemanticAnalyzer::new();
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table("a", Some(vec!["x".into()]), false);
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table("b", Some(vec!["y".into()]), false);
        let model = analyzer.analyze("SELECT x FROM a; SELECT y FROM b;", &catalog, &lenient());

        let cols0 = model.statements()[0].lineage().unwrap().into_inner();
        assert_eq!(cols0[0].origin.as_ref().unwrap().table, "a");

        let cols1 = model.statements()[1].lineage().unwrap().into_inner();
        assert_eq!(cols1[0].origin.as_ref().unwrap().table, "b");

        let last_cols = model.lineage().unwrap().into_inner();
        assert_eq!(last_cols[0].origin.as_ref().unwrap().table, "b");
    }

    #[test]
    fn defined_relations_for_create_table() {
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze(
            "CREATE TABLE users (id INTEGER, name TEXT);",
            &catalog,
            &lenient(),
        );
        let defs = model.statements()[0].defined_relations();
        assert_eq!(defs.len(), 1);
        assert_eq!(defs[0].name, "users");
        assert!(!defs[0].is_view);
    }

    #[test]
    fn defined_relations_for_create_view() {
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("CREATE VIEW v AS SELECT 1;", &catalog, &lenient());
        let defs = model.statements()[0].defined_relations();
        assert_eq!(defs.len(), 1);
        assert_eq!(defs[0].name, "v");
        assert!(defs[0].is_view);
    }

    #[test]
    fn defined_relations_empty_for_select() {
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT 1;", &catalog, &lenient());
        assert!(model.statements()[0].defined_relations().is_empty());
    }

    #[test]
    fn parse_error_produces_statement_model_with_diagnostic() {
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT;", &catalog, &lenient());
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
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT 1;", &catalog, &lenient());
        assert!(!model.has_diagnostics());
        assert_eq!(model.diagnostic_count(), 0);
    }

    #[test]
    fn unexpanded_view_surfaced_on_view_access() {
        let mut analyzer = SemanticAnalyzer::new();
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_view("active_users", Some(vec!["id".into(), "name".into()]));

        let model = analyzer.analyze("SELECT id FROM active_users", &catalog, &lenient());

        let unresolved = model.statements().last().unwrap().unexpanded_views();
        assert_eq!(unresolved, ["active_users".to_string()].as_slice());
    }

    #[test]
    fn table_access_has_no_unexpanded_views() {
        let mut analyzer = SemanticAnalyzer::new();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into()]),
            false,
        );
        let model = analyzer.analyze("SELECT id FROM users", &catalog, &lenient());
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
        let mut analyzer = SemanticAnalyzer::new();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("CREATE TABLE t(x)", &catalog, &lenient());
        assert!(model.lineage().is_none());
    }

    #[test]
    fn recursive_cte_does_not_stack_overflow() {
        let mut analyzer = SemanticAnalyzer::new();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze(
            "WITH RECURSIVE cte(id) AS (
                SELECT id FROM users
                UNION ALL
                SELECT id FROM cte
            ) SELECT id FROM cte",
            &catalog,
            &lenient(),
        );
        let lineage = model.lineage().expect("should be a query");
        let cols = lineage.into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(cols[0].name, "id");
    }
}
