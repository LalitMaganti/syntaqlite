// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Semantic analysis and validation.
//!
//! Validates SQL against a known database schema — resolving table, column,
//! and function references and producing structured [`Diagnostic`] values with
//! byte-offset spans and "did you mean?" suggestions.
//!
//! The most commonly used types ([`SemanticAnalyzer`], [`Catalog`],
//! [`CatalogLayer`], [`Diagnostic`], [`Severity`], [`ValidationConfig`]) are
//! re-exported at the crate root. This module also provides:
//!
//! - [`SemanticModel`] — the result of a single analysis pass.
//! - [`DiagnosticMessage`] — structured message variants for pattern matching.
//! - [`Help`] — "did you mean?" suggestion attached to a diagnostic.
//! - [`AnalysisMode`] — document vs. execute mode for DDL accumulation.
//! - [`CatalogLayerContents`] — the data stored in a single catalog layer.
//! - [`AritySpec`], [`FunctionCategory`] — function metadata for catalog
//!   registration.
//!
//! # Example
//!
//! ```
//! use syntaqlite::semantic::{
//!     SemanticAnalyzer, Catalog, CatalogLayer, ValidationConfig,
//!     DiagnosticMessage,
//! };
//!
//! let mut analyzer = SemanticAnalyzer::new();
//! let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
//! catalog.layer_mut(CatalogLayer::Database)
//!     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
//!
//! let model = analyzer.analyze(
//!     "SELECT id, nme FROM users",
//!     &catalog,
//!     &ValidationConfig::default(),
//! );
//!
//! // "nme" is close to "name" — expect a diagnostic with a suggestion.
//! assert!(model.has_diagnostics());
//! ```

#[cfg(feature = "validation")]
pub(crate) mod analyzer;
#[cfg(feature = "validation")]
pub(crate) mod catalog;
#[cfg(feature = "validation")]
pub(crate) mod ddl;
pub(crate) mod diagnostics;
#[cfg(feature = "validation")]
#[expect(unreachable_pub)]
pub(crate) mod ffi;
#[cfg(feature = "validation")]
pub(crate) mod fuzzy;
#[cfg(feature = "validation")]
mod lineage;
#[cfg(feature = "validation")]
pub(crate) mod model;
#[cfg(feature = "validation")]
pub(crate) mod render;

// ── Public re-exports ─────────────────────────────────────────────────────────

#[doc(inline)]
#[cfg(feature = "validation")]
pub use analyzer::SemanticAnalyzer;
#[doc(inline)]
#[cfg(feature = "validation")]
pub use catalog::{AritySpec, Catalog, CatalogLayer, CatalogLayerContents, FunctionCategory};
#[doc(inline)]
pub use diagnostics::{Diagnostic, DiagnosticMessage, Help, Severity};
#[doc(inline)]
#[cfg(feature = "validation")]
pub use lineage::{
    ColumnLineage, ColumnOrigin, LineageResult, PhysicalTableAccess, RelationAccess, RelationKind,
};
#[doc(inline)]
#[cfg(feature = "validation")]
pub use model::{DefinedRelation, SemanticModel, StatementModel};

// ── Module resolution ────────────────────────────────────────────────────────

/// Callback for resolving `INCLUDE PERFETTO MODULE` (or similar) import
/// statements during semantic analysis.
///
/// Implement this trait to teach the [`SemanticAnalyzer`] how to locate
/// module source text. The analyzer calls [`resolve`](Self::resolve) when
/// it encounters an import statement; if the module is found, its DDL is
/// analyzed and accumulated into the catalog so that subsequent statements
/// can reference the imported definitions.
///
/// # Example
///
/// ```
/// use syntaqlite::semantic::ModuleResolver;
///
/// struct InMemoryResolver {
///     modules: std::collections::HashMap<String, String>,
/// }
///
/// impl ModuleResolver for InMemoryResolver {
///     fn resolve(&self, module_path: &str) -> Option<String> {
///         self.modules.get(module_path).cloned()
///     }
/// }
/// ```
pub trait ModuleResolver {
    /// Given a dotted module path (e.g. `"slices.flow"`), return the SQL
    /// source text for that module.
    ///
    /// Return `None` if the module cannot be found — the analyzer will emit
    /// an [`UnknownModule`](DiagnosticMessage::UnknownModule) diagnostic.
    fn resolve(&self, module_path: &str) -> Option<String>;
}

/// A [`ModuleResolver`] that maps dotted module paths to files under a
/// directory root.
///
/// The path `slices.flow` is resolved to `<root>/slices/flow.sql`.
///
/// # Example
///
/// ```no_run
/// use syntaqlite::semantic::{DirectoryModuleResolver, ModuleResolver};
/// use std::path::PathBuf;
///
/// let resolver = DirectoryModuleResolver::new(PathBuf::from("stdlib/"));
/// assert_eq!(
///     resolver.resolve("slices.flow").is_some(),
///     std::path::Path::new("stdlib/slices/flow.sql").exists(),
/// );
/// ```
pub struct DirectoryModuleResolver {
    root: std::path::PathBuf,
}

impl DirectoryModuleResolver {
    /// Create a resolver rooted at the given directory.
    pub fn new(root: std::path::PathBuf) -> Self {
        Self { root }
    }
}

impl ModuleResolver for DirectoryModuleResolver {
    fn resolve(&self, module_path: &str) -> Option<String> {
        let rel = module_path.replace('.', std::path::MAIN_SEPARATOR_STR);
        let path = self.root.join(rel).with_extension("sql");
        std::fs::read_to_string(path).ok()
    }
}

/// Whether statements are being analyzed (editing a file) or executed
/// (running sequentially in a session).
///
/// In `Document` mode, DDL from previous [`SemanticAnalyzer::analyze`] calls
/// is discarded — each call analyzes a fresh document.
///
/// In `Execute` mode, DDL accumulates across calls — a `CREATE TABLE` in one
/// call makes the table visible to subsequent calls, matching the behaviour of
/// an interactive database session.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum AnalysisMode {
    /// Statements are being analyzed (e.g. editing a SQL file).
    /// DDL resets between `analyze()` calls.
    #[default]
    Document,
    /// Statements are being executed sequentially.
    /// DDL accumulates across `analyze()` calls.
    Execute,
}

/// Severity level for a check category.
///
/// Follows the Rust/Clippy convention: `allow` suppresses the diagnostic,
/// `warn` emits a warning, `deny` emits an error.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CheckLevel {
    /// Suppress the diagnostic entirely.
    Allow,
    /// Emit a warning (does not cause a non-zero exit code).
    Warn,
    /// Emit an error (causes a non-zero exit code).
    Deny,
}

impl CheckLevel {
    /// Parse from a string (`"allow"`, `"warn"`, `"deny"`).
    ///
    /// # Errors
    ///
    /// Returns an error if `s` is not a recognized level name.
    pub fn parse(s: &str) -> Result<Self, String> {
        match s {
            "allow" => Ok(CheckLevel::Allow),
            "warn" => Ok(CheckLevel::Warn),
            "deny" => Ok(CheckLevel::Deny),
            _ => Err(format!(
                "invalid check level: {s:?} (expected allow, warn, or deny)"
            )),
        }
    }

    /// Convert to [`Severity`] for diagnostic emission.
    /// Returns `None` for `Allow` (diagnostic should be suppressed).
    pub fn to_severity(self) -> Option<Severity> {
        match self {
            CheckLevel::Allow => None,
            CheckLevel::Warn => Some(Severity::Warning),
            CheckLevel::Deny => Some(Severity::Error),
        }
    }
}

/// Per-category check levels.
///
/// Each field controls whether and at what severity a category of diagnostic
/// is emitted. Uses the Rust/Clippy convention: `allow` (suppressed), `warn`,
/// `deny` (error).
///
/// # Categories
///
/// | Name | Field | What it controls | Default |
/// |------|-------|-----------------|---------|
/// | `parse-errors` | [`parse_errors`](Self::parse_errors) | Syntax errors from the parser | `deny` |
/// | `unknown-table` | [`unknown_table`](Self::unknown_table) | Unresolved table/view references | `warn` |
/// | `unknown-column` | [`unknown_column`](Self::unknown_column) | Unresolved column references | `warn` |
/// | `unknown-function` | [`unknown_function`](Self::unknown_function) | Unresolved function names | `warn` |
/// | `function-arity` | [`function_arity`](Self::function_arity) | Wrong number of function arguments | `warn` |
/// | `cte-columns` | [`cte_columns`](Self::cte_columns) | CTE column count mismatches | `deny` |
///
/// The group name `schema` is a shorthand for `unknown-table`,
/// `unknown-column`, `unknown-function`, and `function-arity`.
#[derive(Clone, Copy, Debug)]
pub struct CheckConfig {
    pub(crate) parse_errors: CheckLevel,
    pub(crate) unknown_table: CheckLevel,
    pub(crate) unknown_column: CheckLevel,
    pub(crate) unknown_function: CheckLevel,
    pub(crate) function_arity: CheckLevel,
    pub(crate) cte_columns: CheckLevel,
}

impl Default for CheckConfig {
    fn default() -> Self {
        CheckConfig {
            parse_errors: CheckLevel::Deny,
            unknown_table: CheckLevel::Warn,
            unknown_column: CheckLevel::Warn,
            unknown_function: CheckLevel::Warn,
            function_arity: CheckLevel::Warn,
            cte_columns: CheckLevel::Deny,
        }
    }
}

impl CheckConfig {
    // ── Getters ──────────────────────────────────────────────────────────

    /// Level for parse errors.
    pub fn parse_errors(&self) -> CheckLevel {
        self.parse_errors
    }
    /// Level for unknown table references.
    pub fn unknown_table(&self) -> CheckLevel {
        self.unknown_table
    }
    /// Level for unknown column references.
    pub fn unknown_column(&self) -> CheckLevel {
        self.unknown_column
    }
    /// Level for unknown function references.
    pub fn unknown_function(&self) -> CheckLevel {
        self.unknown_function
    }
    /// Level for function arity mismatches.
    pub fn function_arity(&self) -> CheckLevel {
        self.function_arity
    }
    /// Level for CTE column count mismatches.
    pub fn cte_columns(&self) -> CheckLevel {
        self.cte_columns
    }

    // ── Builders ─────────────────────────────────────────────────────────

    /// Set the level for parse errors.
    #[must_use]
    pub fn with_parse_errors(mut self, level: CheckLevel) -> Self {
        self.parse_errors = level;
        self
    }
    /// Set the level for unknown table references.
    #[must_use]
    pub fn with_unknown_table(mut self, level: CheckLevel) -> Self {
        self.unknown_table = level;
        self
    }
    /// Set the level for unknown column references.
    #[must_use]
    pub fn with_unknown_column(mut self, level: CheckLevel) -> Self {
        self.unknown_column = level;
        self
    }
    /// Set the level for unknown function references.
    #[must_use]
    pub fn with_unknown_function(mut self, level: CheckLevel) -> Self {
        self.unknown_function = level;
        self
    }
    /// Set the level for function arity mismatches.
    #[must_use]
    pub fn with_function_arity(mut self, level: CheckLevel) -> Self {
        self.function_arity = level;
        self
    }
    /// Set the level for CTE column count mismatches.
    #[must_use]
    pub fn with_cte_columns(mut self, level: CheckLevel) -> Self {
        self.cte_columns = level;
        self
    }
    /// Set all schema checks (`unknown-table`, `unknown-column`,
    /// `unknown-function`, `function-arity`).
    #[must_use]
    pub fn with_schema(mut self, level: CheckLevel) -> Self {
        self.unknown_table = level;
        self.unknown_column = level;
        self.unknown_function = level;
        self.function_arity = level;
        self
    }
    /// Set all checks.
    #[must_use]
    pub fn with_all(mut self, level: CheckLevel) -> Self {
        self.parse_errors = level;
        self.unknown_table = level;
        self.unknown_column = level;
        self.unknown_function = level;
        self.function_arity = level;
        self.cte_columns = level;
        self
    }

    // ── Name-based dispatch (for CLI/config file) ────────────────────────

    /// All check category names (for CLI help and error messages).
    pub const CATEGORY_NAMES: &[&str] = &[
        "parse-errors",
        "unknown-table",
        "unknown-column",
        "unknown-function",
        "function-arity",
        "cte-columns",
    ];

    /// Group names that expand to multiple categories.
    pub const GROUP_NAMES: &[&str] = &["schema", "all"];

    /// Set a category by name. For CLI and config file dispatch.
    ///
    /// # Errors
    ///
    /// Returns `Err` if the name is unknown.
    pub fn set_by_name(mut self, name: &str, level: CheckLevel) -> Result<Self, String> {
        match name {
            "parse-errors" => self.parse_errors = level,
            "unknown-table" => self.unknown_table = level,
            "unknown-column" => self.unknown_column = level,
            "unknown-function" => self.unknown_function = level,
            "function-arity" => self.function_arity = level,
            "cte-columns" => self.cte_columns = level,
            "schema" => self = self.with_schema(level),
            "all" => self = self.with_all(level),
            _ => return Err(format!("unknown check category: {name}")),
        }
        Ok(self)
    }
}

/// Configuration for semantic validation.
///
/// Controls how the [`SemanticAnalyzer`] reports diagnostics and
/// generates "did you mean?" suggestions:
///
/// - **`checks`** — per-category severity levels (`allow`/`warn`/`deny`).
///   See [`CheckConfig`].
/// - **`suggestion_threshold`** (`2` by default) — maximum Levenshtein
///   distance for "did you mean?" suggestions. Set to `0` to disable
///   suggestions entirely.
///
/// # Example
///
/// ```
/// # use syntaqlite::semantic::{CheckLevel, Severity};
/// # use syntaqlite::ValidationConfig;
/// let config = ValidationConfig::default();
/// assert_eq!(config.suggestion_threshold(), 2);
///
/// // Deny all schema checks (errors instead of warnings).
/// let checks = config.checks().with_schema(CheckLevel::Deny);
/// let strict = config.with_checks(checks);
/// ```
#[derive(Clone, Copy)]
pub struct ValidationConfig {
    suggestion_threshold: usize,
    checks: CheckConfig,
}

impl Default for ValidationConfig {
    fn default() -> Self {
        ValidationConfig {
            suggestion_threshold: 2,
            checks: CheckConfig::default(),
        }
    }
}

impl ValidationConfig {
    /// Maximum Levenshtein distance for "did you mean?" suggestions.
    pub fn suggestion_threshold(&self) -> usize {
        self.suggestion_threshold
    }

    /// Per-category check levels.
    pub fn checks(&self) -> CheckConfig {
        self.checks
    }

    /// Set the maximum Levenshtein distance for suggestions.
    #[must_use]
    pub fn with_suggestion_threshold(mut self, threshold: usize) -> Self {
        self.suggestion_threshold = threshold;
        self
    }

    /// Set per-category check config.
    #[must_use]
    pub fn with_checks(mut self, checks: CheckConfig) -> Self {
        self.checks = checks;
        self
    }

    /// Set all schema checks to `deny` (errors).
    ///
    /// Convenience method equivalent to setting `unknown-table`,
    /// `unknown-column`, `unknown-function`, and `function-arity` to
    /// [`CheckLevel::Deny`].
    #[must_use]
    pub fn with_strict_schema(mut self) -> Self {
        self.checks = self.checks.with_schema(CheckLevel::Deny);
        self
    }
}

#[cfg(test)]
mod check_config_tests {
    use super::*;

    #[test]
    fn check_level_parse() {
        assert_eq!(CheckLevel::parse("allow").unwrap(), CheckLevel::Allow);
        assert_eq!(CheckLevel::parse("warn").unwrap(), CheckLevel::Warn);
        assert_eq!(CheckLevel::parse("deny").unwrap(), CheckLevel::Deny);
        assert!(CheckLevel::parse("invalid").is_err());
    }

    #[test]
    fn check_level_to_severity() {
        assert_eq!(CheckLevel::Allow.to_severity(), None);
        assert_eq!(CheckLevel::Warn.to_severity(), Some(Severity::Warning));
        assert_eq!(CheckLevel::Deny.to_severity(), Some(Severity::Error));
    }

    #[test]
    fn default_levels() {
        let c = CheckConfig::default();
        assert_eq!(c.parse_errors(), CheckLevel::Deny);
        assert_eq!(c.unknown_table(), CheckLevel::Warn);
        assert_eq!(c.unknown_column(), CheckLevel::Warn);
        assert_eq!(c.unknown_function(), CheckLevel::Warn);
        assert_eq!(c.function_arity(), CheckLevel::Warn);
        assert_eq!(c.cte_columns(), CheckLevel::Deny);
    }

    #[test]
    fn builder_individual() {
        let c = CheckConfig::default().with_unknown_table(CheckLevel::Deny);
        assert_eq!(c.unknown_table(), CheckLevel::Deny);
        assert_eq!(c.unknown_column(), CheckLevel::Warn); // unchanged
    }

    #[test]
    fn builder_schema_group() {
        let c = CheckConfig::default().with_schema(CheckLevel::Allow);
        assert_eq!(c.unknown_table(), CheckLevel::Allow);
        assert_eq!(c.unknown_column(), CheckLevel::Allow);
        assert_eq!(c.unknown_function(), CheckLevel::Allow);
        assert_eq!(c.function_arity(), CheckLevel::Allow);
        // Non-schema checks unchanged.
        assert_eq!(c.parse_errors(), CheckLevel::Deny);
        assert_eq!(c.cte_columns(), CheckLevel::Deny);
    }

    #[test]
    fn builder_all_group() {
        let c = CheckConfig::default().with_all(CheckLevel::Allow);
        assert_eq!(c.parse_errors(), CheckLevel::Allow);
        assert_eq!(c.unknown_table(), CheckLevel::Allow);
        assert_eq!(c.cte_columns(), CheckLevel::Allow);
    }

    #[test]
    fn set_by_name_works() {
        let c = CheckConfig::default()
            .set_by_name("unknown-table", CheckLevel::Deny)
            .unwrap();
        assert_eq!(c.unknown_table(), CheckLevel::Deny);
    }

    #[test]
    fn set_by_name_unknown_category_errors() {
        let result = CheckConfig::default().set_by_name("nonexistent", CheckLevel::Warn);
        assert!(result.is_err());
    }

    #[test]
    fn with_strict_schema_sets_deny() {
        let config = ValidationConfig::default().with_strict_schema();
        assert_eq!(config.checks().unknown_table(), CheckLevel::Deny);
        assert_eq!(config.checks().unknown_column(), CheckLevel::Deny);
        assert_eq!(config.checks().unknown_function(), CheckLevel::Deny);
        assert_eq!(config.checks().function_arity(), CheckLevel::Deny);
        // Non-schema checks unchanged.
        assert_eq!(config.checks().cte_columns(), CheckLevel::Deny);
        assert_eq!(config.checks().parse_errors(), CheckLevel::Deny);
    }
}
