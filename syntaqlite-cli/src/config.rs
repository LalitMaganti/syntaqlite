// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Project configuration file (`syntaqlite.toml`) discovery, parsing, and merging.

use std::path::{Path, PathBuf};

use indexmap::IndexMap;
use serde::Deserialize;

/// Top-level project configuration from `syntaqlite.toml`.
#[derive(Debug, Default, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub(crate) struct ProjectConfig {
    /// Default schema for files not matching any glob in `[schemas]`.
    pub schema: Option<Vec<String>>,

    /// Glob → schema file mapping. Order is preserved (first match wins).
    #[serde(default)]
    pub schemas: IndexMap<String, Vec<String>>,

    /// `SQLite` version to emulate (e.g. "3.47.0", "latest").
    pub sqlite_version: Option<String>,

    /// `SQLite` compile-time flags to enable.
    #[serde(default)]
    pub sqlite_cflags: Vec<String>,

    /// Formatting options.
    #[serde(default)]
    pub format: FormatOptions,

    /// Per-category check toggles.
    #[serde(default)]
    pub checks: CheckOptions,
}

/// Per-category check levels from the `[checks]` section.
/// Values are strings: `"allow"`, `"warn"`, or `"deny"`.
#[derive(Debug, Default, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub(crate) struct CheckOptions {
    pub parse_errors: Option<String>,
    pub unknown_table: Option<String>,
    pub unknown_column: Option<String>,
    pub unknown_function: Option<String>,
    pub function_arity: Option<String>,
    pub cte_columns: Option<String>,
    /// Shorthand: sets all schema checks.
    pub schema: Option<String>,
    /// Shorthand: sets all checks.
    pub all: Option<String>,
}

/// Formatting options from the `[format]` section.
#[derive(Debug, Default, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub(crate) struct FormatOptions {
    pub line_width: Option<usize>,
    pub indent_width: Option<usize>,
    pub keyword_case: Option<String>,
    pub semicolons: Option<bool>,
}

/// Load config from an explicit file path.
/// Returns `(config, directory containing the config file)`.
pub(crate) fn load(config_path: &Path) -> Option<(ProjectConfig, PathBuf)> {
    let contents = match std::fs::read_to_string(config_path) {
        Ok(c) => c,
        Err(e) => {
            eprintln!("warning: failed to read {}: {e}", config_path.display());
            return None;
        }
    };
    let config: ProjectConfig = match toml::from_str(&contents) {
        Ok(c) => c,
        Err(e) => {
            eprintln!("warning: failed to parse {}: {e}", config_path.display());
            return None;
        }
    };
    let dir = config_path.parent()?.to_path_buf();
    Some((config, dir))
}

/// Walk up from `start` looking for `syntaqlite.toml`.
/// Returns `(config, directory containing the config file)`.
pub(crate) fn discover(start: &Path) -> Option<(ProjectConfig, PathBuf)> {
    let mut dir = start.to_path_buf();
    loop {
        let candidate = dir.join("syntaqlite.toml");
        if candidate.is_file() {
            return load(&candidate);
        }
        dir = dir.parent()?.to_path_buf();
    }
}

/// How the project config file should be resolved for a given invocation.
pub(crate) enum ConfigMode<'a> {
    /// Use an explicit config file path (`--config`).
    Explicit(&'a str),
    /// Auto-discover by walking up from the current directory (default).
    Discover,
    /// Disable config file loading entirely (`--no-config`).
    Disabled,
}

/// Resolve the config for a given mode — explicit path, walk up from CWD, or skip.
pub(crate) fn resolve(mode: &ConfigMode<'_>) -> Option<(ProjectConfig, PathBuf)> {
    match mode {
        ConfigMode::Explicit(p) => load(Path::new(p)),
        ConfigMode::Discover => {
            let cwd = std::env::current_dir().unwrap_or_default();
            discover(&cwd)
        }
        ConfigMode::Disabled => None,
    }
}

/// Given a SQL file path and a config, resolve which schema files apply.
pub(crate) fn resolve_schemas(
    sql_path: &Path,
    config: &ProjectConfig,
    config_dir: &Path,
) -> Vec<PathBuf> {
    let relative = sql_path.strip_prefix(config_dir).unwrap_or(sql_path);
    let relative_str = relative.to_string_lossy();

    // Check [schemas] globs in order (first match wins).
    for (glob_pattern, schema_files) in &config.schemas {
        if glob_match(glob_pattern, &relative_str) {
            return schema_files.iter().map(|s| config_dir.join(s)).collect();
        }
    }

    // Fall back to top-level `schema` key.
    if let Some(schema) = &config.schema {
        return schema.iter().map(|s| config_dir.join(s)).collect();
    }

    vec![]
}

/// Simple glob matching using the `glob` crate's `Pattern`.
fn glob_match(pattern: &str, path: &str) -> bool {
    glob::Pattern::new(pattern)
        .map(|p| {
            p.matches_with(
                path,
                glob::MatchOptions {
                    case_sensitive: true,
                    require_literal_separator: true,
                    require_literal_leading_dot: false,
                },
            )
        })
        .unwrap_or(false)
}

#[cfg(test)]
#[expect(clippy::unwrap_used)]
mod tests {
    use std::fs;

    use super::*;

    #[test]
    fn discover_walks_up() {
        let dir = tempfile::tempdir().unwrap();
        let nested = dir.path().join("a").join("b").join("c");
        fs::create_dir_all(&nested).unwrap();

        // Place config at root.
        fs::write(
            dir.path().join("syntaqlite.toml"),
            r#"
schema = ["schema.sql"]

[format]
line-width = 120
"#,
        )
        .unwrap();

        let (config, config_dir) = discover(&nested).expect("should find config");
        assert_eq!(config_dir, dir.path());
        assert_eq!(config.schema.as_ref().unwrap(), &["schema.sql"]);
        assert_eq!(config.format.line_width, Some(120));
    }

    #[test]
    fn discover_returns_none_when_missing() {
        let dir = tempfile::tempdir().unwrap();
        assert!(discover(dir.path()).is_none());
    }

    #[test]
    fn discover_finds_nearest() {
        let dir = tempfile::tempdir().unwrap();
        let inner = dir.path().join("inner");
        fs::create_dir_all(&inner).unwrap();

        fs::write(
            dir.path().join("syntaqlite.toml"),
            "schema = [\"outer.sql\"]\n",
        )
        .unwrap();
        fs::write(inner.join("syntaqlite.toml"), "schema = [\"inner.sql\"]\n").unwrap();

        let (config, config_dir) = discover(&inner).expect("should find inner config");
        assert_eq!(config_dir, inner);
        assert_eq!(config.schema.as_ref().unwrap(), &["inner.sql"]);
    }

    #[test]
    fn resolve_schemas_glob_match() {
        let config: ProjectConfig = toml::from_str(
            r#"
[schemas]
"src/**/*.sql" = ["schema/main.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test.sql"]
"migrations/*.sql" = []
"#,
        )
        .unwrap();

        let dir = Path::new("/project");

        // Matches first glob.
        let schemas = resolve_schemas(Path::new("/project/src/queries/foo.sql"), &config, dir);
        assert_eq!(schemas, vec![PathBuf::from("/project/schema/main.sql")]);

        // Matches second glob.
        let schemas = resolve_schemas(Path::new("/project/tests/bar.sql"), &config, dir);
        assert_eq!(
            schemas,
            vec![
                PathBuf::from("/project/schema/main.sql"),
                PathBuf::from("/project/schema/test.sql"),
            ]
        );

        // Matches third glob (empty schemas).
        let schemas = resolve_schemas(Path::new("/project/migrations/001.sql"), &config, dir);
        assert!(schemas.is_empty());

        // No match, no fallback.
        let schemas = resolve_schemas(Path::new("/project/other/file.sql"), &config, dir);
        assert!(schemas.is_empty());
    }

    #[test]
    fn resolve_schemas_fallback() {
        let config: ProjectConfig = toml::from_str(
            r#"
schema = ["default.sql"]

[schemas]
"src/**/*.sql" = ["main.sql"]
"#,
        )
        .unwrap();

        let dir = Path::new("/project");

        // Matches glob.
        let schemas = resolve_schemas(Path::new("/project/src/foo.sql"), &config, dir);
        assert_eq!(schemas, vec![PathBuf::from("/project/main.sql")]);

        // Falls back to `schema`.
        let schemas = resolve_schemas(Path::new("/project/other/foo.sql"), &config, dir);
        assert_eq!(schemas, vec![PathBuf::from("/project/default.sql")]);
    }

    #[test]
    fn parse_full_config() {
        let config: ProjectConfig = toml::from_str(
            r#"
schema = ["schema.sql"]
sqlite-version = "3.47.0"
sqlite-cflags = ["SQLITE_ENABLE_MATH_FUNCTIONS", "SQLITE_ENABLE_FTS5"]

[schemas]
"src/**/*.sql" = ["schema/main.sql", "schema/views.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []

[format]
line-width = 100
indent-width = 4
keyword-case = "lower"
semicolons = false
"#,
        )
        .unwrap();

        assert_eq!(config.schema.as_ref().unwrap(), &["schema.sql"]);
        assert_eq!(config.sqlite_version.as_deref(), Some("3.47.0"));
        assert_eq!(
            config.sqlite_cflags,
            &["SQLITE_ENABLE_MATH_FUNCTIONS", "SQLITE_ENABLE_FTS5"]
        );
        assert_eq!(config.schemas.len(), 3);
        assert_eq!(config.format.line_width, Some(100));
        assert_eq!(config.format.indent_width, Some(4));
        assert_eq!(config.format.keyword_case.as_deref(), Some("lower"));
        assert_eq!(config.format.semicolons, Some(false));
    }

    #[test]
    fn parse_minimal_config() {
        let config: ProjectConfig = toml::from_str("").unwrap();
        assert!(config.schema.is_none());
        assert!(config.schemas.is_empty());
        assert!(config.sqlite_version.is_none());
        assert!(config.sqlite_cflags.is_empty());
        assert!(config.format.line_width.is_none());
    }

    #[test]
    fn parse_sqlite_version_only() {
        let config: ProjectConfig = toml::from_str("sqlite-version = \"latest\"\n").unwrap();
        assert_eq!(config.sqlite_version.as_deref(), Some("latest"));
        assert!(config.sqlite_cflags.is_empty());
    }

    #[test]
    fn parse_sqlite_cflags_only() {
        let config: ProjectConfig =
            toml::from_str("sqlite-cflags = [\"SQLITE_ENABLE_FTS5\"]\n").unwrap();
        assert!(config.sqlite_version.is_none());
        assert_eq!(config.sqlite_cflags, &["SQLITE_ENABLE_FTS5"]);
    }

    #[test]
    fn glob_match_patterns() {
        assert!(glob_match("**/*.sql", "src/foo.sql"));
        assert!(glob_match("src/**/*.sql", "src/a/b/c.sql"));
        assert!(!glob_match("src/**/*.sql", "tests/a.sql"));
        assert!(glob_match("migrations/*.sql", "migrations/001.sql"));
        assert!(!glob_match("migrations/*.sql", "migrations/a/001.sql"));
    }
}
