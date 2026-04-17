// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![expect(
    missing_docs,
    reason = "bin crate; internal lib shim exists only to support integration tests"
)]

use clap::{Parser, Subcommand, ValueEnum};

#[cfg(feature = "builtin-sqlite")]
mod config;

#[cfg(feature = "builtin-sqlite")]
mod runtime;

#[cfg(feature = "builtin-sqlite")]
mod codegen;

#[cfg(feature = "builtin-sqlite")]
mod lineage_output;

#[cfg(feature = "mcp")]
#[expect(
    clippy::needless_pass_by_value,
    reason = "rmcp #[tool(aggr)] requires by-value params"
)]
mod mcp;

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
#[command(
    name = "syntaqlite",
    about = "SQL formatting and analysis tools",
    version
)]
pub(crate) struct Cli {
    /// Path to `syntaqlite.toml` config file.
    /// When omitted, discovered by walking up from the current directory.
    #[cfg(feature = "builtin-sqlite")]
    #[arg(short = 'c', long = "config", global = true)]
    pub(crate) config: Option<String>,

    /// Disable automatic config file discovery.
    #[cfg(feature = "builtin-sqlite")]
    #[arg(long = "no-config", global = true, conflicts_with = "config")]
    pub(crate) no_config: bool,

    /// Path to a shared library (.so/.dylib/.dll) providing a dialect.
    #[cfg(feature = "builtin-sqlite")]
    #[arg(long = "dialect", global = true)]
    pub(crate) dialect_path: Option<String>,

    /// Dialect name for symbol lookup.
    /// When omitted, the loader resolves `syntaqlite_grammar`.
    /// With a name, it resolves `syntaqlite_<name>_grammar`.
    #[cfg(feature = "builtin-sqlite")]
    #[arg(long, requires = "dialect_path", global = true)]
    pub(crate) dialect_name: Option<String>,

    /// `SQLite` version to emulate (e.g. "3.47.0", "latest").
    #[cfg(feature = "builtin-sqlite")]
    #[arg(long, global = true)]
    pub(crate) sqlite_version: Option<String>,

    /// Enable a `SQLite` compile-time flag (e.g. `SQLITE_ENABLE_ORDERED_SET_AGGREGATES`).
    /// Can be specified multiple times.
    #[cfg(feature = "builtin-sqlite")]
    #[arg(long, global = true)]
    pub(crate) sqlite_cflag: Vec<String>,

    #[command(subcommand)]
    pub(crate) command: Command,
}

#[derive(Subcommand)]
pub(crate) enum Command {
    /// Parse SQL and report results
    #[cfg(feature = "builtin-sqlite")]
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
    #[cfg(feature = "builtin-sqlite")]
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
    #[cfg(feature = "builtin-sqlite")]
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
    #[cfg(feature = "builtin-sqlite")]
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
    #[cfg(feature = "builtin-sqlite")]
    Lsp,
    /// Start the MCP server (stdio)
    #[cfg(feature = "mcp")]
    Mcp,
    /// Generate dialect C sources and Rust bindings for external dialects.
    #[cfg(feature = "builtin-sqlite")]
    Dialect(codegen::DialectArgs),
    /// Print version information
    Version,
    #[cfg(feature = "builtin-sqlite")]
    #[command(flatten)]
    DialectTool(codegen::ToolCommand),
}

/// Run the CLI.
#[cfg(feature = "builtin-sqlite")]
pub fn run(name: &str, dialect: Option<syntaqlite::any::AnyDialect>) {
    let cli =
        Cli::try_parse_from(std::iter::once(name.to_string()).chain(std::env::args().skip(1)))
            .unwrap_or_else(|e| e.exit());

    let result = runtime::dispatch(cli, dialect);

    if let Err(e) = result {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}
