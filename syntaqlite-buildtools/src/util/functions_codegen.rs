// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Generates a Rust source file containing the `SQLite` built-in function catalog.
//!
//! Reads `functions.json` (extracted from `SQLite` source) and emits a static
//! array of `FunctionEntry` values with availability rules that can be filtered
//! at runtime by `DialectEnv`.

use std::fmt::Write as _;

use serde::Deserialize;

use super::rust_writer::RustWriter;

// ── JSON schema ─────────────────────────────────────────────────────

#[derive(Deserialize)]
struct FunctionsFile {
    functions: Vec<JsonFunction>,
}

#[derive(Deserialize)]
struct JsonFunction {
    name: String,
    arities: Vec<i16>,
    category: String,
    availability: Vec<JsonAvailability>,
}

#[derive(Deserialize)]
struct JsonAvailability {
    since: String,
    until: Option<String>,
    cflag: Option<String>,
    polarity: Option<String>,
}

// ── Version encoding ────────────────────────────────────────────────

/// Convert a version string like `"3.38.5"` to a `SqliteVersion` variant
/// name like `"SqliteVersion::V3_38"`. The patch component is ignored since
/// `SqliteVersion` only tracks major.minor.
fn encode_version(s: &str) -> Result<String, String> {
    let parts: Vec<&str> = s.split('.').collect();
    if parts.len() < 2 {
        return Err(format!(
            "bad version string '{s}': expected at least major.minor"
        ));
    }
    let major: u32 = parts[0]
        .parse()
        .map_err(|e| format!("bad major in version '{s}': {e}"))?;
    let minor: u32 = parts[1]
        .parse()
        .map_err(|e| format!("bad minor in version '{s}': {e}"))?;
    if major != 3 {
        return Err(format!(
            "unsupported major version {major} in '{s}': only SQLite v3 is supported"
        ));
    }
    Ok(format!("SqliteVersion::V3_{minor}"))
}

// ── Cflag name → index mapping ──────────────────────────────────────

/// Map a cflag name (e.g. `"SQLITE_OMIT_JSON"`) to its `SYNQ_CFLAG_IDX_*` index.
///
/// Delegates to [`super::cflag_registry::cflag_index`], which is the single
/// source of truth for cflag index assignments.
pub(crate) fn cflag_index(name: &str) -> Option<u32> {
    super::cflag_registry::cflag_index(name)
}

// ── Code generation ─────────────────────────────────────────────────

/// Read `functions.json` from `json_path`, generate the catalog Rust source, and
/// write it to `output_path` (creating parent directories as needed).
///
/// # Errors
///
/// Returns an error if reading, parsing, or writing fails.
pub(crate) fn write_functions_catalog_file(
    json_path: &str,
    output_path: &str,
) -> Result<(), String> {
    use std::fs;
    use std::path::Path;

    let json = fs::read_to_string(json_path).map_err(|e| format!("reading {json_path}: {e}"))?;
    let content = generate_functions_catalog(&json)?;
    let out = Path::new(output_path);
    if let Some(parent) = out.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("creating output directory: {e}"))?;
    }
    fs::write(out, content).map_err(|e| format!("writing {}: {e}", out.display()))?;
    eprintln!("wrote {output_path}");
    Ok(())
}

/// Generate the `functions_catalog.rs` Rust source from `functions.json` content.
///
/// # Errors
///
/// Returns an error if JSON parsing fails or an unknown category/cflag is encountered.
pub(crate) fn generate_functions_catalog(json_content: &str) -> Result<String, String> {
    let file: FunctionsFile =
        serde_json::from_str(json_content).map_err(|e| format!("parsing functions.json: {e}"))?;

    // Scan categories + polarities used by the catalog so we only emit the
    // aliases we actually reference — unused imports would trigger a warning.
    let mut used_categories: std::collections::BTreeSet<&'static str> =
        std::collections::BTreeSet::new();
    let mut used_polarities: std::collections::BTreeSet<&'static str> =
        std::collections::BTreeSet::new();
    for func in &file.functions {
        let cat = match func.category.as_str() {
            "scalar" => "Scalar as Sc",
            "aggregate" => "Aggregate as Ag",
            "window" => "Window as Wn",
            "table_valued" => "TableValued as Tv",
            other => {
                return Err(format!(
                    "unknown category '{other}' for function '{}'",
                    func.name
                ));
            }
        };
        used_categories.insert(cat);
        for avail in &func.availability {
            match avail.polarity.as_deref() {
                Some("enable") | None => {
                    used_polarities.insert("Enable as E");
                }
                Some("omit") => {
                    used_polarities.insert("Omit as O");
                }
                other => {
                    return Err(format!(
                        "unknown polarity '{:?}' in function '{}'",
                        other, func.name
                    ));
                }
            }
        }
    }
    let cat_aliases = used_categories.into_iter().collect::<Vec<_>>().join(", ");
    let pol_aliases = used_polarities.into_iter().collect::<Vec<_>>().join(", ");

    let mut w = RustWriter::new();
    w.file_header();
    w.line("//! Static catalog of `SQLite` built-in functions with version/cflag availability.");
    w.newline();
    w.line("use crate::dialect::{AvailabilityRule, CflagPolarity, FunctionCategory, FunctionEntry, FunctionInfo, SqliteVersion};");
    w.newline();

    // Aliases keep each FunctionEntry line short so the whole catalog fits on
    // one line per function under `#[rustfmt::skip]`.
    w.line("use AvailabilityRule as A;");
    let _ = writeln!(w, "use CflagPolarity::{{{pol_aliases}}};");
    let _ = writeln!(w, "use FunctionCategory::{{{cat_aliases}}};");
    w.line("use FunctionEntry as F;");
    w.line("use FunctionInfo as I;");
    w.line("use SqliteVersion as V;");
    w.newline();

    // Main catalog array. `#[rustfmt::skip]` keeps each entry on one line so
    // the generated file stays compact (one line per function).
    let count = file.functions.len();
    let _ = writeln!(w, "/// All {count} `SQLite` built-in functions.");
    w.line("#[rustfmt::skip]");
    w.line("pub(crate) static SQLITE_FUNCTIONS: &[FunctionEntry<'static>] = &[");
    w.indent();
    for func in &file.functions {
        emit_function_entry(&mut w, func)?;
    }
    w.close_block("];");
    w.newline();

    Ok(w.finish())
}

fn emit_function_entry(w: &mut RustWriter, func: &JsonFunction) -> Result<(), String> {
    let cat = match func.category.as_str() {
        "scalar" => "Sc",
        "aggregate" => "Ag",
        "window" => "Wn",
        "table_valued" => "Tv",
        other => {
            return Err(format!(
                "unknown category '{other}' for function '{}'",
                func.name
            ));
        }
    };
    let name_escaped = func.name.replace('\\', "\\\\").replace('"', "\\\"");
    let arities = func
        .arities
        .iter()
        .map(ToString::to_string)
        .collect::<Vec<_>>()
        .join(", ");
    let mut rules: Vec<String> = Vec::new();
    for avail in &func.availability {
        let since = short_version(&avail.since)?;
        let until = match &avail.until {
            Some(v) => format!("Some({})", short_version(v)?),
            None => "None".to_string(),
        };
        let (cflag_idx_str, polarity) = match &avail.cflag {
            Some(name) => {
                let idx = cflag_index(name).ok_or_else(|| {
                    format!("unknown cflag '{}' in function '{}'", name, func.name)
                })?;
                let pol = match avail.polarity.as_deref() {
                    Some("enable") => "E",
                    Some("omit") => "O",
                    other => {
                        return Err(format!(
                            "unknown polarity '{:?}' for cflag '{}' in function '{}'",
                            other, name, func.name
                        ));
                    }
                };
                (idx.to_string(), pol)
            }
            None => ("u32::MAX".to_string(), "E"),
        };
        rules.push(format!(
            "A {{ since: {since}, until: {until}, cflag_index: {cflag_idx_str}, cflag_polarity: {polarity} }}"
        ));
    }
    let rules_str = rules.join(", ");
    let _ = writeln!(
        w,
        "F {{ info: I {{ name: \"{name_escaped}\", arities: &[{arities}], category: {cat} }}, availability: &[{rules_str}] }},"
    );
    Ok(())
}

/// Like [`encode_version`] but returns the short `V::V3_NN` form for the
/// re-aliased import in the catalog module.
pub(crate) fn short_version(s: &str) -> Result<String, String> {
    let full = encode_version(s)?;
    // "SqliteVersion::V3_38" -> "V::V3_38"
    Ok(full.replace("SqliteVersion::", "V::"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn generate_from_minimal_json() {
        let json = r#"{
            "functions": [
                {
                    "name": "abs",
                    "arities": [0, 1],
                    "category": "scalar",
                    "availability": [
                        { "since": "3.30.1" }
                    ]
                }
            ]
        }"#;
        let result = generate_functions_catalog(json).unwrap();
        assert!(result.contains("SQLITE_FUNCTIONS"));
        assert!(result.contains("\"abs\""));
        // `Scalar` is aliased to `Sc` in the generated catalog.
        assert!(result.contains("category: Sc"));
        assert!(result.contains("arities: &[0, 1]"));
        assert!(result.contains("use crate::dialect::"));
    }

    #[test]
    fn generate_with_cflag() {
        let json = r#"{
            "functions": [
                {
                    "name": "acos",
                    "arities": [1],
                    "category": "scalar",
                    "availability": [
                        {
                            "since": "3.35.5",
                            "cflag": "SQLITE_ENABLE_MATH_FUNCTIONS",
                            "polarity": "enable"
                        }
                    ]
                }
            ]
        }"#;
        let result = generate_functions_catalog(json).unwrap();
        assert!(result.contains("cflag_index: 36"));
        // `Enable` is aliased to `E` in the generated catalog.
        assert!(result.contains("cflag_polarity: E"));
    }
}
