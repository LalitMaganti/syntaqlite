// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Tests for the `CliApp` trait and the runtime visibility rules it controls.
//!
//! Exercised via the `cli_wrapper` example, which implements `CliApp` with
//! both override surfaces disabled. Cargo doesn't auto-build examples for
//! `cargo test`, so we build it on demand and locate the binary relative to
//! this test's path.

use std::path::PathBuf;
use std::process::Command;
use std::sync::OnceLock;

static EXAMPLE_PATH: OnceLock<PathBuf> = OnceLock::new();

fn example_bin() -> &'static PathBuf {
    EXAMPLE_PATH.get_or_init(|| {
        let status = Command::new(env!("CARGO"))
            .args([
                "build",
                "--manifest-path",
                concat!(env!("CARGO_MANIFEST_DIR"), "/Cargo.toml"),
                "--example",
                "cli_wrapper",
            ])
            .status()
            .expect("build cli_wrapper example");
        assert!(status.success(), "cargo build --example cli_wrapper failed");

        // current_exe is target/<profile>/deps/<test>-<hash>; the example
        // lands at target/<profile>/examples/cli_wrapper.
        let test_exe = std::env::current_exe().expect("current_exe");
        let deps = test_exe.parent().expect("deps dir");
        let profile = deps.parent().expect("profile dir");
        profile.join("examples").join(if cfg!(windows) {
            "cli_wrapper.exe"
        } else {
            "cli_wrapper"
        })
    })
}

fn run(args: &[&str]) -> (String, String, i32) {
    let out = Command::new(example_bin())
        .args(args)
        .output()
        .expect("spawn cli_wrapper");
    (
        String::from_utf8(out.stdout).expect("utf8 stdout"),
        String::from_utf8(out.stderr).expect("utf8 stderr"),
        out.status.code().unwrap_or(-1),
    )
}

#[test]
fn help_uses_app_name_and_about() {
    let (stdout, _stderr, code) = run(&["--help"]);
    assert_eq!(code, 0);
    assert!(
        stdout.contains("syntaqlite-example"),
        "expected app name in help, got:\n{stdout}"
    );
    assert!(
        stdout.contains("Example wrapper that pre-specifies"),
        "expected app about in help, got:\n{stdout}"
    );
}

#[test]
fn dialect_override_hidden_from_help() {
    let (stdout, _, code) = run(&["--help"]);
    assert_eq!(code, 0);
    assert!(
        !stdout.contains("--dialect "),
        "--dialect should be hidden when allow_dialect_override=false:\n{stdout}"
    );
}

#[test]
fn dialect_override_rejected_when_disabled() {
    let (_, stderr, code) = run(&["--dialect", "/tmp/fake.so", "parse", "-e", "SELECT 1"]);
    assert_eq!(code, 2, "stderr was: {stderr}");
    assert!(
        stderr.contains("--dialect is not supported"),
        "stderr: {stderr}"
    );
}

#[test]
fn sqlite_tuning_rejected_when_disabled() {
    let (_, stderr, code) = run(&["--sqlite-version", "3.47.0", "parse", "-e", "SELECT 1"]);
    assert_eq!(code, 2, "stderr was: {stderr}");
    assert!(
        stderr.contains("--sqlite-version is not supported"),
        "stderr: {stderr}"
    );
}

#[test]
fn baked_dialect_handles_command_without_override() {
    let (stdout, _, code) = run(&["parse", "-e", "SELECT 1", "-o", "summary"]);
    assert_eq!(code, 0);
    assert!(stdout.contains("1 statements parsed"), "stdout: {stdout}");
}
