// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::path::PathBuf;
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

#[test]
fn syntaqlite_syntax_no_sqlite_feature_path_compiles() {
    let repo_root = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .expect("syntaqlite-buildtools should live inside workspace root")
        .to_path_buf();

    let nonce = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system clock before UNIX_EPOCH")
        .as_nanos();
    let target_dir = std::env::temp_dir().join(format!(
        "syntaqlite-no-sqlite-check-{}-{nonce}",
        std::process::id()
    ));

    let output = Command::new("cargo")
        .current_dir(&repo_root)
        .arg("check")
        .arg("-p")
        .arg("syntaqlite-syntax")
        .arg("--no-default-features")
        .env("CARGO_TARGET_DIR", &target_dir)
        .env("RUSTC_WRAPPER", "")
        .output()
        .expect("failed to run cargo check for no-sqlite path");
    let _ = std::fs::remove_dir_all(&target_dir);

    if !output.status.success() {
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        panic!(
            "cargo check -p syntaqlite-syntax --no-default-features failed\nstdout:\n{stdout}\nstderr:\n{stderr}"
        );
    }
}
