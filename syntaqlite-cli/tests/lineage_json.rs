// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Round-trip tests for the lineage JSON schema emitted by
//! `syntaqlite lineage -o json`.

use std::io::Write;
use std::process::{Command, Stdio};

fn bin() -> &'static str {
    env!("CARGO_BIN_EXE_syntaqlite")
}

fn run_lineage(args: &[&str], sql: &str) -> (String, String, i32) {
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
fn emits_schema_version_and_stdin_file_name() {
    let (stdout, _, code) = run_lineage(
        &["lineage", "-o", "json"],
        "CREATE TABLE t(a INTEGER);\nSELECT a FROM t;\n",
    );
    assert_eq!(code, 0, "unexpected non-zero exit");
    let recs = records(&stdout);
    assert_eq!(recs.len(), 2);
    for r in &recs {
        assert_eq!(r["schema_version"], 0);
        assert_eq!(r["file"], "<stdin>");
        assert_eq!(r["kind"], "lineage");
    }
    assert_eq!(recs[1]["columns"][0]["origin"]["table"], "t");
    assert_eq!(recs[1]["columns"][0]["origin"]["column"], "a");
}

#[test]
fn error_record_on_parse_failure_and_exit_one() {
    let (stdout, _, code) = run_lineage(&["lineage", "-o", "json"], "SELECT FROM;\n");
    assert_eq!(code, 1, "exit code should be 1 on error");
    let recs = records(&stdout);
    assert_eq!(recs.len(), 1);
    assert_eq!(recs[0]["kind"], "error");
    assert_eq!(recs[0]["stage"], "parse");
    assert_eq!(recs[0]["schema_version"], 0);
}

#[test]
fn scope_tables_drops_columns_field() {
    let (stdout, _, _) = run_lineage(
        &["lineage", "-o", "json", "tables"],
        "CREATE TABLE t(a INTEGER);\nSELECT a FROM t;\n",
    );
    let recs = records(&stdout);
    assert_eq!(recs.len(), 2);
    for r in &recs {
        assert!(r.get("columns").is_none(), "columns should be dropped");
        assert!(r.get("relations").is_some(), "relations should be present");
        assert!(
            r.get("physical_tables").is_some(),
            "physical_tables should be present"
        );
    }
}

#[test]
fn scope_columns_drops_tables_fields() {
    let (stdout, _, _) = run_lineage(
        &["lineage", "-o", "json", "columns"],
        "CREATE TABLE t(a INTEGER);\nSELECT a FROM t;\n",
    );
    let recs = records(&stdout);
    assert_eq!(recs.len(), 2);
    for r in &recs {
        assert!(r.get("columns").is_some(), "columns should be present");
        assert!(r.get("relations").is_none(), "relations should be dropped");
        assert!(
            r.get("physical_tables").is_none(),
            "physical_tables should be dropped"
        );
    }
}

#[test]
fn unexpanded_view_marks_partial() {
    let (stdout, _, _) = run_lineage(
        &["lineage", "-o", "json"],
        "CREATE TABLE users (id INTEGER);\n\
         CREATE VIEW u AS SELECT id FROM users;\n\
         SELECT id FROM u;\n",
    );
    let recs = records(&stdout);
    let last = recs.last().expect("at least one record");
    assert_eq!(last["status"], "partial");
    let reasons = last["partial_reasons"].as_array().expect("reasons array");
    assert_eq!(reasons.len(), 1);
    assert_eq!(reasons[0]["code"], "unexpanded_view");
    assert_eq!(reasons[0]["view"], "u");
}
