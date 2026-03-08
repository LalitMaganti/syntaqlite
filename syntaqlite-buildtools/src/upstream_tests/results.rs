// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Result types for upstream test log parsing and aggregation.

use serde::{Deserialize, Serialize};

/// A single log entry from the C extension (one per SQL statement).
#[derive(Debug, Deserialize)]
pub struct LogEntry {
    /// The SQL statement that was evaluated.
    pub sql: String,
    /// Whether `sqlite3_prepare_v2()` succeeded.
    pub sqlite_ok: bool,
    /// Error message from `sqlite3_prepare_v2()`, if it failed.
    #[serde(default)]
    pub sqlite_error: Option<String>,
    /// Whether syntaqlite's parser accepted the SQL.
    pub parse_ok: bool,
    /// Error message from syntaqlite's parser, if it failed.
    #[serde(default)]
    pub parse_error: Option<String>,
    /// Diagnostics from syntaqlite's validator (empty if parsing failed).
    #[serde(default)]
    pub diagnostics: Option<Vec<DiagnosticEntry>>,
}

/// A single diagnostic from syntaqlite's validator.
#[derive(Debug, Deserialize)]
pub struct DiagnosticEntry {
    /// Severity level (0=error, 1=warning, 2=info, 3=hint).
    pub severity: u32,
    /// Human-readable diagnostic message.
    pub message: String,
    /// Byte offset of the start of the diagnostic range.
    pub start: u32,
    /// Byte offset of the end of the diagnostic range.
    pub end: u32,
}

/// Results from a single test file.
pub struct FileResult {
    /// Test file name (e.g., `select1.test`).
    pub file: String,
    /// Per-statement log entries.
    pub entries: Vec<LogEntry>,
    /// Error message if tclsh failed to run the test file.
    pub error: Option<String>,
}

/// Aggregated summary of all test results.
#[derive(Debug, Serialize, Deserialize)]
pub struct Summary {
    /// Total number of SQL statements evaluated.
    pub total: u64,
    /// Statements that syntaqlite parsed successfully.
    pub parse_ok: u64,
    /// Statements that syntaqlite failed to parse.
    pub parse_error: u64,
    /// Statements accepted by both `SQLite` and syntaqlite.
    pub both_accept: u64,
    /// Statements rejected by both `SQLite` and syntaqlite.
    pub both_reject: u64,
    /// Statements accepted by `SQLite` but rejected by syntaqlite.
    pub false_positive: u64,
    /// Statements rejected by `SQLite` but accepted by syntaqlite.
    pub gap: u64,
}

impl Summary {
    /// Aggregate results from all file results.
    pub fn from_results(results: &[FileResult]) -> Self {
        let mut summary = Summary {
            total: 0,
            parse_ok: 0,
            parse_error: 0,
            both_accept: 0,
            both_reject: 0,
            false_positive: 0,
            gap: 0,
        };

        for file_result in results {
            for entry in &file_result.entries {
                summary.total += 1;

                if entry.parse_ok {
                    summary.parse_ok += 1;
                } else {
                    summary.parse_error += 1;
                }

                let syntaqlite_ok = entry.parse_ok
                    && entry
                        .diagnostics
                        .as_ref()
                        .is_none_or(Vec::is_empty);

                match (entry.sqlite_ok, syntaqlite_ok) {
                    (true, true) => summary.both_accept += 1,
                    (false, false) => summary.both_reject += 1,
                    (true, false) => summary.false_positive += 1,
                    (false, true) => summary.gap += 1,
                }
            }
        }

        summary
    }
}
