// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Example: build a CLI binary that pre-specifies a dialect.
//!
//! Implementing [`syntaqlite_cli::CliApp`] lets a downstream crate ship its
//! own `[[bin]]` (e.g. `syntaqlite-perfetto`) without compiling a shared
//! library and passing it via `--dialect` at runtime.
//!
//! Run with: `cargo run --example cli_wrapper -- --help`

use syntaqlite_cli::{CliApp, run};
use syntaqlite::any::AnyDialect;

struct MyCli;

impl CliApp for MyCli {
    fn name(&self) -> &str {
        "syntaqlite-example"
    }

    fn about(&self) -> &str {
        "Example wrapper that pre-specifies the SQLite dialect"
    }

    fn default_dialect(&self) -> Option<AnyDialect> {
        Some(syntaqlite::sqlite_dialect().into())
    }

    // Both override surfaces stay off — end users can't swap the dialect or
    // tweak SQLite version/cflags. The flags are hidden from --help and
    // rejected if passed.
}

fn main() {
    run(&MyCli);
}
