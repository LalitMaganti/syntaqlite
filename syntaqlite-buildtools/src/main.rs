// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Internal bootstrap and code generation tool.
//!
//! This binary has no dependency on any generated files, so it can be built
//! from a completely clean checkout and used to regenerate everything.
//!
//! Subcommands:
//!   codegen-sqlite          — regenerate the internal `SQLite` dialect (stage 2)
//!   codegen-sqlite-parser   — regenerate built-in catalogs (stage 1b)
//!   sqlite-extract          — extract C fragments from raw `SQLite` source (stage 1)
//!   update-data             — audit cflags and extract built-in catalogs from amalgamations
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
    /// Generates built-in catalog modules and optionally `cflag_versions.rs`
    /// from pre-existing inputs.
    #[command(name = "codegen-sqlite-parser")]
    CodegenSqliteParser(CodegenSqliteParserArgs),

    /// Extract C fragments from raw `SQLite` source (stage 1).
    #[command(name = "sqlite-extract")]
    SqliteExtract(SqliteExtractArgs),

    /// Audit cflag availability and extract built-in catalogs from amalgamations.
    ///
    /// Runs the cflag audit (`version_cflags.json` + cflags.rs) followed by
    /// built-in extraction (`functions.json` and `relations.json`) in one shot.
    #[command(name = "update-data")]
    UpdateData(UpdateDataArgs),

    /// Analyze multiple `SQLite` source versions to find fragment variants.
    #[command(name = "analyze-versions")]
    AnalyzeVersions(AnalyzeVersionsArgs),

    /// Produce a C amalgamation of the syntaqlite-syntax parser/tokenizer
    /// (single .h + .c pair) from the runtime and dialect source trees.
    #[command(name = "amalgamate-syntax")]
    AmalgamateSyntax(AmalgamateSyntaxArgs),

    /// Produce a combined C header for the full syntaqlite API (parser + formatter + validator).
    /// Merges syntaqlite-syntax headers with syntaqlite Rust FFI headers into one syntaqlite.h.
    Amalgamate(AmalgamateHeaderArgs),

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
    #[arg(long = "functions-json")]
    functions_input: Option<String>,
    /// Path to relations.json (from sqlite-vendored/data/relations.json).
    /// When provided, generates `relations_catalog.rs` at its hardcoded workspace path.
    #[arg(long = "relations-json")]
    relations_source: Option<String>,
    /// Path to `version_cflags.json` (from sqlite-vendored/data/version_cflags.json).
    /// Supplies categories and minimum versions for generated cflag APIs.
    #[arg(long = "version-cflags-json", required = true)]
    version_cflags: String,
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
    /// Path to `version_cflags.json`; existing categories are preserved.
    #[arg(long, required = true)]
    version_cflags_output: String,
    /// Output path for functions.json.
    #[arg(long, required = true)]
    functions_output: String,
    /// Output path for relations.json.
    #[arg(long, required = true)]
    relations_output: String,
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

// ── amalgamate-syntax ─────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct AmalgamateSyntaxArgs {
    /// Dialect name (e.g. "sqlite").
    #[arg(long, required = true)]
    dialect: String,
    /// Directory containing runtime C sources (csrc/ and include/ subdirs).
    #[arg(long, required = true)]
    runtime_dir: String,
    /// Directory containing dialect C sources (csrc/ and include/ subdirs).
    #[arg(long, required = true)]
    dialect_dir: String,
    /// Output directory for the amalgamated .h and .c files.
    #[arg(long, required = true)]
    output_dir: String,
    /// Compile out macro expansion support (`SYNTAQLITE_OMIT_MACROS`).
    /// Use for dialects that do not use macro calls.
    #[arg(long, default_value_t = false)]
    omit_macros: bool,
}

// ── amalgamate ────────────────────────────────────────────────────────────────

#[derive(clap::Args)]
struct AmalgamateHeaderArgs {
    /// Directory containing syntaqlite-syntax source (with csrc/ and include/ subdirs).
    #[arg(long, required = true)]
    syntax_dir: String,
    /// Directory containing syntaqlite Rust FFI headers (include/ subdir).
    #[arg(long, required = true)]
    lib_dir: String,
    /// Output path for the amalgamated syntaqlite.h header.
    #[arg(long, required = true)]
    output: String,
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
            functions_json: args.functions_input.clone(),
            relations_json: args.relations_source.clone(),
            version_cflags_json: Some(args.version_cflags.clone()),
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
            relations_output: args.relations_output.clone(),
        }
        .run(),
        Command::AnalyzeVersions(args) => commands::AnalyzeVersions {
            sqlite_source_dir: args.sqlite_source_dir.clone(),
            output_dir: args.output_dir.clone(),
        }
        .run(),
        Command::AmalgamateSyntax(args) => (|| -> Result<(), String> {
            use std::path::Path;
            let runtime = Path::new(&args.runtime_dir);
            let dialect = Path::new(&args.dialect_dir);
            let output = Path::new(&args.output_dir);
            let out = syntaqlite_buildtools::amalgamate::amalgamate_full(
                &args.dialect,
                runtime,
                dialect,
                args.omit_macros,
            )?;
            std::fs::create_dir_all(output).map_err(|e| format!("creating output dir: {e}"))?;
            let h_path = output.join(format!("syntaqlite_{}.h", args.dialect));
            let c_path = output.join(format!("syntaqlite_{}.c", args.dialect));
            std::fs::write(&h_path, &out.header)
                .map_err(|e| format!("writing {}: {e}", h_path.display()))?;
            std::fs::write(&c_path, &out.source)
                .map_err(|e| format!("writing {}: {e}", c_path.display()))?;
            eprintln!("wrote {}", h_path.display());
            eprintln!("wrote {}", c_path.display());
            Ok(())
        })(),
        Command::Amalgamate(args) => (|| -> Result<(), String> {
            use std::path::Path;
            let syntax_dir = Path::new(&args.syntax_dir);
            let lib_dir = Path::new(&args.lib_dir);
            let output = Path::new(&args.output);
            let header = syntaqlite_buildtools::amalgamate::amalgamate_header(syntax_dir, lib_dir)?;
            if let Some(parent) = output.parent() {
                std::fs::create_dir_all(parent).map_err(|e| format!("creating output dir: {e}"))?;
            }
            std::fs::write(output, &header)
                .map_err(|e| format!("writing {}: {e}", output.display()))?;
            eprintln!("wrote {}", output.display());
            Ok(())
        })(),
        Command::Lemon { args } => syntaqlite_buildtools::run_lemon(args),
        Command::Mkkeyword { args } => syntaqlite_buildtools::run_mkkeyword(args),
    };

    if let Err(e) = result {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}
