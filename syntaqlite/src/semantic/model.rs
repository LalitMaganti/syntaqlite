// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Result types for a single semantic analysis pass.

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::{AnyTokenType, TokenCategory};

use std::collections::HashMap;

use super::diagnostics::Diagnostic;
use super::lineage::{ColumnLineage, LineageResult, QueryLineage, RelationAccess, TableAccess};

// ── Stored per-statement positions ───────────────────────────────────────────

/// A token position recorded during parsing.
///
/// `token_type` is grammar-agnostic (`AnyTokenType`) so that the semantic
/// analyzer works with any dialect, not just the built-in `SQLite` grammar.
#[derive(Debug, Clone)]
pub(crate) struct StoredToken {
    pub(crate) offset: usize,
    pub(crate) length: usize,
    pub(crate) token_type: AnyTokenType,
    pub(crate) flags: ParserTokenFlags,
}

/// A comment position recorded during parsing.
#[derive(Debug, Clone)]
pub(crate) struct StoredComment {
    pub(crate) offset: usize,
    pub(crate) length: usize,
}

// ── Output types ──────────────────────────────────────────────────────────────

/// A semantic token for syntax highlighting.
#[derive(Debug, Clone)]
pub(crate) struct SemanticToken {
    /// Byte offset in the source text.
    pub offset: usize,
    /// Length in bytes.
    pub length: usize,
    /// Token category for highlighting.
    pub category: TokenCategory,
}

/// Semantic completion context derived from parser stack state.
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
#[derive(Debug)]
pub(crate) struct CompletionInfo {
    /// Terminal token types valid at the cursor (grammar-agnostic).
    pub tokens: Vec<AnyTokenType>,
    /// Semantic context (expression vs table-ref).
    pub context: CompletionContext,
    /// If the cursor follows `qualifier DOT`, this is the qualifier text.
    pub qualifier: Option<String>,
}

// ── Resolved symbols ──────────────────────────────────────────────────────────

/// A definition site that a reference points to.
#[derive(Debug, Clone)]
pub(crate) struct DefinitionLocation {
    pub start: usize,
    pub end: usize,
    /// If `Some`, the definition is in a different file (e.g. an external schema).
    pub file_uri: Option<String>,
}

/// Result of a go-to-definition lookup: the origin span (reference token the
/// user clicked on) plus the target definition location.
#[derive(Debug, Clone)]
pub(crate) struct DefinitionResult {
    /// Byte offset of the start of the reference token.
    pub origin_start: usize,
    /// Byte offset of the end of the reference token.
    pub origin_end: usize,
    /// The definition site this reference resolves to.
    pub target: DefinitionLocation,
}

/// A symbol resolution recorded during the validation pass.
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
#[derive(Debug, Clone)]
pub(crate) struct Resolution {
    pub start: usize,
    pub end: usize,
    pub symbol: ResolvedSymbol,
}

// ── SemanticModel ─────────────────────────────────────────────────────────────

/// Result of a single analysis pass.
///
/// Owns the source text, stored token/comment positions, and all diagnostics
/// (both parse errors and semantic issues). Produced by
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
    pub(crate) tokens: Vec<StoredToken>,
    pub(crate) comments: Vec<StoredComment>,
    pub(crate) diagnostics: Vec<Diagnostic>,
    pub(crate) resolutions: Vec<Resolution>,
    /// Same-file definition offsets keyed by lowercase name (table) or
    /// `table.column` (column). Used by find-references and rename to
    /// locate definition sites within the document.
    pub(crate) definition_offsets: HashMap<String, (usize, usize)>,
    /// Column lineage for the last query statement, if any.
    pub(crate) query_lineage: Option<QueryLineage>,
}

impl SemanticModel {
    /// The source text that was analyzed.
    pub fn source(&self) -> &str {
        &self.source
    }

    /// All diagnostics produced by the analysis pass (parse errors + semantic issues).
    pub fn diagnostics(&self) -> &[Diagnostic] {
        &self.diagnostics
    }

    /// Per-column lineage: for each result column, which table.column it traces to.
    ///
    /// Returns `None` if the analyzed statement is not a query.
    /// Returns `Some(Complete(...))` when all columns are fully resolved.
    /// Returns `Some(Partial(...))` when some view bodies are unavailable.
    pub fn lineage(&self) -> Option<LineageResult<&[ColumnLineage]>> {
        self.query_lineage.as_ref().map(|ql| {
            if ql.complete {
                LineageResult::Complete(ql.columns.as_slice())
            } else {
                LineageResult::Partial(ql.columns.as_slice())
            }
        })
    }

    /// Relations (tables, views, CTEs, subquery aliases) directly referenced in FROM.
    ///
    /// Returns `None` if the analyzed statement is not a query.
    pub fn relations_accessed(&self) -> Option<LineageResult<&[RelationAccess]>> {
        self.query_lineage.as_ref().map(|ql| {
            if ql.complete {
                LineageResult::Complete(ql.relations.as_slice())
            } else {
                LineageResult::Partial(ql.relations.as_slice())
            }
        })
    }

    /// Physical tables accessed (after resolving CTEs, subqueries, views).
    ///
    /// Returns `None` if the analyzed statement is not a query.
    pub fn tables_accessed(&self) -> Option<LineageResult<&[TableAccess]>> {
        self.query_lineage.as_ref().map(|ql| {
            if ql.complete {
                LineageResult::Complete(ql.tables.as_slice())
            } else {
                LineageResult::Partial(ql.tables.as_slice())
            }
        })
    }

    /// Find the resolved symbol at a byte offset, if any.
    pub(crate) fn resolution_at(&self, offset: usize) -> Option<&ResolvedSymbol> {
        self.resolutions
            .iter()
            .find(|r| offset >= r.start && offset < r.end)
            .map(|r| &r.symbol)
    }

    /// Find the definition location for the symbol at a byte offset, if any.
    pub(crate) fn definition_at(&self, offset: usize) -> Option<DefinitionResult> {
        self.resolutions
            .iter()
            .find(|r| offset >= r.start && offset < r.end)
            .and_then(|r| match &r.symbol {
                ResolvedSymbol::Table { definition, .. }
                | ResolvedSymbol::Column { definition, .. } => {
                    definition.as_ref().map(|d| DefinitionResult {
                        origin_start: r.start,
                        origin_end: r.end,
                        target: d.clone(),
                    })
                }
                ResolvedSymbol::Function { .. } => None,
            })
    }

    /// Find all resolutions in this model that match the given symbol identity.
    pub(crate) fn references_matching(&self, kind: &SymbolIdentity) -> Vec<(usize, usize)> {
        self.resolutions
            .iter()
            .filter(|r| kind.matches(&r.symbol))
            .map(|r| (r.start, r.end))
            .collect()
    }
}

/// Identity of a symbol for matching across resolutions (find-references / rename).
#[derive(Debug)]
pub(crate) enum SymbolIdentity {
    Table(String),
    Column { table: String, column: String },
}

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
