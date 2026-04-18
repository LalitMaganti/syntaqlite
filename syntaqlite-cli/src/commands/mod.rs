// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! One module per subcommand. Each exposes a `run` entry point called from
//! [`crate::dispatch`].

pub(crate) mod fmt;
pub(crate) mod lineage;
pub(crate) mod lsp;
pub(crate) mod parse;
pub(crate) mod tokenize;
pub(crate) mod validate;

#[cfg(feature = "codegen")]
pub(crate) mod codegen;

#[cfg(feature = "mcp")]
pub(crate) mod mcp;
