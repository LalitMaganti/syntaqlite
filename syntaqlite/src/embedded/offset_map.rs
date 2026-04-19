// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Maps byte offsets in processed SQL text back to host-file byte offsets.
//!
//! When interpolation holes are replaced with placeholder identifiers, the
//! offsets shift because `{some_long_expr}` might become `__hole_0__` (or
//! vice-versa). The `OffsetMap` handles this translation.

use syntaqlite_syntax::source::DocOffset;

use super::{EmbeddedFragment, HOLE_PLACEHOLDER};

/// An entry in the offset map: a region where the SQL text and host text differ.
#[derive(Debug)]
struct Segment {
    /// Start offset in SQL text.
    sql_start: usize,
    /// Length of this segment in SQL text.
    sql_len: usize,
    /// Length of this segment in host file.
    host_len: usize,
}

/// Maps byte offsets from processed SQL text back to host-file positions.
pub(crate) struct OffsetMap {
    /// Base offset of the SQL content in the host file.
    base_offset: usize,
    /// Sorted segments where SQL and host offsets diverge (holes).
    segments: Vec<Segment>,
}

impl OffsetMap {
    /// Build an offset map from an `EmbeddedFragment`.
    pub(crate) fn new(fragment: &EmbeddedFragment) -> Self {
        let segments = fragment
            .holes()
            .iter()
            .map(|h| Segment {
                sql_start: h.sql_offset(),
                sql_len: HOLE_PLACEHOLDER.len(),
                host_len: h.host_range().len(),
            })
            .collect();

        OffsetMap {
            base_offset: fragment.sql_range().start,
            segments,
        }
    }

    /// Convert a SQL-text byte offset to a host-file byte offset.
    ///
    /// Returns `None` if the offset falls inside a hole placeholder, since
    /// those regions correspond to host-language expressions, not SQL.
    pub(crate) fn to_host(&self, sql_offset: DocOffset) -> Option<DocOffset> {
        // Walk through segments to compute the cumulative drift.
        let mut drift: isize = 0;
        let sql = sql_offset.as_usize();

        for seg in &self.segments {
            if sql < seg.sql_start {
                break;
            }
            if sql < seg.sql_start + seg.sql_len {
                // Inside a hole placeholder — no meaningful host mapping.
                return None;
            }
            // Past this hole: accumulate the difference in lengths.
            drift += seg.host_len.cast_signed() - seg.sql_len.cast_signed();
        }

        // Apply base offset and accumulated drift.
        let host = (sql.cast_signed() + self.base_offset.cast_signed() + drift).cast_unsigned();
        Some(DocOffset::from_raw(u32::try_from(host).ok()?))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::embedded::{EmbeddedFragment, Hole};

    #[test]
    fn identity_map_no_holes() {
        let fragment = EmbeddedFragment {
            sql_range: 10..30,
            sql_text: "SELECT * FROM users".to_string(),
            holes: vec![],
        };
        let map = OffsetMap::new(&fragment);
        // Offset 0 in SQL → offset 10 in host.
        assert_eq!(
            map.to_host(DocOffset::from_raw(0)),
            Some(DocOffset::from_raw(10))
        );
        assert_eq!(
            map.to_host(DocOffset::from_raw(7)),
            Some(DocOffset::from_raw(17))
        );
    }

    #[test]
    fn single_hole_shorter() {
        // Host: "SELECT * FROM {table_name}" (range 10..36)
        // SQL:  "SELECT * FROM __h__!()"
        // Hole: {table_name} at host 24..36 (12 bytes), placeholder at sql offset 14 (8 bytes)
        let ph = HOLE_PLACEHOLDER; // "__h__!()" = 8 bytes
        let sql_text = format!("SELECT * FROM {ph}");
        let fragment = EmbeddedFragment {
            sql_range: 10..36,
            sql_text,
            holes: vec![Hole {
                host_range: 24..36,
                sql_offset: 14,
            }],
        };
        let map = OffsetMap::new(&fragment);

        // Before hole: offset 0 → 10, offset 13 → 23.
        assert_eq!(
            map.to_host(DocOffset::from_raw(0)),
            Some(DocOffset::from_raw(10))
        );
        assert_eq!(
            map.to_host(DocOffset::from_raw(13)),
            Some(DocOffset::from_raw(23))
        );

        // Inside hole: returns None (host-language expression, not SQL).
        assert_eq!(map.to_host(DocOffset::from_raw(14)), None);
        assert_eq!(map.to_host(DocOffset::from_raw(18)), None);
    }

    #[test]
    fn placeholder_longer_than_host_hole_no_overlap() {
        // Reproduces the datetime('now') highlighting bug:
        // When a placeholder (__h__!(), 8 bytes) is longer than the host
        // hole ({total}, 7 bytes), an emitted semantic token using the
        // placeholder length would extend past the hole boundary, overlapping
        // with subsequent tokens like `datetime`.
        //
        // Host content (starting at offset 2):
        //   "VALUES ({customer_id}, {total}, datetime('now'))"
        //
        // SQL text:
        //   "VALUES (__h__!(), __h__!(), datetime('now'))"
        let ph = HOLE_PLACEHOLDER;
        let sql_text = format!("VALUES ({ph}, {ph}, datetime('now'))");
        let fragment = EmbeddedFragment {
            sql_range: 2..50,
            sql_text,
            holes: vec![
                Hole {
                    host_range: 10..25, // {customer_id} = 15 bytes
                    sql_offset: 8,
                },
                Hole {
                    host_range: 27..34, // {total} = 7 bytes
                    sql_offset: 18,     // 8 (offset of first ph) + 8 (ph len) + 2 (", ")
                },
            ],
        };
        let map = OffsetMap::new(&fragment);

        // Inside holes: must return None so no semantic token is emitted.
        assert_eq!(
            map.to_host(DocOffset::from_raw(8)),
            None,
            "first placeholder start"
        );
        assert_eq!(
            map.to_host(DocOffset::from_raw(18)),
            None,
            "second placeholder start"
        );
        assert_eq!(
            map.to_host(DocOffset::from_raw(22)),
            None,
            "second placeholder mid"
        );

        // `datetime` sits after both placeholders in SQL text.
        // sql_offset of "datetime" = 8 + 8 + 2 + 8 + 2 = 28
        let datetime_sql_offset = DocOffset::from_raw(28);
        let datetime_host = map.to_host(datetime_sql_offset);
        assert_eq!(
            datetime_host,
            Some(DocOffset::from_raw(36)),
            "datetime must map to host offset 36"
        );
    }
}
