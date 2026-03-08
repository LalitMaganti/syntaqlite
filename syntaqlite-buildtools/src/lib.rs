// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![cfg_attr(test, expect(clippy::unwrap_used, clippy::similar_names))]

//! Bootstrap and codegen tool for syntaqlite.
//!
//! Contains extraction, codegen, and parser generation pipelines.

/// Shared utilities (parsers, C/Rust writers, case conversion, tool_run).
pub(crate) mod util;

// Stage 1: extraction from raw SQLite source.
/// Extraction from raw SQLite source (stage 1).
pub(crate) mod extract;

// Stage 2/3: codegen pipeline.
/// `.synq` to C/Rust codegen (AST model, node/meta/dialect/fmt codegen).
pub(crate) mod dialect_codegen;

pub(crate) mod parser_tools;

// Re-export parser_tools submodules at crate root for API stability.
pub use parser_tools::{amalgamate, base_files};

/// Output resolver: `OutputLayout` and `write_codegen_artifacts`.
pub mod output_resolver;

/// SQLite source version analysis and fragment diffing.
pub mod version_analysis;

/// Bootstrap command implementations.
pub mod commands;

/// Upstream SQLite test runner.
pub mod upstream_tests;

#[cfg(test)]
mod grammar_verify;
#[cfg(test)]
mod no_sqlite_compile;

// --- Codegen pipeline types and functions (stage 2/3) ---

/// Codegen pipeline types and functions (stage 2/3).
pub mod codegen_api;

/// Lemon parser generator subprocess entry point.
pub fn run_lemon(args: &[String]) -> ! {
    parser_tools::lemon::run_lemon(args)
}

/// Mkkeyword hash generator subprocess entry point.
pub fn run_mkkeyword(args: &[String]) -> ! {
    parser_tools::mkkeyword::run_mkkeyword(args)
}
