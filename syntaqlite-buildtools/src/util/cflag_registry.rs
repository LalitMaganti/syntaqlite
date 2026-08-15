// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stable cflag index registry — the single source of truth for name → index mapping.
//!
//! `CFLAG_REGISTRY` is the only place indices are hardcoded. All other cflag
//! data (categories, `since` versions) either derives from this table or comes
//! from the auto-generated `version_cflags.json`.
//!
//! # Invariants
//!
//! - **Never reorder or reuse indices.** Indices are part of the public API
//!   (they appear in generated C headers and `SqliteFlag` discriminants).
//! - Parser flags (those with `"parser"` in their categories) occupy indices 0–21,
//!   matching the C compact `SYNQ_CFLAG_IDX_*` values in `cflags.h` exactly.
//! - Non-parser flags occupy indices 22–41.
//! - New flags **must be appended** with the next unused index.

/// Stable cflag registry: `(sqlite_flag_name, index, categories)`.
///
/// - `sqlite_flag_name`: canonical `SQLite` flag name (e.g. `"SQLITE_OMIT_WINDOWFUNC"`).
/// - `index`: permanent bit-index in `SqliteFlags` / `SYNQ_CFLAG_IDX_*` C constant.
///   **Never reorder or reuse.**
/// - `categories`: editorial knowledge of what this flag affects.
///   - `"parser"`:     affects keyword recognition or SQL syntax
///   - `"functions"`:  affects built-in SQL function availability
///   - `"relations"`:  affects built-in relation availability
///   - `"vtable"`:     affects virtual table module availability
///   - `"extensions"`: enables optional extension modules (FTS, `RTree`, etc.)
///
/// `SYNQ_CFLAG_IDX_*` constant names are derived automatically via [`synq_const_name`];
/// there is no need to store them here.
///
/// Note: `since` version data lives in `version_cflags.json` and is injected at codegen
/// time via `--version-cflags-json`. See <https://github.com/LalitMaganti/syntaqlite/issues/6>
/// for the planned refactor to move categories there too.
pub(crate) const CFLAG_REGISTRY: &[(&str, u32, &[&str])] = &[
    // ── Parser flags (0–21, matching C compact SYNQ_CFLAG_IDX_* values) ────────
    ("SQLITE_OMIT_ALTERTABLE", 0, &["parser"]),
    ("SQLITE_OMIT_ANALYZE", 1, &["parser"]),
    ("SQLITE_OMIT_ATTACH", 2, &["parser"]),
    ("SQLITE_OMIT_AUTOINCREMENT", 3, &["parser"]),
    ("SQLITE_OMIT_CAST", 4, &["parser"]),
    ("SQLITE_OMIT_COMPOUND_SELECT", 5, &["parser"]),
    ("SQLITE_OMIT_CTE", 6, &["parser"]),
    ("SQLITE_OMIT_EXPLAIN", 7, &["parser"]),
    ("SQLITE_OMIT_FOREIGN_KEY", 8, &["parser"]),
    ("SQLITE_OMIT_GENERATED_COLUMNS", 9, &["parser"]),
    ("SQLITE_OMIT_PRAGMA", 10, &["parser"]),
    ("SQLITE_OMIT_REINDEX", 11, &["parser"]),
    ("SQLITE_OMIT_RETURNING", 12, &["parser"]),
    ("SQLITE_OMIT_SUBQUERY", 13, &["parser"]),
    ("SQLITE_OMIT_TEMPDB", 14, &["parser", "relations"]),
    ("SQLITE_OMIT_TRIGGER", 15, &["parser"]),
    ("SQLITE_OMIT_VACUUM", 16, &["parser"]),
    ("SQLITE_OMIT_VIEW", 17, &["parser"]),
    ("SQLITE_OMIT_VIRTUALTABLE", 18, &["parser", "vtable"]),
    ("SQLITE_OMIT_WINDOWFUNC", 19, &["parser", "functions"]),
    (
        "SQLITE_ENABLE_ORDERED_SET_AGGREGATES",
        20,
        &["parser", "functions"],
    ),
    ("SQLITE_ENABLE_UPDATE_DELETE_LIMIT", 21, &["parser"]),
    // ── Non-parser flags (22–41, append new flags after 41) ─────────────────────
    ("SQLITE_OMIT_COMPILEOPTION_DIAGS", 22, &["functions"]),
    ("SQLITE_OMIT_DATETIME_FUNCS", 23, &["functions"]),
    ("SQLITE_OMIT_FLOATING_POINT", 24, &["functions"]),
    ("SQLITE_OMIT_JSON", 25, &["functions"]),
    ("SQLITE_OMIT_LOAD_EXTENSION", 26, &["functions"]),
    ("SQLITE_ENABLE_BYTECODE_VTAB", 27, &["vtable"]),
    ("SQLITE_ENABLE_CARRAY", 28, &["vtable"]),
    ("SQLITE_ENABLE_DBPAGE_VTAB", 29, &["vtable"]),
    ("SQLITE_ENABLE_DBSTAT_VTAB", 30, &["vtable"]),
    ("SQLITE_ENABLE_FTS3", 31, &["extensions", "functions"]),
    ("SQLITE_ENABLE_FTS4", 32, &["extensions"]),
    ("SQLITE_ENABLE_FTS5", 33, &["extensions", "functions"]),
    ("SQLITE_ENABLE_GEOPOLY", 34, &["extensions", "functions"]),
    ("SQLITE_ENABLE_JSON1", 35, &["functions"]),
    ("SQLITE_ENABLE_MATH_FUNCTIONS", 36, &["functions"]),
    ("SQLITE_ENABLE_OFFSET_SQL_FUNC", 37, &["functions"]),
    ("SQLITE_ENABLE_PERCENTILE", 38, &["functions"]),
    ("SQLITE_ENABLE_RTREE", 39, &["extensions"]),
    ("SQLITE_ENABLE_STMTVTAB", 40, &["vtable"]),
    ("SQLITE_SOUNDEX", 41, &["functions"]),
];

/// Look up the stable index for a cflag by name.
///
/// Returns `None` if the flag is not in [`CFLAG_REGISTRY`].
pub(crate) fn cflag_index(name: &str) -> Option<u32> {
    CFLAG_REGISTRY
        .iter()
        .find(|(n, _, _)| *n == name)
        .map(|(_, i, _)| *i)
}

/// Derive the `SYNQ_CFLAG_IDX_*` C constant name from a `SQLITE_*` flag name.
///
/// Strips the `"SQLITE_"` prefix and prepends `"SYNQ_CFLAG_IDX_"`.
pub(crate) fn synq_const_name(flag_name: &str) -> String {
    let suffix = flag_name.strip_prefix("SQLITE_").unwrap_or(flag_name);
    format!("SYNQ_CFLAG_IDX_{suffix}")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn indices_are_unique_and_dense() {
        let mut indices: Vec<u32> = CFLAG_REGISTRY.iter().map(|(_, i, _)| *i).collect();
        indices.sort_unstable();
        for (pos, &idx) in indices.iter().enumerate() {
            assert_eq!(
                idx,
                u32::try_from(pos).expect("registry has fewer than u32::MAX entries"),
                "CFLAG_REGISTRY indices must be 0-based and contiguous; gap at position {pos}"
            );
        }
    }

    #[test]
    fn parser_flags_occupy_indices_0_to_21() {
        for &(name, idx, cats) in CFLAG_REGISTRY {
            if cats.contains(&"parser") {
                assert!(
                    idx < 22,
                    "parser flag {name} has index {idx}, expected < 22"
                );
            } else {
                assert!(
                    idx >= 22,
                    "non-parser flag {name} has index {idx}, expected >= 22"
                );
            }
        }
    }

    #[test]
    fn synq_const_name_derives_correctly() {
        assert_eq!(
            synq_const_name("SQLITE_OMIT_ALTERTABLE"),
            "SYNQ_CFLAG_IDX_OMIT_ALTERTABLE"
        );
        assert_eq!(synq_const_name("SQLITE_SOUNDEX"), "SYNQ_CFLAG_IDX_SOUNDEX");
    }
}
