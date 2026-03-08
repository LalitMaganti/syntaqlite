// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Internal bootstrap and code generation tool.
//!
//! This binary has no dependency on any generated files, so it can be built
//! from a completely clean checkout and used to regenerate everything.
//!
//! Subcommands:
//!   codegen-sqlite          — regenerate the internal `SQLite` dialect (stage 2)
//!   codegen-sqlite-parser   — regenerate `functions_catalog` (stage 1b)
//!   sqlite-extract          — extract C fragments from raw `SQLite` source (stage 1)
//!   update-data             — audit cflags and extract function catalog from amalgamations
//!   analyze-versions        — analyze `SQLite` source version history

use clap::{Parser, Subcommand};

use syntaqlite_buildtools::commands;

#[derive(Parser)]
#[command(about = "Internal bootstrap and code generation tool for syntaqlite")]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Subcommand)]
enum Command {
    /// Regenerate the internal `SQLite` dialect C sources and Rust bindings (stage 2).
    ///
    /// Writes generated files to the hardcoded workspace paths in
    /// syntaqlite-syntax/.
    #[command(name = "codegen-sqlite")]
    CodegenSqlite(CodegenSqliteArgs),

    /// Regenerate internal Rust artifacts for the `SQLite` parser crate (stage 1b).
    ///
    /// Generates `functions_catalog.rs` and optionally `cflag_versions.rs`
    /// from pre-existing inputs.
    #[command(name = "codegen-sqlite-parser")]
    CodegenSqliteParser(CodegenSqliteParserArgs),

    /// Extract C fragments from raw `SQLite` source (stage 1).
    #[command(name = "sqlite-extract")]
    SqliteExtract(SqliteExtractArgs),

    /// Audit cflag availability and extract function catalog from amalgamations.
    ///
    /// Runs the cflag audit (`version_cflags.json` + cflags.rs) followed by
    /// function extraction (functions.json) in one shot.
    #[command(name = "update-data")]
    UpdateData(UpdateDataArgs),

    /// Analyze multiple `SQLite` source versions to find fragment variants.
    #[command(name = "analyze-versions")]
    AnalyzeVersions(AnalyzeVersionsArgs),

    /// Run upstream `SQLite` tests against syntaqlite parser and validator.
    #[command(name = "run-upstream-tests")]
    RunUpstreamTests(RunUpstreamTestsArgs),

    /// Hidden subprocess entry point for the Lemon parser generator.
    #[command(hide = true)]
    Lemon {
        #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
        args: Vec<String>,
    },

    /// Hidden subprocess entry point for the mkkeyword hash generator.
    #[command(hide = true)]
    Mkkeyword {
        #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
        args: Vec<String>,
    },
}

// ── codegen-sqlite ────────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct CodegenSqliteArgs {
    /// Directory containing .y grammar action files.
    #[arg(long, required = true)]
    actions_dir: String,

    /// Directory containing .synq node definitions.
    #[arg(long, required = true)]
    nodes_dir: String,
}

// ── codegen-sqlite-parser ─────────────────────────────────────────────────────

#[derive(clap::Args)]
struct CodegenSqliteParserArgs {
    /// Path to functions.json (from sqlite-vendored/data/functions.json).
    /// When provided, generates `functions_catalog.rs` at its hardcoded workspace path.
    #[arg(long)]
    functions_json: Option<String>,
    /// Path to `version_cflags.json` (from sqlite-vendored/data/version_cflags.json).
    /// When provided, emits `MIN_VERSIONS` and `min_version_int()` into cflags.rs.
    #[arg(long)]
    version_cflags_json: Option<String>,
}

// ── sqlite-extract ────────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct SqliteExtractArgs {
    /// Path to the `SQLite` source tree root (containing src/ and tool/).
    #[arg(long, required = true)]
    sqlite_src: String,
    /// Output directory for vendored files (sqlite-vendored/).
    #[arg(long, required = true)]
    output_dir: String,
    /// Path to the parser-actions directory (containing .y files).
    #[arg(long, required = true)]
    actions_dir: String,
    /// Path to the parser-nodes directory (containing .synq files).
    #[arg(long, required = true)]
    nodes_dir: String,
}

// ── update-data ───────────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct UpdateDataArgs {
    /// Directory containing amalgamations (e.g., sqlite-amalgamations/3.35.5/sqlite3.c).
    #[arg(long, required = true)]
    amalgamation_dir: String,
    /// Output path for `version_cflags.json`.
    #[arg(long, required = true)]
    version_cflags_output: String,
    /// Output path for functions.json.
    #[arg(long, required = true)]
    functions_output: String,
}

// ── run-upstream-tests ────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct RunUpstreamTestsArgs {
    /// Path to `SQLite` source tree `test/` directory.
    #[arg(long, required = true)]
    test_dir: String,
    /// Path to compiled `tclsyntaqlite` shared library.
    #[arg(long, required = true)]
    extension_lib: String,
    /// Path to `tester_shim.tcl`.
    #[arg(long, required = true)]
    tester_shim: String,
    /// Path to baseline file (created on first run).
    #[arg(long)]
    baseline: Option<String>,
    /// Update baseline with current results.
    #[arg(long)]
    rebaseline: bool,
    /// Filter test files by pattern.
    #[arg(long)]
    filter: Option<String>,
    /// Number of parallel test jobs.
    #[arg(long, short, default_value = "1")]
    jobs: usize,
    /// Enable validation (not just parsing).
    #[arg(long)]
    validate: bool,
}

// ── analyze-versions ──────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct AnalyzeVersionsArgs {
    /// Directory containing per-version `SQLite` source trees.
    /// Expected layout: <dir>/3.35.0/src/tokenize.c, etc.
    #[arg(long, required = true)]
    sqlite_source_dir: String,
    /// Output directory for variant files.
    #[arg(long, required = true)]
    output_dir: String,
}

// ── main ──────────────────────────────────────────────────────────────────────

fn main() {
    let cli = Cli::parse();

    let result: Result<(), String> = match &cli.command {
        Command::CodegenSqlite(args) => commands::SqliteCodegen {
            actions_dir: args.actions_dir.clone(),
            nodes_dir: args.nodes_dir.clone(),
        }
        .run(),
        Command::CodegenSqliteParser(args) => commands::SqliteParserCodegen {
            functions_json: args.functions_json.clone(),
            version_cflags_json: args.version_cflags_json.clone(),
        }
        .run(),
        Command::SqliteExtract(args) => commands::SqliteExtract {
            sqlite_src: args.sqlite_src.clone(),
            output_dir: args.output_dir.clone(),
            actions_dir: args.actions_dir.clone(),
            nodes_dir: args.nodes_dir.clone(),
        }
        .run(),
        Command::UpdateData(args) => commands::UpdateData {
            amalgamation_dir: args.amalgamation_dir.clone(),
            version_cflags_output: args.version_cflags_output.clone(),
            functions_output: args.functions_output.clone(),
        }
        .run(),
        Command::AnalyzeVersions(args) => commands::AnalyzeVersions {
            sqlite_source_dir: args.sqlite_source_dir.clone(),
            output_dir: args.output_dir.clone(),
        }
        .run(),
        Command::RunUpstreamTests(args) => {
            use syntaqlite_buildtools::upstream_tests;
            let config = upstream_tests::RunConfig {
                test_dir: args.test_dir.clone().into(),
                extension_lib: args.extension_lib.clone().into(),
                tester_shim: args.tester_shim.clone().into(),
                baseline: args.baseline.clone().map(Into::into),
                rebaseline: args.rebaseline,
                filter: args.filter.clone(),
                jobs: args.jobs,
                validate: args.validate,
            };
            upstream_tests::run(&config).map(|_| ())
        }
        Command::Lemon { args } => syntaqlite_buildtools::run_lemon(args),
        Command::Mkkeyword { args } => syntaqlite_buildtools::run_mkkeyword(args),
    };

    if let Err(e) = result {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}
