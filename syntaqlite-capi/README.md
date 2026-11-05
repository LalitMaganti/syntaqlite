# syntaqlite-capi

libsyntaqlite, syntaqlite's C library: the parser, tokenizer, formatter, and analyzer for SQLite SQL. Building this crate produces `libsyntaqlite` as a static and a shared library that export the whole C API.

`include/syntaqlite.h` declares the whole API. It includes the formatter, analyzer, and RPC headers from this crate's `include/` directory and the parser, tokenizer, and SQLite dialect headers from the `syntaqlite-syntax` crate's, so put both directories on the include path.

Rust users should depend on [`syntaqlite`](https://crates.io/crates/syntaqlite) instead. To use the C API without building it, download the prebuilt library from the [latest release](https://github.com/LalitMaganti/syntaqlite/releases); see [Installing the C libraries](https://docs.syntaqlite.com/latest/guides/c-install/).

On Linux, building the shared library needs lld, which ships with Rust.
