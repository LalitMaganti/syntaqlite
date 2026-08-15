// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 1 built-in catalog extraction: compile `SQLite` amalgamations with
//! various cflag combinations and probe the resulting library for functions
//! and always-present relations.
//!
//! Two-phase approach:
//!   Phase 1 (audit): Scan each version's sqlite3.c to determine which cflags
//!     are referenced. Write `version_cflags.json` to data/.
//!   Phase 2 (extract): For each version, compile only with flags that version
//!     actually knows about. Write `functions.json` and `relations.json` to data/.

use std::collections::{BTreeMap, BTreeSet};
use std::fmt;
use std::fs;
use std::io::Write;
use std::path::Path;

use super::amalgamation_probe;

/// Overrides for flags whose probe compile args differ from the default `-D{name}`.
///
/// - `None` = skip probing entirely (flag cannot be safely compiled on modern toolchains).
/// - `Some(defines)` = use these compiler defines instead of the default `-D{name}`.
///
/// Flags not listed here use `-D{flag_name}` as the only compile define.
const PROBE_OVERRIDES: &[(&str, Option<&[&str]>)] = &[
    // Redefines `double` as `sqlite_int64`, causing compile failures on all
    // modern compilers. Extremely niche (embedded systems with no FPU).
    ("SQLITE_OMIT_FLOATING_POINT", None),
    // Requires RTREE to be enabled alongside for correct compilation.
    (
        "SQLITE_ENABLE_GEOPOLY",
        Some(&["-DSQLITE_ENABLE_GEOPOLY", "-DSQLITE_ENABLE_RTREE"]),
    ),
];

/// Returns the compile defines for probing `flag_name`, or `None` to skip probing.
fn probe_defines(flag_name: &str) -> Option<Vec<String>> {
    if let Some((_, override_val)) = PROBE_OVERRIDES.iter().find(|(n, _)| *n == flag_name) {
        return override_val.map(|defs| defs.iter().map(|s| (*s).to_string()).collect());
    }
    Some(vec![format!("-D{flag_name}")])
}

/// Returns `"omit"` for `SQLITE_OMIT_*` flags, `"enable"` for all others.
fn flag_polarity(flag_name: &str) -> &'static str {
    if flag_name.starts_with("SQLITE_OMIT_") {
        "omit"
    } else {
        "enable"
    }
}

/// Returns the (`flag_name`, polarity) pairs to probe for the given available flags.
///
/// Only flags in `CFLAG_REGISTRY` affecting functions or relations are
/// considered, minus any with `None` in `PROBE_OVERRIDES`.
fn probeable_flags(available: &BTreeSet<String>) -> Vec<(&'static str, &'static str)> {
    use crate::util::cflag_registry::CFLAG_REGISTRY;
    CFLAG_REGISTRY
        .iter()
        .filter_map(|&(name, _, cats)| {
            if !(cats.contains(&"functions") || cats.contains(&"relations"))
                || !available.contains(name)
            {
                return None;
            }
            // Skip flags that cannot be probed (e.g. compile failures).
            if PROBE_OVERRIDES
                .iter()
                .any(|(n, defs)| *n == name && defs.is_none())
            {
                return None;
            }
            Some((name, flag_polarity(name)))
        })
        .collect()
}

/// Build the C probe program. Relation candidates are extracted from each
/// amalgamation's own preprocessor definitions rather than maintained here.
fn probe_c(relation_candidates: &BTreeSet<String>) -> String {
    let candidates = relation_candidates
        .iter()
        .map(|name| format!("    \"{name}\","))
        .collect::<Vec<_>>()
        .join("\n");
    format!(
        r#"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>

static const char *relation_candidates[] = {{
{candidates}
    NULL
}};

int main(void) {{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {{
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }}

    rc = sqlite3_prepare_v2(db, "PRAGMA function_list", -1, &stmt, 0);
    if (rc != SQLITE_OK) {{
        fprintf(stderr, "Cannot prepare: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }}

    while (sqlite3_step(stmt) == SQLITE_ROW) {{
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        int builtin = sqlite3_column_int(stmt, 1);
        const char *ftype = (const char *)sqlite3_column_text(stmt, 2);
        /* enc column is at index 3, skip */
        int narg = sqlite3_column_int(stmt, 4);
        /* flags column is at index 5 */

        /* Emit both builtin and extension-registered functions.
         * Extension-registered functions (builtin=0) are still compiled into the
         * amalgamation and available — they're just registered via
         * sqlite3BuiltinExtensions[] instead of sqlite3RegisterBuiltinFunctions(). */
        (void)builtin;
        printf("F\t%s\t%d\t%s\n", name, narg, ftype);
    }}
    sqlite3_finalize(stmt);

    for (int i = 0; relation_candidates[i] != NULL; i++) {{
        const char *name = relation_candidates[i];
        char *sql = sqlite3_mprintf("SELECT * FROM \"%w\"", name);
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_free(sql);
        if (rc != SQLITE_OK) {{
            continue;
        }}

        sqlite3_stmt *rowid_stmt;
        sql = sqlite3_mprintf("SELECT rowid FROM \"%w\"", name);
        int rowid_rc = sqlite3_prepare_v2(db, sql, -1, &rowid_stmt, 0);
        sqlite3_free(sql);
        if (rowid_rc == SQLITE_OK) {{
            sqlite3_finalize(rowid_stmt);
        }}

        printf("R\t%s\t%d", name, rowid_rc != SQLITE_OK);
        for (int col = 0; col < sqlite3_column_count(stmt); col++) {{
            printf("\t%s", sqlite3_column_name(stmt, col));
        }}
        printf("\n");
        sqlite3_finalize(stmt);
    }}

    sqlite3_close(db);
    return 0;
}}
"#
    )
}

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

/// A single function entry from `PRAGMA function_list`.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub(crate) struct FunctionEntry {
    pub(crate) name: String,
    pub(crate) narg: i32,
    pub(crate) func_type: String,
    pub(crate) builtin: bool,
}

/// A built-in relation discovered by preparing a query against a name taken
/// from that `SQLite` version's own schema-table macros.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub(crate) struct RelationEntry {
    pub(crate) name: String,
    pub(crate) columns: Vec<String>,
    pub(crate) without_rowid: bool,
}

/// The built-ins available for a particular compilation.
#[derive(Debug, Clone)]
pub(crate) struct ProbeSet {
    pub(crate) functions: BTreeSet<FunctionEntry>,
    pub(crate) relations: BTreeSet<RelationEntry>,
}

/// Describes how a cflag affects built-ins for a particular version.
#[derive(Debug, Clone)]
pub(crate) struct CflagEffect {
    pub(crate) flag: String,
    pub(crate) polarity: String,
    pub(crate) affected_functions: BTreeSet<String>,
    pub(crate) affected_relations: BTreeSet<RelationEntry>,
}

/// A function's availability rule.
#[derive(Debug, Clone, serde::Serialize)]
pub(crate) struct AvailabilityRule {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) since: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) until: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) cflag: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) polarity: Option<String>,
}

/// Complete catalog entry for a function.
#[derive(Debug, Clone, serde::Serialize)]
pub(crate) struct FunctionCatalogEntry {
    pub(crate) name: String,
    pub(crate) arities: Vec<i32>,
    pub(crate) category: String,
    pub(crate) availability: Vec<AvailabilityRule>,
}

/// The complete extracted function catalog.
#[derive(Debug, Clone, serde::Serialize)]
pub(crate) struct FunctionCatalog {
    pub(crate) functions: Vec<FunctionCatalogEntry>,
}

/// Complete catalog entry for an always-present built-in relation.
#[derive(Debug, Clone, serde::Serialize)]
pub(crate) struct RelationCatalogEntry {
    pub(crate) name: String,
    pub(crate) columns: Vec<String>,
    pub(crate) without_rowid: bool,
    pub(crate) availability: Vec<AvailabilityRule>,
}

/// The complete extracted relation catalog.
#[derive(Debug, Clone, serde::Serialize)]
pub(crate) struct RelationCatalog {
    pub(crate) relations: Vec<RelationCatalogEntry>,
}

/// A single cflag availability entry (cflag-centric format).
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub(crate) struct CflagAvailabilityEntry {
    pub(crate) name: String,
    pub(crate) since: String,
    pub(crate) categories: Vec<String>,
}

/// Complete cflag availability data (cflag-centric format).
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub(crate) struct CflagAvailability {
    pub(crate) cflags: Vec<CflagAvailabilityEntry>,
}

// ---------------------------------------------------------------------------
// Version helper
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
struct Version {
    major: u32,
    minor: u32,
    patch: u32,
    sub_patch: u32,
}

impl Version {
    fn parse(s: &str) -> Result<Self, String> {
        let parts: Vec<&str> = s.split('.').collect();
        if parts.len() < 3 || parts.len() > 4 {
            return Err(format!("invalid version: {s}"));
        }
        Ok(Self {
            major: parts[0]
                .parse()
                .map_err(|_| format!("bad major: {}", parts[0]))?,
            minor: parts[1]
                .parse()
                .map_err(|_| format!("bad minor: {}", parts[1]))?,
            patch: parts[2]
                .parse()
                .map_err(|_| format!("bad patch: {}", parts[2]))?,
            sub_patch: if parts.len() == 4 {
                parts[3]
                    .parse()
                    .map_err(|_| format!("bad sub: {}", parts[3]))?
            } else {
                0
            },
        })
    }
}

impl fmt::Display for Version {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.sub_patch > 0 {
            write!(
                f,
                "{}.{}.{}.{}",
                self.major, self.minor, self.patch, self.sub_patch
            )
        } else {
            write!(f, "{}.{}.{}", self.major, self.minor, self.patch)
        }
    }
}

/// Discover available amalgamation versions from a directory.
///
/// Expected layout: `amalgamation_dir/3.35.5/sqlite3.c`
pub(crate) fn discover_versions(amalgamation_dir: &Path) -> Result<Vec<String>, String> {
    let entries = fs::read_dir(amalgamation_dir)
        .map_err(|e| format!("cannot read {}: {e}", amalgamation_dir.display()))?;
    let mut versions = Vec::new();
    for entry in entries.flatten() {
        if entry.path().is_dir() {
            let name = entry.file_name().to_string_lossy().to_string();
            if Version::parse(&name).is_ok() && entry.path().join("sqlite3.c").exists() {
                versions.push(name);
            }
        }
    }
    versions.sort_by(|a, b| {
        Version::parse(a)
            .expect("valid version")
            .cmp(&Version::parse(b).expect("valid version"))
    });
    Ok(versions)
}

// ---------------------------------------------------------------------------
// Phase 1: Audit — scan sqlite3.c for flag references
// ---------------------------------------------------------------------------

/// All flag names we look for during audit (all entries from `CFLAG_REGISTRY`).
fn all_flag_names() -> Vec<&'static str> {
    crate::util::cflag_registry::CFLAG_REGISTRY
        .iter()
        .map(|(name, _, _)| *name)
        .collect()
}

/// Scan a single version's sqlite3.c for references to our flags.
fn scan_version_cflags(sqlite3_c: &str) -> Vec<String> {
    let flags = all_flag_names();
    let mut found = Vec::new();
    for flag in flags {
        if sqlite3_c.contains(flag) {
            found.push(flag.to_string());
        }
    }
    found
}

/// Discover candidate built-in relation names from quoted `sqlite_*` values in
/// the amalgamation's preprocessor definitions. The probe subsequently keeps
/// only names that can actually be prepared as relations in that build.
fn discover_relation_candidates(sqlite3_c: &str) -> BTreeSet<String> {
    let mut candidates = BTreeSet::new();
    for line in sqlite3_c.lines() {
        if !line.trim_start().starts_with("#define") {
            continue;
        }
        for (idx, part) in line.split('"').enumerate() {
            if idx % 2 == 1
                && part.starts_with("sqlite_")
                && part
                    .bytes()
                    .all(|byte| byte.is_ascii_alphanumeric() || byte == b'_')
            {
                candidates.insert(part.to_string());
            }
        }
    }
    candidates
}

/// Audit all versions and write cflag-centric availability data to `output_path`.
///
/// Returns `CflagAvailability` with each cflag's earliest observed version (`since`).
///
/// # Errors
///
/// Returns an error if no amalgamation versions are found, or if reading/writing files fails.
///
/// # Panics
///
/// Panics if the discovered versions list is non-empty but `first()`/`last()` returns `None`
/// (should be unreachable).
pub(crate) fn audit_version_cflags(
    amalgamation_dir: &Path,
    output_path: &Path,
) -> Result<CflagAvailability, String> {
    let versions = discover_versions(amalgamation_dir)?;
    if versions.is_empty() {
        return Err(format!(
            "no amalgamation versions found in {}",
            amalgamation_dir.display()
        ));
    }

    eprintln!(
        "Auditing cflags for {} versions: {} .. {}",
        versions.len(),
        versions.first().expect("versions is non-empty"),
        versions.last().expect("versions is non-empty")
    );

    // Scan each version's sqlite3.c for flag references.
    let mut per_version: BTreeMap<String, Vec<String>> = BTreeMap::new();
    for version in &versions {
        let sqlite3_c_path = amalgamation_dir.join(version).join("sqlite3.c");
        let source = fs::read_to_string(&sqlite3_c_path)
            .map_err(|e| format!("reading {}: {e}", sqlite3_c_path.display()))?;
        let flags = scan_version_cflags(&source);
        eprintln!("  {version}: {} flags", flags.len());
        per_version.insert(version.clone(), flags);
    }

    // Compute `since` per cflag (earliest version where it appears).
    let availability = compute_cflag_availability(&per_version);

    // Write cflag-centric JSON output.
    let json = serde_json::to_string_pretty(&availability)
        .map_err(|e| format!("serializing availability: {e}"))?;
    if let Some(parent) = output_path.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("creating output dir: {e}"))?;
    }
    fs::write(output_path, format!("{json}\n"))
        .map_err(|e| format!("writing {}: {e}", output_path.display()))?;

    eprintln!("Wrote {}", output_path.display());
    Ok(availability)
}

/// Compute cflag availability from per-version scan results.
///
/// For each cflag in `CFLAG_REGISTRY`, finds the earliest version where it
/// appears in the amalgamation source. Cflags not observed in any version
/// get `since: "0"`.
fn compute_cflag_availability(per_version: &BTreeMap<String, Vec<String>>) -> CflagAvailability {
    // Sort versions.
    let mut sorted_versions: Vec<&String> = per_version.keys().collect();
    sorted_versions.sort_by(|a, b| {
        Version::parse(a)
            .expect("valid version")
            .cmp(&Version::parse(b).expect("valid version"))
    });

    let mut entries = Vec::new();
    for &(flag_name, _, categories) in crate::util::cflag_registry::CFLAG_REGISTRY {
        // Find earliest version containing this flag.
        let since = sorted_versions
            .iter()
            .find(|v| {
                per_version
                    .get(**v)
                    .is_some_and(|flags| flags.iter().any(|f| f == flag_name))
            })
            .map_or_else(|| "0".to_string(), |v| (*v).clone());

        entries.push(CflagAvailabilityEntry {
            name: flag_name.to_string(),
            since,
            categories: categories.iter().map(|s| (*s).to_string()).collect(),
        });
    }

    CflagAvailability { cflags: entries }
}

/// Convert a dotted version string to `SQLite`'s integer encoding.
///
/// `"3.35.0"` → `3035000`, `"0"` → `0`.
fn version_string_to_int(s: &str) -> i32 {
    if s == "0" {
        return 0;
    }
    let parts: Vec<&str> = s.split('.').collect();
    if parts.len() < 3 {
        return 0;
    }
    let major: i32 = parts[0].parse().unwrap_or(0);
    let minor: i32 = parts[1].parse().unwrap_or(0);
    let patch: i32 = parts[2].parse().unwrap_or(0);
    major * 1_000_000 + minor * 1_000 + patch
}

// ---------------------------------------------------------------------------
// Phase 2: Extract — compile with version-appropriate flags
// ---------------------------------------------------------------------------

/// Compile and run the built-in probe, returning parsed functions and relations.
fn compile_and_run_probe(
    amalgamation_dir: &Path,
    build_dir: &Path,
    defines: &[&str],
    label: &str,
    relation_candidates: &BTreeSet<String>,
) -> Result<ProbeSet, String> {
    let source = probe_c(relation_candidates);
    let binary =
        amalgamation_probe::compile_probe(amalgamation_dir, build_dir, defines, &source, label)?;
    let stdout = amalgamation_probe::run_probe(&binary)?;
    Ok(parse_probe_output(&stdout))
}

/// Parse tab-separated probe output into a [`ProbeSet`].
fn parse_probe_output(stdout: &str) -> ProbeSet {
    let mut functions = BTreeSet::new();
    let mut relations = BTreeSet::new();
    for line in stdout.lines() {
        let parts: Vec<&str> = line.split('\t').collect();
        match parts.as_slice() {
            ["F", name, narg, func_type] => {
                functions.insert(FunctionEntry {
                    name: (*name).to_string(),
                    narg: narg.parse().unwrap_or(0),
                    func_type: (*func_type).to_string(),
                    builtin: true,
                });
            }
            ["R", name, without_rowid, columns @ ..] => {
                relations.insert(RelationEntry {
                    name: (*name).to_string(),
                    columns: columns.iter().map(|column| (*column).to_string()).collect(),
                    without_rowid: *without_rowid == "1",
                });
            }
            _ => {}
        }
    }
    ProbeSet {
        functions,
        relations,
    }
}

/// Build the baseline compile defines for a version — all available ENABLE flags ON.
fn baseline_defines_for(available: &BTreeSet<String>) -> Vec<String> {
    use crate::util::cflag_registry::CFLAG_REGISTRY;
    let mut defines = Vec::new();
    for &(name, _, cats) in CFLAG_REGISTRY {
        if !(cats.contains(&"functions") || cats.contains(&"relations"))
            || !available.contains(name)
        {
            continue;
        }
        if flag_polarity(name) == "enable"
            && let Some(defs) = probe_defines(name)
        {
            defines.extend(defs);
        }
    }
    defines
}

/// Build the test defines for a specific cflag.
///
/// For OMIT flags: baseline ENABLE flags + the OMIT flag itself.
/// For ENABLE flags: baseline minus the ENABLE flag being tested.
fn test_defines_for(flag_name: &str, polarity: &str, available: &BTreeSet<String>) -> Vec<String> {
    use crate::util::cflag_registry::CFLAG_REGISTRY;
    let mut defines = Vec::new();
    for &(name, _, cats) in CFLAG_REGISTRY {
        if !(cats.contains(&"functions") || cats.contains(&"relations"))
            || !available.contains(name)
        {
            continue;
        }
        if polarity == "omit" {
            // OMIT test: all ENABLE flags in baseline, plus the OMIT flag itself.
            if flag_polarity(name) == "enable"
                && let Some(defs) = probe_defines(name)
            {
                defines.extend(defs);
            }
            if name == flag_name
                && let Some(defs) = probe_defines(name)
            {
                defines.extend(defs);
            }
        } else {
            // ENABLE test: baseline minus this flag.
            if flag_polarity(name) == "enable"
                && name != flag_name
                && let Some(defs) = probe_defines(name)
            {
                defines.extend(defs);
            }
        }
    }
    defines
}

/// Known-broken version × flag combinations.
///
/// `SQLITE_OMIT`_* flags are acknowledged by the `SQLite` project as poorly tested
/// (see FAQ: "I get a compiler error if I use the `SQLITE_OMIT`_... compile-time
/// options"). These specific combos fail to compile on their respective versions.
///
/// Each entry: (`version_prefix`, `flag_name`, reason).
const KNOWN_BROKEN: &[(&str, &str, &str)] = &[
    // SQLITE_OMIT_WINDOWFUNC: parser actions reference struct members
    // (pWinDefn, etc.) that are compiled out. Broken on all tested versions
    // including 3.51.2. The amalgamation's generated parser code doesn't
    // properly guard these references.
    (
        "3.",
        "SQLITE_OMIT_WINDOWFUNC",
        "parser references compiled-out struct members (pWinDefn)",
    ),
];

fn is_known_broken(version: &str, flag_name: &str) -> bool {
    KNOWN_BROKEN
        .iter()
        .any(|(ver_prefix, flag, _)| version.starts_with(ver_prefix) && *flag == flag_name)
}

/// Extract built-in data for a single version.
fn extract_version(
    amalgamation_dir: &Path,
    version: &str,
    build_dir: &Path,
    available_flags: &BTreeSet<String>,
    relation_candidates: &BTreeSet<String>,
) -> Result<(ProbeSet, Vec<CflagEffect>), String> {
    // Compile and run baseline (all available ENABLE flags ON, no OMIT flags).
    let bl_defs = baseline_defines_for(available_flags);
    let baseline_refs: Vec<&str> = bl_defs.iter().map(String::as_str).collect();
    let baseline = compile_and_run_probe(
        amalgamation_dir,
        build_dir,
        &baseline_refs,
        "baseline",
        relation_candidates,
    )?;

    let baseline_names: BTreeSet<String> =
        baseline.functions.iter().map(|f| f.name.clone()).collect();

    eprintln!(
        "    baseline: {} functions, {} relations",
        baseline_names.len(),
        baseline.relations.len()
    );

    let mut effects = Vec::new();

    // Test each probeable cflag.
    for (flag_name, polarity) in probeable_flags(available_flags) {
        if is_known_broken(version, flag_name) {
            eprintln!("    {flag_name}: known broken on {version}, skipping");
            continue;
        }

        let test_defs = test_defines_for(flag_name, polarity, available_flags);
        let probe_refs: Vec<&str> = test_defs.iter().map(String::as_str).collect();
        let label = format!("{version}_{flag_name}");

        let test_set = compile_and_run_probe(
            amalgamation_dir,
            build_dir,
            &probe_refs,
            &label,
            relation_candidates,
        )
        .map_err(|e| format!("{version}/{flag_name}: {e}"))?;

        let test_names: BTreeSet<String> =
            test_set.functions.iter().map(|f| f.name.clone()).collect();

        let affected_functions: BTreeSet<String> =
            baseline_names.difference(&test_names).cloned().collect();
        let affected_relations: BTreeSet<RelationEntry> = baseline
            .relations
            .difference(&test_set.relations)
            .cloned()
            .collect();
        if !affected_functions.is_empty() || !affected_relations.is_empty() {
            eprintln!(
                "    {flag_name}: {} functions, {} relations affected",
                affected_functions.len(),
                affected_relations.len()
            );
            effects.push(CflagEffect {
                flag: flag_name.to_string(),
                polarity: polarity.to_string(),
                affected_functions,
                affected_relations,
            });
        }
    }

    Ok((baseline, effects))
}

/// Run the full extraction pipeline across all versions.
///
/// Requires a pre-computed audit (`version_cflags.json`) to know which flags
/// each version supports.
///
/// # Errors
///
/// Returns an error if reading audit data, discovering versions, compiling probes, or writing
/// the output catalog fails.
///
/// # Panics
///
/// Panics if the discovered versions list is non-empty but `first()`/`last()` returns `None`
/// (should be unreachable).
pub(crate) fn extract_builtin_catalogs(
    amalgamation_dir: &Path,
    audit_path: &Path,
    functions_output_path: &Path,
    relations_output_path: &Path,
) -> Result<(FunctionCatalog, RelationCatalog), String> {
    // Load audit data (cflag-centric format).
    let audit_json = fs::read_to_string(audit_path)
        .map_err(|e| format!("reading {}: {e}", audit_path.display()))?;
    let availability: CflagAvailability = serde_json::from_str(&audit_json)
        .map_err(|e| format!("parsing {}: {e}", audit_path.display()))?;

    let versions = discover_versions(amalgamation_dir)?;
    if versions.is_empty() {
        return Err(format!(
            "no amalgamation versions found in {}",
            amalgamation_dir.display()
        ));
    }

    eprintln!(
        "Extracting built-ins from {} versions: {} .. {}",
        versions.len(),
        versions.first().expect("versions is non-empty"),
        versions.last().expect("versions is non-empty")
    );

    // Use a persistent build directory alongside the amalgamations so that
    // compiled .o files survive across runs.
    let build_root = amalgamation_dir.join(".build-cache");
    fs::create_dir_all(&build_root).map_err(|e| format!("creating build cache dir: {e}"))?;

    let mut per_version: Vec<VersionBuiltins> = Vec::new();
    let mut all_function_entries: BTreeMap<String, BTreeSet<(i32, String)>> = BTreeMap::new();

    for version in &versions {
        let amal_dir = amalgamation_dir.join(version);
        let build_dir = build_root.join(version);

        // Derive available flags: a flag is available if since <= this version.
        let ver_int = version_string_to_int(version);
        let available: BTreeSet<String> = availability
            .cflags
            .iter()
            .filter(|e| {
                let since_int = version_string_to_int(&e.since);
                since_int > 0 && since_int <= ver_int
            })
            .map(|e| e.name.clone())
            .collect();

        let sqlite3_c_path = amal_dir.join("sqlite3.c");
        let sqlite3_c = fs::read_to_string(&sqlite3_c_path)
            .map_err(|e| format!("reading {}: {e}", sqlite3_c_path.display()))?;
        let relation_candidates = discover_relation_candidates(&sqlite3_c);
        if relation_candidates.is_empty() {
            return Err(format!(
                "{version}: no sqlite_* relation candidates found in preprocessor definitions"
            ));
        }

        eprintln!(
            "  {version} ({} flags available, {} relation candidates)...",
            available.len(),
            relation_candidates.len()
        );

        let (baseline, effects) = extract_version(
            &amal_dir,
            version,
            &build_dir,
            &available,
            &relation_candidates,
        )
        .map_err(|e| format!("{version}: {e}"))?;

        // Keep the function catalog's established 3.30 extraction floor.
        // Older amalgamations are included to cover the full supported range
        // for relation and cflag extraction; their introspection pragma output
        // is not complete enough to establish function availability.
        let include_functions = ver_int >= version_string_to_int("3.30.0");
        let function_names: BTreeSet<String> = if include_functions {
            baseline.functions.iter().map(|f| f.name.clone()).collect()
        } else {
            BTreeSet::new()
        };

        if include_functions {
            for entry in &baseline.functions {
                all_function_entries
                    .entry(entry.name.clone())
                    .or_default()
                    .insert((entry.narg, entry.func_type.clone()));
            }
        }

        per_version.push(VersionBuiltins {
            version: version.clone(),
            function_names,
            relations: baseline.relations,
            effects,
        });
    }

    let function_catalog = build_function_catalog(&per_version, &all_function_entries);
    let relation_catalog = build_relation_catalog(&per_version);

    write_catalog(functions_output_path, &function_catalog)?;
    write_catalog(relations_output_path, &relation_catalog)?;

    eprintln!(
        "Wrote {} functions to {}",
        function_catalog.functions.len(),
        functions_output_path.display()
    );
    eprintln!(
        "Wrote {} relations to {}",
        relation_catalog.relations.len(),
        relations_output_path.display()
    );

    Ok((function_catalog, relation_catalog))
}

fn write_catalog(path: &Path, value: &impl serde::Serialize) -> Result<(), String> {
    let json =
        serde_json::to_string_pretty(value).map_err(|e| format!("serializing catalog: {e}"))?;
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("creating output dir: {e}"))?;
    }
    let mut file =
        fs::File::create(path).map_err(|e| format!("creating {}: {e}", path.display()))?;
    file.write_all(json.as_bytes())
        .map_err(|e| format!("writing {}: {e}", path.display()))?;
    file.write_all(b"\n")
        .map_err(|e| format!("writing {}: {e}", path.display()))
}

// ---------------------------------------------------------------------------
// Catalog construction
// ---------------------------------------------------------------------------

#[derive(Debug)]
struct VersionBuiltins {
    version: String,
    function_names: BTreeSet<String>,
    relations: BTreeSet<RelationEntry>,
    effects: Vec<CflagEffect>,
}

fn build_function_catalog(
    per_version: &[VersionBuiltins],
    all_entries: &BTreeMap<String, BTreeSet<(i32, String)>>,
) -> FunctionCatalog {
    let all_names: BTreeSet<&str> = per_version
        .iter()
        .flat_map(|version| version.function_names.iter().map(String::as_str))
        .collect();

    let mut functions = Vec::new();

    for name in &all_names {
        let entries = all_entries.get(*name);
        let arities: Vec<i32> = entries
            .map(|e| {
                let mut a: Vec<i32> = e.iter().map(|(n, _)| *n).collect();
                a.sort_unstable();
                a.dedup();
                a
            })
            .unwrap_or_default();

        let category = entries
            .map_or("scalar", |e| {
                if e.iter().any(|(_, t)| t == "w") {
                    "window"
                } else if e.iter().any(|(_, t)| t == "a") {
                    "aggregate"
                } else {
                    "scalar"
                }
            })
            .to_string();

        let availability = compute_function_availability(name, per_version);

        functions.push(FunctionCatalogEntry {
            name: name.to_string(),
            arities,
            category,
            availability,
        });
    }

    FunctionCatalog { functions }
}

fn build_relation_catalog(per_version: &[VersionBuiltins]) -> RelationCatalog {
    let all_relations: BTreeSet<RelationEntry> = per_version
        .iter()
        .flat_map(|version| version.relations.iter().cloned())
        .collect();
    let relations = all_relations
        .into_iter()
        .map(|relation| RelationCatalogEntry {
            availability: compute_relation_availability(&relation, per_version),
            name: relation.name,
            columns: relation.columns,
            without_rowid: relation.without_rowid,
        })
        .collect();
    RelationCatalog { relations }
}

fn compute_function_availability(
    func_name: &str,
    per_version: &[VersionBuiltins],
) -> Vec<AvailabilityRule> {
    compute_availability(per_version.iter().map(|version| {
        let gating_cflag = version
            .effects
            .iter()
            .find(|effect| effect.affected_functions.contains(func_name))
            .map(|effect| (effect.flag.clone(), effect.polarity.clone()));
        (
            version.version.clone(),
            version.function_names.contains(func_name),
            gating_cflag,
        )
    }))
}

fn compute_relation_availability(
    relation: &RelationEntry,
    per_version: &[VersionBuiltins],
) -> Vec<AvailabilityRule> {
    compute_availability(per_version.iter().map(|version| {
        let gating_cflag = version
            .effects
            .iter()
            .find(|effect| effect.affected_relations.contains(relation))
            .map(|effect| (effect.flag.clone(), effect.polarity.clone()));
        (
            version.version.clone(),
            version.relations.contains(relation),
            gating_cflag,
        )
    }))
}

fn compute_availability(
    states: impl IntoIterator<Item = (String, bool, Option<(String, String)>)>,
) -> Vec<AvailabilityRule> {
    let version_states: Vec<_> = states.into_iter().collect();
    let mut rules = Vec::new();
    let mut i = 0;
    while i < version_states.len() {
        let (ref ver, present, ref cflag) = version_states[i];
        if !present {
            i += 1;
            continue;
        }

        let mut j = i + 1;
        while j < version_states.len() {
            let (_, next_present, ref next_cflag) = version_states[j];
            if !next_present || next_cflag != cflag {
                break;
            }
            j += 1;
        }

        let since = ver.clone();
        let until = if j < version_states.len() {
            Some(version_states[j].0.clone())
        } else {
            None
        };

        let rule = match cflag {
            Some((flag, polarity)) => AvailabilityRule {
                since: Some(since),
                until,
                cflag: Some(flag.clone()),
                polarity: Some(polarity.clone()),
            },
            None => AvailabilityRule {
                since: Some(since),
                until,
                cflag: None,
                polarity: None,
            },
        };

        rules.push(rule);
        i = j;
    }

    rules
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_parse_and_display() {
        let v = Version::parse("3.35.5").unwrap();
        assert_eq!(v.to_string(), "3.35.5");
    }

    #[test]
    fn baseline_defines_only_uses_available() {
        let available: BTreeSet<String> = ["SQLITE_ENABLE_MATH_FUNCTIONS", "SQLITE_SOUNDEX"]
            .into_iter()
            .map(String::from)
            .collect();

        let defs = baseline_defines_for(&available);
        assert!(
            defs.iter()
                .any(|d| d.as_str() == "-DSQLITE_ENABLE_MATH_FUNCTIONS")
        );
        assert!(defs.iter().any(|d| d.as_str() == "-DSQLITE_SOUNDEX"));
        // FTS5 is not in available, should not appear.
        assert!(!defs.iter().any(|d| d.as_str() == "-DSQLITE_ENABLE_FTS5"));
        // No OMIT flags in baseline.
        assert!(!defs.iter().any(|d| d.contains("OMIT")));
    }

    #[test]
    fn test_defines_omit_adds_flag() {
        let available: BTreeSet<String> = ["SQLITE_OMIT_JSON", "SQLITE_ENABLE_MATH_FUNCTIONS"]
            .into_iter()
            .map(String::from)
            .collect();

        let defs = test_defines_for("SQLITE_OMIT_JSON", "omit", &available);
        assert!(defs.iter().any(|d| d.as_str() == "-DSQLITE_OMIT_JSON"));
        assert!(
            defs.iter()
                .any(|d| d.as_str() == "-DSQLITE_ENABLE_MATH_FUNCTIONS")
        );
    }

    #[test]
    fn test_defines_enable_removes_flag() {
        let available: BTreeSet<String> = ["SQLITE_ENABLE_MATH_FUNCTIONS", "SQLITE_SOUNDEX"]
            .into_iter()
            .map(String::from)
            .collect();

        let defs = test_defines_for("SQLITE_ENABLE_MATH_FUNCTIONS", "enable", &available);
        assert!(
            !defs
                .iter()
                .any(|d| d.as_str() == "-DSQLITE_ENABLE_MATH_FUNCTIONS")
        );
        assert!(defs.iter().any(|d| d.as_str() == "-DSQLITE_SOUNDEX"));
    }

    #[test]
    fn scan_finds_flags_in_source() {
        let source = r"
#ifdef SQLITE_ENABLE_MATH_FUNCTIONS
  /* math stuff */
#endif
#ifndef SQLITE_OMIT_JSON
  /* json stuff */
#endif
";
        let flags = scan_version_cflags(source);
        assert!(flags.contains(&"SQLITE_ENABLE_MATH_FUNCTIONS".to_string()));
        assert!(flags.contains(&"SQLITE_OMIT_JSON".to_string()));
        assert!(!flags.contains(&"SQLITE_ENABLE_FTS5".to_string()));
    }

    #[test]
    fn discovers_relation_candidates_from_defines() {
        let source = r#"
#define MASTER_NAME "sqlite_master"
#define NOT_A_DEFINE "sqlite_ignored"
const char *runtime_string = "sqlite_not_a_candidate";
#define TEMP_MASTER_NAME "sqlite_temp_master"
"#;
        assert_eq!(
            discover_relation_candidates(source),
            BTreeSet::from([
                "sqlite_ignored".to_string(),
                "sqlite_master".to_string(),
                "sqlite_temp_master".to_string(),
            ])
        );
    }

    #[test]
    fn parses_functions_and_relations_from_probe() {
        let output = "F\tabs\t1\ts\nR\tsqlite_schema\t0\ttype\tname\tsql\n";
        let parsed = parse_probe_output(output);
        assert_eq!(parsed.functions.len(), 1);
        assert_eq!(
            parsed.relations,
            BTreeSet::from([RelationEntry {
                name: "sqlite_schema".to_string(),
                columns: vec!["type".to_string(), "name".to_string(), "sql".to_string()],
                without_rowid: false,
            }])
        );
    }

    #[test]
    fn compute_availability_always_present() {
        let states = [
            ("3.30.0".into(), true, None),
            ("3.35.0".into(), true, None),
            ("3.40.0".into(), true, None),
        ];
        let rules = compute_availability(states);
        assert_eq!(rules.len(), 1);
        assert_eq!(rules[0].since.as_deref(), Some("3.30.0"));
        assert!(rules[0].until.is_none());
        assert!(rules[0].cflag.is_none());
    }

    #[test]
    fn compute_availability_cflag_change() {
        let states = [
            (
                "3.35.0".into(),
                true,
                Some(("SQLITE_ENABLE_JSON1".into(), "enable".into())),
            ),
            (
                "3.38.0".into(),
                true,
                Some(("SQLITE_OMIT_JSON".into(), "omit".into())),
            ),
        ];
        let rules = compute_availability(states);
        assert_eq!(rules.len(), 2);
        assert_eq!(rules[0].cflag.as_deref(), Some("SQLITE_ENABLE_JSON1"));
        assert_eq!(rules[0].polarity.as_deref(), Some("enable"));
        assert_eq!(rules[0].until.as_deref(), Some("3.38.0"));
        assert_eq!(rules[1].cflag.as_deref(), Some("SQLITE_OMIT_JSON"));
        assert_eq!(rules[1].polarity.as_deref(), Some("omit"));
        assert!(rules[1].until.is_none());
    }
}
