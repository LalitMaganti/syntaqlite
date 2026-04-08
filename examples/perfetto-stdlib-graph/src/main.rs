//! `PerfettoSQL` standard library dependency graph analyzer.
//!
//! Scans the `PerfettoSQL` stdlib `.sql` files using syntaqlite's semantic
//! analyzer to extract table/view/function/macro definitions and their
//! dependencies, and outputs the graph as JSON, interactive HTML, ASCII,
//! or Graphviz DOT.

mod extract;
mod model;
mod output;
mod resolve;

use std::fs::File;
use std::io::{self, BufWriter, Write};
use std::path::PathBuf;
use std::process::ExitCode;

use clap::{Parser, Subcommand};
use syntaqlite::any::AnyDialect;

/// Analyze the `PerfettoSQL` standard library dependency graph.
#[derive(Parser)]
#[command(name = "perfetto-stdlib-graph")]
struct Cli {
    /// Path to the `PerfettoSQL` stdlib directory.
    #[arg(
        long,
        default_value = "third_party/src/perfetto/src/trace_processor/perfetto_sql/stdlib"
    )]
    stdlib: PathBuf,

    /// Path to the compiled Perfetto dialect shared library (`.dylib`/`.so`).
    #[arg(long)]
    dialect: PathBuf,

    /// Output subcommand.
    #[command(subcommand)]
    command: Command,
}

/// Output format.
#[derive(Subcommand)]
enum Command {
    /// Output the dependency graph as JSON.
    Json {
        /// Output file path (stdout if omitted).
        #[arg(short, long)]
        output: Option<PathBuf>,
        /// Pretty-print JSON.
        #[arg(long)]
        pretty: bool,
    },
    /// Output a self-contained interactive HTML visualization.
    Html {
        /// Output file path (stdout if omitted).
        #[arg(short, long)]
        output: Option<PathBuf>,
    },
    /// Output an ASCII summary or per-definition dependency tree.
    Ascii {
        /// Show dependencies for a specific table/view/function/macro.
        #[arg(long)]
        table: Option<String>,
        /// Include transitive dependencies and dependents.
        #[arg(long)]
        transitive: bool,
    },
    /// Output Graphviz DOT format.
    Dot {
        /// Output file path (stdout if omitted).
        #[arg(short, long)]
        output: Option<PathBuf>,
        /// Cluster nodes by top-level module.
        #[arg(long)]
        cluster: bool,
        /// Generate graph for a specific table only.
        #[arg(long)]
        table: Option<String>,
    },
}

fn run() -> Result<(), String> {
    let cli = Cli::parse();

    if !cli.stdlib.exists() {
        return Err(format!(
            "stdlib path does not exist: {}",
            cli.stdlib.display()
        ));
    }
    if !cli.dialect.exists() {
        return Err(format!(
            "dialect library does not exist: {}\n\
             Run tools/build-perfetto-dialect to compile it first.",
            cli.dialect.display()
        ));
    }

    // Load the Perfetto dialect dynamically.
    let dialect_path = cli.dialect.to_string_lossy();
    let dialect = AnyDialect::load(&dialect_path, Some("perfetto"))
        .map_err(|e| format!("failed to load dialect: {e}"))?;

    eprintln!("Analyzing stdlib at {}...", cli.stdlib.display());

    let extraction = extract::extract_stdlib(&cli.stdlib, dialect)?;
    let graph = resolve::build_graph(extraction);

    eprintln!(
        "Found {} definitions, {} edges, {} external refs",
        graph.stats.total_definitions, graph.stats.total_edges, graph.stats.external_refs,
    );

    match cli.command {
        Command::Json { output, pretty } => {
            let mut w = open_output(output.as_deref())?;
            output::json::write_json(&graph, &mut w, pretty)?;
        }
        Command::Html { output } => {
            let mut w = open_output(output.as_deref())?;
            output::html::write_html(&graph, &mut w)?;
        }
        Command::Ascii { table, transitive } => {
            let mut w = BufWriter::new(io::stdout().lock());
            if let Some(name) = &table {
                output::ascii::write_table_detail(&graph, name, transitive, &mut w)?;
            } else {
                output::ascii::write_summary(&graph, &mut w)?;
            }
        }
        Command::Dot {
            output,
            cluster,
            table,
        } => {
            let mut w = open_output(output.as_deref())?;
            if let Some(name) = &table {
                output::dot::write_dot_for_table(&graph, name, &mut w)?;
            } else {
                output::dot::write_dot(&graph, &mut w, cluster)?;
            }
        }
    }

    Ok(())
}

fn open_output(path: Option<&std::path::Path>) -> Result<Box<dyn Write>, String> {
    match path {
        Some(p) => {
            let f =
                File::create(p).map_err(|e| format!("failed to create {}: {e}", p.display()))?;
            Ok(Box::new(BufWriter::new(f)))
        }
        None => Ok(Box::new(BufWriter::new(io::stdout().lock()))),
    }
}

fn main() -> ExitCode {
    match run() {
        Ok(()) => ExitCode::SUCCESS,
        Err(msg) => {
            eprintln!("error: {msg}");
            ExitCode::FAILURE
        }
    }
}
