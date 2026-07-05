// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Build script for `syntaqlite-cli`.

fn main() {
    // On Linux, dialect plugins built with `-DSYNTAQLITE_OMIT_RUNTIME` strip
    // the runtime side and resolve it against the host binary at `dlopen` time.
    // Two groups of runtime symbols therefore have to live in the host's
    // `.dynsym`:
    //
    //   * the extent hooks (`synq_extent_on_shift` / `synq_extent_on_reduce` /
    //     `synq_extent_fold_below_into_top`), referenced by the dialect's
    //     parse tables; and
    //   * the generic `syntaqlite_{parser,tokenizer}_create_with_dialect`
    //     entry points, which the dialect-pinned
    //     `syntaqlite_{parser,tokenizer}_create_<dialect>()` wrappers (emitted
    //     into every amalgamation, OMIT_RUNTIME included) call through.
    //
    // Hardened toolchains (e.g. nixpkgs) link with `-Wl,-z,now`, so *all* of a
    // plugin's undefined symbols must resolve at load — not just the ones it
    // happens to call — which is why the create wrappers must be exported even
    // though the CLI itself creates parsers via its own statically-linked copy.
    //
    // We export ONLY this scoped set via `--dynamic-list`, deliberately NOT the
    // whole symbol table via `--export-dynamic`. `--export-dynamic` places every
    // global symbol — including the parser/tokenizer's own `Synq*Parse*`
    // functions — into `.dynsym`, where ELF interposition rules let the host's
    // copies override the identically-named symbols of a *self-contained* plugin
    // (one that bundles its own runtime) at load time. That silently swaps the
    // plugin's grammar for the host's SQLite parser. The create wrappers are
    // safe to export: they only allocate and dispatch through the passed
    // dialect's function pointers, so interposing them cannot change a grammar.
    //
    // macOS uses two-level namespaces (no flat interposition) and exports
    // executable symbols on demand; Windows uses export libraries.
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("linux") {
        let out_dir = std::env::var("OUT_DIR").expect("OUT_DIR set by cargo");
        let list_path = std::path::Path::new(&out_dir).join("dynamic-symbols.txt");
        std::fs::write(
            &list_path,
            "{\n  \
             synq_extent_on_shift;\n  \
             synq_extent_on_reduce;\n  \
             synq_extent_fold_below_into_top;\n  \
             syntaqlite_parser_create_with_dialect;\n  \
             syntaqlite_tokenizer_create_with_dialect;\n\
             };\n",
        )
        .expect("write dynamic symbol list");
        println!(
            "cargo:rustc-link-arg-bins=-Wl,--dynamic-list={}",
            list_path.display()
        );
    }
}
