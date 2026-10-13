// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Diagnostic types for analysis.
//!
//! [`Diagnostic`] values carry byte-offset spans, structured messages, and
//! optional "did you mean?" suggestions. Both parse errors and semantic
//! issues (unknown table, wrong arity, etc.) are represented uniformly.

#[cfg(feature = "analysis")]
pub(crate) mod fuzzy;
#[cfg(feature = "analysis")]
pub(crate) mod render;

use crate::source::{DocOffset, DocRange, LayerRange};

/// A diagnostic produced by parsing or analysis.
///
/// Every diagnostic carries a byte-offset range into the source text,
/// a structured [`DiagnosticMessage`], a [`Severity`] level, and an
/// optional [`Help`] suggestion (e.g. "did you mean 'users'?").
///
/// You typically obtain diagnostics from [`Analysis::diagnostics`] after
/// calling [`Analyzer::analyze`].
///
/// [`Analysis::diagnostics`]: crate::analysis::Analysis::diagnostics
/// [`Analyzer::analyze`]: crate::Analyzer::analyze
///
/// # Example
///
/// ```
/// # use syntaqlite::analysis::Severity;
/// # use syntaqlite::{AnalysisContext, Analyzer, Catalog};
/// # let mut analyzer = Analyzer::new();
/// # let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// # let mut ctx = AnalysisContext::new(&mut catalog);
/// let model = analyzer.analyze("SELECT 1 FROM no_such_table", &mut ctx);
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

/// A diagnostic produced by analysis, with location, message,
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
/// # use syntaqlite::{AnalysisContext, Analyzer, Catalog};
/// # use syntaqlite::analysis::DiagnosticMessage;
/// # let mut analyzer = Analyzer::new();
/// # let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// # let mut ctx = AnalysisContext::new(&mut catalog);
/// let model = analyzer.analyze("SELECT no_such_func(1)", &mut ctx);
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
///             // Every other variant.
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
    /// A column was qualified with its table name where `SQLite` forbids
    /// qualified names.
    QualifiedRef {
        /// The table qualifier.
        table: String,
        /// The column name.
        column: String,
        /// Where the qualified name appeared.
        context: QualifiedRefContext,
    },
    /// `CREATE INDEX` targets a view, which `SQLite` cannot index.
    IndexOnView {
        /// The view name.
        name: String,
    },
    /// `DROP TABLE` names a view, or `DROP VIEW` names a table.
    DropWrongKind {
        /// The relation name.
        name: String,
        /// Whether the relation is a view.
        is_view: bool,
    },
    /// A DDL statement changes one of `SQLite`'s built-in tables.
    BuiltinModify {
        /// The built-in table name.
        name: String,
        /// What the statement tried to do.
        change: BuiltinChange,
    },
    /// The arms of a compound SELECT have different numbers of columns.
    CompoundColumns {
        /// The compound operator as written, such as `UNION ALL`.
        operator: String,
        /// Columns in the left arm.
        left: usize,
        /// Columns in the right arm.
        right: usize,
    },
    /// A window specification names a window the query doesn't define.
    UnknownWindow {
        /// The window name.
        name: String,
    },
    /// An ON clause references a table joined after it, where `SQLite`
    /// forbids that.
    OnClauseOrder {
        /// The later table, as named in the query.
        name: String,
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
            Self::QualifiedRef {
                table,
                column,
                context,
            } => write!(
                f,
                "qualified name '{table}.{column}' is not allowed in {context}"
            ),
            Self::IndexOnView { name } => write!(f, "cannot index view '{name}'"),
            Self::DropWrongKind {
                name,
                is_view: true,
            } => write!(f, "use DROP VIEW to drop view '{name}'"),
            Self::DropWrongKind {
                name,
                is_view: false,
            } => write!(f, "use DROP TABLE to drop table '{name}'"),
            Self::BuiltinModify { name, change } => match change {
                BuiltinChange::Drop => write!(f, "built-in table '{name}' may not be dropped"),
                BuiltinChange::Alter => write!(f, "built-in table '{name}' may not be altered"),
                BuiltinChange::Index => write!(f, "built-in table '{name}' may not be indexed"),
                BuiltinChange::Trigger => {
                    write!(f, "cannot create a trigger on built-in table '{name}'")
                }
            },
            Self::CompoundColumns {
                operator,
                left,
                right,
            } => write!(
                f,
                "SELECTs to the left and right of {operator} have {left} and {right} result columns"
            ),
            Self::UnknownWindow { name } => write!(f, "unknown window '{name}'"),
            Self::OnClauseOrder { name } => {
                write!(
                    f,
                    "ON clause references '{name}', which is joined to its right"
                )
            }
            Self::UnknownModule { name } => write!(f, "unknown module '{name}'"),
            Self::ParseError(msg) => f.write_str(msg),
        }
    }
}

/// What a [`DiagnosticMessage::BuiltinModify`] statement tried to do.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BuiltinChange {
    /// `DROP TABLE`.
    Drop,
    /// `ALTER TABLE`.
    Alter,
    /// `CREATE INDEX`.
    Index,
    /// `CREATE TRIGGER`.
    Trigger,
}

impl BuiltinChange {
    fn as_str(self) -> &'static str {
        match self {
            Self::Drop => "drop",
            Self::Alter => "alter",
            Self::Index => "index",
            Self::Trigger => "trigger",
        }
    }
}

/// Where a [`DiagnosticMessage::QualifiedRef`] appeared.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum QualifiedRefContext {
    /// An index column list, including PRIMARY KEY and UNIQUE constraints.
    IndexExpression,
    /// A generated column expression.
    GeneratedColumn,
}

impl QualifiedRefContext {
    fn as_str(self) -> &'static str {
        match self {
            Self::IndexExpression => "index_expression",
            Self::GeneratedColumn => "generated_column",
        }
    }
}

impl std::fmt::Display for QualifiedRefContext {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(match self {
            Self::IndexExpression => "an index expression",
            Self::GeneratedColumn => "a generated column",
        })
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
    /// Blocking issue that should fail analysis.
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
fn kind_and_name<S: serde::Serializer>(
    serializer: S,
    kind: &str,
    name: &str,
) -> Result<S::Ok, S::Error> {
    use serde::ser::SerializeMap;
    let mut m = serializer.serialize_map(Some(2))?;
    m.serialize_entry("kind", kind)?;
    m.serialize_entry("name", name)?;
    m.end()
}

#[cfg(feature = "serde")]
impl serde::Serialize for DiagnosticMessage {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        match self {
            Self::ParseError(_) => serializer.serialize_none(),
            Self::UnknownTable { name } => kind_and_name(serializer, "unknown_table", name),
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
            Self::UnknownFunction { name } => kind_and_name(serializer, "unknown_function", name),
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
            Self::QualifiedRef {
                table,
                column,
                context,
            } => {
                let mut m = serializer.serialize_map(Some(4))?;
                m.serialize_entry("kind", "qualified_ref")?;
                m.serialize_entry("table", table)?;
                m.serialize_entry("column", column)?;
                m.serialize_entry("context", context.as_str())?;
                m.end()
            }
            Self::IndexOnView { name } => kind_and_name(serializer, "index_on_view", name),
            Self::DropWrongKind { name, is_view } => {
                let mut m = serializer.serialize_map(Some(3))?;
                m.serialize_entry("kind", "drop_wrong_kind")?;
                m.serialize_entry("name", name)?;
                m.serialize_entry("is_view", is_view)?;
                m.end()
            }
            Self::BuiltinModify { name, change } => {
                let mut m = serializer.serialize_map(Some(3))?;
                m.serialize_entry("kind", "builtin_modify")?;
                m.serialize_entry("name", name)?;
                m.serialize_entry("change", change.as_str())?;
                m.end()
            }
            Self::CompoundColumns {
                operator,
                left,
                right,
            } => {
                let mut m = serializer.serialize_map(Some(4))?;
                m.serialize_entry("kind", "compound_columns")?;
                m.serialize_entry("operator", operator)?;
                m.serialize_entry("left", left)?;
                m.serialize_entry("right", right)?;
                m.end()
            }
            Self::UnknownWindow { name } => kind_and_name(serializer, "unknown_window", name),
            Self::OnClauseOrder { name } => kind_and_name(serializer, "on_clause_order", name),
            Self::UnknownModule { name } => kind_and_name(serializer, "unknown_module", name),
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
