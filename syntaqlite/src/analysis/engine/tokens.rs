// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Neutral analysis-time data captured by passes.
//!
//! These types are the common currency between the walker, token-capturing
//! passes, and higher-layer consumers. They carry no LSP-protocol shape —
//! anything LSP-specific (resolved symbols, go-to-def targets, the cached
//! document bundle) lives in `crate::lsp`.

use syntaqlite_syntax::ParserTokenFlags;
use syntaqlite_syntax::any::{AnyTokenType, TokenCategory};
use syntaqlite_syntax::source::{DocLen, DocOffset};

// ── Token positions ──────────────────────────────────────────────────────────

/// A parser token captured during analysis.
#[derive(Debug, Clone, Copy)]
pub(crate) struct StoredToken {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
    pub(crate) token_type: AnyTokenType,
    pub(crate) flags: ParserTokenFlags,
}

/// A comment captured during analysis.
#[derive(Debug, Clone, Copy)]
pub(crate) struct StoredComment {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
}

/// A token classified for editor-style syntax highlighting.
#[derive(Debug, Clone, Copy)]
pub(crate) struct SemanticToken {
    pub(crate) offset: DocOffset,
    pub(crate) length: DocLen,
    pub(crate) category: TokenCategory,
}
