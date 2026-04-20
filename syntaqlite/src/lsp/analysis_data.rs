// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP-local storage captured during a semantic-analyzer pass.
//!
//! The semantic layer emits events via [`AnalysisObserver`]; this module
//! defines the concrete storage (tokens, comments, resolutions, definition
//! offsets) plus an observer implementation that fills it. Nothing here is
//! visible to the semantic layer.

use std::collections::HashMap;

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::{AnyTokenType, TokenCategory};
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

use crate::dialect::AnyDialect;
use crate::semantic::catalog::{AritySpec, FunctionCategory};
use crate::semantic::observer::AnalysisObserver;

// ── Token positions ──────────────────────────────────────────────────────────

/// A parser token observed during analysis.
#[derive(Debug, Clone)]
pub(crate) struct StoredToken {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
    pub(crate) token_type: AnyTokenType,
    pub(crate) flags: ParserTokenFlags,
}

/// A comment observed during analysis.
#[derive(Debug, Clone)]
pub(crate) struct StoredComment {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
}

/// A token classified for editor syntax highlighting.
#[derive(Debug, Clone)]
pub(crate) struct SemanticToken {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
    pub(crate) category: TokenCategory,
}

// ── Completion ───────────────────────────────────────────────────────────────

/// Semantic completion context derived from parser stack state.
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum CompletionContext {
    Unknown = 0,
    Expression = 1,
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
    pub(crate) tokens: Vec<AnyTokenType>,
    pub(crate) context: CompletionContext,
    pub(crate) qualifier: Option<String>,
}

// ── Resolved symbols ─────────────────────────────────────────────────────────

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

// ── External definition registry ─────────────────────────────────────────────

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

// ── DocumentAnalysisData ─────────────────────────────────────────────────────

/// All the data an editor needs that was captured during a single analysis
/// pass. Populated by [`LspObserver`].
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

// ── LspObserver ──────────────────────────────────────────────────────────────

/// Observer that fills a [`DocumentAnalysisData`] during a
/// [`SemanticAnalyzer`](crate::semantic::SemanticAnalyzer) pass.
///
/// The observer holds a reference to an optional external-definition registry
/// so it can correlate resolutions with cross-file definition sites.
pub(crate) struct LspObserver<'a> {
    pub(crate) data: DocumentAnalysisData,
    external: Option<&'a ExternalDefinitions>,
}

impl<'a> LspObserver<'a> {
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

impl AnalysisObserver for LspObserver<'_> {
    fn wants_tokens(&self) -> bool {
        true
    }
    fn wants_comments(&self) -> bool {
        true
    }
    fn wants_definitions(&self) -> bool {
        true
    }
    fn wants_references(&self) -> bool {
        true
    }

    fn on_token(
        &mut self,
        offset: DocOffset,
        length: DocLen,
        token_type: AnyTokenType,
        flags: ParserTokenFlags,
    ) {
        self.data.tokens.push(StoredToken {
            offset,
            length,
            token_type,
            flags,
        });
    }

    fn on_comment(&mut self, offset: DocOffset, length: DocLen) {
        self.data.comments.push(StoredComment { offset, length });
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

    fn on_table_reference(&mut self, range: DocRange, name: &str, columns: Option<&[String]>) {
        let definition = self.lookup_table_definition(name);
        self.data.resolutions.push(Resolution {
            range,
            symbol: ResolvedSymbol::Table {
                name: name.to_string(),
                columns: columns.map(<[String]>::to_vec),
                definition,
            },
        });
    }

    fn on_column_reference(
        &mut self,
        range: DocRange,
        table: &str,
        column: &str,
        all_columns: &[String],
    ) {
        let definition = self.lookup_column_definition(table, column);
        self.data.resolutions.push(Resolution {
            range,
            symbol: ResolvedSymbol::Column {
                column: column.to_string(),
                table: table.to_string(),
                all_columns: all_columns.to_vec(),
                definition,
            },
        });
    }

    fn on_function_reference(
        &mut self,
        range: DocRange,
        name: &str,
        category: FunctionCategory,
        arities: &[AritySpec],
    ) {
        let cat_str = match category {
            FunctionCategory::Scalar => "scalar function",
            FunctionCategory::Aggregate => "aggregate function",
            FunctionCategory::Window => "window function",
        };
        let arity_strs: Vec<String> = arities.iter().map(|a| format_arity(name, *a)).collect();
        self.data.resolutions.push(Resolution {
            range,
            symbol: ResolvedSymbol::Function {
                category: cat_str.to_string(),
                arities: arity_strs,
            },
        });
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
