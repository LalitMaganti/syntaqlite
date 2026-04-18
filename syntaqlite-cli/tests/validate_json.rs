// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Round-trip tests for the `syntaqlite validate -o json` ndjson schema.

use std::io::Write;
use std::process::{Command, Stdio};

fn bin() -> &'static str {
    env!("CARGO_BIN_EXE_syntaqlite")
}

fn run(args: &[&str], sql: &str) -> (String, String, i32) {
    let mut child = Command::new(bin())
        .args(args)
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .expect("spawn syntaqlite");
    child
        .stdin
        .as_mut()
        .expect("stdin")
        .write_all(sql.as_bytes())
        .expect("write stdin");
    let out = child.wait_with_output().expect("wait");
    (
        String::from_utf8(out.stdout).expect("utf8 stdout"),
        String::from_utf8(out.stderr).expect("utf8 stderr"),
        out.status.code().unwrap_or(-1),
    )
}

fn records(stdout: &str) -> Vec<serde_json::Value> {
    stdout
        .lines()
        .filter(|l| !l.trim().is_empty())
        .map(|l| serde_json::from_str::<serde_json::Value>(l).expect("valid JSON"))
        .collect()
}

#[test]
fn clean_input_emits_no_records_and_exit_zero() {
    let (stdout, _, code) = run(
        &["validate", "-o", "json"],
        "CREATE TABLE t(a INTEGER);\nSELECT a FROM t;\n",
    );
    assert_eq!(code, 0, "clean input should exit 0");
    assert!(
        records(&stdout).is_empty(),
        "clean input should produce no records, got: {stdout}"
    );
}

#[test]
fn parse_error_emits_diagnostic_record_and_exit_one() {
    let (stdout, _, code) = run(&["validate", "-o", "json"], "SELECT FROM;\n");
    assert_eq!(code, 1, "parse error should exit 1");
    let recs = records(&stdout);
    assert!(!recs.is_empty(), "expected at least one diagnostic record");
    let r = &recs[0];
    assert_eq!(r["kind"], "diagnostic");
    assert_eq!(r["schema_version"], 0);
    assert_eq!(r["file"], "<stdin>");
    assert_eq!(r["severity"], "error");
    assert!(r["message"].is_string(), "message must be present");
    assert!(
        r["start_offset"].is_number(),
        "start_offset must be a number"
    );
    assert!(r["end_offset"].is_number(), "end_offset must be a number");
}

#[test]
fn unknown_table_with_schema_is_error_with_help() {
    let (stdout, _, code) = run(
        &["validate", "-o", "json", "-D", "schema"],
        "CREATE TABLE users(id INTEGER);\nSELECT id FROM usr;\n",
    );
    assert_eq!(code, 1, "unknown table under -D schema should exit 1");
    let recs = records(&stdout);
    let unknown = recs
        .iter()
        .find(|r| {
            r["severity"] == "error" && r["message"].as_str().is_some_and(|s| s.contains("usr"))
        })
        .expect("expected an error mentioning unknown table 'usr'");
    assert_eq!(unknown["kind"], "diagnostic");
    // Suggestion "did you mean 'users'?" should flow through as `help`.
    let help = unknown["help"].as_str().unwrap_or("");
    assert!(
        help.contains("users"),
        "help should suggest 'users', got {help:?}"
    );
}

#[test]
fn severity_allow_suppresses_record() {
    let (stdout, _, code) = run(
        &["validate", "-o", "json", "-A", "unknown-table"],
        "SELECT id FROM usr;\n",
    );
    assert_eq!(code, 0, "allow should not error");
    assert!(
        records(&stdout).is_empty(),
        "allow should suppress records, got: {stdout}"
    );
}

#[test]
fn multiple_statements_emit_multiple_records() {
    let (stdout, _, code) = run(
        &["validate", "-o", "json", "-D", "schema"],
        "SELECT id FROM a;\nSELECT x FROM b;\n",
    );
    assert_eq!(code, 1);
    let recs = records(&stdout);
    assert!(recs.len() >= 2, "expected two diagnostics, got {recs:?}");
}
