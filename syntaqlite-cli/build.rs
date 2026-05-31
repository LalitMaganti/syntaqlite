// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Build script for `syntaqlite-cli`.

fn main() {
    // On Linux, dialect plugins built with `-DSYNTAQLITE_OMIT_RUNTIME` strip
    // the runtime-side extent hooks (`synq_extent_on_shift` /
    // `synq_extent_on_reduce`) and resolve them against the host binary at
    // `dlopen` time. Those symbols must therefore live in the host's `.dynsym`.
    //
    // We export ONLY those two hooks via `--dynamic-list`, deliberately NOT the
    // whole symbol table via `--export-dynamic`. `--export-dynamic` places every
    // global symbol — including the parser/tokenizer's own `Synq*Parse*`
    // functions — into `.dynsym`, where ELF interposition rules let the host's
    // copies override the identically-named symbols of a *self-contained* plugin
    // (one that bundles its own runtime) at load time. That silently swaps the
    // plugin's grammar for the host's SQLite parser. A scoped dynamic list
    // exposes the hooks without enabling that interposition.
    //
    // macOS uses two-level namespaces (no flat interposition) and exports
    // executable symbols on demand; Windows uses export libraries.
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("linux") {
        let out_dir = std::env::var("OUT_DIR").expect("OUT_DIR set by cargo");
        let list_path = std::path::Path::new(&out_dir).join("dynamic-symbols.txt");
        std::fs::write(
            &list_path,
            "{\n  synq_extent_on_shift;\n  synq_extent_on_reduce;\n};\n",
        )
        .expect("write dynamic symbol list");
        println!(
            "cargo:rustc-link-arg-bins=-Wl,--dynamic-list={}",
            list_path.display()
        );
    }
}
