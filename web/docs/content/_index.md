+++
title = "syntaqlite"
+++

syntaqlite provides a CLI and language server for parsing, formatting, and statically
analyzing SQLite SQL. libsyntaqlite contains the parser, formatter, and analyzer used by
those tools, with APIs for Rust, Python, JavaScript/WASM, and C.

Both are built from SQLite's own grammar and tokenizer and can target specific SQLite
versions and compile-time flags.

> **Note:** syntaqlite and libsyntaqlite are at 0.x. Their APIs and command-line interface
> may change before 1.0.
