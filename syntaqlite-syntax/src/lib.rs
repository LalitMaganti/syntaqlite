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
//! syntaqlite = { version = "0.2.15", default-features = false, features = ["sqlite"] }
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

// Token/comment data types shared across grammars.
#[cfg(feature = "sqlite-minimal")]
#[doc(inline)]
pub use parser::{Comment, CommentKind, CommentSpan, CompletionContext, ParserTokenFlags};

// Top-level tokenizer types.
#[cfg(feature = "sqlite-minimal")]
#[doc(inline)]
pub use sqlite::tokens::TokenType;
#[cfg(feature = "sqlite")]
#[doc(inline)]
pub use tokenizer::{Token, Tokenizer};

/// Cross-cutting utilities for grammar configuration and compatibility.
///
/// Reach for this module when you need to pin parser behavior to a target
/// `SQLite` release or inspect feature-flag state.
#[cfg(feature = "sqlite-minimal")]
pub mod util;

/// Type-erased variants of every parser and tokenizer type.
///
/// **Most code should not need this module.** If you are working with the
/// `SQLite` grammar — which is the common case — use the top-level
/// [`Parser`], [`ParseSession`], [`Tokenizer`], and [`Token`] types instead.
///
/// ## When to use `any`
///
/// Reach for this module only when you need to work with grammars generically,
/// without knowing the grammar at compile time. The primary use cases are:
///
/// - **Multi-grammar infrastructure** — tools that accept an [`any::AnyGrammar`]
///   from the caller and operate on whichever grammar is handed in (e.g. a
///   generic formatter, a language-server host, or a test harness that runs
///   against several grammars).
/// - **Storage without a lifetime parameter** — [`any::AnyNodeId`] identifies a
///   node in a parse arena and can be stored freely; typed node references
///   borrow the arena and cannot outlive it.
/// - **FFI and plugin boundaries** — [`any::AnyGrammar`] is `Copy + Send + Sync`
///   and is the natural unit to pass across crate boundaries or plugin APIs.
///
/// ## Caveats
///
/// The `Any*` types erase the grammar's token and node enums, replacing them
/// with raw `u32` ordinals. You lose exhaustive `match` on token kinds and
/// the typed accessor methods on AST nodes. Prefer the typed API whenever the
/// grammar is known statically.
#[cfg(feature = "sqlite-minimal")]
pub mod any {
    #[doc(inline)]
    pub use crate::ast::{AnyNode, AnyNodeId, AnyNodeTag, AnyTokenType, FieldValue, NodeFields};
    #[doc(inline)]
    pub use crate::grammar::{AnyGrammar, FieldKind, FieldMeta, KeywordEntry, TokenCategory};
    #[doc(inline)]
    pub use crate::parser::{
        AnyIncrementalParseSession, AnyParseError, AnyParseSession, AnyParsedStatement, AnyParser,
        AnyParserToken, MacroRegion, ParseOutcome,
    };
    #[doc(inline)]
    pub use crate::tokenizer::{AnyToken, AnyTokenizer};
}

/// Generic, grammar-parameterized variants of parser/tokenizer types.
///
/// **Most code should not need this module.** Application code working with
/// the `SQLite` grammar should use the top-level [`Parser`], [`ParseSession`],
/// [`Tokenizer`], and [`Token`] types, which are thin wrappers over the typed
/// internals already instantiated for `SQLite`.
///
/// ## When to use `typed`
///
/// In practice, you will rarely import from this module directly. Its contents
/// are primarily consumed by the grammar generator: the traits
/// [`GrammarNodeType`](typed::GrammarNodeType),
/// [`GrammarTokenType`](typed::GrammarTokenType), and
/// [`TypedNodeId`](typed::TypedNodeId) are implemented automatically for each
/// grammar's generated node and token enums. The generator produces correct
/// implementations — you do not implement or import these manually.
///
/// If you need to write code that works across grammars without grammar-specific
/// types, use [`any`] instead, which provides type-erased equivalents that are
/// far easier to work with.
#[cfg(feature = "sqlite-minimal")]
pub mod typed {
    #[doc(inline)]
    pub use crate::ast::{GrammarNodeType, GrammarTokenType, TypedNodeId, TypedNodeList};
    #[doc(inline)]
    pub use crate::grammar::TypedGrammar;
    #[doc(inline)]
    pub use crate::parser::{
        ParseOutcome, TypedIncrementalParseSession, TypedParseError, TypedParseSession,
        TypedParsedStatement, TypedParser, TypedParserToken,
    };
    #[doc(inline)]
    pub use crate::tokenizer::{TypedToken, TypedTokenizer};

    // Only exposed for use in generated code, not public API.
    #[doc(hidden)]
    pub use crate::grammar::ffi::CGrammar;

    /// Top-level grammar handle for the `SQLite` grammar.
    ///
    /// Most code should not need to call `grammar()` directly; the top-level [`crate::Parser`]
    /// and [`crate::Tokenizer`] types construct it internally. However, if you need to
    /// work with the grammar directly — for example, to inspect its token and node
    /// metadata, or to construct a parser or tokenizer manually — you can obtain a
    /// handle with `grammar()`.
    /// Top-level typed grammar handle type for the `SQLite` grammar.
    ///
    /// Useful when you need to name the type parameter in `TypedDialect<Grammar>`.
    #[cfg(feature = "sqlite")]
    pub use crate::sqlite::grammar::Grammar;
    #[cfg(feature = "sqlite")]
    pub use crate::sqlite::grammar::grammar;
}

/// Generated typed AST for the built-in `SQLite` grammar.
///
/// Re-exports every generated node struct, enum, and accessor type for the
/// `SQLite` grammar. Import from here when you need to name concrete node
/// types — for example, when pattern-matching on a [`ParsedStatement`] or
/// traversing the parse tree.
#[cfg(feature = "sqlite")]
pub mod nodes {
    pub use crate::sqlite::ast::*;
}

// Top-level incremental parse session type (SQLite grammar).
#[cfg(feature = "sqlite")]
#[doc(inline)]
pub use parser::IncrementalParseSession;

// ==== Internal modules ====

#[cfg(feature = "sqlite-minimal")]
pub(crate) mod ast;
#[cfg(feature = "sqlite-minimal")]
mod grammar;
#[cfg(feature = "sqlite-minimal")]
pub(crate) mod parser;
#[cfg(feature = "sqlite-minimal")]
pub(crate) mod tokenizer;

#[cfg(feature = "sqlite-minimal")]
pub(crate) mod sqlite;

#[cfg(feature = "embedded-sources")]
pub mod embedded_sources;
