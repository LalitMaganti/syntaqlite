// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Clap argument definitions.
//!
//! Each subcommand gets its own `#[derive(clap::Args)]` struct so that
//! dispatch can thread the whole bag of arguments through with a single
//! pattern (`Command::Fmt(args) => commands::fmt::run(..., &args)`).

use clap::{Args, Parser, Subcommand, ValueEnum};

// ── Output format enums ────────────────────────────────────────────────────

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

#[derive(Clone, Copy, Default, ValueEnum)]
pub(crate) enum ValidateOutput {
    /// Rustc-style rendered diagnostics on stderr (default)
    #[default]
    Text,
    /// Newline-delimited JSON, one record per diagnostic
    Json,
}

#[derive(Clone, Copy, Default, ValueEnum)]
pub(crate) enum IntrospectOutput {
    /// Human-readable text (default)
    #[default]
    Text,
    /// JSON or newline-delimited JSON
    Json,
}

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

// ── Top-level CLI + Command enum ──────────────────────────────────────────

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
    Parse(ParseArgs),
    /// Format SQL
    Fmt(FmtArgs),
    /// Validate SQL and report diagnostics
    Validate(ValidateArgs),
    /// Extract column and table lineage from SQL
    Lineage(LineageArgs),
    /// Start the language server (stdio)
    Lsp,
    /// Start the MCP server (stdio)
    #[cfg(feature = "mcp")]
    Mcp,
    /// Start the long-lived JSON RPC server over stdio
    Serve,
    /// Tokenize SQL and print the token stream
    Tokenize(TokenizeArgs),
    /// Dialect codegen (generate C + Rust sources for custom dialects)
    #[cfg(feature = "codegen")]
    Dialect {
        #[command(subcommand)]
        command: DialectSubcommand,
    },
    /// Print version information
    Version,
    #[cfg(feature = "codegen")]
    #[command(flatten)]
    DialectTool(crate::commands::codegen::ToolCommand),
}

// ── Per-subcommand argument structs ───────────────────────────────────────

#[derive(Args)]
pub(crate) struct ParseArgs {
    /// SQL files or glob patterns (reads stdin if omitted)
    pub(crate) files: Vec<String>,
    /// SQL expression to process directly (instead of files or stdin)
    #[arg(short = 'e', long = "expression", conflicts_with = "files")]
    pub(crate) expression: Option<String>,
    /// Output format
    #[arg(short, long, value_enum, default_value_t = ParseOutput::Text)]
    pub(crate) output: ParseOutput,
}

#[derive(Args)]
pub(crate) struct FmtArgs {
    /// SQL files or glob patterns (reads stdin if omitted)
    pub(crate) files: Vec<String>,
    /// SQL expression to format directly (instead of files or stdin)
    #[arg(short = 'e', long = "expression", conflicts_with = "files")]
    pub(crate) expression: Option<String>,
    /// Maximum line width
    #[arg(short = 'w', long)]
    pub(crate) line_width: Option<usize>,
    /// Spaces per indentation level
    #[arg(short = 't', long)]
    pub(crate) indent_width: Option<usize>,
    /// Keyword casing
    #[arg(short = 'k', long, value_enum)]
    pub(crate) keyword_case: Option<KeywordCasing>,
    /// Write formatted output back to file(s) in place
    #[arg(short = 'i', long)]
    pub(crate) in_place: bool,
    /// Check if files are formatted (exit 1 if not)
    #[arg(long, conflicts_with = "in_place")]
    pub(crate) check: bool,
    /// Append semicolons after each statement
    #[arg(long)]
    pub(crate) semicolons: Option<bool>,
    /// Output mode (formatted, bytecode, doc-tree)
    #[arg(short, long, value_enum, default_value_t = FmtOutput::Formatted)]
    pub(crate) output: FmtOutput,
}

#[derive(Args)]
pub(crate) struct ValidateArgs {
    /// SQL files or glob patterns (reads stdin if omitted)
    pub(crate) files: Vec<String>,
    /// SQL expression to validate directly (instead of files or stdin)
    #[arg(short = 'e', long = "expression", conflicts_with = "files")]
    pub(crate) expression: Option<String>,
    /// Schema DDL file(s) to load before validation (repeatable, supports globs)
    #[arg(long)]
    pub(crate) schema: Vec<String>,
    /// Allow (suppress) a check category (repeatable; use "schema" or "all" for groups)
    #[arg(short = 'A', long = "allow")]
    pub(crate) allow: Vec<String>,
    /// Warn on a check category (repeatable)
    #[arg(short = 'W', long = "warn")]
    pub(crate) warn: Vec<String>,
    /// Deny (error) a check category (repeatable)
    #[arg(short = 'D', long = "deny")]
    pub(crate) deny: Vec<String>,
    /// [experimental] Host language for embedded SQL extraction (python, typescript)
    #[arg(long = "experimental-lang")]
    pub(crate) lang: Option<HostLanguage>,
    /// Output format
    #[arg(short, long, value_enum, default_value_t = ValidateOutput::Text)]
    pub(crate) output: ValidateOutput,
}

#[derive(Args)]
pub(crate) struct LineageArgs {
    /// SQL files or glob patterns (reads stdin if omitted)
    pub(crate) files: Vec<String>,
    /// SQL expression to analyze directly (instead of files or stdin)
    #[arg(short = 'e', long = "expression", conflicts_with = "files")]
    pub(crate) expression: Option<String>,
    /// Schema DDL file(s) to load before analysis (repeatable, supports globs)
    #[arg(long)]
    pub(crate) schema: Vec<String>,
    /// Output format
    #[arg(short, long, value_enum, default_value_t = LineageOutput::Json)]
    pub(crate) output: LineageOutput,
    /// Restrict output to a subset (combined output is the default)
    #[command(subcommand)]
    pub(crate) scope: Option<LineageScope>,
}

#[derive(Args)]
pub(crate) struct TokenizeArgs {
    /// SQL files or glob patterns (reads stdin if omitted)
    pub(crate) files: Vec<String>,
    /// SQL expression to tokenize directly (instead of files or stdin)
    #[arg(short = 'e', long = "expression", conflicts_with = "files")]
    pub(crate) expression: Option<String>,
    /// Output format
    #[arg(short, long, value_enum, default_value_t = IntrospectOutput::Text)]
    pub(crate) output: IntrospectOutput,
}

#[derive(Clone, Copy, Subcommand)]
pub(crate) enum LineageScope {
    /// Emit only relations and physical tables (drop columns)
    Tables,
    /// Emit only column lineage (drop relations and physical tables)
    Columns,
}

#[cfg(feature = "codegen")]
#[derive(Subcommand)]
pub(crate) enum DialectSubcommand {
    /// Generate dialect C sources and Rust bindings for external dialects
    Generate(crate::commands::codegen::DialectArgs),
}
