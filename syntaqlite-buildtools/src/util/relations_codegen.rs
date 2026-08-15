// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Generates the Rust catalog of always-present `SQLite` built-in relations.

use std::fmt::Write as _;

use serde::Deserialize;

use super::rust_writer::RustWriter;

#[derive(Deserialize)]
struct RelationsFile {
    relations: Vec<JsonRelation>,
}

#[derive(Deserialize)]
struct JsonRelation {
    name: String,
    columns: Vec<String>,
    without_rowid: bool,
    availability: Vec<JsonAvailability>,
}

#[derive(Deserialize)]
struct JsonAvailability {
    since: String,
    until: Option<String>,
    cflag: Option<String>,
    polarity: Option<String>,
}

/// Read `relations.json`, generate the catalog Rust source, and write it.
pub(crate) fn write_relations_catalog_file(
    json_path: &str,
    output_path: &str,
) -> Result<(), String> {
    use std::fs;
    use std::path::Path;

    let json = fs::read_to_string(json_path).map_err(|e| format!("reading {json_path}: {e}"))?;
    let content = generate_relations_catalog(&json)?;
    let out = Path::new(output_path);
    if let Some(parent) = out.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("creating output directory: {e}"))?;
    }
    fs::write(out, content).map_err(|e| format!("writing {}: {e}", out.display()))?;
    eprintln!("wrote {output_path}");
    Ok(())
}

/// Generate `relations_catalog.rs` from extracted relation JSON.
pub(crate) fn generate_relations_catalog(json_content: &str) -> Result<String, String> {
    let file: RelationsFile =
        serde_json::from_str(json_content).map_err(|e| format!("parsing relations.json: {e}"))?;

    let mut used_polarities = std::collections::BTreeSet::new();
    for relation in &file.relations {
        for availability in &relation.availability {
            match availability.polarity.as_deref() {
                Some("enable") | None => {
                    used_polarities.insert("Enable as E");
                }
                Some("omit") => {
                    used_polarities.insert("Omit as O");
                }
                other => {
                    return Err(format!(
                        "unknown polarity '{other:?}' in relation '{}'",
                        relation.name
                    ));
                }
            }
        }
    }
    let polarity_aliases = used_polarities.into_iter().collect::<Vec<_>>().join(", ");

    let mut w = RustWriter::new();
    w.file_header();
    w.line("//! Static catalog of `SQLite` built-in relations with version/cflag availability.");
    w.newline();
    w.line("use crate::dialect::{AvailabilityRule, BuiltinRelationEntry, BuiltinRelationInfo, CflagPolarity, SqliteVersion};");
    w.newline();
    w.line("use AvailabilityRule as A;");
    w.line("use BuiltinRelationEntry as R;");
    w.line("use BuiltinRelationInfo as I;");
    let _ = writeln!(w, "use CflagPolarity::{{{polarity_aliases}}};");
    w.line("use SqliteVersion as V;");
    w.newline();
    let _ = writeln!(
        w,
        "/// All {} always-present `SQLite` built-in relations.",
        file.relations.len()
    );
    w.line("#[rustfmt::skip]");
    w.line("pub(crate) static SQLITE_RELATIONS: &[BuiltinRelationEntry<'static>] = &[");
    w.indent();
    for relation in &file.relations {
        emit_relation_entry(&mut w, relation)?;
    }
    w.close_block("];");
    w.newline();
    Ok(w.finish())
}

fn emit_relation_entry(w: &mut RustWriter, relation: &JsonRelation) -> Result<(), String> {
    let name = escape(&relation.name);
    let columns = relation
        .columns
        .iter()
        .map(|column| format!("\"{}\"", escape(column)))
        .collect::<Vec<_>>()
        .join(", ");
    let mut rules = Vec::new();
    for availability in &relation.availability {
        let since = super::functions_codegen::short_version(&availability.since)?;
        let until = match &availability.until {
            Some(version) => format!(
                "Some({})",
                super::functions_codegen::short_version(version)?
            ),
            None => "None".to_string(),
        };
        let (cflag_index, polarity) = match &availability.cflag {
            Some(cflag) => {
                let index = super::functions_codegen::cflag_index(cflag).ok_or_else(|| {
                    format!("unknown cflag '{cflag}' in relation '{}'", relation.name)
                })?;
                let polarity = match availability.polarity.as_deref() {
                    Some("enable") => "E",
                    Some("omit") => "O",
                    other => {
                        return Err(format!(
                            "unknown polarity '{other:?}' for cflag '{cflag}' in relation '{}'",
                            relation.name
                        ));
                    }
                };
                (index.to_string(), polarity)
            }
            None => ("u32::MAX".to_string(), "E"),
        };
        rules.push(format!(
            "A {{ since: {since}, until: {until}, cflag_index: {cflag_index}, cflag_polarity: {polarity} }}"
        ));
    }
    let rules = rules.join(", ");
    let without_rowid = relation.without_rowid;
    let _ = writeln!(
        w,
        "R {{ info: I {{ name: \"{name}\", columns: &[{columns}], without_rowid: {without_rowid} }}, availability: &[{rules}] }},"
    );
    Ok(())
}

fn escape(value: &str) -> String {
    value.replace('\\', "\\\\").replace('"', "\\\"")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn generates_relation_with_version_and_cflag() {
        let json = r#"{
          "relations": [{
            "name": "sqlite_temp_schema",
            "columns": ["type", "name"],
            "without_rowid": false,
            "availability": [{
              "since": "3.33.0",
              "cflag": "SQLITE_OMIT_TEMPDB",
              "polarity": "omit"
            }]
          }]
        }"#;
        let generated = generate_relations_catalog(json).unwrap();
        assert!(generated.contains("SQLITE_RELATIONS"));
        assert!(generated.contains("name: \"sqlite_temp_schema\""));
        assert!(generated.contains("columns: &[\"type\", \"name\"]"));
        assert!(generated.contains("cflag_index: 14"));
        assert!(generated.contains("cflag_polarity: O"));
    }
}
