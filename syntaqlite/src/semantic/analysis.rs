// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Outputs captured during a semantic-analyzer pass.
//!
//! The analyzer fires events through [`AnalysisObserver`](super::observer::AnalysisObserver);
//! consumers — today the LSP host, tomorrow each composable analysis pass —
//! collect those events into the data types defined here. Nothing in this
//! module depends on LSP protocol machinery.

use std::collections::HashMap;

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::{AnyTokenType, TokenCategory};
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

use crate::dialect::AnyDialect;

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

// ── DocumentAnalysisData ─────────────────────────────────────────────────────

/// All the data captured during a single analysis pass that downstream
/// consumers (editors, LSP services) query after the fact. Populated by an
/// [`AnalysisObserver`](super::observer::AnalysisObserver) impl.
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
