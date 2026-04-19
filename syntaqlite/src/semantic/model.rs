// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Result types for a single semantic analysis pass.

#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
use syntaqlite_syntax::ParserTokenFlags;
#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
use syntaqlite_syntax::any::{AnyTokenType, TokenCategory};
#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

#[cfg(feature = "lsp")]
use std::collections::HashMap;

use super::diagnostics::Diagnostic;
use super::lineage::{
    ColumnLineage, LineageResult, PhysicalTableAccess, QueryLineage, RelationAccess,
};

// ── Stored per-statement positions ───────────────────────────────────────────

/// A token position recorded during parsing.
///
/// `token_type` is dialect-agnostic (`AnyTokenType`) so that the semantic
/// analyzer works with any dialect, not just the built-in `SQLite` dialect.
#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
#[derive(Debug, Clone)]
pub(crate) struct StoredToken {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
    pub(crate) token_type: AnyTokenType,
    pub(crate) flags: ParserTokenFlags,
}

/// A comment position recorded during parsing.
#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
#[derive(Debug, Clone)]
pub(crate) struct StoredComment {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
}

// ── Output types ──────────────────────────────────────────────────────────────

/// A semantic token for syntax highlighting.
#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
#[derive(Debug, Clone)]
pub(crate) struct SemanticToken {
    /// Document-absolute byte offset in the source text.
    pub offset: DocOffset,
    /// Length in bytes.
    pub length: DocLen,
    /// Token category for highlighting.
    pub category: TokenCategory,
}

/// Semantic completion context derived from parser stack state.
#[cfg(feature = "lsp")]
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum CompletionContext {
    /// Could not determine context.
    Unknown = 0,
    /// Cursor is in an expression position (functions/values expected).
    Expression = 1,
    /// Cursor is in a table-reference position (table/view names expected).
    TableRef = 2,
}

#[cfg(feature = "lsp")]
impl CompletionContext {
    pub(crate) fn from_parser(v: syntaqlite_syntax::CompletionContext) -> Self {
        match v {
            syntaqlite_syntax::CompletionContext::Expression => Self::Expression,
            syntaqlite_syntax::CompletionContext::TableRef => Self::TableRef,
            syntaqlite_syntax::CompletionContext::Unknown => Self::Unknown,
        }
    }
}

/// Expected tokens and semantic context at a cursor position.
#[cfg(feature = "lsp")]
#[derive(Debug)]
pub(crate) struct CompletionInfo {
    /// Terminal token types valid at the cursor (dialect-agnostic).
    pub tokens: Vec<AnyTokenType>,
    /// Semantic context (expression vs table-ref).
    pub context: CompletionContext,
    /// If the cursor follows `qualifier DOT`, this is the qualifier text.
    pub qualifier: Option<String>,
}

// ── Resolved symbols ──────────────────────────────────────────────────────────

/// A definition site that a reference points to.
#[cfg(feature = "lsp")]
#[derive(Debug, Clone)]
pub(crate) struct DefinitionLocation {
    pub range: DocRange,
    /// If `Some`, the definition is in a different file (e.g. an external schema).
    pub file_uri: Option<String>,
}

/// Result of a go-to-definition lookup: the origin span (reference token the
/// user clicked on) plus the target definition location.
#[cfg(feature = "lsp")]
#[derive(Debug, Clone)]
pub(crate) struct DefinitionResult {
    /// Document-absolute byte range of the reference token.
    pub origin: DocRange,
    /// The definition site this reference resolves to.
    pub target: DefinitionLocation,
}

/// A symbol resolution recorded during the validation pass.
#[cfg(feature = "lsp")]
#[derive(Debug, Clone)]
pub(crate) enum ResolvedSymbol {
    /// A table or view reference that resolved successfully.
    Table {
        name: String,
        columns: Option<Vec<String>>,
        /// Where this table/CTE was defined (byte offsets), if known.
        definition: Option<DefinitionLocation>,
    },
    /// A column reference that resolved successfully.
    Column {
        column: String,
        table: String,
        all_columns: Vec<String>,
        /// Where this column was defined (byte offsets), if known.
        definition: Option<DefinitionLocation>,
    },
    /// A function call that resolved successfully.
    Function {
        category: String,
        arities: Vec<String>,
    },
}

/// A resolved symbol at a specific source location.
#[cfg(feature = "lsp")]
#[derive(Debug, Clone)]
pub(crate) struct Resolution {
    pub range: DocRange,
    pub symbol: ResolvedSymbol,
}

// ── Per-statement model ──────────────────────────────────────────────────────

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
/// Owns the source text, stored token/comment positions, and per-statement
/// analysis results (diagnostics, lineage, defined relations). Produced by
/// [`SemanticAnalyzer::analyze`](super::analyzer::SemanticAnalyzer::analyze).
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
    #[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
    pub(crate) tokens: Vec<StoredToken>,
    #[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
    pub(crate) comments: Vec<StoredComment>,
    pub(crate) statements: Vec<StatementModel>,
    #[cfg(feature = "lsp")]
    pub(crate) resolutions: Vec<Resolution>,
    /// Same-file definition offsets keyed by lowercase name (table) or
    /// `table.column` (column). Used by find-references and rename to
    /// locate definition sites within the document.
    #[cfg(feature = "lsp")]
    pub(crate) definition_offsets: HashMap<String, DocRange>,
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

    /// Semantic tokens (for syntax highlighting) derived from the analyzed
    /// tokens plus comments, classified by `dialect`.
    #[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
    pub(crate) fn semantic_tokens(
        &self,
        dialect: &crate::dialect::AnyDialect,
    ) -> Vec<SemanticToken> {
        use syntaqlite_syntax::any::TokenCategory;
        let mut out = Vec::new();
        for t in &self.tokens {
            let cat = dialect.classify_token(t.token_type, t.flags);
            if cat != TokenCategory::Other {
                out.push(SemanticToken {
                    offset: t.offset,
                    length: t.length,
                    category: cat,
                });
            }
        }
        for c in &self.comments {
            out.push(SemanticToken {
                offset: c.offset,
                length: c.length,
                category: TokenCategory::Comment,
            });
        }
        out.sort_by_key(|t| t.offset);
        out
    }
}

#[cfg(feature = "lsp")]
impl SemanticModel {
    /// Find the resolved symbol at a byte offset, if any.
    pub(crate) fn resolution_at(&self, offset: DocOffset) -> Option<&ResolvedSymbol> {
        self.resolutions
            .iter()
            .find(|r| offset >= r.range.start && offset < r.range.end)
            .map(|r| &r.symbol)
    }

    /// Find the definition location for the symbol at a byte offset, if any.
    pub(crate) fn definition_at(&self, offset: DocOffset) -> Option<DefinitionResult> {
        self.resolutions
            .iter()
            .find(|r| offset >= r.range.start && offset < r.range.end)
            .and_then(|r| match &r.symbol {
                ResolvedSymbol::Table { definition, .. }
                | ResolvedSymbol::Column { definition, .. } => {
                    definition.as_ref().map(|d| DefinitionResult {
                        origin: r.range,
                        target: d.clone(),
                    })
                }
                ResolvedSymbol::Function { .. } => None,
            })
    }

    /// Find all resolutions in this model that match the given symbol identity.
    pub(crate) fn references_matching(&self, kind: &SymbolIdentity) -> Vec<DocRange> {
        self.resolutions
            .iter()
            .filter(|r| kind.matches(&r.symbol))
            .map(|r| r.range)
            .collect()
    }
}

/// Identity of a symbol for matching across resolutions (find-references / rename).
#[cfg(feature = "lsp")]
#[derive(Debug)]
pub(crate) enum SymbolIdentity {
    Table(String),
    Column { table: String, column: String },
}

#[cfg(feature = "lsp")]
impl SymbolIdentity {
    /// Derive the identity from a `ResolvedSymbol`.
    pub(crate) fn from_resolved(sym: &ResolvedSymbol) -> Option<Self> {
        match sym {
            ResolvedSymbol::Table { name, .. } => {
                Some(SymbolIdentity::Table(name.to_ascii_lowercase()))
            }
            ResolvedSymbol::Column { column, table, .. } => Some(SymbolIdentity::Column {
                table: table.to_ascii_lowercase(),
                column: column.to_ascii_lowercase(),
            }),
            ResolvedSymbol::Function { .. } => None,
        }
    }

    fn matches(&self, sym: &ResolvedSymbol) -> bool {
        match (self, sym) {
            (SymbolIdentity::Table(name), ResolvedSymbol::Table { name: n, .. }) => {
                n.eq_ignore_ascii_case(name)
            }
            (
                SymbolIdentity::Column { table, column },
                ResolvedSymbol::Column {
                    table: t,
                    column: c,
                    ..
                },
            ) => t.eq_ignore_ascii_case(table) && c.eq_ignore_ascii_case(column),
            _ => false,
        }
    }

    /// Key into `definition_offsets` for this symbol.
    pub(crate) fn definition_key(&self) -> String {
        match self {
            SymbolIdentity::Table(name) => name.clone(),
            SymbolIdentity::Column { table, column } => format!("{table}.{column}"),
        }
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
