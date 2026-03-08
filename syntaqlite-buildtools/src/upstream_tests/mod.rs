// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Upstream `SQLite` test runner.
//!
//! Discovers `.test` files from the `SQLite` source tree, runs each through
//! `tclsh` with our custom extension and shim, collects JSON log output,
//! and compares results against a baseline.

pub mod results;

use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::path::{Path, PathBuf};
use std::process::Command;

use results::{FileResult, LogEntry, Summary};

/// Configuration for a test run.
pub struct RunConfig {
    /// Path to the `SQLite` source tree `test/` directory.
    pub test_dir: PathBuf,
    /// Path to the compiled `tclsyntaqlite` shared library.
    pub extension_lib: PathBuf,
    /// Path to `tester_shim.tcl`.
    pub tester_shim: PathBuf,
    /// Path to baseline file (optional — created on first run).
    pub baseline: Option<PathBuf>,
    /// Update baseline with current results.
    pub rebaseline: bool,
    /// Filter test files by glob pattern (e.g., "select*").
    pub filter: Option<String>,
    /// Number of parallel jobs.
    pub jobs: usize,
    /// Enable validation (not just parsing).
    pub validate: bool,
}

/// Run the upstream test suite.
///
/// # Errors
///
/// Returns an error if no test files are found, tclsh fails to run,
/// or baseline comparison detects regressions.
pub fn run(config: &RunConfig) -> Result<Summary, String> {
    // Discover test files.
    let test_files = discover_test_files(&config.test_dir, config.filter.as_deref())?;
    if test_files.is_empty() {
        return Err("No test files found".into());
    }
    eprintln!("Found {} test files", test_files.len());

    // Run tests (sequentially for now; parallelism can be added later).
    let mut file_results = Vec::new();
    for (i, test_file) in test_files.iter().enumerate() {
        let name = test_file
            .file_name()
            .unwrap_or_default()
            .to_string_lossy()
            .into_owned();
        eprint!("\r[{}/{}] {name}...", i + 1, test_files.len());

        match run_single_test(config, test_file) {
            Ok(result) => file_results.push(result),
            Err(e) => {
                file_results.push(FileResult {
                    file: name,
                    entries: Vec::new(),
                    error: Some(e),
                });
            }
        }
    }
    eprintln!(); // Clear progress line.

    // Aggregate summary.
    let summary = Summary::from_results(&file_results);

    // Print summary.
    eprintln!();
    eprintln!("=== Upstream Test Summary ===");
    eprintln!("Files run:            {}", file_results.len());
    eprintln!(
        "Files with errors:    {}",
        file_results.iter().filter(|r| r.error.is_some()).count()
    );
    eprintln!();
    eprintln!("Total SQL statements: {}", summary.total);
    eprintln!("  Parse OK:           {}", summary.parse_ok);
    eprintln!("  Parse error:        {}", summary.parse_error);
    eprintln!();
    eprintln!(
        "  Both accept:        {} (agreement)",
        summary.both_accept
    );
    eprintln!(
        "  Both reject:        {} (agreement)",
        summary.both_reject
    );
    eprintln!(
        "  False positives:    {} (syntaqlite rejects valid SQL)",
        summary.false_positive
    );
    eprintln!(
        "  Gaps:               {} (syntaqlite misses prepare-time error)",
        summary.gap
    );

    // Baseline comparison.
    if let Some(baseline_path) = &config.baseline {
        if config.rebaseline {
            write_baseline(baseline_path, &summary)?;
            eprintln!("\nBaseline written to {}", baseline_path.display());
        } else if baseline_path.exists() {
            let old = read_baseline(baseline_path)?;
            let regressions = compare_baselines(&old, &summary);
            if regressions > 0 {
                eprintln!("\n{regressions} regression(s) detected!");
                return Err(format!("{regressions} regression(s) from baseline"));
            }
            eprintln!("\nNo regressions from baseline.");
        } else {
            write_baseline(baseline_path, &summary)?;
            eprintln!(
                "\nNo baseline found. Created initial baseline at {}",
                baseline_path.display()
            );
        }
    }

    Ok(summary)
}

/// Discover `.test` files in the test directory.
fn discover_test_files(test_dir: &Path, filter: Option<&str>) -> Result<Vec<PathBuf>, String> {
    let entries = fs::read_dir(test_dir)
        .map_err(|e| format!("Failed to read test directory {}: {e}", test_dir.display()))?;

    let mut files: Vec<PathBuf> = entries
        .filter_map(Result::ok)
        .map(|e| e.path())
        .filter(|p| p.extension().is_some_and(|ext| ext == "test"))
        .filter(|p| {
            if let Some(pat) = filter {
                let name = p.file_stem().unwrap_or_default().to_string_lossy();
                name.contains(pat)
            } else {
                true
            }
        })
        .collect();

    files.sort();
    Ok(files)
}

/// Run a single test file through tclsh.
fn run_single_test(config: &RunConfig, test_file: &Path) -> Result<FileResult, String> {
    let name = test_file
        .file_name()
        .unwrap_or_default()
        .to_string_lossy()
        .into_owned();

    // Create a temporary log file for this test.
    let log_file = std::env::temp_dir().join(format!("syntaqlite_test_{name}.jsonl"));

    // Build the tclsh command.
    // The script loads the extension, sources the shim, then sources the test file.
    let script = format!(
        "load {} Tclsyntaqlite\n\
         source {}\n\
         source {}\n\
         syntaqlite_summary\n",
        config.extension_lib.display(),
        config.tester_shim.display(),
        test_file.display(),
    );

    let output = Command::new("tclsh")
        .env("SYNTAQLITE_TEST_LOG", &log_file)
        .env(
            "SYNTAQLITE_TEST_VALIDATE",
            if config.validate { "1" } else { "0" },
        )
        .env("tcl_interactive", "0")
        .arg("-")
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::piped())
        .stderr(std::process::Stdio::piped())
        .spawn()
        .and_then(|mut child| {
            if let Some(ref mut stdin) = child.stdin {
                stdin.write_all(script.as_bytes()).ok();
            }
            child.wait_with_output()
        })
        .map_err(|e| format!("Failed to run tclsh for {name}: {e}"))?;

    // Parse log entries.
    let entries = if log_file.exists() {
        parse_log_file(&log_file)?
    } else {
        Vec::new()
    };

    // Clean up temp file.
    let _ = fs::remove_file(&log_file);

    Ok(FileResult {
        file: name,
        entries,
        error: if output.status.success() {
            None
        } else {
            let stderr = String::from_utf8_lossy(&output.stderr);
            Some(format!(
                "tclsh exited with status {}: {}",
                output.status,
                stderr.chars().take(500).collect::<String>()
            ))
        },
    })
}

/// Parse JSON lines log file.
fn parse_log_file(path: &Path) -> Result<Vec<LogEntry>, String> {
    let file =
        fs::File::open(path).map_err(|e| format!("Failed to open log {}: {e}", path.display()))?;
    let reader = BufReader::new(file);

    let mut entries = Vec::new();
    for line in reader.lines() {
        let line = line.map_err(|e| format!("Failed to read log line: {e}"))?;
        if line.trim().is_empty() {
            continue;
        }
        match serde_json::from_str::<LogEntry>(&line) {
            Ok(entry) => entries.push(entry),
            Err(e) => {
                // Skip malformed entries.
                eprintln!("Warning: malformed log entry: {e}");
            }
        }
    }
    Ok(entries)
}

/// Write baseline to a JSON file.
fn write_baseline(path: &Path, summary: &Summary) -> Result<(), String> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)
            .map_err(|e| format!("Failed to create baseline directory: {e}"))?;
    }
    let json = serde_json::to_string_pretty(summary)
        .map_err(|e| format!("Failed to serialize baseline: {e}"))?;
    fs::write(path, json).map_err(|e| format!("Failed to write baseline: {e}"))
}

/// Read baseline from a JSON file.
fn read_baseline(path: &Path) -> Result<Summary, String> {
    let content =
        fs::read_to_string(path).map_err(|e| format!("Failed to read baseline: {e}"))?;
    serde_json::from_str(&content).map_err(|e| format!("Failed to parse baseline: {e}"))
}

/// Compare current results against baseline. Returns number of regressions.
fn compare_baselines(old: &Summary, new: &Summary) -> u64 {
    let mut regressions: u64 = 0;

    // A regression is when false_positive count increases (syntaqlite
    // started rejecting SQL that `SQLite` accepts).
    if new.false_positive > old.false_positive {
        eprintln!(
            "  Regression: false_positive increased from {} to {}",
            old.false_positive, new.false_positive
        );
        regressions += new.false_positive - old.false_positive;
    }

    // Also flag if parse_ok decreased.
    if new.parse_ok < old.parse_ok {
        eprintln!(
            "  Regression: parse_ok decreased from {} to {}",
            old.parse_ok, new.parse_ok
        );
        regressions += old.parse_ok - new.parse_ok;
    }

    regressions
}
