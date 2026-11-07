// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Minimal build script: compile vendored C tools into the binary.
//!
use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    let manifest_dir = env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR must be set");
    let sources_dir = PathBuf::from(&manifest_dir).join("sqlite-vendored/sources");

    // Compile lemon.c (parser generator tool, called as subprocess).
    cc::Build::new()
        .file(sources_dir.join("lemon.c"))
        .define("main", "lemon_main")
        .flag_if_supported("-Wno-type-limits")
        .compile("lemon");

    println!("cargo:rerun-if-changed=sqlite-vendored/sources/lemon.c");
}
