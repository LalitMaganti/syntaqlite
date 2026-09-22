// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Parser generation, tokenizer/keyword assembly, base files, and amalgamation.

pub mod amalgamate;
pub mod base_files;
mod base_files_tables;
pub(crate) mod grammar_codegen;
pub(crate) mod keyword_hash;
pub(crate) mod lemon;
pub(crate) mod literal_tokens;
pub(crate) mod mkkeyword;
pub(crate) mod parser_pipeline;
pub(crate) mod sqlite_fragments;
pub(crate) mod tokenizer_assembly;
