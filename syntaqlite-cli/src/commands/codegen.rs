// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `dialect` subcommand: generate C sources and Rust bindings for external dialects.

use std::fs;
use std::path::Path;

use syntaqlite_buildtools::codegen_api::MacroStyle;

fn ensure_dir(path: &Path, label: &str) -> Result<(), String> {
    fs::create_dir_all(path).map_err(|e| format!("Failed to create {label}: {e}"))
}

fn write_file(path: &Path, content: impl AsRef<[u8]>) -> Result<(), String> {
    fs::write(path, content).map_err(|e| format!("Failed to write {}: {}", path.display(), e))
}

/// Output type for the dialect command.
#[derive(clap::ValueEnum, Clone)]
pub(crate) enum OutputType {
    /// TypedDialectEnv-only amalgamation (default).
    TypedDialectEnv,
    /// Raw C/H/Rust files, flat layout.
    Raw,
    /// Runtime + dialect inlined into one self-contained file pair.
    Full,
    /// Runtime amalgamation only.
    RuntimeOnly,
}

/// Generate dialect C sources and Rust bindings for external dialects.
///
/// Base `SQLite` grammar and node files are embedded in the binary.
/// When `--actions-dir` / `--nodes-dir` are provided, those extension
/// files are merged with the base (same-name files replace the base).
#[derive(clap::Parser)]
pub(crate) struct DialectArgs {
    /// `TypedDialectEnv` identifier (e.g. "mydialect").
    #[arg(long, required = true)]
    name: String,

    /// Output directory for generated files.
    #[arg(long)]
    output_dir: Option<String>,

    /// Directory containing .y grammar action files.
    #[arg(long)]
    actions_dir: Option<String>,

    /// Directory containing .synq node definitions.
    #[arg(long)]
    nodes_dir: Option<String>,

    /// Output type.
    #[arg(long, value_enum, default_value_t = OutputType::TypedDialectEnv)]
    output_type: OutputType,

    /// Default path for the runtime header (dialect-only mode only).
    #[arg(long, default_value = "syntaqlite_runtime.h")]
    runtime_header: String,

    /// Default path for the extension header (dialect-only mode only).
    #[arg(long, default_value = "syntaqlite_dialect.h")]
    ext_header: String,

    /// Macro invocation style: "none" or "rust" (for `name!(...)` patterns).
    #[arg(long, value_enum, default_value_t = CliMacroStyle::None)]
    macro_style: CliMacroStyle,
}

#[derive(Clone, Copy, Default, clap::ValueEnum)]
pub(crate) enum CliMacroStyle {
    #[default]
    None,
    Rust,
}

// Hidden subcommands forwarded to the lemon/mkkeyword subprocess invocations.
// These must be present in any binary that calls the codegen pipeline;
// `generate_codegen_artifacts()` spawns the current executable with these.
#[derive(clap::Subcommand)]
pub(crate) enum ToolCommand {
    #[command(hide = true)]
    Lemon {
        #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
        args: Vec<String>,
    },
    #[command(hide = true)]
    Mkkeyword {
        #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
        args: Vec<String>,
    },
}

pub(crate) fn generate(args: &DialectArgs) -> Result<(), String> {
    let name = &args.name;
    let actions_dir = args.actions_dir.as_deref();
    let nodes_dir = args.nodes_dir.as_deref();
    let macro_style = match args.macro_style {
        CliMacroStyle::None => MacroStyle::None,
        CliMacroStyle::Rust => MacroStyle::Rust,
    };
    let require_output_dir = |type_name: &str| -> Result<String, String> {
        args.output_dir
            .clone()
            .ok_or_else(|| format!("--output-dir is required for --output-type={type_name}"))
    };

    match &args.output_type {
        OutputType::TypedDialectEnv => generate_dialect(
            name,
            actions_dir,
            nodes_dir,
            &require_output_dir("dialect")?,
            &args.runtime_header,
            &args.ext_header,
            macro_style,
        ),
        OutputType::Raw => generate_dialect_raw(
            name,
            actions_dir,
            nodes_dir,
            &require_output_dir("raw")?,
            macro_style,
        ),
        OutputType::Full => generate_dialect_full(
            name,
            actions_dir,
            nodes_dir,
            &require_output_dir("full")?,
            macro_style,
        ),
        OutputType::RuntimeOnly => generate_runtime(&require_output_dir("runtime-only")?),
    }
}

pub(crate) fn dispatch_tool(cmd: ToolCommand) -> Result<(), String> {
    match cmd {
        ToolCommand::Lemon { args } => syntaqlite_buildtools::run_lemon(&args),
        ToolCommand::Mkkeyword { args } => syntaqlite_buildtools::run_mkkeyword(&args),
    }
}

fn generate_dialect(
    dialect: &str,
    actions_dir: Option<&str>,
    nodes_dir: Option<&str>,
    output_dir: &str,
    runtime_header: &str,
    ext_header: &str,
    macro_style: MacroStyle,
) -> Result<(), String> {
    use syntaqlite_buildtools::amalgamate;

    let temp_dir = tempfile::TempDir::new().map_err(|e| format!("creating temp directory: {e}"))?;
    let temp = temp_dir.path();
    let (merged_y, merged_synq) = load_extensions(actions_dir, nodes_dir)?;
    codegen_to_dir(&merged_y, &merged_synq, temp, dialect, macro_style)?;

    let out = Path::new(output_dir);
    ensure_dir(out, "output dir")?;
    let result =
        amalgamate::amalgamate_dialect(dialect, temp, Some(runtime_header), Some(ext_header))?;
    write_file(&out.join(format!("syntaqlite_{dialect}.h")), &result.header)?;
    write_file(&out.join(format!("syntaqlite_{dialect}.c")), &result.source)?;
    eprintln!("wrote {}/syntaqlite_{dialect}.{{h,c}}", out.display());
    Ok(())
}

fn generate_dialect_full(
    dialect: &str,
    actions_dir: Option<&str>,
    nodes_dir: Option<&str>,
    output_dir: &str,
    macro_style: MacroStyle,
) -> Result<(), String> {
    use syntaqlite_buildtools::amalgamate;

    let runtime_temp =
        tempfile::TempDir::new().map_err(|e| format!("creating runtime temp directory: {e}"))?;
    syntaqlite_buildtools::base_files::write_runtime_headers_to_dir(runtime_temp.path())
        .map_err(|e| format!("writing runtime headers: {e}"))?;

    let dialect_temp =
        tempfile::TempDir::new().map_err(|e| format!("creating dialect temp directory: {e}"))?;
    let (merged_y, merged_synq) = load_extensions(actions_dir, nodes_dir)?;
    codegen_to_dir(
        &merged_y,
        &merged_synq,
        dialect_temp.path(),
        dialect,
        macro_style,
    )?;

    let out = Path::new(output_dir);
    ensure_dir(out, "output dir")?;
    let result =
        amalgamate::amalgamate_full(dialect, runtime_temp.path(), dialect_temp.path(), false)?;
    write_file(&out.join(format!("syntaqlite_{dialect}.h")), &result.header)?;
    write_file(&out.join(format!("syntaqlite_{dialect}.c")), &result.source)?;
    eprintln!(
        "wrote {}/syntaqlite_{dialect}.{{h,c}} (full)",
        out.display()
    );
    Ok(())
}

fn generate_runtime(output_dir: &str) -> Result<(), String> {
    use syntaqlite_buildtools::amalgamate;

    let temp_dir = tempfile::TempDir::new().map_err(|e| format!("creating temp directory: {e}"))?;
    let temp = temp_dir.path();
    syntaqlite_buildtools::base_files::write_runtime_headers_to_dir(temp)
        .map_err(|e| format!("writing runtime headers: {e}"))?;

    let out = Path::new(output_dir);
    ensure_dir(out, "output dir")?;
    let result = amalgamate::amalgamate_runtime(temp)?;
    write_file(&out.join("syntaqlite_runtime.h"), &result.header)?;
    write_file(&out.join("syntaqlite_runtime.c"), &result.source)?;
    if let Some(ext) = &result.ext_header {
        write_file(&out.join("syntaqlite_dialect.h"), ext)?;
    }
    eprintln!(
        "wrote {}/syntaqlite_runtime.{{h,c}} + syntaqlite_dialect.h",
        out.display()
    );
    Ok(())
}

fn generate_dialect_raw(
    dialect: &str,
    actions_dir: Option<&str>,
    nodes_dir: Option<&str>,
    output_dir: &str,
    macro_style: MacroStyle,
) -> Result<(), String> {
    use syntaqlite_buildtools::codegen_api::{DialectCodegenJob, DialectNaming};
    use syntaqlite_buildtools::output_resolver::OutputLayout;

    let (merged_y, merged_synq) = load_extensions(actions_dir, nodes_dir)?;
    let dialect_spec = DialectNaming::new(dialect);
    let layout = OutputLayout::for_external(
        Path::new(output_dir),
        dialect,
        &dialect_spec.include_dir_name(),
    );
    DialectCodegenJob::new(&dialect_spec, &merged_y, &merged_synq)
        .with_base_synq(syntaqlite_buildtools::base_files::base_synq_files())
        .with_macro_style(macro_style)
        .write_to(
            &layout,
            &|dir| ensure_dir(dir, "output directory"),
            &|path, content| write_file(path, content),
        )?;
    eprintln!("wrote raw dialect files to {output_dir}");
    Ok(())
}

/// A set of named files: `(filename, content)` pairs.
type NamedFiles = Vec<(String, String)>;

/// Load extension `.y` and `.synq` files and merge them with the base file sets.
fn load_extensions(
    actions_dir: Option<&str>,
    nodes_dir: Option<&str>,
) -> Result<(NamedFiles, NamedFiles), String> {
    use syntaqlite_buildtools::base_files;
    use syntaqlite_buildtools::codegen_api::read_named_files_from_dir;

    let read_ext = |dir: Option<&str>, ext: &str| -> Result<NamedFiles, String> {
        Ok(match dir {
            Some(d) => read_named_files_from_dir(d, ext)?,
            None => Vec::new(),
        })
    };
    let merged_y =
        base_files::merge_file_sets(base_files::base_y_files(), &read_ext(actions_dir, "y")?);
    let merged_synq =
        base_files::merge_file_sets(base_files::base_synq_files(), &read_ext(nodes_dir, "synq")?);
    Ok((merged_y, merged_synq))
}

/// Run the codegen pipeline from merged in-memory file sets into a temp directory.
fn codegen_to_dir(
    y_files: &NamedFiles,
    synq_files: &NamedFiles,
    temp_root: &Path,
    dialect_name: &str,
    macro_style: MacroStyle,
) -> Result<(), String> {
    use syntaqlite_buildtools::codegen_api::{DialectCodegenJob, DialectNaming};
    use syntaqlite_buildtools::output_resolver::OutputLayout;

    let dialect_spec = DialectNaming::new(dialect_name);
    let layout =
        OutputLayout::for_amalg_temp(temp_root, dialect_name, &dialect_spec.include_dir_name());
    DialectCodegenJob::new(&dialect_spec, y_files, synq_files)
        .with_base_synq(syntaqlite_buildtools::base_files::base_synq_files())
        .with_macro_style(macro_style)
        .write_to(
            &layout,
            &|dir| ensure_dir(dir, "output directory"),
            &|path, content| write_file(path, content),
        )
}
