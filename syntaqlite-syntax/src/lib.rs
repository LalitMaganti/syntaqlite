// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![cfg_attr(test, allow(clippy::unwrap_used, clippy::similar_names))]

//! Low-level tokenizer and parser for `SQLite` SQL.
//!
//! This crate is split from `syntaqlite` for internal build reasons — it is
//! **not** a separate user-facing library. All public types are re-exported by
//! [`syntaqlite`](https://docs.rs/syntaqlite). **Depend on `syntaqlite`
//! directly.** If you only need parsing (no formatting or validation), disable
//! the default features and enable just `sqlite`:
//!
//! ```toml
//! [dependencies]
//! syntaqlite = { version = "0.5.2", default-features = false, features = ["sqlite"] }
//! ```

// ==== Public API ====

// Top level parser types.
#[cfg(feature = "sqlite-minimal")]
#[doc(inline)]
pub use parser::ParserConfig;
#[cfg(feature = "sqlite")]
#[doc(inline)]
pub use parser::{
    ParseError, ParseErrorKind, ParseOutcome, ParseSession, ParsedStatement, Parser, ParserToken,
};

// Token/comment data types shared across dialects.
#[cfg(feature = "sqlite-minimal")]
#[doc(inline)]
pub use parser::{
    Comment, CommentKind, CommentSide, CommentSpan, CompletionContext,
    MACRO_BODY_CALL_ARG_INTERNAL, ParserTokenFlags,
};

// Top-level tokenizer types.
#[cfg(feature = "sqlite-minimal")]
#[doc(inline)]
pub use sqlite::tokens::TokenType;
#[cfg(feature = "sqlite")]
#[doc(inline)]
pub use tokenizer::{Token, Tokenizer};

/// Cross-cutting utilities for dialect configuration and compatibility.
///
/// Reach for this module when you need to pin parser behavior to a target
/// `SQLite` release or inspect feature-flag state.
#[cfg(feature = "sqlite-minimal")]
pub mod util;

/// Type-erased variants of every parser and tokenizer type.
///
/// **Most code should not need this module.** If you are working with the
/// `SQLite` dialect — which is the common case — use the top-level
/// [`Parser`], [`ParseSession`], [`Tokenizer`], and [`Token`] types instead.
///
/// ## When to use `any`
///
/// Reach for this module only when you need to work with dialects generically,
/// without knowing the concrete dialect at compile time. The primary use cases are:
///
/// - **Multi-dialect infrastructure** — tools that accept an [`any::AnyDialect`]
///   from the caller and operate on whichever dialect is handed in (e.g. a
///   generic formatter, a language-server host, or a test harness that runs
///   against several dialects).
/// - **Storage without a lifetime parameter** — [`any::AnyNodeId`] identifies a
///   node in a parse arena and can be stored freely; typed node references
///   borrow the arena and cannot outlive it.
/// - **FFI and plugin boundaries** — [`any::AnyDialect`] is `Copy + Send + Sync`
///   and is the natural unit to pass across crate boundaries or plugin APIs.
///
/// ## Caveats
///
/// The `Any*` types erase the dialect's token and node enums, replacing them
/// with raw `u32` ordinals. You lose exhaustive `match` on token kinds and
/// the typed accessor methods on AST nodes. Prefer the typed API whenever the
/// dialect is known statically.
#[cfg(feature = "sqlite-minimal")]
pub mod any {
    #[doc(inline)]
    pub use crate::ast::{
        AnyNode, AnyNodeId, AnyNodeTag, AnyTokenType, FIELD_ABSENT, FieldValue, NodeFields,
        TextSpan,
    };
    #[doc(inline)]
    pub use crate::dialect::{AnyDialect, FieldKind, FieldMeta, KeywordEntry, TokenCategory};
    #[doc(inline)]
    pub use crate::parser::{
        AnyIncrementalParseSession, AnyParseError, AnyParseSession, AnyParsedStatement, AnyParser,
        AnyParserToken, ArgOrigin, MacroArgSegment, MacroCallArg, MacroRewrite, ParseOutcome,
        TracebackFrame,
    };
    #[doc(inline)]
    pub use crate::tokenizer::{AnyToken, AnyTokenizer};
}

/// Generic, dialect-parameterized variants of parser/tokenizer types.
///
/// **Most code should not need this module.** Application code working with
/// the `SQLite` dialect should use the top-level [`Parser`], [`ParseSession`],
/// [`Tokenizer`], and [`Token`] types, which are thin wrappers over the typed
/// internals already instantiated for `SQLite`.
///
/// ## When to use `typed`
///
/// In practice, you will rarely import from this module directly. Its contents
/// are primarily consumed by the dialect codegen: the traits
/// [`GrammarNodeType`](typed::GrammarNodeType),
/// [`GrammarTokenType`](typed::GrammarTokenType), and
/// [`TypedNodeId`](typed::TypedNodeId) are implemented automatically for each
/// dialect's generated node and token enums. The codegen produces correct
/// implementations — you do not implement or import these manually.
///
/// If you need to write code that works across dialects without dialect-specific
/// types, use [`any`] instead, which provides type-erased equivalents that are
/// far easier to work with.
#[cfg(feature = "sqlite-minimal")]
pub mod typed {
    #[doc(inline)]
    pub use crate::ast::{GrammarNodeType, GrammarTokenType, TypedNodeId, TypedNodeList};
    #[doc(inline)]
    pub use crate::dialect::TypedDialect;
    #[doc(inline)]
    pub use crate::parser::{
        ParseOutcome, TypedIncrementalParseSession, TypedParseError, TypedParseSession,
        TypedParsedStatement, TypedParser, TypedParserToken,
    };
    #[doc(inline)]
    pub use crate::tokenizer::{TypedToken, TypedTokenizer};

    // Only exposed for use in generated code, not public API.
    #[doc(hidden)]
    pub use crate::dialect::ffi::{CDialect, CDialectTemplate};

    /// Top-level typed dialect handle for the `SQLite` dialect.
    ///
    /// Most code should not need to call `dialect()` directly; the top-level [`crate::Parser`]
    /// and [`crate::Tokenizer`] types construct it internally. However, if you need to
    /// work with the dialect directly — for example, to inspect its token and node
    /// metadata, or to construct a parser or tokenizer manually — you can obtain a
    /// handle with `dialect()`.
    #[cfg(feature = "sqlite")]
    pub use crate::sqlite::dialect::Dialect;
    #[cfg(feature = "sqlite")]
    pub use crate::sqlite::dialect::dialect;
}

/// Generated typed AST for the built-in `SQLite` dialect.
///
/// Re-exports every generated node struct, enum, and accessor type for the
/// `SQLite` dialect. Import from here when you need to name concrete node
/// types — for example, when pattern-matching on a [`ParsedStatement`] or
/// traversing the parse tree.
#[cfg(feature = "sqlite")]
pub mod nodes {
    pub use crate::sqlite::ast::*;
}

// Top-level incremental parse session type (SQLite dialect).
#[cfg(feature = "sqlite")]
#[doc(inline)]
pub use parser::IncrementalParseSession;

// Macro expansion types (used by downstream crates like syntaqlite).
#[cfg(feature = "sqlite-minimal")]
pub use parser::{MacroArg, MacroLookup, MacroOutput};

// ==== Internal modules ====

#[cfg(feature = "sqlite-minimal")]
pub(crate) mod ast;
#[cfg(feature = "sqlite-minimal")]
mod dialect;
#[cfg(feature = "sqlite-minimal")]
pub(crate) mod parser;
pub mod source;
#[cfg(feature = "sqlite-minimal")]
pub(crate) mod tokenizer;

#[cfg(feature = "sqlite-minimal")]
pub(crate) mod sqlite;

#[cfg(feature = "embedded-sources")]
pub mod embedded_sources;
