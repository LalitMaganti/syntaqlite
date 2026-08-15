+++
title = "syntaqlite"
+++

syntaqlite provides a fast CLI and language server for parsing, formatting, and statically
analyzing SQLite SQL. libsyntaqlite is the fast, embeddable parser, formatter, and
analyzer that powers those tools, with APIs for Rust, Python, JavaScript/WASM, and C.

Both are built from SQLite's own grammar and tokenizer and can target specific SQLite
versions and compile-time flags.

> **Note:** syntaqlite and libsyntaqlite are at 0.x. Their APIs and command-line interface
> may change before 1.0.
