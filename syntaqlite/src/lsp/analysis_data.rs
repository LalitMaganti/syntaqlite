// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP-shaped analysis data and the capture pass that populates it.
//!
//! The types in this module describe analysis results in the shape LSP
//! services want: resolved symbols with markdown-friendly fields, cross-file
//! definition locations, per-document bundles. Generic token/comment data
//! lives in [`crate::analysis::analysis`].
//!
//! - [`LspCapturePass`] — a [`WalkPass`] impl that fills a
//!   [`DocumentAnalysisData`] directly during analysis.
//! - [`ExternalDefinitions`] — a cross-file definition-site registry consulted
//!   by the capture pass to correlate resolutions with schema files.

use std::collections::HashMap;

use syntaqlite_syntax::any::TokenCategory;
use syntaqlite_syntax::any::{AnyParseError, AnyParsedStatement};
use syntaqlite_syntax::source::{DocOffset, DocRange};

use crate::analysis::catalog::{
    AritySpec, ColumnResolution, FunctionCategory, FunctionCheckResult,
};
use crate::analysis::engine::tokens::{SemanticToken, StoredComment, StoredToken};
use crate::analysis::engine::walker::{
    CallEvent, ColumnRefEvent, SourceRefEvent, WalkCtx, WalkPass,
};
use crate::dialect::AnyDialect;

// ── Resolved symbols ──────────────────────────────────────────────────────────

/// A definition site that a reference points to.
#[derive(Debug, Clone)]
pub(crate) struct DefinitionLocation {
    pub(crate) range: DocRange,
    /// `Some` when the definition lives in another file (external schema).
    pub(crate) file_uri: Option<String>,
}

/// Result of a go-to-definition lookup.
#[derive(Debug, Clone)]
pub(crate) struct DefinitionResult {
    pub(crate) origin: DocRange,
    pub(crate) target: DefinitionLocation,
}

/// A symbol resolution recorded during the validation pass.
#[derive(Debug, Clone)]
pub(crate) enum ResolvedSymbol {
    Table {
        name: String,
        columns: Option<Vec<String>>,
        definition: Option<DefinitionLocation>,
    },
    Column {
        column: String,
        table: String,
        all_columns: Vec<String>,
        definition: Option<DefinitionLocation>,
    },
    Function {
        category: String,
        arities: Vec<String>,
    },
}

#[derive(Debug, Clone)]
pub(crate) struct Resolution {
    pub(crate) range: DocRange,
    pub(crate) symbol: ResolvedSymbol,
}

/// Identity of a symbol for matching across resolutions.
#[derive(Debug)]
pub(crate) enum SymbolIdentity {
    Table(String),
    Column { table: String, column: String },
}

impl SymbolIdentity {
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

// ── DocumentAnalysisData ──────────────────────────────────────────────────────

/// Per-document bundle of everything LSP services query after analysis:
/// tokens, comments, resolved references, definition sites. Populated by
/// [`LspCapturePass`] during a semantic-analyzer pass.
#[derive(Debug, Default)]
pub(crate) struct DocumentAnalysisData {
    pub(crate) tokens: Vec<StoredToken>,
    pub(crate) comments: Vec<StoredComment>,
    pub(crate) resolutions: Vec<Resolution>,
    /// Maps `lowercase(name)` → `DocRange` for same-file definition sites.
    /// Keys for columns look like `"table.column"` (lowercased).
    pub(crate) definition_offsets: HashMap<String, DocRange>,
}

impl DocumentAnalysisData {
    pub(crate) fn semantic_tokens(&self, dialect: &AnyDialect) -> Vec<SemanticToken> {
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

    /// The resolution whose span contains `offset`, if any.
    pub(crate) fn resolution_at(&self, offset: DocOffset) -> Option<&Resolution> {
        self.resolutions
            .iter()
            .find(|r| offset >= r.range.start && offset < r.range.end)
    }

    /// Find all resolutions in this document that match the given identity.
    pub(crate) fn references_matching(&self, kind: &SymbolIdentity) -> Vec<DocRange> {
        self.resolutions
            .iter()
            .filter(|r| kind.matches(&r.symbol))
            .map(|r| r.range)
            .collect()
    }

    /// Go-to-definition target for the resolution at `offset`, if any.
    pub(crate) fn definition_at(&self, offset: DocOffset) -> Option<DefinitionResult> {
        self.resolution_at(offset).and_then(|r| match &r.symbol {
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
}

// ── External definition registry ──────────────────────────────────────────────

/// A definition site in a source file other than the one being analyzed
/// (e.g. an external schema that was loaded via `from_ddl`).
#[derive(Debug, Clone)]
pub(crate) struct ExternalDefinitionSite {
    pub(crate) file_uri: String,
    pub(crate) range: DocRange,
}

/// Definition sites collected while parsing external schema files.
///
/// Keyed by `lowercase(name)` for tables/views and by
/// `lowercase(table).lowercase(column)` for columns.
#[derive(Debug, Default, Clone)]
pub(crate) struct ExternalDefinitions {
    sites: HashMap<String, ExternalDefinitionSite>,
}

impl ExternalDefinitions {
    pub(crate) fn new() -> Self {
        Self::default()
    }

    pub(crate) fn insert_relation(&mut self, name: &str, file_uri: &str, range: DocRange) {
        self.sites.insert(
            name.to_ascii_lowercase(),
            ExternalDefinitionSite {
                file_uri: file_uri.to_string(),
                range,
            },
        );
    }

    pub(crate) fn insert_column(
        &mut self,
        table: &str,
        column: &str,
        file_uri: &str,
        range: DocRange,
    ) {
        let key = format!(
            "{}.{}",
            table.to_ascii_lowercase(),
            column.to_ascii_lowercase()
        );
        self.sites.insert(
            key,
            ExternalDefinitionSite {
                file_uri: file_uri.to_string(),
                range,
            },
        );
    }

    pub(crate) fn relation(&self, name: &str) -> Option<&ExternalDefinitionSite> {
        self.sites.get(&name.to_ascii_lowercase())
    }

    pub(crate) fn column(&self, table: &str, column: &str) -> Option<&ExternalDefinitionSite> {
        let key = format!(
            "{}.{}",
            table.to_ascii_lowercase(),
            column.to_ascii_lowercase()
        );
        self.sites.get(&key)
    }
}

// ── LspCapturePass ────────────────────────────────────────────────────────────

/// Walk-time pass that fills a [`DocumentAnalysisData`] with everything LSP
/// services need: resolved references, definition sites, tokens, comments.
pub(crate) struct LspCapturePass<'a> {
    pub(crate) data: DocumentAnalysisData,
    external: Option<&'a ExternalDefinitions>,
}

impl<'a> LspCapturePass<'a> {
    pub(crate) fn new(external: Option<&'a ExternalDefinitions>) -> Self {
        Self {
            data: DocumentAnalysisData::default(),
            external,
        }
    }

    pub(crate) fn into_data(self) -> DocumentAnalysisData {
        self.data
    }

    fn lookup_table_definition(&self, name: &str) -> Option<DefinitionLocation> {
        let lower = name.to_ascii_lowercase();
        if let Some(&range) = self.data.definition_offsets.get(&lower) {
            return Some(DefinitionLocation {
                range,
                file_uri: None,
            });
        }
        self.external.and_then(|ext| {
            ext.relation(name).map(|site| DefinitionLocation {
                range: site.range,
                file_uri: Some(site.file_uri.clone()),
            })
        })
    }

    fn lookup_column_definition(&self, table: &str, column: &str) -> Option<DefinitionLocation> {
        let key = format!(
            "{}.{}",
            table.to_ascii_lowercase(),
            column.to_ascii_lowercase()
        );
        if let Some(&range) = self.data.definition_offsets.get(&key) {
            return Some(DefinitionLocation {
                range,
                file_uri: None,
            });
        }
        self.external.and_then(|ext| {
            ext.column(table, column).map(|site| DefinitionLocation {
                range: site.range,
                file_uri: Some(site.file_uri.clone()),
            })
        })
    }
}

impl WalkPass for LspCapturePass<'_> {
    const WANTS_SOURCE_REF: bool = true;
    const WANTS_COLUMN_REF: bool = true;
    const WANTS_CALL: bool = true;
    const WANTS_RELATION_DEFINITION: bool = true;
    const WANTS_COLUMN_DEFINITION: bool = true;
    const WANTS_STATEMENT_CONTEXT: bool = true;

    fn on_source_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: SourceRefEvent<'_>,
    ) {
        if !ev.resolved {
            return;
        }
        let (columns, _without_rowid) = cx.catalog.table_source_info(ev.name);
        let definition = self.lookup_table_definition(ev.name);
        self.data.resolutions.push(Resolution {
            range: ev.range,
            symbol: ResolvedSymbol::Table {
                name: ev.name.to_string(),
                columns,
                definition,
            },
        });
    }

    fn on_column_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        ev: ColumnRefEvent<'_>,
    ) {
        let ColumnResolution::Found { table, all_columns } = ev.resolution else {
            return;
        };
        if table.is_empty() {
            return;
        }
        let definition = self.lookup_column_definition(table, ev.column);
        self.data.resolutions.push(Resolution {
            range: ev.range,
            symbol: ResolvedSymbol::Column {
                column: ev.column.to_string(),
                table: table.clone(),
                all_columns: all_columns.clone(),
                definition,
            },
        });
    }

    fn on_call(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: CallEvent<'_>,
    ) {
        if !matches!(ev.result, FunctionCheckResult::Ok) {
            return;
        }
        let Some((category, arities)) = cx.catalog.function_signature(ev.name) else {
            return;
        };
        let cat_str = match category {
            FunctionCategory::Scalar => "scalar function",
            FunctionCategory::Aggregate => "aggregate function",
            FunctionCategory::Window => "window function",
        };
        let arity_strs: Vec<String> = arities.iter().map(|a| format_arity(ev.name, *a)).collect();
        self.data.resolutions.push(Resolution {
            range: ev.range,
            symbol: ResolvedSymbol::Function {
                category: cat_str.to_string(),
                arities: arity_strs,
            },
        });
    }

    fn on_relation_definition(&mut self, name: &str, range: DocRange) {
        self.data
            .definition_offsets
            .insert(name.to_ascii_lowercase(), range);
    }

    fn on_column_definition(&mut self, table: &str, column: &str, range: DocRange) {
        let key = format!(
            "{}.{}",
            table.to_ascii_lowercase(),
            column.to_ascii_lowercase()
        );
        self.data.definition_offsets.insert(key, range);
    }

    fn on_parsed_statement(&mut self, stmt: &AnyParsedStatement<'_>) {
        let base = stmt.statement_base();
        for tok in stmt.tokens() {
            self.data.tokens.push(StoredToken {
                offset: tok.offset().to_doc(base),
                length: tok.length().into(),
                token_type: tok.token_type(),
                flags: tok.flags(),
            });
        }
        for c in stmt.comments() {
            self.data.comments.push(StoredComment {
                offset: c.offset().to_doc(base),
                length: c.length().into(),
            });
        }
    }

    fn on_parse_error(&mut self, err: &AnyParseError<'_>) {
        let base = err.statement_base();
        for tok in err.tokens() {
            self.data.tokens.push(StoredToken {
                offset: tok.offset().to_doc(base),
                length: tok.length().into(),
                token_type: tok.token_type(),
                flags: tok.flags(),
            });
        }
        for c in err.comments() {
            self.data.comments.push(StoredComment {
                offset: c.offset().to_doc(base),
                length: c.length().into(),
            });
        }
    }
}

fn format_arity(name: &str, arity: AritySpec) -> String {
    match arity {
        AritySpec::Exact(n) => {
            let params: Vec<String> = (0..n).map(|i| format!("arg{}", i + 1)).collect();
            format!("{}({})", name, params.join(", "))
        }
        AritySpec::AtLeast(n) => {
            let mut params: Vec<String> = (0..n).map(|i| format!("arg{}", i + 1)).collect();
            params.push("...".to_string());
            format!("{}({})", name, params.join(", "))
        }
        AritySpec::Any => format!("{name}(...)"),
    }
}
