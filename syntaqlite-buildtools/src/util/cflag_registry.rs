// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stable cflag index registry — the single source of truth for name → index mapping.
//!
//! `CFLAG_REGISTRY` is the only place indices are hardcoded. All other cflag
//! metadata (categories and `since` versions) comes from `version_cflags.json`.
//!
//! # Invariants
//!
//! - **Never reorder or reuse indices.** Indices are part of the public API
//!   (they appear in generated C headers and `SqliteFlag` discriminants).
//! - Parser flags (those with `"parser"` in their categories) occupy indices 0–21,
//!   matching the C compact `SYNQ_CFLAG_IDX_*` values in `cflags.h` exactly.
//! - Non-parser flags occupy indices 22–41.
//! - New flags **must be appended** with the next unused index.

/// Number of flags in the compact parser cflag bitset.
///
/// The corresponding registry entries permanently occupy indices 0–21, so their
/// stable global indices are also their parser-group-local indices.
pub(crate) const PARSER_CFLAG_COUNT: u32 = 22;

/// Stable cflag registry: `(sqlite_flag_name, index)`.
///
/// - `sqlite_flag_name`: canonical `SQLite` flag name (e.g. `"SQLITE_OMIT_WINDOWFUNC"`).
/// - `index`: permanent bit-index in `SqliteFlags` / `SYNQ_CFLAG_IDX_*` C constant.
///   **Never reorder or reuse.**
///
/// Categories and minimum versions live exclusively in `version_cflags.json`.
/// `SYNQ_CFLAG_IDX_*` constant names are derived via [`synq_const_name`].
pub(crate) const CFLAG_REGISTRY: &[(&str, u32)] = &[
    // ── Parser flags (0–21, matching C compact SYNQ_CFLAG_IDX_* values) ────────
    ("SQLITE_OMIT_ALTERTABLE", 0),
    ("SQLITE_OMIT_ANALYZE", 1),
    ("SQLITE_OMIT_ATTACH", 2),
    ("SQLITE_OMIT_AUTOINCREMENT", 3),
    ("SQLITE_OMIT_CAST", 4),
    ("SQLITE_OMIT_COMPOUND_SELECT", 5),
    ("SQLITE_OMIT_CTE", 6),
    ("SQLITE_OMIT_EXPLAIN", 7),
    ("SQLITE_OMIT_FOREIGN_KEY", 8),
    ("SQLITE_OMIT_GENERATED_COLUMNS", 9),
    ("SQLITE_OMIT_PRAGMA", 10),
    ("SQLITE_OMIT_REINDEX", 11),
    ("SQLITE_OMIT_RETURNING", 12),
    ("SQLITE_OMIT_SUBQUERY", 13),
    ("SQLITE_OMIT_TEMPDB", 14),
    ("SQLITE_OMIT_TRIGGER", 15),
    ("SQLITE_OMIT_VACUUM", 16),
    ("SQLITE_OMIT_VIEW", 17),
    ("SQLITE_OMIT_VIRTUALTABLE", 18),
    ("SQLITE_OMIT_WINDOWFUNC", 19),
    ("SQLITE_ENABLE_ORDERED_SET_AGGREGATES", 20),
    ("SQLITE_ENABLE_UPDATE_DELETE_LIMIT", 21),
    // ── Non-parser flags (22–41, append new flags after 41) ─────────────────────
    ("SQLITE_OMIT_COMPILEOPTION_DIAGS", 22),
    ("SQLITE_OMIT_DATETIME_FUNCS", 23),
    ("SQLITE_OMIT_FLOATING_POINT", 24),
    ("SQLITE_OMIT_JSON", 25),
    ("SQLITE_OMIT_LOAD_EXTENSION", 26),
    ("SQLITE_ENABLE_BYTECODE_VTAB", 27),
    ("SQLITE_ENABLE_CARRAY", 28),
    ("SQLITE_ENABLE_DBPAGE_VTAB", 29),
    ("SQLITE_ENABLE_DBSTAT_VTAB", 30),
    ("SQLITE_ENABLE_FTS3", 31),
    ("SQLITE_ENABLE_FTS4", 32),
    ("SQLITE_ENABLE_FTS5", 33),
    ("SQLITE_ENABLE_GEOPOLY", 34),
    ("SQLITE_ENABLE_JSON1", 35),
    ("SQLITE_ENABLE_MATH_FUNCTIONS", 36),
    ("SQLITE_ENABLE_OFFSET_SQL_FUNC", 37),
    ("SQLITE_ENABLE_PERCENTILE", 38),
    ("SQLITE_ENABLE_RTREE", 39),
    ("SQLITE_ENABLE_STMTVTAB", 40),
    ("SQLITE_SOUNDEX", 41),
];

/// Look up the stable index for a cflag by name.
///
/// Returns `None` if the flag is not in [`CFLAG_REGISTRY`].
pub(crate) fn cflag_index(name: &str) -> Option<u32> {
    CFLAG_REGISTRY
        .iter()
        .find(|(n, _)| *n == name)
        .map(|(_, i)| *i)
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
        let mut indices: Vec<u32> = CFLAG_REGISTRY.iter().map(|(_, i)| *i).collect();
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
    fn parser_flags_occupy_the_compact_index_prefix() {
        for (expected, &(name, idx)) in CFLAG_REGISTRY
            .iter()
            .take(PARSER_CFLAG_COUNT as usize)
            .enumerate()
        {
            assert_eq!(
                idx,
                u32::try_from(expected).expect("parser cflag count fits u32"),
                "parser flag {name} must retain its compact index"
            );
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
