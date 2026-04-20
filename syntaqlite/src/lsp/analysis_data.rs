// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP-side capture pass and per-file glue.
//!
//! The core analysis output types live in [`crate::semantic::analysis`]. This
//! module owns:
//!
//! - [`LspCapturePass`] — a [`WalkPass`] impl that fills a
//!   [`DocumentAnalysisData`] directly during analysis. Replaces the old
//!   observer-based capture.
//! - [`ExternalDefinitions`] — a cross-file definition-site registry consulted
//!   by the capture pass to correlate resolutions with schema files.
//! - [`CompletionInfo`] / [`CompletionContext`] — the completion-probe output
//!   consumed by the LSP completion service.

use std::collections::HashMap;

use syntaqlite_syntax::any::{AnyParseError, AnyParsedStatement, AnyTokenType};
use syntaqlite_syntax::source::DocRange;

use crate::semantic::analysis::{
    DefinitionLocation, DocumentAnalysisData, Resolution, ResolvedSymbol, StoredComment,
    StoredToken,
};
use crate::semantic::analyzer::walker::{
    CallEvent, ColumnRefEvent, SourceRefEvent, WalkCtx, WalkPass,
};
use crate::semantic::catalog::{
    AritySpec, ColumnResolution, FunctionCategory, FunctionCheckResult,
};

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

// ── LspCapturePass ────────────────────────────────────────────────────────────

/// Walk-time pass that fills a [`DocumentAnalysisData`] with everything LSP
/// services need: resolved references, definition sites, tokens, comments.
///
/// The pass holds a reference to an optional external-definition registry so it
/// can correlate resolutions with cross-file definition sites.
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
