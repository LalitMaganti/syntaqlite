// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP-side glue for capturing semantic-analyzer events.
//!
//! The analysis output types themselves live in [`crate::semantic::analysis`].
//! This module holds:
//!
//! - [`LspObserver`] — an [`AnalysisObserver`] impl that fills a
//!   [`DocumentAnalysisData`] during a pass.
//! - [`ExternalDefinitions`] — a cross-file definition-site registry consulted
//!   by the observer to correlate resolutions with schema files.
//! - [`CompletionInfo`] / [`CompletionContext`] — the completion-probe output
//!   consumed by the LSP completion service.

use std::collections::HashMap;

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::AnyTokenType;
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

use crate::semantic::analysis::{
    DefinitionLocation, DocumentAnalysisData, Resolution, ResolvedSymbol, StoredComment,
    StoredToken,
};
use crate::semantic::catalog::{AritySpec, FunctionCategory};
use crate::semantic::observer::AnalysisObserver;

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
