// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Configuration types for analysis.

use super::diagnostics::Severity;

/// Whether statements are being analyzed (editing a file) or executed
/// (running sequentially in a session).
///
/// In `Document` mode, DDL from previous [`Analyzer::analyze`] calls
/// is discarded — each call analyzes a fresh document.
///
/// In `Execute` mode, DDL accumulates across calls — a `CREATE TABLE` in one
/// call makes the table visible to subsequent calls, matching the behaviour of
/// an interactive database session.
///
/// [`Analyzer::analyze`]: super::Analyzer::analyze
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
/// | `qualified-ref` | [`qualified_ref`](Self::qualified_ref) | Qualified names where `SQLite` forbids them | `deny` |
/// | `index-on-view` | [`index_on_view`](Self::index_on_view) | `CREATE INDEX` on a view | `deny` |
/// | `drop-wrong-kind` | [`drop_wrong_kind`](Self::drop_wrong_kind) | `DROP TABLE` on a view or `DROP VIEW` on a table | `deny` |
/// | `builtin-modify` | [`builtin_modify`](Self::builtin_modify) | `DROP`, `ALTER`, `CREATE INDEX` or `CREATE TRIGGER` on a built-in table such as `sqlite_schema` | `deny` |
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
    pub(crate) qualified_ref: CheckLevel,
    pub(crate) index_on_view: CheckLevel,
    pub(crate) drop_wrong_kind: CheckLevel,
    pub(crate) builtin_modify: CheckLevel,
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
            qualified_ref: CheckLevel::Deny,
            index_on_view: CheckLevel::Deny,
            drop_wrong_kind: CheckLevel::Deny,
            builtin_modify: CheckLevel::Deny,
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
    /// Level for qualified names where `SQLite` forbids them.
    pub fn qualified_ref(&self) -> CheckLevel {
        self.qualified_ref
    }
    /// Level for `CREATE INDEX` on a view.
    pub fn index_on_view(&self) -> CheckLevel {
        self.index_on_view
    }
    /// Level for dropping a table with DROP VIEW or a view with DROP TABLE.
    pub fn drop_wrong_kind(&self) -> CheckLevel {
        self.drop_wrong_kind
    }
    /// Level for DDL that changes a built-in table.
    pub fn builtin_modify(&self) -> CheckLevel {
        self.builtin_modify
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
    /// Set the level for qualified names where `SQLite` forbids them.
    #[must_use]
    pub fn with_qualified_ref(mut self, level: CheckLevel) -> Self {
        self.qualified_ref = level;
        self
    }
    /// Set the level for `CREATE INDEX` on a view.
    #[must_use]
    pub fn with_index_on_view(mut self, level: CheckLevel) -> Self {
        self.index_on_view = level;
        self
    }
    /// Set the level for dropping a table with DROP VIEW or a view with DROP TABLE.
    #[must_use]
    pub fn with_drop_wrong_kind(mut self, level: CheckLevel) -> Self {
        self.drop_wrong_kind = level;
        self
    }
    /// Set the level for DDL that changes a built-in table.
    #[must_use]
    pub fn with_builtin_modify(mut self, level: CheckLevel) -> Self {
        self.builtin_modify = level;
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
        self.qualified_ref = level;
        self.index_on_view = level;
        self.drop_wrong_kind = level;
        self.builtin_modify = level;
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
        "qualified-ref",
        "index-on-view",
        "drop-wrong-kind",
        "builtin-modify",
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
            "qualified-ref" => self.qualified_ref = level,
            "index-on-view" => self.index_on_view = level,
            "drop-wrong-kind" => self.drop_wrong_kind = level,
            "builtin-modify" => self.builtin_modify = level,
            "schema" => self = self.with_schema(level),
            "all" => self = self.with_all(level),
            _ => return Err(format!("unknown check category: {name}")),
        }
        Ok(self)
    }
}

/// Configuration for analysis.
///
/// Controls how the [`Analyzer`] reports diagnostics and
/// generates "did you mean?" suggestions:
///
/// - **`checks`** — per-category severity levels (`allow`/`warn`/`deny`).
///   See [`CheckConfig`].
/// - **`suggestion_threshold`** (`2` by default) — maximum Levenshtein
///   distance for "did you mean?" suggestions. Set to `0` to disable
///   suggestions entirely.
///
/// [`Analyzer`]: super::Analyzer
///
/// # Example
///
/// ```
/// # use syntaqlite::analysis::{CheckLevel, Severity};
/// # use syntaqlite::AnalysisConfig;
/// let config = AnalysisConfig::default();
/// assert_eq!(config.suggestion_threshold(), 2);
///
/// // Deny all schema checks (errors instead of warnings).
/// let checks = config.checks().with_schema(CheckLevel::Deny);
/// let strict = config.with_checks(checks);
/// ```
#[derive(Clone, Copy)]
pub struct AnalysisConfig {
    suggestion_threshold: usize,
    checks: CheckConfig,
}

impl Default for AnalysisConfig {
    fn default() -> Self {
        AnalysisConfig {
            suggestion_threshold: 2,
            checks: CheckConfig::default(),
        }
    }
}

impl AnalysisConfig {
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
mod tests {
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
        assert_eq!(c.qualified_ref(), CheckLevel::Deny);
        assert_eq!(c.index_on_view(), CheckLevel::Deny);
        assert_eq!(c.drop_wrong_kind(), CheckLevel::Deny);
        assert_eq!(c.builtin_modify(), CheckLevel::Deny);
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
        let config = AnalysisConfig::default().with_strict_schema();
        assert_eq!(config.checks().unknown_table(), CheckLevel::Deny);
        assert_eq!(config.checks().unknown_column(), CheckLevel::Deny);
        assert_eq!(config.checks().unknown_function(), CheckLevel::Deny);
        assert_eq!(config.checks().function_arity(), CheckLevel::Deny);
        // Non-schema checks unchanged.
        assert_eq!(config.checks().cte_columns(), CheckLevel::Deny);
        assert_eq!(config.checks().parse_errors(), CheckLevel::Deny);
    }
}
