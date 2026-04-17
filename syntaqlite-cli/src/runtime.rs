// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Runtime SQL commands (ast, fmt, lsp, validate) that require the `syntaqlite` crate.

use std::fs;
use std::io::{self, IsTerminal, Read};
use std::ops::Deref;
use std::path::PathBuf;

use clap::ValueEnum;
use syntaqlite::any::AnyDialect;
use syntaqlite::any::{AnyParser, ParseOutcome};
use syntaqlite::fmt::FormatError;
use syntaqlite::fmt::KeywordCase;
use syntaqlite::semantic::DiagnosticMessage;
use syntaqlite::semantic::Severity;
use syntaqlite::util::DiagnosticRenderer;
use syntaqlite::{
    Catalog, Diagnostic, FormatConfig, Formatter, SemanticAnalyzer, ValidationConfig,
};

use crate::config::{self, FormatOptions, ProjectConfig};

use super::{Cli, Command, LineageOutput, LineageScope};

#[derive(Clone, Copy, ValueEnum)]
pub(crate) enum KeywordCasing {
    Upper,
    Lower,
}

#[derive(Clone, Copy, ValueEnum)]
pub(crate) enum HostLanguage {
    Python,
    Typescript,
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

fn require_dialect(dialect: Option<AnyDialect>) -> Result<AnyDialect, String> {
    dialect.ok_or_else(|| {
        "this command requires a dialect; build with --features=builtin-sqlite or use --dialect"
            .to_string()
    })
}

pub(crate) fn dispatch(cli: Cli, dialect: Option<AnyDialect>) -> Result<(), String> {
    let base = if let Some(path) = &cli.dialect_path {
        Some(
            AnyDialect::load(path, cli.dialect_name.as_deref()).unwrap_or_else(|e| {
                eprintln!("error: {e}");
                std::process::exit(1);
            }),
        )
    } else {
        dialect
    };

    let config_mode = if cli.no_config {
        ConfigMode::Disabled
    } else if let Some(ref path) = cli.config {
        ConfigMode::Explicit(path)
    } else {
        ConfigMode::Discover
    };

    // Discover config early so we can merge sqlite-version / sqlite-cflags.
    let project_config = discover_config_for_paths(&[], &config_mode);

    let configured = match base {
        Some(d) => {
            // CLI flags take precedence over config file values.
            let version = cli.sqlite_version.as_ref().or_else(|| {
                project_config
                    .as_ref()
                    .and_then(|(c, _)| c.sqlite_version.as_ref())
            });
            let cli_cflags = &cli.sqlite_cflag;
            let config_cflags;
            let cflags = if cli_cflags.is_empty() {
                config_cflags = project_config
                    .as_ref()
                    .map(|(c, _)| c.sqlite_cflags.as_slice())
                    .unwrap_or_default();
                config_cflags
            } else {
                cli_cflags.as_slice()
            };
            Some(apply_version_cflags(d, version, cflags)?)
        }
        None => None,
    };

    dispatch_commands(cli.command, configured, &config_mode)
}

fn apply_version_cflags(
    mut dialect: AnyDialect,
    version: Option<&String>,
    cflags: &[String],
) -> Result<AnyDialect, String> {
    use syntaqlite::util::{SqliteFlags, SqliteVersion};

    if let Some(v) = version {
        let ver = SqliteVersion::parse_with_latest(v)
            .map_err(|e| format!("invalid sqlite-version {v:?}: {e}"))?;
        dialect = dialect.with_version(ver);
    }

    if !cflags.is_empty() {
        let mut flags = SqliteFlags::default();
        for name in cflags {
            let flag = syntaqlite::util::SqliteFlag::from_name(name)
                .ok_or_else(|| format!("unknown sqlite-cflag: {name}"))?;
            flags = flags.with(flag);
        }
        dialect = dialect.with_cflags(flags);
    }

    Ok(dialect)
}

/// How the config file should be resolved.
enum ConfigMode<'a> {
    /// Use an explicit config file path (`--config`).
    Explicit(&'a str),
    /// Auto-discover by walking up from the current directory (default).
    Discover,
    /// Disable config file loading entirely (`--no-config`).
    Disabled,
}

fn dispatch_commands(
    command: Command,
    dialect: Option<AnyDialect>,
    config: &ConfigMode<'_>,
) -> Result<(), String> {
    match command {
        Command::Parse {
            files,
            expression,
            output,
        } => require_dialect(dialect)
            .and_then(|d| cmd_parse(&d, &files, expression.as_deref(), output)),
        Command::Validate {
            files,
            expression,
            schema,
            allow,
            warn,
            deny,
            lang,
        } => require_dialect(dialect).and_then(|d| {
            let file_config = discover_config_for_paths(&files, config);
            // If --schema is given, use it directly. Otherwise, resolve from config file.
            let resolved_schemas = if schema.is_empty() {
                resolve_schemas_from_config_with(&files, file_config.as_ref())
            } else {
                schema
            };
            let has_schema = !resolved_schemas.is_empty();
            let checks = build_check_config(
                has_schema,
                file_config.as_ref().map(|(c, _)| &c.checks),
                &allow,
                &warn,
                &deny,
            )?;
            cmd_validate(
                &d,
                &files,
                expression.as_deref(),
                &resolved_schemas,
                lang,
                checks,
            )
        }),
        Command::Lineage {
            files,
            expression,
            schema,
            output,
            scope,
        } => dispatch_lineage(
            dialect,
            config,
            &files,
            expression.as_deref(),
            schema,
            output,
            scope,
        ),
        Command::Lsp => require_dialect(dialect).and_then(|d| cmd_lsp(d, config)),
        #[cfg(feature = "mcp")]
        Command::Mcp => require_dialect(dialect).and_then(crate::mcp::cmd_mcp),
        Command::Fmt {
            files,
            expression,
            line_width,
            indent_width,
            keyword_case,
            in_place,
            check,
            semicolons,
            output,
        } => {
            let file_config = discover_config_for_paths(&files, config);
            let config = build_format_config(
                file_config.as_ref().map(|(c, _)| &c.format),
                line_width,
                indent_width,
                keyword_case,
                semicolons,
            );
            require_dialect(dialect).and_then(|d| {
                cmd_fmt(
                    &d,
                    &files,
                    expression.as_deref(),
                    &config,
                    in_place,
                    check,
                    output,
                )
            })
        }
        Command::Version => {
            println!("syntaqlite {}", env!("CARGO_PKG_VERSION"));
            Ok(())
        }
        Command::Dialect(args) => crate::codegen::dispatch_dialect(&args),
        Command::DialectTool(cmd) => crate::codegen::dispatch_tool(cmd),
    }
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

/// Resolve the SQL source: `-e` expression, files, or stdin (in that priority order).
///
/// The `on_inline` callback receives `(source, label)` where label is `"<expression>"` or
/// `"<stdin>"` depending on the input source.
fn process_files(
    files: &[String],
    expression: Option<&str>,
    on_inline: impl FnOnce(&str, &str) -> Result<(), String>,
    mut on_file: impl FnMut(&str, &PathBuf, bool) -> Result<(), String>,
) -> Result<(), String> {
    if let Some(expr) = expression {
        return on_inline(expr, "<expression>");
    }

    let paths = expand_paths(files)?;

    if paths.is_empty() {
        return on_inline(&read_stdin()?, "<stdin>");
    }

    let multi = paths.len() > 1;
    for path in &paths {
        let source = fs::read_to_string(path).map_err(|e| format!("{}: {e}", path.display()))?;
        on_file(&source, path, multi)?;
    }
    Ok(())
}

fn cmd_parse(
    dialect: &AnyDialect,
    files: &[String],
    expression: Option<&str>,
    output: crate::ParseOutput,
) -> Result<(), String> {
    let mut total_stmts = 0u64;
    let mut total_errors = 0u64;
    let mut json_nodes: Vec<serde_json::Value> = Vec::new();

    let mut process_source = |source: &str, file: &str| {
        let (s, e, nodes) = cmd_parse_source(dialect, source, file, output);
        total_stmts += s;
        total_errors += e;
        json_nodes.extend(nodes);
    };

    if let Some(expr) = expression {
        process_source(expr, "<expression>");
    } else {
        let paths = expand_paths(files)?;

        if paths.is_empty() {
            let source = read_stdin()?;
            process_source(&source, "<stdin>");
        } else {
            let multi = paths.len() > 1;
            for path in &paths {
                let source =
                    fs::read_to_string(path).map_err(|e| format!("{}: {e}", path.display()))?;
                let file = path.display().to_string();
                if multi && matches!(output, crate::ParseOutput::Text) {
                    println!("==> {file} <==");
                }
                process_source(&source, &file);
            }
        }
    }

    match output {
        crate::ParseOutput::Summary => {
            println!("{total_stmts} statements parsed, {total_errors} errors");
        }
        crate::ParseOutput::Json => {
            println!(
                "{}",
                serde_json::to_string_pretty(&json_nodes)
                    .map_err(|e| format!("JSON serialization failed: {e}"))?
            );
        }
        crate::ParseOutput::Text => {}
    }

    if total_errors > 0 {
        Err(format!("{total_errors} syntax error(s)"))
    } else {
        Ok(())
    }
}

/// Parse a single source. Returns `(stmt_count, error_count, json_nodes)`.
fn cmd_parse_source(
    dialect: &AnyDialect,
    source: &str,
    file: &str,
    output: crate::ParseOutput,
) -> (u64, u64, Vec<serde_json::Value>) {
    let parser = AnyParser::new(dialect.deref().clone());
    let mut session = parser.parse(source);
    let mut ast_out = String::new();
    let mut json_nodes: Vec<serde_json::Value> = Vec::new();
    let mut error_diags: Vec<Diagnostic> = Vec::new();
    let mut count = 0u64;

    loop {
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                match output {
                    crate::ParseOutput::Text => {
                        if count > 0 {
                            ast_out.push_str("----\n");
                        }
                        stmt.dump(&mut ast_out, 0);
                    }
                    crate::ParseOutput::Json => {
                        let val = stmt
                            .erase()
                            .root_node()
                            .map_or(serde_json::Value::Null, |n| {
                                serde_json::to_value(n).unwrap_or(serde_json::Value::Null)
                            });
                        json_nodes.push(val);
                    }
                    crate::ParseOutput::Summary => {}
                }
                count += 1;
            }
            ParseOutcome::Err(err) => {
                let start = err.offset().unwrap_or(0);
                let end = start + err.length().unwrap_or(0);
                error_diags.push(Diagnostic::new(
                    start,
                    end,
                    DiagnosticMessage::ParseError(err.message().to_string()),
                    Severity::Error,
                    None,
                ));
            }
            ParseOutcome::Done => break,
        }
    }

    if matches!(output, crate::ParseOutput::Text) {
        print!("{ast_out}");
    }

    let n_err = error_diags.len() as u64;
    if !error_diags.is_empty() {
        DiagnosticRenderer::new(source, file)
            .render_diagnostics(&error_diags, &mut io::stderr())
            .ok();
    }
    (count, n_err, json_nodes)
}

fn cmd_lsp(dialect: AnyDialect, config: &ConfigMode<'_>) -> Result<(), String> {
    let mut lsp_config = syntaqlite::lsp::LspConfig::default();

    let found = match config {
        ConfigMode::Explicit(p) => config::load(std::path::Path::new(p)),
        ConfigMode::Discover => {
            let cwd = std::env::current_dir().unwrap_or_default();
            config::discover(&cwd)
        }
        ConfigMode::Disabled => None,
    };

    if let Some((project_config, config_dir)) = found {
        eprintln!(
            "syntaqlite-lsp: found config at {}",
            config_dir.join("syntaqlite.toml").display()
        );

        // Apply format config.
        lsp_config.format_config = Some(build_format_config(
            Some(&project_config.format),
            None,
            None,
            None,
            None,
        ));

        // Build per-file schema map from [schemas] globs and top-level `schema` key.
        let default_catalog = if let Some(ref schema) = project_config.schema {
            let paths: Vec<String> = schema
                .iter()
                .map(|s| config_dir.join(s).to_string_lossy().into_owned())
                .collect();
            match build_schema_catalog(&dialect, &paths) {
                Ok(catalog) => Some(catalog),
                Err(e) => {
                    eprintln!("syntaqlite-lsp: failed to load default schema: {e}");
                    None
                }
            }
        } else {
            None
        };

        let mut schema_entries = Vec::new();
        for (glob_str, schema_files) in &project_config.schemas {
            let pattern = match glob::Pattern::new(glob_str) {
                Ok(p) => p,
                Err(e) => {
                    eprintln!("syntaqlite-lsp: bad glob pattern {glob_str:?}: {e}");
                    continue;
                }
            };
            let paths: Vec<String> = schema_files
                .iter()
                .map(|s| config_dir.join(s).to_string_lossy().into_owned())
                .collect();
            match build_schema_catalog(&dialect, &paths) {
                Ok(catalog) => schema_entries.push((pattern, catalog)),
                Err(e) => eprintln!("syntaqlite-lsp: failed to load schema for {glob_str:?}: {e}"),
            }
        }

        let has_schema = default_catalog.is_some() || !schema_entries.is_empty();
        if has_schema {
            lsp_config.schema_map = Some(syntaqlite::lsp::SchemaMap::new(
                config_dir.clone(),
                default_catalog,
                schema_entries,
            ));
        }

        // Apply check levels from config file.
        match build_check_config(has_schema, Some(&project_config.checks), &[], &[], &[]) {
            Ok(checks) => {
                lsp_config.validation_config =
                    Some(ValidationConfig::default().with_checks(checks));
            }
            Err(e) => eprintln!("syntaqlite-lsp: invalid [checks] config: {e}"),
        }
    }

    syntaqlite::lsp::LspServer::run_with_config(dialect, lsp_config)
        .map_err(|e| format!("LSP error: {e}"))
}

fn cmd_fmt(
    dialect: &AnyDialect,
    files: &[String],
    expression: Option<&str>,
    config: &FormatConfig,
    in_place: bool,
    check: bool,
    output: crate::FmtOutput,
) -> Result<(), String> {
    // Debug output modes bypass normal formatting.
    if !matches!(output, crate::FmtOutput::Formatted) {
        return cmd_fmt_debug(dialect, files, expression, config, output);
    }

    let mut errors = Vec::new();
    let mut unformatted = Vec::new();
    process_files(
        files,
        expression,
        |source, label| {
            if in_place || check {
                return Err(format!(
                    "--{} requires file arguments",
                    if check { "check" } else { "in-place" }
                ));
            }
            let out = format_source(dialect, source, config).map_err(|e| {
                render_format_error(&e, source, label);
                format!("{label}: {e}")
            })?;
            print!("{out}");
            Ok(())
        },
        |source, path, multi| {
            match format_source(dialect, source, config) {
                Ok(out) => {
                    if check {
                        if out != source {
                            unformatted.push(path.display().to_string());
                        }
                    } else if in_place {
                        if out != source {
                            fs::write(path, &out)
                                .map_err(|e| format!("{}: {e}", path.display()))?;
                            eprintln!("formatted {}", path.display());
                        }
                    } else {
                        if multi {
                            println!("==> {} <==", path.display());
                        }
                        print!("{out}");
                    }
                }
                Err(e) => {
                    let label = path.display().to_string();
                    render_format_error(&e, source, &label);
                    errors.push(format!("{label}: {e}"));
                }
            }
            Ok(())
        },
    )?;

    if !errors.is_empty() {
        return Err(errors.join("\n"));
    }
    if !unformatted.is_empty() {
        for f in &unformatted {
            eprintln!("would reformat {f}");
        }
        return Err(format!(
            "{} file(s) would be reformatted",
            unformatted.len()
        ));
    }
    Ok(())
}

fn cmd_fmt_debug(
    dialect: &AnyDialect,
    files: &[String],
    expression: Option<&str>,
    config: &FormatConfig,
    output: crate::FmtOutput,
) -> Result<(), String> {
    let mut formatter = Formatter::with_dialect_config(dialect.clone(), config);

    let dump = |formatter: &mut Formatter, source: &str| -> Result<String, String> {
        match output {
            crate::FmtOutput::Bytecode => {
                formatter.dump_bytecode(source).map_err(|e| e.to_string())
            }
            crate::FmtOutput::DocTree => formatter.dump_doc_tree(source).map_err(|e| e.to_string()),
            crate::FmtOutput::Formatted => unreachable!(),
        }
    };

    if let Some(expr) = expression {
        print!("{}", dump(&mut formatter, expr)?);
        return Ok(());
    }

    let paths = expand_paths(files)?;
    if paths.is_empty() {
        let source = read_stdin()?;
        print!("{}", dump(&mut formatter, &source)?);
        return Ok(());
    }

    let multi = paths.len() > 1;
    for path in &paths {
        let source = fs::read_to_string(path).map_err(|e| format!("{}: {e}", path.display()))?;
        if multi {
            println!("==> {} <==", path.display());
        }
        print!("{}", dump(&mut formatter, &source)?);
    }
    Ok(())
}

fn render_format_error(e: &FormatError, source: &str, file: &str) {
    let start = e.offset().unwrap_or(0);
    let end = start + e.length().unwrap_or(0);
    let diag = Diagnostic::new(
        start,
        end,
        DiagnosticMessage::ParseError(e.message().to_owned()),
        Severity::Error,
        None,
    );
    DiagnosticRenderer::new(source, file)
        .render_diagnostic(&diag, &mut io::stderr())
        .ok();
}

fn format_source(
    dialect: &AnyDialect,
    source: &str,
    config: &FormatConfig,
) -> Result<String, FormatError> {
    Formatter::with_dialect_config(dialect.clone(), config).format(source)
}

fn build_schema_catalog(dialect: &AnyDialect, schema_files: &[String]) -> Result<Catalog, String> {
    if schema_files.is_empty() {
        return Ok(Catalog::new(dialect.clone()));
    }
    let paths = expand_paths(schema_files)?;
    let mut sources = Vec::new();
    let mut uris = Vec::new();
    for path in &paths {
        let source =
            fs::read_to_string(path).map_err(|e| format!("schema {}: {e}", path.display()))?;
        uris.push(format!("file://{}", path.display()));
        sources.push(source);
    }
    let pairs: Vec<(&str, Option<&str>)> = sources
        .iter()
        .zip(uris.iter())
        .map(|(s, u)| (s.as_str(), Some(u.as_str())))
        .collect();
    let (catalog, errors) = Catalog::from_ddl(dialect.clone(), &pairs);
    for err in &errors {
        eprintln!("warning: schema: {err}");
    }
    Ok(catalog)
}

fn cmd_validate(
    dialect: &AnyDialect,
    files: &[String],
    expression: Option<&str>,
    schema_files: &[String],
    lang: Option<HostLanguage>,
    checks: syntaqlite::CheckConfig,
) -> Result<(), String> {
    let has_schema = !schema_files.is_empty();
    let schema_catalog = build_schema_catalog(dialect, schema_files)?;
    let config = ValidationConfig::default().with_checks(checks);
    let mut any_errors = false;
    let mut any_diagnostics = false;

    process_files(
        files,
        expression,
        |source, label| {
            let (errors, _diags) = match lang {
                Some(lang) => {
                    let e = validate_embedded_source(
                        dialect,
                        source,
                        label,
                        &config,
                        lang,
                        schema_files,
                    );
                    (e, e)
                }
                None => validate_source(dialect, source, label, &config, &schema_catalog),
            };
            if errors {
                std::process::exit(1);
            }
            Ok(())
        },
        |source, path, multi| {
            let file = path.display().to_string();
            if multi {
                println!("==> {file} <==");
            }
            let (errors, diags) = match lang {
                Some(lang) => {
                    let e = validate_embedded_source(
                        dialect,
                        source,
                        &file,
                        &config,
                        lang,
                        schema_files,
                    );
                    (e, e)
                }
                None => validate_source(dialect, source, &file, &config, &schema_catalog),
            };
            if errors {
                any_errors = true;
            }
            if diags {
                any_diagnostics = true;
            }
            Ok(())
        },
    )?;

    if any_diagnostics && !has_schema {
        emit_no_schema_hint();
    }

    if any_errors {
        std::process::exit(1);
    }
    Ok(())
}

/// Returns `(has_errors, has_any_diagnostics)`.
fn validate_source(
    dialect: &AnyDialect,
    source: &str,
    file: &str,
    config: &ValidationConfig,
    schema_catalog: &Catalog,
) -> (bool, bool) {
    let mut analyzer = SemanticAnalyzer::with_dialect(dialect.clone());
    let model = analyzer.analyze(source, schema_catalog, config);
    let any_diags = model.has_diagnostics();
    let all_diags: Vec<_> = model.diagnostics().cloned().collect();
    let has_errors = DiagnosticRenderer::new(source, file)
        .render_diagnostics(&all_diags, &mut io::stderr())
        .unwrap_or(false);
    (has_errors, any_diags)
}

fn validate_embedded_source(
    dialect: &AnyDialect,
    source: &str,
    file: &str,
    config: &ValidationConfig,
    lang: HostLanguage,
    schema_files: &[String],
) -> bool {
    let fragments = match lang {
        HostLanguage::Python => syntaqlite::embedded::extract_python(source),
        HostLanguage::Typescript => syntaqlite::embedded::extract_typescript(source),
    };
    if fragments.is_empty() {
        eprintln!("no SQL fragments found in {file}");
        return false;
    }

    // Build an owned catalog for the embedded analyzer.
    let catalog = match build_schema_catalog(dialect, schema_files) {
        Ok(c) => c,
        Err(e) => {
            eprintln!("error: {e}");
            return true;
        }
    };
    let diags = syntaqlite::embedded::EmbeddedAnalyzer::new(dialect.clone())
        .with_catalog(catalog)
        .with_config(*config)
        .validate(&fragments);

    DiagnosticRenderer::new(source, file)
        .render_diagnostics(&diags, &mut io::stderr())
        .unwrap_or(false)
}

fn emit_no_schema_hint() {
    eprintln!(
        "note: no schema provided; unresolved names are reported as warnings. \
         Add a `syntaqlite.toml` with `schema = [\"schema.sql\"]` or pass `--schema` \
         to treat them as errors."
    );
}

// ---------------------------------------------------------------------------
// Config file helpers
// ---------------------------------------------------------------------------

/// Discover `syntaqlite.toml` from an explicit path or by walking up from the current directory.
fn discover_config_for_paths(
    files: &[String],
    config: &ConfigMode<'_>,
) -> Option<(ProjectConfig, PathBuf)> {
    let _ = files; // config discovery is always cwd-based (or explicit --config)
    match config {
        ConfigMode::Explicit(p) => config::load(std::path::Path::new(p)),
        ConfigMode::Discover => {
            let cwd = std::env::current_dir().unwrap_or_default();
            config::discover(&cwd)
        }
        ConfigMode::Disabled => None,
    }
}

/// Build a `FormatConfig` by merging config file options with CLI overrides.
/// CLI args take precedence over config file values, which take precedence over defaults.
fn build_format_config(
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

/// Resolve schema files from an already-discovered config.
fn resolve_schemas_from_config_with(
    files: &[String],
    found: Option<&(ProjectConfig, PathBuf)>,
) -> Vec<String> {
    let Some((config, config_dir)) = found else {
        return vec![];
    };

    let Ok(paths) = expand_paths(files) else {
        return vec![];
    };

    if let Some(first) = paths.first() {
        let canonical = first.canonicalize().unwrap_or_else(|_| first.clone());
        let config_dir_canonical = config_dir
            .canonicalize()
            .unwrap_or_else(|_| config_dir.clone());
        config::resolve_schemas(&canonical, config, &config_dir_canonical)
            .into_iter()
            .map(|p| p.to_string_lossy().into_owned())
            .collect()
    } else {
        vec![]
    }
}

/// Build `CheckConfig` by merging defaults, schema presence, config file, and CLI flags.
/// Resolution order: defaults → `has_schema` → config file `[checks]` → CLI `-A`/`-W`/`-D`.
fn build_check_config(
    has_schema: bool,
    file_opts: Option<&config::CheckOptions>,
    cli_allow: &[String],
    cli_warn: &[String],
    cli_deny: &[String],
) -> Result<syntaqlite::CheckConfig, String> {
    use syntaqlite::semantic::CheckLevel;

    let mut checks = syntaqlite::CheckConfig::default();

    // When a schema is provided, default schema checks to deny (errors).
    if has_schema {
        checks = checks.with_schema(CheckLevel::Deny);
    }

    // Apply config file options.
    if let Some(opts) = file_opts {
        // Group shorthands first (so per-category overrides them).
        if let Some(ref v) = opts.all {
            checks = checks.with_all(CheckLevel::parse(v)?);
        }
        if let Some(ref v) = opts.schema {
            checks = checks.with_schema(CheckLevel::parse(v)?);
        }
        // Per-category overrides.
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

    // Apply CLI flags (last wins per category).
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

// ── Lineage command ─────────────────────────────────────────────────────────

use crate::lineage_output::{
    ErrorRecord, ErrorStage, JsonColumn, JsonOrigin, JsonPartialReason, JsonPhysicalTable,
    JsonRelation, JsonRelationKind, JsonTarget, JsonTargetKind, LineageRecord, SCHEMA_VERSION,
    Status,
};
use syntaqlite::semantic::{RelationKind, StatementModel};

fn dispatch_lineage(
    dialect: Option<AnyDialect>,
    config: &ConfigMode<'_>,
    files: &[String],
    expression: Option<&str>,
    schema: Vec<String>,
    output: LineageOutput,
    scope: Option<LineageScope>,
) -> Result<(), String> {
    require_dialect(dialect).and_then(|d| {
        let file_config = discover_config_for_paths(files, config);
        let resolved_schemas = if schema.is_empty() {
            resolve_schemas_from_config_with(files, file_config.as_ref())
        } else {
            schema
        };
        cmd_lineage(&d, files, expression, &resolved_schemas, output, scope)
    })
}

fn cmd_lineage(
    dialect: &AnyDialect,
    files: &[String],
    expression: Option<&str>,
    schemas: &[String],
    output: LineageOutput,
    scope: Option<LineageScope>,
) -> Result<(), String> {
    let schema_catalog = build_schema_catalog(dialect, schemas)?;
    let config = ValidationConfig::default();
    let any_errors = std::cell::Cell::new(false);

    process_files(
        files,
        expression,
        |source, label| {
            if lineage_process_source(
                dialect,
                &schema_catalog,
                &config,
                source,
                label,
                output,
                scope,
            ) {
                any_errors.set(true);
            }
            Ok(())
        },
        |source, path, _multi| {
            let file = path.display().to_string();
            if lineage_process_source(
                dialect,
                &schema_catalog,
                &config,
                source,
                &file,
                output,
                scope,
            ) {
                any_errors.set(true);
            }
            Ok(())
        },
    )?;

    if any_errors.get() {
        std::process::exit(1);
    }
    Ok(())
}

fn lineage_process_source(
    dialect: &AnyDialect,
    schema_catalog: &Catalog,
    config: &ValidationConfig,
    source: &str,
    label: &str,
    output: LineageOutput,
    scope: Option<LineageScope>,
) -> bool {
    let mut analyzer = SemanticAnalyzer::with_dialect(dialect.clone());
    let model = analyzer.analyze(source, schema_catalog, config);
    let mut had_error = false;
    for (idx, stmt) in model.statements().iter().enumerate() {
        let idx = u32::try_from(idx).unwrap_or(u32::MAX);
        let errors: Vec<_> = stmt
            .diagnostics()
            .iter()
            .filter(|d| d.severity() == Severity::Error)
            .collect();
        if errors.is_empty() {
            emit_lineage(stmt, label, idx, output, scope);
        } else {
            for d in &errors {
                emit_error(d, label, idx, output);
            }
            had_error = true;
        }
    }
    had_error
}

fn emit_lineage(
    stmt: &StatementModel,
    file: &str,
    index: u32,
    output: LineageOutput,
    scope: Option<LineageScope>,
) {
    let (status, partial_reasons) = if stmt.unexpanded_views().is_empty() {
        (Status::Complete, Vec::new())
    } else {
        let reasons = stmt
            .unexpanded_views()
            .iter()
            .map(|v| JsonPartialReason::UnexpandedView { view: v.clone() })
            .collect();
        (Status::Partial, reasons)
    };

    let target = stmt.defined_relations().first().map(|d| JsonTarget {
        name: d.name.clone(),
        kind: if d.is_view {
            JsonTargetKind::View
        } else {
            JsonTargetKind::Table
        },
    });

    let include_columns = matches!(scope, None | Some(LineageScope::Columns));
    let include_tables = matches!(scope, None | Some(LineageScope::Tables));

    let columns = if include_columns {
        Some(
            stmt.lineage()
                .map(|l| {
                    l.into_inner()
                        .iter()
                        .map(|c| JsonColumn {
                            name: c.name.clone(),
                            index: c.index,
                            origin: c.origin.as_ref().map(|o| JsonOrigin {
                                table: o.table.clone(),
                                column: o.column.clone(),
                            }),
                        })
                        .collect()
                })
                .unwrap_or_default(),
        )
    } else {
        None
    };

    let relations = if include_tables {
        Some(
            stmt.relations_accessed()
                .map(|r| {
                    r.into_inner()
                        .iter()
                        .map(|r| JsonRelation {
                            name: r.name.clone(),
                            kind: match r.kind {
                                RelationKind::Table => JsonRelationKind::Table,
                                RelationKind::View => JsonRelationKind::View,
                            },
                        })
                        .collect()
                })
                .unwrap_or_default(),
        )
    } else {
        None
    };

    let physical_tables = if include_tables {
        Some(
            stmt.physical_tables_accessed()
                .map(|t| {
                    t.into_inner()
                        .iter()
                        .map(|t| JsonPhysicalTable {
                            name: t.name.clone(),
                        })
                        .collect()
                })
                .unwrap_or_default(),
        )
    } else {
        None
    };

    let record = LineageRecord {
        kind: "lineage",
        schema_version: SCHEMA_VERSION,
        file: file.to_string(),
        statement_index: index,
        status,
        partial_reasons,
        target,
        columns,
        relations,
        physical_tables,
    };

    match output {
        LineageOutput::Json => match serde_json::to_string(&record) {
            Ok(s) => println!("{s}"),
            Err(e) => eprintln!("error serializing lineage record: {e}"),
        },
        LineageOutput::Text => print_lineage_text(&record),
    }
}

fn emit_error(d: &Diagnostic, file: &str, index: u32, output: LineageOutput) {
    let stage = match d.message() {
        DiagnosticMessage::ParseError(_) => ErrorStage::Parse,
        _ => ErrorStage::Validate,
    };
    let record = ErrorRecord {
        kind: "error",
        schema_version: SCHEMA_VERSION,
        file: file.to_string(),
        statement_index: index,
        stage,
        message: format!("{}", d.message()),
    };

    match output {
        LineageOutput::Json => match serde_json::to_string(&record) {
            Ok(s) => println!("{s}"),
            Err(e) => eprintln!("error serializing error record: {e}"),
        },
        LineageOutput::Text => print_error_text(&record),
    }
}

fn print_error_text(record: &ErrorRecord) {
    let stage = match record.stage {
        ErrorStage::Parse => "parse",
        ErrorStage::Validate => "validate",
    };
    println!("Error");
    println!("  statement: {}", record.statement_index);
    println!("  stage: {stage}");
    println!("  message: {}", record.message);
}

fn print_lineage_text(record: &LineageRecord) {
    let status = match record.status {
        Status::Complete => "complete",
        Status::Partial => "partial",
    };
    println!("Lineage");
    println!("  statement: {}", record.statement_index);
    println!("  status: {status}");

    match &record.target {
        Some(t) => {
            let kind = match t.kind {
                JsonTargetKind::Table => "table",
                JsonTargetKind::View => "view",
            };
            println!("  target: {} ({kind})", t.name);
        }
        None => println!("  target: (none)"),
    }

    if let Some(columns) = &record.columns {
        if columns.is_empty() {
            println!("  columns: (none)");
        } else {
            println!("  columns:");
            for c in columns {
                match &c.origin {
                    Some(o) => println!("    {} <- {}.{}", c.name, o.table, o.column),
                    None => println!("    {} <- (transformed)", c.name),
                }
            }
        }
    }

    if let Some(relations) = &record.relations {
        if relations.is_empty() {
            println!("  relations: (none)");
        } else {
            println!("  relations:");
            for r in relations {
                let kind = match r.kind {
                    JsonRelationKind::Table => "table",
                    JsonRelationKind::View => "view",
                };
                println!("    {} ({kind})", r.name);
            }
        }
    }

    if let Some(physical_tables) = &record.physical_tables {
        if physical_tables.is_empty() {
            println!("  physical_tables: (none)");
        } else {
            println!("  physical_tables:");
            for t in physical_tables {
                println!("    {}", t.name);
            }
        }
    }

    if record.partial_reasons.is_empty() {
        println!("  partial_reasons: (none)");
    } else {
        println!("  partial_reasons:");
        for reason in &record.partial_reasons {
            match reason {
                JsonPartialReason::UnexpandedView { view } => {
                    println!("    unexpanded_view: {view}");
                }
            }
        }
    }
}
