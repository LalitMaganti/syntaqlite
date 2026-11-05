# syntaqlite-capi

The C API for [syntaqlite](https://crates.io/crates/syntaqlite), built as `libsyntaqlite`. It exports the formatter, analyzer, and JSON-RPC entry points declared in `include/syntaqlite/*.h` as a static and shared library.

Rust users should depend on [`syntaqlite`](https://crates.io/crates/syntaqlite) instead. To use the C API without building it, download the prebuilt library from the [latest release](https://github.com/LalitMaganti/syntaqlite/releases); see [Using libsyntaqlite from C](https://docs.syntaqlite.com/latest/guides/c-api/).
