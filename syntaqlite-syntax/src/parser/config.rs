// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

/// Controls optional parser behavior beyond core AST construction.
///
/// Keep defaults for pure parsing. Enable extras only when your tool needs
/// them (for example, token-level highlighting or parser debugging).
#[derive(Debug, Default, Clone, Copy)]
pub struct ParserConfig {
    trace: bool,
    collect_tokens: bool,
    macro_fallback: bool,
}

impl ParserConfig {
    /// Whether parser debug trace logging is enabled. Default: `false`.
    ///
    /// Useful when debugging grammar behavior; usually disabled in production.
    pub fn trace(&self) -> bool {
        self.trace
    }

    /// Whether parser tokens/comments are recorded for each statement. Default: `false`.
    ///
    /// Enable this for tooling that needs precise token streams (formatters,
    /// diagnostics, semantic highlighting).
    pub fn collect_tokens(&self) -> bool {
        self.collect_tokens
    }

    /// Enable or disable parser trace logging.
    #[must_use]
    pub fn with_trace(mut self, trace: bool) -> Self {
        self.trace = trace;
        self
    }

    /// Enable or disable token/comment capture on parse results.
    #[must_use]
    pub fn with_collect_tokens(mut self, collect_tokens: bool) -> Self {
        self.collect_tokens = collect_tokens;
        self
    }

    /// Whether macro fallback is enabled. Default: `false`.
    ///
    /// When enabled and the dialect uses Rust-style macros, unregistered
    /// `name!(args)` calls are consumed as a single `TK_ID` token instead
    /// of causing a parse error. A `MacroRegion` is recorded so the
    /// formatter can emit the call verbatim.
    pub fn macro_fallback(&self) -> bool {
        self.macro_fallback
    }

    /// Enable or disable macro fallback for unregistered macro calls.
    #[must_use]
    pub fn with_macro_fallback(mut self, macro_fallback: bool) -> Self {
        self.macro_fallback = macro_fallback;
        self
    }
}
