// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Diagnostic types for semantic analysis.
//!
//! [`Diagnostic`] values carry byte-offset spans, structured messages, and
//! optional "did you mean?" suggestions. Both parse errors and semantic
//! issues (unknown table, wrong arity, etc.) are represented uniformly.

use crate::source::{DocOffset, DocRange, LayerRange};

/// A diagnostic produced by parsing or semantic analysis.
///
/// Every diagnostic carries a byte-offset range into the source text,
/// a structured [`DiagnosticMessage`], a [`Severity`] level, and an
/// optional [`Help`] suggestion (e.g. "did you mean 'users'?").
///
/// You typically obtain diagnostics from [`SemanticModel::diagnostics`] after
/// calling [`SemanticAnalyzer::analyze`].
///
/// [`SemanticModel::diagnostics`]: crate::semantic::SemanticModel::diagnostics
/// [`SemanticAnalyzer::analyze`]: crate::SemanticAnalyzer::analyze
///
/// # Example
///
/// ```
/// # use syntaqlite::semantic::Severity;
/// # use syntaqlite::{SemanticAnalyzer, Catalog, ValidationConfig};
/// # let mut analyzer = SemanticAnalyzer::new();
/// # let catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// # let config = ValidationConfig::default();
/// let model = analyzer.analyze("SELECT 1 FROM no_such_table", &catalog, &config);
///
/// for diag in model.diagnostics() {
///     println!(
///         "[{:?}] bytes {}..{}: {}",
///         diag.severity(),
///         diag.range().start,
///         diag.range().end,
///         diag.message(),
///     );
///     if let Some(help) = diag.help() {
///         println!("  help: {help}");
///     }
/// }
/// ```
/// One frame in a macro expansion traceback attached to a [`Diagnostic`].
///
/// Frame 0 is the outermost (the call site in the original source); the
/// last frame is the innermost (the position inside the deepest expansion
/// buffer).
#[derive(Debug, Clone)]
pub struct DiagnosticFrame {
    /// Buffer text — the original source for the outermost frame, or an
    /// expansion buffer for inner frames.
    pub buffer: String,
    /// Byte range within `buffer` for this frame's span.
    pub range: LayerRange,
}

/// A semantic diagnostic produced by validation, with location, message,
/// severity, optional help, and an optional macro expansion traceback.
#[derive(Debug, Clone)]
pub struct Diagnostic {
    pub(crate) range: DocRange,
    pub(crate) message: DiagnosticMessage,
    pub(crate) severity: Severity,
    pub(crate) help: Option<Help>,
    /// Macro expansion traceback.  Empty for direct (non-expansion) spans.
    /// Frame 0 is the outermost call site (in the original source).  The
    /// last frame is the innermost position inside the deepest expansion.
    pub(crate) expansion_frames: Vec<DiagnosticFrame>,
}

impl Diagnostic {
    /// Create a new diagnostic.
    pub fn new(
        range: DocRange,
        message: DiagnosticMessage,
        severity: Severity,
        help: Option<Help>,
    ) -> Self {
        Self {
            range,
            message,
            severity,
            help,
            expansion_frames: Vec::new(),
        }
    }

    /// Document-absolute byte range of the diagnostic.
    pub fn range(&self) -> DocRange {
        self.range
    }
    /// Document-absolute byte offset of the start of the diagnostic range.
    pub fn start(&self) -> DocOffset {
        self.range.start
    }
    /// Document-absolute byte offset of the end of the diagnostic range.
    pub fn end(&self) -> DocOffset {
        self.range.end
    }
    /// Structured diagnostic message.
    pub fn message(&self) -> &DiagnosticMessage {
        &self.message
    }
    /// Severity level.
    pub fn severity(&self) -> Severity {
        self.severity
    }
    /// Optional structured help attached to the diagnostic.
    pub fn help(&self) -> Option<&Help> {
        self.help.as_ref()
    }

    /// Macro expansion traceback frames.  Empty if the diagnostic is not
    /// inside a macro expansion.
    pub fn expansion_frames(&self) -> &[DiagnosticFrame] {
        &self.expansion_frames
    }
}

/// Structured diagnostic message.
///
/// Each variant carries the identifiers needed for machine-readable
/// consumption; [`fmt::Display`](std::fmt::Display) produces the human-readable form.
///
/// # Example
///
/// ```
/// # use syntaqlite::{SemanticAnalyzer, Catalog, ValidationConfig};
/// # use syntaqlite::semantic::DiagnosticMessage;
/// # let mut analyzer = SemanticAnalyzer::new();
/// # let catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// # let config = ValidationConfig::default();
/// let model = analyzer.analyze("SELECT no_such_func(1)", &catalog, &config);
///
/// for diag in model.diagnostics() {
///     match diag.message() {
///         DiagnosticMessage::UnknownFunction { name } => {
///             println!("function not found: {name}");
///         }
///         DiagnosticMessage::UnknownTable { name } => {
///             println!("table not found: {name}");
///         }
///         DiagnosticMessage::ParseError(msg) => {
///             println!("parse error: {msg}");
///         }
///         other => {
///             // FunctionArity, UnknownColumn, CteColumnCountMismatch
///             println!("{other}");
///         }
///     }
/// }
/// ```
#[derive(Debug, Clone)]
pub enum DiagnosticMessage {
    /// Referenced table name was not found in any catalog layer.
    UnknownTable {
        /// The unresolved table name.
        name: String,
    },
    /// Referenced column name was not found in the current scope.
    UnknownColumn {
        /// The unresolved column name.
        column: String,
        /// Optional table qualifier used by the query.
        table: Option<String>,
    },
    /// Function name was not found in the function catalog.
    UnknownFunction {
        /// The unresolved function name.
        name: String,
    },
    /// Function exists, but the call arity is invalid.
    FunctionArity {
        /// The function name.
        name: String,
        /// Accepted fixed arities.
        expected: Vec<usize>,
        /// Arity supplied by the call.
        got: usize,
    },
    /// CTE declared column list count does not match the SELECT result column count.
    CteColumnCountMismatch {
        /// The CTE name.
        name: String,
        /// Number of names in the declared column list.
        declared: usize,
        /// Number of result columns in the CTE body.
        actual: usize,
    },
    /// Module import could not be resolved.
    UnknownModule {
        /// The unresolved module path (e.g. `slices.flow`).
        name: String,
    },
    /// Parse error from the parser.
    ParseError(String),
}

impl std::fmt::Display for DiagnosticMessage {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::UnknownTable { name } => write!(f, "unknown table '{name}'"),
            Self::UnknownColumn {
                column,
                table: Some(t),
            } => {
                write!(f, "unknown column '{column}' in table '{t}'")
            }
            Self::UnknownColumn {
                column,
                table: None,
            } => {
                write!(f, "unknown column '{column}'")
            }
            Self::UnknownFunction { name } => write!(f, "unknown function '{name}'"),
            Self::FunctionArity {
                name,
                expected,
                got,
            } => {
                let expected_str: Vec<String> = expected.iter().map(ToString::to_string).collect();
                write!(
                    f,
                    "function '{name}' expects {} argument(s), got {got}",
                    expected_str.join(" or ")
                )
            }
            Self::CteColumnCountMismatch {
                name,
                declared,
                actual,
            } => write!(
                f,
                "table '{name}' has {actual} values for {declared} columns"
            ),
            Self::UnknownModule { name } => write!(f, "unknown module '{name}'"),
            Self::ParseError(msg) => f.write_str(msg),
        }
    }
}

impl DiagnosticMessage {
    /// Returns `true` for parse errors (`Other`), `false` for semantic diagnostics.
    pub fn is_parse_error(&self) -> bool {
        matches!(self, Self::ParseError(_))
    }
}

/// Structured help attached to a [`Diagnostic`].
///
/// When the analyzer finds a close match for an unresolved identifier it
/// attaches a [`Help::Suggestion`] containing the corrected name. Display
/// this as a "did you mean ...?" hint in your UI.
#[derive(Debug, Clone)]
pub enum Help {
    /// A "did you mean?" suggestion with the corrected identifier.
    Suggestion(String),
}

impl std::fmt::Display for Help {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Help::Suggestion(s) => write!(f, "did you mean '{s}'?"),
        }
    }
}

/// Diagnostic severity level.
///
/// Severity levels mirror the LSP `DiagnosticSeverity` enum. Use them to
/// decide how to present issues to the user:
///
/// | Level       | Meaning                                              |
/// |-------------|------------------------------------------------------|
/// | [`Error`](Self::Error)     | Blocking issue -- the query is invalid.  |
/// | [`Warning`](Self::Warning) | Suspicious but non-fatal (e.g. unresolved name in lenient mode). |
/// | [`Info`](Self::Info)       | Informational note, no action required.  |
/// | [`Hint`](Self::Hint)       | Style suggestion or minor improvement.   |
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Severity {
    /// Blocking issue that should fail validation.
    Error,
    /// Suspicious but non-fatal issue.
    Warning,
    /// Informational diagnostic.
    Info,
    /// Lowest-severity diagnostic, usually a suggestion.
    Hint,
}

// ── JSON serialization (feature = "json") ────────────────────────────

#[cfg(feature = "serde")]
impl serde::Serialize for DiagnosticMessage {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        match self {
            Self::ParseError(_) => serializer.serialize_none(),
            Self::UnknownTable { name } => {
                let mut m = serializer.serialize_map(Some(2))?;
                m.serialize_entry("kind", "unknown_table")?;
                m.serialize_entry("name", name)?;
                m.end()
            }
            Self::UnknownColumn { column, table } => {
                let len = if table.is_some() { 3 } else { 2 };
                let mut m = serializer.serialize_map(Some(len))?;
                m.serialize_entry("kind", "unknown_column")?;
                m.serialize_entry("column", column)?;
                if let Some(t) = table {
                    m.serialize_entry("table", t)?;
                }
                m.end()
            }
            Self::UnknownFunction { name } => {
                let mut m = serializer.serialize_map(Some(2))?;
                m.serialize_entry("kind", "unknown_function")?;
                m.serialize_entry("name", name)?;
                m.end()
            }
            Self::CteColumnCountMismatch {
                name,
                declared,
                actual,
            } => {
                let mut m = serializer.serialize_map(Some(4))?;
                m.serialize_entry("kind", "cte_column_count_mismatch")?;
                m.serialize_entry("name", name)?;
                m.serialize_entry("declared", declared)?;
                m.serialize_entry("actual", actual)?;
                m.end()
            }
            Self::FunctionArity {
                name,
                expected,
                got,
            } => {
                let mut m = serializer.serialize_map(Some(4))?;
                m.serialize_entry("kind", "function_arity")?;
                m.serialize_entry("name", name)?;
                m.serialize_entry("expected", expected)?;
                m.serialize_entry("got", got)?;
                m.end()
            }
            Self::UnknownModule { name } => {
                let mut m = serializer.serialize_map(Some(2))?;
                m.serialize_entry("kind", "unknown_module")?;
                m.serialize_entry("name", name)?;
                m.end()
            }
        }
    }
}

#[cfg(feature = "serde")]
impl serde::Serialize for Help {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        match self {
            Self::Suggestion(value) => {
                let mut m = serializer.serialize_map(Some(2))?;
                m.serialize_entry("kind", "suggestion")?;
                m.serialize_entry("value", value)?;
                m.end()
            }
        }
    }
}

/// Serializes as a lowercase string (e.g. `"error"`, `"warning"`).
#[cfg(feature = "serde")]
impl serde::Serialize for Severity {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(match self {
            Self::Error => "error",
            Self::Warning => "warning",
            Self::Info => "info",
            Self::Hint => "hint",
        })
    }
}

/// Serializes with a distinct `"message"` (Display) and `"detail"` (structured)
/// field, matching the shape expected by LSP and WASM consumers.
#[cfg(feature = "serde")]
impl serde::Serialize for Diagnostic {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        let len = if self.help.is_some() { 7 } else { 5 };
        let mut m = serializer.serialize_map(Some(len))?;
        m.serialize_entry("startOffset", &self.range.start.as_u32())?;
        m.serialize_entry("endOffset", &self.range.end.as_u32())?;
        m.serialize_entry("message", &self.message.to_string())?;
        m.serialize_entry("detail", &self.message)?;
        m.serialize_entry("severity", &self.severity)?;
        if let Some(ref help) = self.help {
            m.serialize_entry("help", &help.to_string())?;
            m.serialize_entry("helpDetail", help)?;
        }
        m.end()
    }
}
