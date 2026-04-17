// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Library entry point for the `syntaqlite` CLI.
//!
//! Downstream crates can build their own CLI binary that pre-specifies a
//! dialect by implementing [`CliApp`] and calling [`run`]. The default
//! `syntaqlite` binary in this crate is a thin wrapper that does exactly that
//! with the bundled SQLite dialect.

use clap::{CommandFactory, Parser, Subcommand, ValueEnum};
use syntaqlite::any::AnyDialect;

mod config;
mod runtime;

#[cfg(feature = "codegen")]
mod codegen;

mod lineage_output;

#[cfg(feature = "mcp")]
#[expect(
    clippy::needless_pass_by_value,
    reason = "rmcp #[tool(aggr)] requires by-value params"
)]
mod mcp;

/// Configuration trait for a `syntaqlite` CLI binary.
///
/// Implementors describe the program name, default dialect, and which dialect
/// override surfaces are exposed. Default methods produce the stock CLI's
/// behaviour minus the dialect itself; downstream wrappers typically override
/// [`CliApp::name`] and [`CliApp::default_dialect`].
///
/// All methods have defaults so adding new ones is non-breaking.
pub trait CliApp {
    /// Program name shown in `--help` and used as `argv[0]`.
    fn name(&self) -> &str;

    /// One-line description for `--help`.
    fn about(&self) -> &str {
        "SQL formatting and analysis tools"
    }

    /// Version string for `--version`.
    ///
    /// The default returns this crate's version. Wrapper crates should
    /// override to report their own version.
    fn version(&self) -> &str {
        env!("CARGO_PKG_VERSION")
    }

    /// Dialect baked into this binary. `None` means the user must supply one
    /// via `--dialect` (which requires [`CliApp::allow_dialect_override`]).
    fn default_dialect(&self) -> Option<AnyDialect> {
        None
    }

    /// Whether `--dialect` / `--dialect-name` are exposed to end users.
    /// When the `dynload` feature is disabled the flags are absent
    /// regardless of this setting.
    fn allow_dialect_override(&self) -> bool {
        false
    }

    /// Whether `--sqlite-version` / `--sqlite-cflag` are exposed.
    /// Set to `true` only for SQLite-derived dialects where these knobs apply.
    fn allow_sqlite_tuning(&self) -> bool {
        false
    }
}

#[derive(Clone, Copy, ValueEnum)]
pub(crate) enum ParseOutput {
    /// Print statement/error counts (compact, for benchmarks) [maintainer]
    Summary,
    /// Print the AST as human-readable text
    Text,
    /// Print the AST as JSON
    Json,
}

#[derive(Clone, Copy, Default, ValueEnum)]
pub(crate) enum FmtOutput {
    /// Formatted SQL (default)
    #[default]
    Formatted,
    /// Dump raw interpreter bytecode for each statement [maintainer]
    Bytecode,
    /// Dump the Wadler-Lindig document tree after interpretation [maintainer]
    DocTree,
}

#[derive(Clone, Copy, Default, ValueEnum)]
pub(crate) enum LineageOutput {
    /// Newline-delimited JSON, one record per statement/error (default)
    #[default]
    Json,
    /// Human-readable text
    Text,
}

#[derive(Clone, Copy, Subcommand)]
pub(crate) enum LineageScope {
    /// Emit only relations and physical tables (drop columns)
    Tables,
    /// Emit only column lineage (drop relations and physical tables)
    Columns,
}

#[derive(Parser)]
#[command(about = "SQL formatting and analysis tools")]
pub(crate) struct Cli {
    /// Path to `syntaqlite.toml` config file.
    /// When omitted, discovered by walking up from the current directory.
    #[arg(short = 'c', long = "config", global = true)]
    pub(crate) config: Option<String>,

    /// Disable automatic config file discovery.
    #[arg(long = "no-config", global = true, conflicts_with = "config")]
    pub(crate) no_config: bool,

    /// Path to a shared library (.so/.dylib/.dll) providing a dialect.
    #[cfg(feature = "dynload")]
    #[arg(long = "dialect", global = true)]
    pub(crate) dialect_path: Option<String>,

    /// Dialect name for symbol lookup.
    /// When omitted, the loader resolves `syntaqlite_grammar`.
    /// With a name, it resolves `syntaqlite_<name>_grammar`.
    #[cfg(feature = "dynload")]
    #[arg(long, requires = "dialect_path", global = true)]
    pub(crate) dialect_name: Option<String>,

    /// `SQLite` version to emulate (e.g. "3.47.0", "latest").
    #[arg(long, global = true)]
    pub(crate) sqlite_version: Option<String>,

    /// Enable a `SQLite` compile-time flag (e.g. `SQLITE_ENABLE_ORDERED_SET_AGGREGATES`).
    /// Can be specified multiple times.
    #[arg(long, global = true)]
    pub(crate) sqlite_cflag: Vec<String>,

    #[command(subcommand)]
    pub(crate) command: Command,
}

#[derive(Subcommand)]
pub(crate) enum Command {
    /// Parse SQL and report results
    Parse {
        /// SQL files or glob patterns (reads stdin if omitted)
        files: Vec<String>,
        /// SQL expression to process directly (instead of files or stdin)
        #[arg(short = 'e', long = "expression", conflicts_with = "files")]
        expression: Option<String>,
        /// Output format
        #[arg(short, long, value_enum, default_value_t = ParseOutput::Text)]
        output: ParseOutput,
    },
    /// Format SQL
    Fmt {
        /// SQL files or glob patterns (reads stdin if omitted)
        files: Vec<String>,
        /// SQL expression to format directly (instead of files or stdin)
        #[arg(short = 'e', long = "expression", conflicts_with = "files")]
        expression: Option<String>,
        /// Maximum line width
        #[arg(short = 'w', long)]
        line_width: Option<usize>,
        /// Spaces per indentation level
        #[arg(short = 't', long)]
        indent_width: Option<usize>,
        /// Keyword casing
        #[arg(short = 'k', long, value_enum)]
        keyword_case: Option<runtime::KeywordCasing>,
        /// Write formatted output back to file(s) in place
        #[arg(short = 'i', long)]
        in_place: bool,
        /// Check if files are formatted (exit 1 if not)
        #[arg(long, conflicts_with = "in_place")]
        check: bool,
        /// Append semicolons after each statement
        #[arg(long)]
        semicolons: Option<bool>,
        /// Output mode (formatted, bytecode, doc-tree)
        #[arg(short, long, value_enum, default_value_t = FmtOutput::Formatted)]
        output: FmtOutput,
    },
    /// Validate SQL and report diagnostics
    Validate {
        /// SQL files or glob patterns (reads stdin if omitted)
        files: Vec<String>,
        /// SQL expression to validate directly (instead of files or stdin)
        #[arg(short = 'e', long = "expression", conflicts_with = "files")]
        expression: Option<String>,
        /// Schema DDL file(s) to load before validation (repeatable, supports globs)
        #[arg(long)]
        schema: Vec<String>,
        /// Allow (suppress) a check category (repeatable; use "schema" or "all" for groups)
        #[arg(short = 'A', long = "allow")]
        allow: Vec<String>,
        /// Warn on a check category (repeatable)
        #[arg(short = 'W', long = "warn")]
        warn: Vec<String>,
        /// Deny (error) a check category (repeatable)
        #[arg(short = 'D', long = "deny")]
        deny: Vec<String>,
        /// [experimental] Host language for embedded SQL extraction (python, typescript)
        #[arg(long = "experimental-lang")]
        lang: Option<runtime::HostLanguage>,
    },
    /// Extract column and table lineage from SQL
    Lineage {
        /// SQL files or glob patterns (reads stdin if omitted)
        files: Vec<String>,
        /// SQL expression to analyze directly (instead of files or stdin)
        #[arg(short = 'e', long = "expression", conflicts_with = "files")]
        expression: Option<String>,
        /// Schema DDL file(s) to load before analysis (repeatable, supports globs)
        #[arg(long)]
        schema: Vec<String>,
        /// Output format
        #[arg(short, long, value_enum, default_value_t = LineageOutput::Json)]
        output: LineageOutput,
        /// Restrict output to a subset (combined output is the default)
        #[command(subcommand)]
        scope: Option<LineageScope>,
    },
    /// Start the language server (stdio)
    Lsp,
    /// Start the MCP server (stdio)
    #[cfg(feature = "mcp")]
    Mcp,
    /// Generate dialect C sources and Rust bindings for external dialects.
    #[cfg(feature = "codegen")]
    Dialect(codegen::DialectArgs),
    /// Print version information
    Version,
    #[cfg(feature = "codegen")]
    #[command(flatten)]
    DialectTool(codegen::ToolCommand),
}

/// Build the [`clap::Command`] for the given app, applying runtime visibility
/// rules from the [`CliApp`] trait.
fn build_command<A: CliApp>(app: &A) -> clap::Command {
    // clap's `name`/`about`/`version` builders want `'static` strings.
    // Leaking is fine: these live for the lifetime of the process.
    let name: &'static str = Box::leak(app.name().to_owned().into_boxed_str());
    let about: &'static str = Box::leak(app.about().to_owned().into_boxed_str());
    let version: &'static str = Box::leak(app.version().to_owned().into_boxed_str());

    let cmd = Cli::command()
        .name(name)
        .bin_name(name)
        .about(about)
        .version(version);

    let cmd = if app.allow_sqlite_tuning() {
        cmd
    } else {
        cmd.mut_arg("sqlite_version", |a: clap::Arg| a.hide(true))
            .mut_arg("sqlite_cflag", |a: clap::Arg| a.hide(true))
    };

    #[cfg(feature = "dynload")]
    let cmd = if app.allow_dialect_override() {
        cmd
    } else {
        cmd.mut_arg("dialect_path", |a: clap::Arg| a.hide(true))
            .mut_arg("dialect_name", |a: clap::Arg| a.hide(true))
    };

    cmd
}

/// Run the CLI with the given app configuration.
///
/// Reads `argv` from the process. On error, prints the message to stderr and
/// exits with status 1. On `--help` / `--version`, exits 0.
pub fn run<A: CliApp>(app: &A) {
    use clap::FromArgMatches;

    let cmd = build_command(app);
    let matches = cmd.try_get_matches_from(std::env::args()).unwrap_or_else(|e| e.exit());
    let cli = Cli::from_arg_matches(&matches).unwrap_or_else(|e| e.exit());

    if let Err(e) = enforce_visibility(app, &cli) {
        eprintln!("error: {e}");
        std::process::exit(2);
    }

    if let Err(e) = runtime::dispatch(cli, app.default_dialect()) {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}

/// Reject hidden flags that the trait says aren't allowed but were still passed.
fn enforce_visibility<A: CliApp>(app: &A, cli: &Cli) -> Result<(), String> {
    if !app.allow_sqlite_tuning() {
        if cli.sqlite_version.is_some() {
            return Err("--sqlite-version is not supported by this CLI".to_owned());
        }
        if !cli.sqlite_cflag.is_empty() {
            return Err("--sqlite-cflag is not supported by this CLI".to_owned());
        }
    }

    #[cfg(feature = "dynload")]
    if !app.allow_dialect_override() && cli.dialect_path.is_some() {
        return Err("--dialect is not supported by this CLI".to_owned());
    }

    Ok(())
}

/// Stock CLI configuration: the bundled SQLite dialect with all override
/// surfaces enabled.
#[cfg(feature = "bundled-sqlite-dialect")]
pub struct Stock;

#[cfg(feature = "bundled-sqlite-dialect")]
impl CliApp for Stock {
    fn name(&self) -> &str {
        "syntaqlite"
    }

    fn default_dialect(&self) -> Option<AnyDialect> {
        Some(syntaqlite::sqlite_dialect().into())
    }

    fn allow_dialect_override(&self) -> bool {
        true
    }

    fn allow_sqlite_tuning(&self) -> bool {
        true
    }
}
