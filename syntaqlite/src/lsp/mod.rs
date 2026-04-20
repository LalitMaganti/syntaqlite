// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Language-server support: analysis host, document management, and
//! protocol server.
//!
//! # Overview
//!
//! - [`LspHost`] — stateful document store with lazy per-document
//!   analysis (diagnostics, semantic tokens, completions, formatting,
//!   hover, signature help).
//!   Delegates semantic validation to [`SemanticAnalyzer`](crate::semantic::SemanticAnalyzer).
//! - [`LspServer`] — stdio JSON-RPC server that drives an `LspHost`
//!   in response to LSP messages from an editor.
//!
//! # Example
//!
//! ```
//! use syntaqlite::lsp::LspHost;
//!
//! // Programmatic usage — embed in your own tool:
//! let mut host = LspHost::new();
//! host.update_document("file:///q.sql", 1, "SELECT 1;".into());
//! let tokens = host.semantic_tokens_encoded("file:///q.sql", None);
//! ```

/// Semantic token type names in legend-index order, for use in LSP
/// `SemanticTokensLegend` and Monaco provider registration.
pub(crate) const SEMANTIC_TOKEN_LEGEND: &[&str] = &[
    "keyword",   // 0 — TokenCategory::Keyword
    "parameter", // 1 — TokenCategory::Parameter  (bind params: :name, @var, ?)
    "string",    // 2 — TokenCategory::String
    "number",    // 3 — TokenCategory::Number
    "operator",  // 4 — TokenCategory::Operator    (skipped at encode time)
    "comment",   // 5 — TokenCategory::Comment
    "operator",  // 6 — TokenCategory::Punctuation (skipped at encode time)
    "variable",  // 7 — TokenCategory::Identifier
    "function",  // 8 — TokenCategory::Function
    "type",      // 9 — TokenCategory::Type
];

// Public API starts here.
#[doc(inline)]
pub use host::LspHost;
#[doc(inline)]
pub use host::SchemaMap;
#[doc(inline)]
pub use server::{LspConfig, LspServer};

pub(crate) use crate::semantic::completion::{CompletionContext, CompletionInfo};

// ── LSP-specific types ──────────────────────────────────────────────────

/// A single suggestion returned by [`LspHost::completion_items`].
///
/// Each entry carries a display label (which is also the text to insert) and a
/// [`CompletionKind`] indicating whether it is a keyword or a function name.
/// Convert to your editor's native completion type using [`label()`](Self::label)
/// and [`kind()`](Self::kind).
#[derive(Debug, Clone)]
pub struct CompletionEntry {
    label: String,
    kind: CompletionKind,
}

impl CompletionEntry {
    pub(crate) fn new(label: String, kind: CompletionKind) -> Self {
        CompletionEntry { label, kind }
    }

    /// The label to display and insert.
    pub fn label(&self) -> &str {
        &self.label
    }

    /// What kind of thing is being completed.
    pub fn kind(&self) -> CompletionKind {
        self.kind
    }
}

/// The kind of a [`CompletionEntry`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompletionKind {
    /// A SQL keyword.
    Keyword,
    /// A built-in or user-defined function.
    Function,
    /// A table or view name.
    Table,
    /// A column name.
    Column,
}

impl CompletionKind {
    /// String representation for use in serialization or display.
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Keyword => "keyword",
            Self::Function => "function",
            Self::Table => "table",
            Self::Column => "column",
        }
    }

    /// Sort priority (lower = shown first). Columns and tables are more
    /// relevant than functions in most expression contexts.
    pub fn sort_priority(self) -> u8 {
        match self {
            Self::Column => 0,
            Self::Table => 1,
            Self::Keyword => 2,
            Self::Function => 3,
        }
    }
}

mod analysis_data;
mod completion_service;
mod document_store;
pub(crate) mod host;
mod hover_service;
mod refs_service;
mod semantic_tokens_codec;
mod server;
mod source_map;

pub(crate) use source_map::{SourceMap, utf8_char_len};
