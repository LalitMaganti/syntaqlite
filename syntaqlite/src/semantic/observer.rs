// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Analysis observer — a callback interface for consumers that need to
//! capture incremental events from a [`SemanticAnalyzer`] pass (symbol
//! resolutions, definition sites, tokens, comments).
//!
//! The analyzer calls observer methods as it walks the AST. The default
//! [`NoopObserver`] discards every event, so observer-free callers pay
//! nothing. Editor/LSP-style tools implement their own observer to build
//! hover/go-to-definition/completion data without the analyzer knowing
//! anything about them.
//!
//! The trait is `pub(crate)` — today only the in-crate `lsp` module needs
//! it. Promote to `pub` if and when an out-of-crate consumer appears.

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::AnyTokenType;
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

use super::catalog::{AritySpec, FunctionCategory};

/// Receives incremental events during a [`SemanticAnalyzer`] pass.
///
/// Every method has a default empty body, so implementors only override the
/// events they care about. Events are emitted in the order the analyzer
/// encounters them; ordering across categories is not guaranteed.
///
/// # Capability hints
///
/// The `wants_*` methods let the analyzer skip work when an event category
/// is definitely ignored. Override them to return `true` when you plan to
/// override the matching event method — otherwise the analyzer avoids the
/// per-token or per-definition traversal cost.
///
/// [`SemanticAnalyzer`]: super::analyzer::SemanticAnalyzer
pub(crate) trait AnalysisObserver {
    /// Return `true` to receive [`on_token`](Self::on_token) events.
    fn wants_tokens(&self) -> bool {
        false
    }

    /// Return `true` to receive [`on_comment`](Self::on_comment) events.
    fn wants_comments(&self) -> bool {
        false
    }

    /// Return `true` to receive
    /// [`on_relation_definition`](Self::on_relation_definition) and
    /// [`on_column_definition`](Self::on_column_definition) events.
    fn wants_definitions(&self) -> bool {
        false
    }

    /// Return `true` to receive any of the `on_*_reference` events
    /// ([table](Self::on_table_reference), [column](Self::on_column_reference),
    /// [function](Self::on_function_reference)).
    fn wants_references(&self) -> bool {
        false
    }

    /// A table or view reference that resolved to a known relation.
    fn on_table_reference(&mut self, _range: DocRange, _name: &str, _columns: Option<&[String]>) {}

    /// A column reference that resolved to a known column (or a table with
    /// unknown columns — in which case `all_columns` is empty).
    fn on_column_reference(
        &mut self,
        _range: DocRange,
        _table: &str,
        _column: &str,
        _all_columns: &[String],
    ) {
    }

    /// A function call that resolved to a known overload.
    fn on_function_reference(
        &mut self,
        _range: DocRange,
        _name: &str,
        _category: FunctionCategory,
        _arities: &[AritySpec],
    ) {
    }

    /// A relation (table/view/CTE) defined in the analyzed document, together
    /// with the source range of its name.
    fn on_relation_definition(&mut self, _name: &str, _range: DocRange) {}

    /// A column defined in the analyzed document, together with the source
    /// range of its name.
    fn on_column_definition(&mut self, _table: &str, _column: &str, _range: DocRange) {}

    /// A lexer token observed during parsing.
    fn on_token(
        &mut self,
        _offset: DocOffset,
        _length: DocLen,
        _token_type: AnyTokenType,
        _flags: ParserTokenFlags,
    ) {
    }

    /// A comment observed during parsing.
    fn on_comment(&mut self, _offset: DocOffset, _length: DocLen) {}
}

/// The default observer: ignores every event.
///
/// Used by [`SemanticAnalyzer::analyze`](super::analyzer::SemanticAnalyzer::analyze)
/// when no observer is supplied, so observer-free callers pay no overhead.
pub(crate) struct NoopObserver;

impl AnalysisObserver for NoopObserver {}
