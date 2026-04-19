// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! SQL formatter.
//!
//! Pretty-prints SQL source text with consistent style. The formatter parses
//! each statement, runs a bytecode interpreter over the AST, and renders the
//! result with a Wadler-style document renderer.
//!
//! The most commonly used types ([`Formatter`], [`FormatConfig`],
//! [`KeywordCase`]) are re-exported at the crate root. This module also
//! provides [`FormatError`], returned when a statement fails to parse during
//! formatting.
//!
//! # Example
//!
//! ```
//! use syntaqlite::fmt::{Formatter, FormatConfig, KeywordCase};
//!
//! let mut fmt = Formatter::with_config(
//!     &FormatConfig::default()
//!         .with_keyword_case(KeywordCase::Lower)
//!         .with_indent_width(4),
//! );
//! let output = fmt.format("SELECT 1").unwrap();
//! assert!(output.starts_with("select"));
//! ```

mod comment;
mod doc;
#[cfg(feature = "sqlite")]
#[expect(unreachable_pub)]
pub(crate) mod ffi;
pub(crate) mod formatter;
mod interpret;

#[doc(inline)]
pub use formatter::Formatter;

use crate::source::DocRange;

// ── Config types (formerly config.rs) ────────────────────────────────────

/// Controls how SQL keywords are cased in formatted output.
///
/// ```rust
/// # use syntaqlite::fmt::KeywordCase;
/// # use syntaqlite::{Formatter, FormatConfig};
/// let mut fmt = Formatter::with_config(
///     &FormatConfig::default().with_keyword_case(KeywordCase::Lower),
/// );
/// let out = fmt.format("SELECT 1").unwrap();
/// assert!(out.starts_with("select"));
/// ```
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum KeywordCase {
    /// Convert keywords to UPPER CASE.
    #[default]
    Upper,
    /// Convert keywords to lower case.
    Lower,
}

/// Configuration for the SQL formatter.
///
/// Controls line width, indentation, keyword casing, and semicolons.
/// All settings have sensible defaults (see [`Default`] impl):
///
/// | Setting        | Default                      |
/// |----------------|------------------------------|
/// | `line_width`   | 80                           |
/// | `indent_width` | 2                            |
/// | `keyword_case` | [`KeywordCase::Upper`]       |
/// | `semicolons`   | `true`                       |
///
/// Use the builder methods (`with_*`) to customize:
///
/// ```rust
/// # use syntaqlite::fmt::KeywordCase;
/// # use syntaqlite::FormatConfig;
/// let config = FormatConfig::default()
///     .with_line_width(120)
///     .with_indent_width(4)
///     .with_keyword_case(KeywordCase::Lower)
///     .with_semicolons(false);
///
/// assert_eq!(config.line_width(), 120);
/// assert_eq!(config.indent_width(), 4);
/// assert_eq!(config.keyword_case(), KeywordCase::Lower);
/// assert!(!config.semicolons());
/// ```
///
/// Pass the config to [`Formatter::with_config`](crate::Formatter::with_config)
/// to apply it.
#[derive(Debug, Clone)]
pub struct FormatConfig {
    line_width: usize,
    indent_width: usize,
    keyword_case: KeywordCase,
    semicolons: bool,
}

impl FormatConfig {
    /// Maximum line width before breaking.
    pub fn line_width(&self) -> usize {
        self.line_width
    }

    /// Number of spaces per indentation level.
    pub fn indent_width(&self) -> usize {
        self.indent_width
    }

    /// How SQL keywords are cased.
    pub fn keyword_case(&self) -> KeywordCase {
        self.keyword_case
    }

    /// Whether semicolons are appended after each statement.
    pub fn semicolons(&self) -> bool {
        self.semicolons
    }

    /// Set the maximum line width before breaking.
    #[must_use]
    pub fn with_line_width(mut self, width: usize) -> Self {
        self.line_width = width;
        self
    }

    /// Set the number of spaces per indentation level.
    #[must_use]
    pub fn with_indent_width(mut self, width: usize) -> Self {
        self.indent_width = width;
        self
    }

    /// Set how SQL keywords are cased.
    #[must_use]
    pub fn with_keyword_case(mut self, case: KeywordCase) -> Self {
        self.keyword_case = case;
        self
    }

    /// Set whether semicolons are appended after each statement.
    #[must_use]
    pub fn with_semicolons(mut self, semicolons: bool) -> Self {
        self.semicolons = semicolons;
        self
    }
}

/// An error returned by [`crate::Formatter::format`] when a statement fails to parse.
#[derive(Debug, Clone)]
pub struct FormatError {
    message: String,
    range: Option<DocRange>,
}

impl FormatError {
    pub(crate) fn new(message: String, range: Option<DocRange>) -> Self {
        Self { message, range }
    }

    /// Human-readable error message.
    pub fn message(&self) -> &str {
        &self.message
    }

    /// Document-absolute byte range of the error token, if known.
    pub fn range(&self) -> Option<DocRange> {
        self.range
    }
}

impl std::fmt::Display for FormatError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.message)
    }
}

impl std::error::Error for FormatError {}

impl Default for FormatConfig {
    fn default() -> Self {
        FormatConfig {
            line_width: 80,
            indent_width: 2,
            keyword_case: KeywordCase::Upper,
            semicolons: true,
        }
    }
}
