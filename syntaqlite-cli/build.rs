// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Build script for `syntaqlite-cli`.

fn main() {
    // On Linux, link the `syntaqlite` binary with `-Wl,--export-dynamic` so
    // its symbol table is placed in `.dynsym` and visible to `dlopen`'d
    // dialect plugins. Without this, a dialect `.so` built with
    // `-DSYNTAQLITE_OMIT_RUNTIME` (which strips the runtime-side
    // `synq_extent_on_*` hooks, expecting the host binary to provide them)
    // fails at load time with an undefined-symbol error.
    //
    // macOS exports executable symbols by default; Windows uses a different
    // model (export libraries / `__declspec(dllexport)`).
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("linux") {
        println!("cargo:rustc-link-arg-bins=-Wl,--export-dynamic");
    }
}
