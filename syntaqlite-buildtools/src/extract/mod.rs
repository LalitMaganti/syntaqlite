// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 1: Extract C fragments from raw `SQLite` source.
//!
//! This module is only compiled when the `sqlite-extract` feature is enabled.
//! It reads raw `SQLite` source files (tokenize.c, global.c, sqliteInt.h,
//! mkkeywordhash.c) and produces the committed fragment files in
//! `data/sqlite_fragments/`.

/// `SQLite`'s public domain blessing header, prepended to all extracted fragments
/// to preserve proper attribution.
pub(crate) const SQLITE_BLESSING: &str = "\
/*
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
*/
";

pub(crate) mod amalgamation_probe;
pub(crate) mod base_files;
pub(crate) mod functions;
pub(crate) mod keywords_and_parser;
pub(crate) mod mkkeywordhash;
pub(crate) mod tokenizer;
pub(crate) mod virtual_tables;

use crate::util::cflag_registry::{CFLAG_REGISTRY, synq_const_name};

/// Compute the group-local index of `sqlite_flag` within the `group` category.
///
/// Local indices are 0, 1, 2, … assigned by iterating `CFLAG_REGISTRY` in order
/// and counting only entries whose categories slice contains `group`.
#[must_use]
pub(crate) fn group_local_index(group: &str, sqlite_flag: &str) -> Option<u32> {
    let mut local = 0u32;
    for &(name, _, cats) in CFLAG_REGISTRY {
        if cats.contains(&group) {
            if name == sqlite_flag {
                return Some(local);
            }
            local += 1;
        }
    }
    None
}

fn cflag_field_name(synq_name: &str) -> String {
    synq_name
        .strip_prefix("SYNQ_CFLAG_IDX_")
        .unwrap_or(synq_name)
        .to_lowercase()
}

fn write_cflag_defines(out: &mut String, entries: &[(&str, &str, u32)], count: u32) {
    use std::fmt::Write as _;
    out.push_str("// ── Cflag index constants ───────────────────────────────────────────────\n");
    out.push_str("//\n");
    out.push_str("// These are group-local indices used in keyword/parser tables.\n");
    out.push('\n');
    let mut last_prefix = "";
    for &(_, synq_name, idx) in entries {
        let prefix = if synq_name.contains("OMIT") {
            "OMIT"
        } else {
            "ENABLE"
        };
        if prefix != last_prefix {
            if !last_prefix.is_empty() {
                out.push('\n');
            }
            writeln!(out, "// {prefix} flags:").expect("write to String");
            last_prefix = prefix;
        }
        writeln!(out, "#define {synq_name} {idx}").expect("write to String");
    }
    out.push('\n');
    writeln!(out, "#define SYNQ_CFLAG_IDX_COUNT {count}").expect("write to String");
    out.push('\n');
}

fn write_cflag_struct(
    out: &mut String,
    entries: &[(&str, &str, u32)],
    bits_total: u32,
    padding: u32,
    byte_count: u32,
) {
    use std::fmt::Write as _;
    out.push_str("// ── Named bitfield struct ───────────────────────────────────────────────\n");
    out.push('\n');
    out.push_str("typedef struct SyntaqliteCflags {\n");
    let mut last_prefix = "";
    for &(_, synq_name, _) in entries {
        let prefix = if synq_name.contains("OMIT") {
            "OMIT"
        } else {
            "ENABLE"
        };
        if prefix != last_prefix {
            writeln!(out, "  // {prefix} flags:").expect("write to String");
            last_prefix = prefix;
        }
        let field = cflag_field_name(synq_name);
        writeln!(out, "  uint8_t {field} : 1;").expect("write to String");
    }
    if padding > 0 {
        writeln!(
            out,
            "  // Padding to {bits_total} bits ({byte_count} bytes):"
        )
        .expect("write to String");
        writeln!(out, "  uint8_t _reserved : {padding};").expect("write to String");
    }
    out.push_str("} SyntaqliteCflags;\n");
    out.push('\n');
    out.push_str("#define SYNQ_CFLAGS_DEFAULT {0}\n");
    out.push('\n');
}

fn write_cflag_pinning(out: &mut String, entries: &[(&str, &str, u32)]) {
    use std::fmt::Write as _;
    out.push_str("// ── Compile-time cflag pinning ──────────────────────────────────────────\n");
    out.push_str("//\n");
    out.push_str("// When SYNTAQLITE_SQLITE_CFLAGS is defined, a static const struct is built\n");
    out.push_str("// from individual SYNTAQLITE_CFLAG_* defines.\n");
    out.push('\n');
    out.push_str("#ifdef SYNTAQLITE_SQLITE_CFLAGS\n");
    out.push_str("static const SyntaqliteCflags synq_pinned_cflags = {\n");
    for &(sqlite_flag, synq_name, _) in entries {
        let field = cflag_field_name(synq_name);
        writeln!(out, "#ifdef SYNTAQLITE_CFLAG_{sqlite_flag}").expect("write to String");
        writeln!(out, "    .{field} = 1,").expect("write to String");
        out.push_str("#endif\n");
    }
    out.push_str("};\n");
    out.push_str("#endif  // SYNTAQLITE_SQLITE_CFLAGS\n");
}

/// Generate the `cflags.h` C header for a given cflag group.
///
/// Only entries whose categories contain `group` are included.
/// Indices are re-assigned 0, 1, 2, … (group-local) in table order.
///
/// # Panics
///
/// Never in practice; panics only if the number of cflags exceeds `u32::MAX`.
#[must_use]
pub(crate) fn generate_cflags_h(group: &str) -> String {
    use std::fmt::Write as _;
    let mut local_idx: u32 = 0;
    let entries: Vec<(&str, String, u32)> = CFLAG_REGISTRY
        .iter()
        .filter_map(|&(sqlite_flag, _, cats)| {
            if cats.contains(&group) {
                let idx = local_idx;
                local_idx += 1;
                Some((sqlite_flag, synq_const_name(sqlite_flag), idx))
            } else {
                None
            }
        })
        .collect();
    let entries_ref: Vec<(&str, &str, u32)> = entries
        .iter()
        .map(|(f, n, i)| (*f, n.as_str(), *i))
        .collect();

    let count = u32::try_from(entries_ref.len()).expect("cflag count fits u32");
    let bits_total = count.div_ceil(8) * 8;
    let padding = bits_total - count;
    let byte_count = bits_total / 8;

    let mut out = String::new();
    out.push_str("// Copyright 2025 The syntaqlite Authors. All rights reserved.\n");
    out.push_str("// Licensed under the Apache License, Version 2.0.\n");
    out.push_str("// @generated by syntaqlite-buildtools codegen-sqlite — DO NOT EDIT\n");
    out.push_str("//\n");
    writeln!(
        out,
        "// SQLite compile-time flag constants for the \"{group}\" group."
    )
    .expect("write to String");
    out.push_str("// For use with SyntaqliteGrammar.cflags.\n");
    out.push_str("//\n");
    out.push_str("// Indices are group-local (0-based within this group).\n");
    out.push('\n');
    out.push_str("#ifndef SYNTAQLITE_SQLITE_CFLAGS_H\n");
    out.push_str("#define SYNTAQLITE_SQLITE_CFLAGS_H\n");
    out.push('\n');
    out.push_str("#include <stdint.h>\n");
    out.push_str("#include <string.h>\n");
    out.push('\n');
    write_cflag_defines(&mut out, &entries_ref, count);
    write_cflag_struct(&mut out, &entries_ref, bits_total, padding, byte_count);
    out.push_str("// ── Indexed accessor ────────────────────────────────────────────────────\n");
    out.push_str("//\n");
    out.push_str("// For dynamic cflag lookup (keyword tables etc.).\n");
    out.push('\n');
    out.push_str("static inline int synq_has_cflag(const SyntaqliteCflags* c, int idx) {\n");
    out.push_str("  const uint8_t* bytes = (const uint8_t*)c;\n");
    out.push_str("  return (bytes[idx / 8] >> (idx % 8)) & 1;\n");
    out.push_str("}\n");
    out.push('\n');
    out.push_str("static inline void synq_set_cflag(SyntaqliteCflags* c, int idx) {\n");
    out.push_str("  uint8_t* bytes = (uint8_t*)c;\n");
    out.push_str("  bytes[idx / 8] |= (uint8_t)(1u << (idx % 8));\n");
    out.push_str("}\n");
    out.push('\n');
    write_cflag_pinning(&mut out, &entries_ref);
    out.push('\n');
    out.push_str("#endif  // SYNTAQLITE_SQLITE_CFLAGS_H\n");
    out
}

#[cfg(test)]
mod tests {
    use crate::util::cflag_registry::CFLAG_REGISTRY;

    /// `CFLAG_REGISTRY` flag names must match `version_cflags.json` exactly.
    ///
    /// Catches two directions of drift:
    /// - A flag added to `CFLAG_REGISTRY` without re-running `tools/sqlite-data update-data`
    /// - The JSON being regenerated from a different flag set than the registry
    #[test]
    fn cflag_registry_matches_version_cflags_json() {
        #[derive(serde::Deserialize)]
        struct File {
            cflags: Vec<Entry>,
        }
        #[derive(serde::Deserialize)]
        struct Entry {
            name: String,
        }

        let json_content = include_str!("../../sqlite-vendored/data/version_cflags.json");
        let file: File =
            serde_json::from_str(json_content).expect("version_cflags.json is valid JSON");

        let json_names: std::collections::HashSet<&str> =
            file.cflags.iter().map(|e| e.name.as_str()).collect();
        let registry_names: std::collections::HashSet<&str> =
            CFLAG_REGISTRY.iter().map(|(n, _, _)| *n).collect();

        let mut in_registry_not_json: Vec<&str> =
            registry_names.difference(&json_names).copied().collect();
        in_registry_not_json.sort_unstable();
        let mut in_json_not_registry: Vec<&str> =
            json_names.difference(&registry_names).copied().collect();
        in_json_not_registry.sort_unstable();

        assert!(
            in_registry_not_json.is_empty(),
            "flags in CFLAG_REGISTRY but missing from version_cflags.json \
             (re-run `tools/sqlite-data update-data`): {in_registry_not_json:?}"
        );
        assert!(
            in_json_not_registry.is_empty(),
            "flags in version_cflags.json but missing from CFLAG_REGISTRY: \
             {in_json_not_registry:?}"
        );
    }

    /// Verify that `generate_cflags_h("parser")` is self-consistent: only parser-group
    /// entries from `CFLAG_REGISTRY` appear, with correct group-local indices.
    #[test]
    fn generate_cflags_h_parser_is_consistent() {
        use crate::util::cflag_registry::synq_const_name;

        let generated = super::generate_cflags_h("parser");

        // Parse "#define SYNQ_CFLAG_IDX_*  N" lines from the generated header.
        let mut defines: std::collections::HashMap<String, u32> = std::collections::HashMap::new();
        for line in generated.lines() {
            let line = line.trim();
            if let Some(rest) = line.strip_prefix("#define SYNQ_CFLAG_IDX_") {
                let parts: Vec<&str> = rest.split_whitespace().collect();
                if parts.len() >= 2 {
                    let name = format!("SYNQ_CFLAG_IDX_{}", parts[0]);
                    if let Ok(val) = parts[1].parse::<u32>() {
                        defines.insert(name, val);
                    }
                }
            }
        }

        // Every parser-group entry must appear with its group-local index.
        for &(sqlite_flag, _, cats) in CFLAG_REGISTRY {
            let const_name = synq_const_name(sqlite_flag);
            if !cats.contains(&"parser") {
                assert!(
                    !defines.contains_key(&const_name),
                    "non-parser flag {const_name} should not be in parser cflags.h"
                );
                continue;
            }
            let expected_local = super::group_local_index("parser", sqlite_flag)
                .expect("parser-group flag must have a local index");
            let header_val = defines.get(&const_name);
            assert_eq!(
                header_val,
                Some(&expected_local),
                "{const_name}: expected local index {expected_local}, got {header_val:?}"
            );
        }

        // Every define in the header (except COUNT) must correspond to a parser-group entry.
        let parser_const_names: std::collections::HashSet<String> = CFLAG_REGISTRY
            .iter()
            .filter(|(_, _, cats)| cats.contains(&"parser"))
            .map(|(n, _, _)| synq_const_name(n))
            .collect();
        for (name, val) in &defines {
            if name == "SYNQ_CFLAG_IDX_COUNT" {
                continue;
            }
            assert!(
                parser_const_names.contains(name),
                "cflags.h defines {name}={val} but it is not a parser-group entry in CFLAG_REGISTRY"
            );
        }
    }
}
