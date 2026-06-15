// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Helpers shared across command modules — input loading, schema catalog
//! construction, and CLI/config merge.

use std::fs;
use std::io::{self, IsTerminal, Read};
use std::path::PathBuf;

use syntaqlite::any::AnyDialect;
use syntaqlite::fmt::KeywordCase;
use syntaqlite::{Catalog, CheckConfig, FormatConfig};

use crate::cli::{HostLanguage, KeywordCasing};
use crate::config::{self, CheckOptions, FormatOptions, ProjectConfig};

// ── Input sources ──────────────────────────────────────────────────────────

/// One resolved SQL input: an inline string (stdin / `-e`) or a file.
pub(crate) struct Source {
    pub(crate) text: String,
    /// Present only when the text came from a file on disk. Commands that
    /// need to write back (e.g. `fmt --in-place`) use this.
    pub(crate) path: Option<PathBuf>,
    /// Display label — the path for files, `<stdin>` or `<expression>` otherwise.
    pub(crate) label: String,
}

impl Source {
    pub(crate) fn is_file(&self) -> bool {
        self.path.is_some()
    }
}

/// Resolve the host language for a source. The explicit `--experimental-lang`
/// flag wins; otherwise the content is auto-detected (today: sqlite3 shell
/// scripts). `None` means standalone SQL — the command's plain path.
pub(crate) fn resolve_language(
    explicit: Option<HostLanguage>,
    src: &Source,
    dialect: &AnyDialect,
) -> Option<syntaqlite::embedded::HostLanguage> {
    if let Some(lang) = explicit {
        return Some(lang.into());
    }
    let hint = src
        .path
        .as_deref()
        .and_then(|p| p.extension())
        .and_then(|e| e.to_str());
    syntaqlite::embedded::detect(dialect.clone(), &src.text, hint)
}

/// Resolve the SQL input: `-e` expression, files, or stdin (in that priority order).
pub(crate) fn load_sources(
    files: &[String],
    expression: Option<&str>,
) -> Result<Vec<Source>, String> {
    if let Some(expr) = expression {
        return Ok(vec![Source {
            text: expr.to_owned(),
            path: None,
            label: "<expression>".to_owned(),
        }]);
    }

    let paths = expand_paths(files)?;

    if paths.is_empty() {
        return Ok(vec![Source {
            text: read_stdin()?,
            path: None,
            label: "<stdin>".to_owned(),
        }]);
    }

    paths
        .into_iter()
        .map(|path| {
            let text = fs::read_to_string(&path).map_err(|e| format!("{}: {e}", path.display()))?;
            let label = path.display().to_string();
            Ok(Source {
                text,
                path: Some(path),
                label,
            })
        })
        .collect()
}

/// Expand a list of file paths / glob patterns into concrete paths.
/// Returns an empty vec when the input is empty (meaning: read stdin).
fn expand_paths(patterns: &[String]) -> Result<Vec<PathBuf>, String> {
    let mut out = Vec::new();
    for pat in patterns {
        let matches: Vec<_> = glob::glob(pat)
            .map_err(|e| format!("bad glob pattern {pat:?}: {e}"))?
            .collect();
        if matches.is_empty() {
            return Err(format!("no files matched: {pat}"));
        }
        for entry in matches {
            let path = entry.map_err(|e| format!("glob error: {e}"))?;
            if path.is_file() {
                out.push(path);
            }
        }
    }
    Ok(out)
}

fn read_stdin() -> Result<String, String> {
    if io::stdin().is_terminal() {
        eprintln!("reading from stdin; paste SQL then press Ctrl-D (or pass files as arguments)");
    }
    let mut buf = String::new();
    io::stdin()
        .read_to_string(&mut buf)
        .map_err(|e| format!("reading stdin: {e}"))?;
    Ok(buf)
}

// ── Schema catalog ─────────────────────────────────────────────────────────

/// Build a [`Catalog`] by parsing the given DDL files. Returns an empty
/// catalog when `schema_files` is empty.
pub(crate) fn build_schema_catalog(
    dialect: &AnyDialect,
    schema_files: &[String],
) -> Result<Catalog, String> {
    if schema_files.is_empty() {
        return Ok(Catalog::new(dialect.clone()));
    }
    let paths = expand_paths(schema_files)?;
    let mut sources = Vec::new();
    for path in &paths {
        let source =
            fs::read_to_string(path).map_err(|e| format!("schema {}: {e}", path.display()))?;
        sources.push(source);
    }
    let source_refs: Vec<&str> = sources.iter().map(String::as_str).collect();
    let (catalog, errors) = Catalog::from_ddl(dialect.clone(), &source_refs);
    for err in &errors {
        eprintln!("warning: schema: {err}");
    }
    Ok(catalog)
}

/// Resolve schema files for a given set of input files using the discovered
/// project config. Returns an empty vec when no config or no match.
pub(crate) fn schemas_from_project_config(
    files: &[String],
    found: Option<&(ProjectConfig, PathBuf)>,
) -> Vec<String> {
    let Some((config, config_dir)) = found else {
        return vec![];
    };

    let Ok(paths) = expand_paths(files) else {
        return vec![];
    };

    let Some(first) = paths.first() else {
        return vec![];
    };

    let canonical = first.canonicalize().unwrap_or_else(|_| first.clone());
    let config_dir_canonical = config_dir
        .canonicalize()
        .unwrap_or_else(|_| config_dir.clone());
    config::resolve_schemas(&canonical, config, &config_dir_canonical)
        .into_iter()
        .map(|p| p.to_string_lossy().into_owned())
        .collect()
}

// ── Format / check config merging ─────────────────────────────────────────

/// Build a `FormatConfig` by merging config file options with CLI overrides.
/// Resolution order: defaults → config file → CLI flags (last wins).
pub(crate) fn build_format_config(
    file_opts: Option<&FormatOptions>,
    cli_line_width: Option<usize>,
    cli_indent_width: Option<usize>,
    cli_keyword_case: Option<KeywordCasing>,
    cli_semicolons: Option<bool>,
) -> FormatConfig {
    let file_opts_default = FormatOptions::default();
    let file_opts = file_opts.unwrap_or(&file_opts_default);

    let line_width = cli_line_width.or(file_opts.line_width).unwrap_or(80);
    let indent_width = cli_indent_width.or(file_opts.indent_width).unwrap_or(2);
    let keyword_case = cli_keyword_case
        .map(|k| match k {
            KeywordCasing::Upper => KeywordCase::Upper,
            KeywordCasing::Lower => KeywordCase::Lower,
        })
        .or_else(|| {
            file_opts.keyword_case.as_deref().map(|s| match s {
                "lower" => KeywordCase::Lower,
                _ => KeywordCase::Upper,
            })
        })
        .unwrap_or(KeywordCase::Upper);
    let semicolons = cli_semicolons.or(file_opts.semicolons).unwrap_or(true);

    FormatConfig::default()
        .with_line_width(line_width)
        .with_indent_width(indent_width)
        .with_keyword_case(keyword_case)
        .with_semicolons(semicolons)
}

/// Build `CheckConfig` by merging defaults, schema presence, config file, and CLI flags.
/// Resolution order: defaults → `has_schema` → config file `[checks]` → CLI `-A`/`-W`/`-D`.
pub(crate) fn build_check_config(
    has_schema: bool,
    file_opts: Option<&CheckOptions>,
    cli_allow: &[String],
    cli_warn: &[String],
    cli_deny: &[String],
) -> Result<CheckConfig, String> {
    use syntaqlite::analysis::CheckLevel;

    let mut checks = CheckConfig::default();

    // When a schema is provided, default schema checks to deny (errors).
    if has_schema {
        checks = checks.with_schema(CheckLevel::Deny);
    }

    if let Some(opts) = file_opts {
        // Group shorthands first (per-category overrides them).
        if let Some(ref v) = opts.all {
            checks = checks.with_all(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.schema {
            checks = checks.with_schema(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.parse_errors {
            checks = checks.with_parse_errors(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.unknown_table {
            checks = checks.with_unknown_table(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.unknown_column {
            checks = checks.with_unknown_column(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.unknown_function {
            checks = checks.with_unknown_function(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.function_arity {
            checks = checks.with_function_arity(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.cte_columns {
            checks = checks.with_cte_columns(CheckLevel::parse(v)?);
        }
    }

    // CLI flags win (last-spec-wins per category).
    for name in cli_allow {
        checks = checks.set_by_name(name, CheckLevel::Allow)?;
    }
    for name in cli_warn {
        checks = checks.set_by_name(name, CheckLevel::Warn)?;
    }
    for name in cli_deny {
        checks = checks.set_by_name(name, CheckLevel::Deny)?;
    }

    Ok(checks)
}
