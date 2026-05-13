// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! End-to-end regression test for the `cdylib` dialect-plugin flow:
//!
//!   1. Generate a full sqlite amalgamation.
//!   2. Compile it as a shared library with `-DSYNTAQLITE_OMIT_RUNTIME`,
//!      which strips the runtime-side hooks (`synq_extent_on_shift`,
//!      `synq_extent_on_reduce`) — they must be resolved against the host
//!      binary at `dlopen` time.
//!   3. Load it through `syntaqlite --dialect <path> --dialect-name sqlite`
//!      and parse a query.
//!
//! Without `build.rs` linking the host binary with `-Wl,--export-dynamic`,
//! Linux loaders fail step 3 with:
//!
//!     symbol lookup error: ...: undefined symbol: synq_extent_on_shift
//!
//! macOS executables export symbols by default, so the test passes there
//! regardless of the build.rs flag — but it still exercises the OMIT_RUNTIME
//! .dylib loading path.

use std::path::{Path, PathBuf};
use std::process::Command;

fn bin() -> &'static str {
    env!("CARGO_BIN_EXE_syntaqlite")
}

fn run(cmd: &mut Command, ctx: &str) {
    let out = cmd
        .output()
        .unwrap_or_else(|e| panic!("{ctx}: spawn failed: {e}"));
    assert!(
        out.status.success(),
        "{ctx} failed (exit={:?})\nstdout:\n{}\nstderr:\n{}",
        out.status.code(),
        String::from_utf8_lossy(&out.stdout),
        String::from_utf8_lossy(&out.stderr),
    );
}

fn shared_lib_path(dir: &Path, stem: &str) -> PathBuf {
    let ext = if cfg!(target_os = "macos") {
        "dylib"
    } else {
        "so"
    };
    dir.join(format!("lib{stem}.{ext}"))
}

#[test]
#[cfg_attr(
    target_os = "windows",
    ignore = "dlopen export model differs on Windows"
)]
fn omit_runtime_dialect_plugin_resolves_against_host() {
    let tmp = tempfile::TempDir::new().expect("tempdir");
    let amalg_dir = tmp.path();

    // 1. Generate the full sqlite amalgamation (syntaqlite_sqlite.{h,c}).
    run(
        Command::new(bin())
            .args([
                "dialect",
                "generate",
                "--name",
                "sqlite",
                "--output-type",
                "full",
            ])
            .arg("--output-dir")
            .arg(amalg_dir),
        "syntaqlite dialect generate",
    );

    let src = amalg_dir.join("syntaqlite_sqlite.c");
    assert!(
        src.exists(),
        "amalgamation source not written: {}",
        src.display()
    );

    // 2. Compile as a shared library with OMIT_RUNTIME defined. This strips
    //    `synq_extent_on_*` from the .so, requiring the host binary to
    //    provide them at load time.
    //
    //    Linux: `ld` permits undefined symbols in shared objects by default.
    //    macOS: `ld` rejects them unless `-undefined dynamic_lookup` defers
    //           resolution to `dlopen` time.
    let lib = shared_lib_path(amalg_dir, "sqlite_dialect_plugin");
    let mut cc = Command::new("cc");
    cc.args(["-shared", "-fPIC", "-DSYNTAQLITE_OMIT_RUNTIME"])
        .arg("-I")
        .arg(amalg_dir)
        .arg("-o")
        .arg(&lib)
        .arg(&src);
    if cfg!(target_os = "macos") {
        cc.args(["-Wl,-undefined,dynamic_lookup"]);
    }
    run(&mut cc, "cc -shared with -DSYNTAQLITE_OMIT_RUNTIME");

    // 3. Load the plugin through the CLI and parse a query. On Linux,
    //    without `-Wl,--export-dynamic` on the host binary, this fails
    //    inside `dlopen` (lazy-bind) or on first parse call with
    //    `undefined symbol: synq_extent_on_shift`.
    let out = Command::new(bin())
        .arg("--dialect")
        .arg(&lib)
        .args(["--dialect-name", "sqlite", "fmt", "-e", "SELECT 1"])
        .output()
        .expect("spawn syntaqlite --dialect");

    let stdout = String::from_utf8_lossy(&out.stdout);
    let stderr = String::from_utf8_lossy(&out.stderr);
    assert!(
        out.status.success(),
        "loading OMIT_RUNTIME dialect plugin failed — host likely lacks \
         --export-dynamic (see syntaqlite-cli/build.rs)\n\
         stdout:\n{stdout}\nstderr:\n{stderr}",
    );
    assert!(
        stdout.to_uppercase().contains("SELECT"),
        "expected formatted SELECT in stdout; got:\n{stdout}",
    );
}
