// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![cfg_attr(test, expect(clippy::unwrap_used))]
#![allow(rustdoc::redundant_explicit_links)]

//! Fast, accurate SQL tooling for `SQLite` and its dialects.
//!
//! This crate provides parsing, formatting, and semantic validation for SQL,
//! built on `SQLite`'s own tokenizer and grammar rules. Four design principles
//! guide the library:
//!
//! - **Reliability** — uses `SQLite`'s own grammar rules; formatting is round-trip safe and validation mirrors real engine behaviour.
//! - **Speed** — all core types ([`Formatter`], [`SemanticAnalyzer`], [`Catalog`]) are designed for reuse across many inputs without re-allocation.
//! - **Portability** — the core formatting and validation engine has no runtime dependencies beyond the standard library; optional features (`lsp`, `serde`) pull in additional crates.
//! - **Flexibility** — supports multiple database dialects that extend `SQLite`'s grammar with their own tokens and rules.
//!
//! # Parsing
//!
//! Use [`Parser`](crate::Parser) to parse SQL source text into a typed AST:
//!
//! ```rust
//! use syntaqlite::parse::ParseErrorKind;
//! use syntaqlite::{Parser, ParseOutcome};
//!
//! let parser = Parser::new();
//! let mut session = parser.parse("SELECT 1; SELECT 2");
//! loop {
//!     match session.next() {
//!         ParseOutcome::Ok(stmt) => println!("{:?}", stmt.root()),
//!         ParseOutcome::Err(e) => {
//!             eprintln!("parse error: {}", e.message());
//!             if e.kind() == ParseErrorKind::Fatal { break; }
//!         }
//!         ParseOutcome::Done => break,
//!     }
//! }
//! ```
//!
//! See the [`parse`] module for additional types like
//! [`IncrementalParseSession`](parse::IncrementalParseSession),
//! [`ParserConfig`](parse::ParserConfig), and
//! [`ParserToken`](parse::ParserToken).
//!
//! # Validation
//!
//! Use [`SemanticAnalyzer`] to check SQL against a known schema. The analyzer
//! produces a [`SemanticModel`](semantic::SemanticModel) containing structured
//! [`Diagnostic`] values with byte-offset spans and "did you mean?" suggestions.
//!
//! ```rust
//! use syntaqlite::semantic::CatalogLayer;
//! use syntaqlite::{
//!     SemanticAnalyzer, Catalog, ValidationConfig, sqlite_dialect,
//! };
//!
//! let mut analyzer = SemanticAnalyzer::new();
//! let mut catalog = Catalog::new(sqlite_dialect());
//!
//! // Register a table so the analyzer can resolve column references.
//! catalog.layer_mut(CatalogLayer::Database)
//!     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
//!
//! let config = ValidationConfig::default();
//! let model = analyzer.analyze("SELECT id, name FROM users", &catalog, &config);
//!
//! // All names resolve — no diagnostics.
//! assert!(model.diagnostics().is_empty());
//! ```
//!
//! For richer output, use [`DiagnosticRenderer`](util::DiagnosticRenderer) to produce rustc-style
//! error messages with source context and underlines.
//!
//! # Formatting
//!
//! Use [`Formatter`] to pretty-print SQL with consistent style. The formatter
//! parses each statement, runs a bytecode interpreter over the AST, and
//! renders the result with a Wadler-style pretty-printer.
//!
//! ```rust
//! # use syntaqlite::fmt::KeywordCase;
//! # use syntaqlite::{Formatter, FormatConfig};
//! let mut fmt = Formatter::with_config(
//!     &FormatConfig::default()
//!         .with_keyword_case(KeywordCase::Lower)
//!         .with_line_width(60),
//! );
//!
//! let output = fmt.format("select id,name from users where active=1").unwrap();
//! assert!(output.starts_with("select"));
//! assert!(output.contains("from"));
//! ```
//!
//! See [`FormatConfig`] for all available options (line width, indent width,
//! keyword casing, semicolons).
//!
//! # Features
//!
//! - `sqlite` *(default)*: enables the built-in `SQLite` grammar, [`Dialect`],
//!   and re-exports [`Parser`](crate::Parser), [`Tokenizer`](parse::Tokenizer), and typed AST [`nodes`].
//! - `fmt` *(default)*: enables [`Formatter`], [`FormatConfig`], and
//!   [`KeywordCase`](fmt::KeywordCase).
//! - `validation` *(default)*: enables [`SemanticAnalyzer`], [`Catalog`],
//!   [`Diagnostic`], and related types.
//! - `lsp`: enables [`LspServer`](lsp::LspServer) and [`lsp::LspHost`] for editor integration.
//! - `experimental-embedded`: enables [`embedded`] SQL extraction from Python
//!   and TypeScript/JavaScript source files.
//! - `serde`: adds `Serialize`/`Deserialize` impls for diagnostics and AST
//!   nodes.
//! - `serde-json`: adds JSON convenience helpers (catalog from JSON, AST dump).
//!
//! # Choosing an API
//!
//! - Use [`Parser`](crate::Parser) and [`Tokenizer`](parse::Tokenizer) for parsing and tokenizing SQL.
//! - Use [`SemanticAnalyzer`] + [`Catalog`] when you need to validate SQL
//!   against a database schema (table/column/function resolution).
//! - Use [`Formatter`] when you need to pretty-print or normalize SQL text.
//! - Use [`LspServer`](lsp::LspServer) (requires the `lsp` feature) to embed a full
//!   Language Server Protocol implementation in an editor or tool.
//! - Use [`typed`] when building reusable code over known generated grammars.
//! - Use [`any`] when grammar choice happens at runtime or crosses
//!   FFI/plugin boundaries.

// ── Modules ─────────────────────────────────────────────────────────────────

#[cfg(feature = "fmt")]
pub(crate) mod dialect;

#[cfg(feature = "fmt")]
pub mod fmt;

#[cfg(feature = "validation")]
pub mod semantic;

// `sqlite` module is always present; individual sub-modules are gated inside it.
pub(crate) mod sqlite;

#[cfg(feature = "lsp")]
pub mod lsp;

/// Embedded SQL extraction from host language sources.
///
/// # Experimental
///
/// This module is experimental and its API may change in future releases.
/// Enable with the `experimental-embedded` cargo feature.
#[cfg(feature = "experimental-embedded")]
pub mod embedded;

pub mod util;

// ── Primary re-exports (crate root convenience) ─────────────────────────────

// Parsing — only the two types needed to parse and consume results.
#[doc(inline)]
#[cfg(feature = "sqlite")]
pub use syntaqlite_syntax::ParseOutcome;
#[doc(inline)]
#[cfg(feature = "sqlite")]
pub use syntaqlite_syntax::Parser;

// Formatting — the formatter and its config.
#[doc(inline)]
#[cfg(feature = "fmt")]
pub use fmt::FormatConfig;
#[doc(inline)]
#[cfg(feature = "fmt")]
pub use fmt::formatter::Formatter;

// Validation — the core types needed for a validation pass.
#[doc(inline)]
#[cfg(feature = "validation")]
pub use semantic::Catalog;
#[doc(inline)]
#[cfg(feature = "validation")]
pub use semantic::Diagnostic;
#[doc(inline)]
#[cfg(feature = "validation")]
pub use semantic::SemanticAnalyzer;
#[doc(inline)]
#[cfg(feature = "validation")]
pub use semantic::ValidationConfig;
#[doc(inline)]
#[cfg(feature = "validation")]
pub use semantic::{CheckConfig, CheckLevel};

// Dialect.
#[doc(inline)]
#[cfg(feature = "sqlite")]
pub use sqlite::dialect::Dialect;
/// Returns the built-in `SQLite` dialect handle.
///
/// Returns a [`Dialect`] (the SQLite-specific newtype). Call `.erase()` or
/// `.into()` to obtain an [`AnyDialect`](any::AnyDialect) when a type-erased handle is needed.
#[cfg(feature = "sqlite")]
pub fn sqlite_dialect() -> Dialect {
    Dialect::new()
}

// ── Parsing module ──────────────────────────────────────────────────────────

/// Tokenizer, parser, and related types for `SQLite` SQL.
///
/// [`Parser`](crate::Parser) and [`ParseOutcome`](crate::ParseOutcome) are
/// re-exported at the crate root for convenience. This module provides the
/// full set, including:
///
/// - [`IncrementalParseSession`](self::parse::IncrementalParseSession) — feed
///   tokens one at a time (useful for editors and completion engines).
/// - [`ParserConfig`](self::parse::ParserConfig) — optional parser behaviour
///   knobs.
/// - [`ParserToken`](self::parse::ParserToken) — per-token metadata from a
///   parsed statement.
/// - [`ParserTokenFlags`](self::parse::ParserTokenFlags) — parser-inferred
///   semantic flags for individual tokens.
/// - [`CommentKind`](self::parse::CommentKind) — SQL comment style.
/// - [`MacroRegion`](self::parse::MacroRegion) — byte range of a macro-call
///   placeholder (used by the embedded SQL extractor).
///
/// # Example
///
/// ```
/// use syntaqlite::parse::{Parser, ParseOutcome, ParseErrorKind, ParserConfig};
///
/// // Use ParserConfig to tweak parser behaviour.
/// let config = ParserConfig::default();
/// let parser = Parser::with_config(&config);
/// let mut session = parser.parse("SELECT 1");
/// match session.next() {
///     ParseOutcome::Ok(stmt) => assert!(stmt.root().is_some()),
///     _ => panic!("expected successful parse"),
/// }
/// ```
pub mod parse {
    #[doc(inline)]
    pub use syntaqlite_syntax::any::MacroRegion;
    #[doc(inline)]
    pub use syntaqlite_syntax::{CommentKind, ParserConfig, ParserTokenFlags};
    #[doc(inline)]
    #[cfg(feature = "sqlite")]
    pub use syntaqlite_syntax::{
        IncrementalParseSession, ParseError, ParseErrorKind, ParseOutcome, ParseSession,
        ParsedStatement, Parser, ParserToken, Token, TokenType, Tokenizer,
    };
}

/// Type-erased (grammar-agnostic) parser and tokenizer types.
pub mod any {
    #[doc(inline)]
    pub use syntaqlite_syntax::any::*;

    #[doc(inline)]
    #[cfg(feature = "fmt")]
    pub use crate::dialect::AnyDialect;

    /// C-ABI types for loading external dialect plugins.
    #[cfg(feature = "fmt")]
    pub mod ffi {
        #[doc(inline)]
        pub use crate::dialect::ffi::CDialectTemplate;
    }
}

/// Typed (grammar-parameterized) parser and tokenizer infrastructure.
pub mod typed {
    #[doc(inline)]
    pub use syntaqlite_syntax::typed::*;

    #[doc(inline)]
    #[cfg(feature = "fmt")]
    pub use crate::dialect::TypedDialect;
}

/// Generated typed AST nodes for the built-in `SQLite` grammar.
#[cfg(feature = "sqlite")]
pub mod nodes {
    #[doc(inline)]
    pub use syntaqlite_syntax::nodes::*;
}
