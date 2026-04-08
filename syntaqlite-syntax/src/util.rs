// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// ── Public API ───────────────────────────────────────────────────────────────

// `SqliteSyntaxFlag` is always available — ordinals are stable across all dialects.
#[doc(inline)]
pub use crate::sqlite::cflags::SqliteSyntaxFlag;

/// Snapshot of C-parser compatibility flags for the `SQLite` grammar.
///
/// This type mirrors the `SyntaqliteCflags` C struct (3 bytes, 22 meaningful
/// bits, compact indices 0–21) and is used to configure the C parser at
/// runtime. It covers only the grammar-level parser flags defined in
/// `include/syntaqlite/cflags.h`.
///
/// For the full set of `SQLite` compile-time flags — including non-parser flags
/// like `SQLITE_ENABLE_MATH_FUNCTIONS` — use `syntaqlite::util::SqliteFlags`.
#[derive(Debug, Clone, Copy, Default)]
pub struct SqliteSyntaxFlags(pub(crate) ffi::CCflags);

impl SqliteSyntaxFlags {
    /// Returns `true` if parser flag `flag` is enabled.
    #[inline]
    pub fn has(&self, flag: SqliteSyntaxFlag) -> bool {
        self.0.has(flag as u32)
    }

    /// Return a copy of these flags with `flag` enabled.
    #[must_use]
    pub fn with(mut self, flag: SqliteSyntaxFlag) -> Self {
        self.0.set(flag as u32);
        self
    }
}

/// `SQLite` compatibility target used to select grammar behavior.
///
/// Pin this when your application needs to parse according to a specific
/// `SQLite` release. Patch versions are intentionally ignored.
#[expect(missing_docs)]
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum SqliteVersion {
    V3_12,
    V3_13,
    V3_14,
    V3_15,
    V3_16,
    V3_17,
    V3_18,
    V3_19,
    V3_20,
    V3_21,
    V3_22,
    V3_23,
    V3_24,
    V3_25,
    V3_26,
    V3_27,
    V3_28,
    V3_29,
    V3_30,
    V3_31,
    V3_32,
    V3_33,
    V3_34,
    V3_35,
    V3_36,
    V3_37,
    V3_38,
    V3_39,
    V3_40,
    V3_41,
    V3_42,
    V3_43,
    V3_44,
    V3_45,
    V3_46,
    V3_47,
    V3_48,
    V3_49,
    V3_50,
    V3_51,
    /// No version constraint — use the latest grammar rules.
    Latest,
}

impl SqliteVersion {
    /// Parse a version string or the literal `"latest"`.
    ///
    /// The string `"latest"` (case-insensitive) maps to [`SqliteVersion::Latest`].
    /// All other inputs are forwarded to [`SqliteVersion::parse`].
    ///
    /// # Errors
    /// Returns `Err` if the version string is not recognised.
    pub fn parse_with_latest(s: &str) -> Result<Self, String> {
        let s = s.trim();
        if s.eq_ignore_ascii_case("latest") {
            return Ok(Self::Latest);
        }
        Self::parse(s).ok_or_else(|| format!("unknown or unsupported SQLite version: '{s}'"))
    }

    /// Parse a version string, ignoring the patch component.
    ///
    /// Accepts `"3.35"`, `"3.35.0"`, `"3.35.5"`, etc.
    /// Returns `None` if the version is not in the supported range.
    pub fn parse(s: &str) -> Option<Self> {
        let mut parts = s.splitn(3, '.');
        let major: u32 = parts.next()?.parse().ok()?;
        let minor: u32 = parts.next()?.parse().ok()?;
        // patch component is ignored
        if major != 3 {
            return None;
        }
        Some(match minor {
            12 => Self::V3_12,
            13 => Self::V3_13,
            14 => Self::V3_14,
            15 => Self::V3_15,
            16 => Self::V3_16,
            17 => Self::V3_17,
            18 => Self::V3_18,
            19 => Self::V3_19,
            20 => Self::V3_20,
            21 => Self::V3_21,
            22 => Self::V3_22,
            23 => Self::V3_23,
            24 => Self::V3_24,
            25 => Self::V3_25,
            26 => Self::V3_26,
            27 => Self::V3_27,
            28 => Self::V3_28,
            29 => Self::V3_29,
            30 => Self::V3_30,
            31 => Self::V3_31,
            32 => Self::V3_32,
            33 => Self::V3_33,
            34 => Self::V3_34,
            35 => Self::V3_35,
            36 => Self::V3_36,
            37 => Self::V3_37,
            38 => Self::V3_38,
            39 => Self::V3_39,
            40 => Self::V3_40,
            41 => Self::V3_41,
            42 => Self::V3_42,
            43 => Self::V3_43,
            44 => Self::V3_44,
            45 => Self::V3_45,
            46 => Self::V3_46,
            47 => Self::V3_47,
            48 => Self::V3_48,
            49 => Self::V3_49,
            50 => Self::V3_50,
            51 => Self::V3_51,
            _ => return None,
        })
    }
}

// ── Crate-internal ───────────────────────────────────────────────────────────

impl SqliteVersion {
    /// Convert from `SQLite`'s `SQLITE_VERSION_NUMBER` integer format.
    ///
    /// Returns `SqliteVersion::Latest` for `i32::MAX` and for any unrecognised
    /// value (e.g. a version newer than the highest known variant).
    pub fn from_int(v: i32) -> Self {
        match v {
            3_012_000 => Self::V3_12,
            3_013_000 => Self::V3_13,
            3_014_000 => Self::V3_14,
            3_015_000 => Self::V3_15,
            3_016_000 => Self::V3_16,
            3_017_000 => Self::V3_17,
            3_018_000 => Self::V3_18,
            3_019_000 => Self::V3_19,
            3_020_000 => Self::V3_20,
            3_021_000 => Self::V3_21,
            3_022_000 => Self::V3_22,
            3_023_000 => Self::V3_23,
            3_024_000 => Self::V3_24,
            3_025_000 => Self::V3_25,
            3_026_000 => Self::V3_26,
            3_027_000 => Self::V3_27,
            3_028_000 => Self::V3_28,
            3_029_000 => Self::V3_29,
            3_030_000 => Self::V3_30,
            3_031_000 => Self::V3_31,
            3_032_000 => Self::V3_32,
            3_033_000 => Self::V3_33,
            3_034_000 => Self::V3_34,
            3_035_000 => Self::V3_35,
            3_036_000 => Self::V3_36,
            3_037_000 => Self::V3_37,
            3_038_000 => Self::V3_38,
            3_039_000 => Self::V3_39,
            3_040_000 => Self::V3_40,
            3_041_000 => Self::V3_41,
            3_042_000 => Self::V3_42,
            3_043_000 => Self::V3_43,
            3_044_000 => Self::V3_44,
            3_045_000 => Self::V3_45,
            3_046_000 => Self::V3_46,
            3_047_000 => Self::V3_47,
            3_048_000 => Self::V3_48,
            3_049_000 => Self::V3_49,
            3_050_000 => Self::V3_50,
            3_051_000 => Self::V3_51,
            _ => Self::Latest,
        }
    }

    /// Convert to `SQLite`'s `SQLITE_VERSION_NUMBER` integer format.
    ///
    /// Uses the formula `major * 1_000_000 + minor * 1_000`, matching the
    /// `SQLITE_VERSION_NUMBER` C macro (e.g. `V3_35` → `3035000`).
    pub fn as_int(self) -> i32 {
        match self {
            Self::V3_12 => 3_012_000,
            Self::V3_13 => 3_013_000,
            Self::V3_14 => 3_014_000,
            Self::V3_15 => 3_015_000,
            Self::V3_16 => 3_016_000,
            Self::V3_17 => 3_017_000,
            Self::V3_18 => 3_018_000,
            Self::V3_19 => 3_019_000,
            Self::V3_20 => 3_020_000,
            Self::V3_21 => 3_021_000,
            Self::V3_22 => 3_022_000,
            Self::V3_23 => 3_023_000,
            Self::V3_24 => 3_024_000,
            Self::V3_25 => 3_025_000,
            Self::V3_26 => 3_026_000,
            Self::V3_27 => 3_027_000,
            Self::V3_28 => 3_028_000,
            Self::V3_29 => 3_029_000,
            Self::V3_30 => 3_030_000,
            Self::V3_31 => 3_031_000,
            Self::V3_32 => 3_032_000,
            Self::V3_33 => 3_033_000,
            Self::V3_34 => 3_034_000,
            Self::V3_35 => 3_035_000,
            Self::V3_36 => 3_036_000,
            Self::V3_37 => 3_037_000,
            Self::V3_38 => 3_038_000,
            Self::V3_39 => 3_039_000,
            Self::V3_40 => 3_040_000,
            Self::V3_41 => 3_041_000,
            Self::V3_42 => 3_042_000,
            Self::V3_43 => 3_043_000,
            Self::V3_44 => 3_044_000,
            Self::V3_45 => 3_045_000,
            Self::V3_46 => 3_046_000,
            Self::V3_47 => 3_047_000,
            Self::V3_48 => 3_048_000,
            Self::V3_49 => 3_049_000,
            Self::V3_50 => 3_050_000,
            Self::V3_51 => 3_051_000,
            Self::Latest => i32::MAX,
        }
    }
}

// ── ffi ───────────────────────────────────────────────────────────────────────

pub(crate) mod ffi {
    /// Mirrors C `SyntaqliteCflags` from `include/syntaqlite/cflags.h`.
    ///
    /// A packed bitfield over the parser-group compile-time flags (22 flags,
    /// packed into 3 bytes). Indices match the `SYNQ_CFLAG_IDX_*` C constants
    /// and the generated [`crate::sqlite::cflags::SqliteSyntaxFlag`] Rust enum.
    #[repr(C)]
    #[derive(Clone, Copy, Default)]
    pub(crate) struct CCflags {
        pub(super) bytes: [u8; 3],
    }

    #[expect(dead_code)]
    impl CCflags {
        pub(crate) const fn new() -> Self {
            Self { bytes: [0; 3] }
        }

        #[inline]
        pub(crate) fn has(self, idx: u32) -> bool {
            let byte = idx / 8;
            let bit = idx % 8;
            (byte < 3) && (self.bytes[byte as usize] >> bit) & 1 != 0
        }

        #[inline]
        pub(crate) fn set(&mut self, idx: u32) {
            let byte = idx / 8;
            let bit = idx % 8;
            if byte < 3 {
                self.bytes[byte as usize] |= 1 << bit;
            }
        }

        #[inline]
        pub(crate) fn clear(&mut self, idx: u32) {
            let byte = idx / 8;
            let bit = idx % 8;
            if byte < 3 {
                self.bytes[byte as usize] &= !(1 << bit);
            }
        }

        #[inline]
        pub(crate) fn clear_all(&mut self) {
            self.bytes = [0; 3];
        }
    }

    impl std::fmt::Debug for CCflags {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(f, "Cflags({:02x?})", &self.bytes)
        }
    }
}

// ── Tests ─────────────────────────────────────────────────────────────────────

/// Return `true` if `name` looks like a completable keyword symbol.
///
/// Keyword symbols use all-uppercase names with digits and underscores
/// (e.g. `SELECT`, `LEFT_JOIN`). This is a pure naming-convention predicate
/// and does not depend on grammar version or flags.
pub fn is_suggestable_keyword(name: &str) -> bool {
    !name.is_empty()
        && name
            .bytes()
            .all(|b| b.is_ascii_uppercase() || b.is_ascii_digit() || b == b'_')
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_major_minor() {
        assert_eq!(SqliteVersion::parse("3.35"), Some(SqliteVersion::V3_35));
    }

    #[test]
    fn parse_with_patch() {
        assert_eq!(SqliteVersion::parse("3.35.5"), Some(SqliteVersion::V3_35));
    }

    #[test]
    fn parse_unknown_minor() {
        assert_eq!(SqliteVersion::parse("3.99"), None);
    }

    #[test]
    fn as_int_spot_check() {
        assert_eq!(SqliteVersion::V3_35.as_int(), 3_035_000);
        assert_eq!(SqliteVersion::V3_51.as_int(), 3_051_000);
        assert_eq!(SqliteVersion::Latest.as_int(), i32::MAX);
    }
}
