// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Byte ↔ (line, UTF-16 column) mapping for LSP protocol positions.
//!
//! LSP measures positions in UTF-16 code units; the parser emits byte
//! offsets.  `SourceMap` keeps that translation in one place so every
//! `didOpen` → `publishDiagnostics` → `hover` round-trip walks the source
//! bytes the same way.  Each direction is a single O(n) walk.

use syntaqlite_syntax::source::{DocOffset, DocText, Utf16Col, Utf16Line};

/// Converts between document-absolute byte offsets and 0-based
/// (line, UTF-16 column) positions for a fixed source string.  Zero-cost
/// to construct — borrows the source.
#[derive(Copy, Clone)]
pub(crate) struct SourceMap<'a> {
    src: &'a DocText,
}

impl<'a> SourceMap<'a> {
    /// Wrap `source` as a position map.
    pub(crate) const fn new(source: &'a DocText) -> Self {
        Self { src: source }
    }

    /// Convert a batch of byte offsets to `(line, utf16_col)` positions in
    /// one O(n) pass, returning results in the same order as `offsets`.
    ///
    /// Batching amortizes the source walk: `n` offsets cost
    /// O(n + source_len), versus O(n × source_len) for single conversions.
    pub(crate) fn byte_offsets_to_utf16(
        &self,
        offsets: &[DocOffset],
    ) -> Vec<(Utf16Line, Utf16Col)> {
        if offsets.is_empty() {
            return Vec::new();
        }

        let mut indexed: Vec<(usize, usize)> = offsets
            .iter()
            .enumerate()
            .map(|(i, o)| (o.as_usize(), i))
            .collect();
        indexed.sort_unstable_by_key(|&(o, _)| o);

        let src = self.src.as_str().as_bytes();
        let len = src.len();
        let mut result = vec![(Utf16Line::default(), Utf16Col::default()); offsets.len()];
        let mut line: u32 = 0;
        let mut col_utf16: u32 = 0;
        let mut pos: usize = 0;

        for (offset, orig_idx) in indexed {
            let offset = offset.min(len);
            while pos < offset {
                if src[pos] == b'\n' {
                    line += 1;
                    col_utf16 = 0;
                    pos += 1;
                } else {
                    let char_len = utf8_char_len(src[pos]);
                    // 4-byte UTF-8 chars are a surrogate pair in UTF-16;
                    // everything else is one UTF-16 code unit.
                    col_utf16 += if char_len == 4 { 2 } else { 1 };
                    pos += char_len;
                }
            }
            result[orig_idx] = (Utf16Line::from_raw(line), Utf16Col::from_raw(col_utf16));
        }

        result
    }

    /// Convert a 0-based `(line, utf16_col)` position to a document-absolute
    /// byte offset.  Clamps to end-of-document when `line` is past the last
    /// line; clamps to end-of-line when `col` is past the line's length.
    pub(crate) fn utf16_to_byte_offset(&self, line: Utf16Line, col: Utf16Col) -> DocOffset {
        let src = self.src.as_str().as_bytes();
        let len = src.len();
        let target_line = line.as_usize();
        let mut cur_line: usize = 0;
        let mut line_start: usize = 0;

        while cur_line < target_line && line_start < len {
            match src[line_start..].iter().position(|&b| b == b'\n') {
                Some(nl) => {
                    line_start += nl + 1;
                    cur_line += 1;
                }
                None => return DocOffset::from_raw(u32::try_from(len).unwrap_or(u32::MAX)),
            }
        }

        let line_end = src[line_start..]
            .iter()
            .position(|&b| b == b'\n')
            .map_or(len, |rel| line_start + rel);

        let target_col = col.as_u32();
        let mut byte_pos = line_start;
        let mut utf16_col: u32 = 0;
        while byte_pos < line_end && utf16_col < target_col {
            let char_len = utf8_char_len(src[byte_pos]);
            utf16_col += if char_len == 4 { 2 } else { 1 };
            byte_pos += char_len;
        }
        DocOffset::from_raw(u32::try_from(byte_pos).unwrap_or(u32::MAX))
    }
}

/// Length of a UTF-8 character from its leading byte.
/// Returns 1 for ASCII and for invalid/continuation bytes (defensive).
#[inline]
pub(crate) const fn utf8_char_len(lead: u8) -> usize {
    match lead {
        0xC0..=0xDF => 2,
        0xE0..=0xEF => 3,
        0xF0..=0xF7 => 4,
        _ => 1,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn utf16(line: u32, col: u32) -> (Utf16Line, Utf16Col) {
        (Utf16Line::from_raw(line), Utf16Col::from_raw(col))
    }

    #[test]
    fn ascii_byte_to_utf16() {
        let src = DocText::new("ab\ncd");
        let map = SourceMap::new(src);
        let positions = map.byte_offsets_to_utf16(&[
            DocOffset::from_raw(0),
            DocOffset::from_raw(1),
            DocOffset::from_raw(2),
            DocOffset::from_raw(3),
            DocOffset::from_raw(4),
        ]);
        assert_eq!(positions[0], utf16(0, 0));
        assert_eq!(positions[1], utf16(0, 1));
        assert_eq!(positions[2], utf16(0, 2));
        assert_eq!(positions[3], utf16(1, 0));
        assert_eq!(positions[4], utf16(1, 1));
    }

    #[test]
    fn two_byte_char_is_one_utf16_unit() {
        // 'é' = 2 UTF-8 bytes, 1 UTF-16 code unit
        let src = DocText::new("aé b");
        let map = SourceMap::new(src);
        let positions = map.byte_offsets_to_utf16(&[
            DocOffset::from_raw(0),
            DocOffset::from_raw(3),
            DocOffset::from_raw(4),
        ]);
        assert_eq!(positions[0], utf16(0, 0));
        assert_eq!(positions[1], utf16(0, 2));
        assert_eq!(positions[2], utf16(0, 3));
    }

    #[test]
    fn four_byte_char_is_two_utf16_units() {
        // '😀' = 4 UTF-8 bytes, 2 UTF-16 code units (surrogate pair)
        let src = DocText::new("a😀b");
        let map = SourceMap::new(src);
        let positions =
            map.byte_offsets_to_utf16(&[DocOffset::from_raw(0), DocOffset::from_raw(5)]);
        assert_eq!(positions[0], utf16(0, 0));
        assert_eq!(positions[1], utf16(0, 3));
    }

    #[test]
    fn utf16_to_byte_ascii() {
        let src = DocText::new("ab\ncd");
        let map = SourceMap::new(src);
        let (l, c) = utf16(1, 1);
        assert_eq!(map.utf16_to_byte_offset(l, c), DocOffset::from_raw(4));
    }

    #[test]
    fn utf16_to_byte_four_byte_char() {
        let src = DocText::new("a😀b");
        let map = SourceMap::new(src);
        let (l, c) = utf16(0, 3);
        assert_eq!(map.utf16_to_byte_offset(l, c), DocOffset::from_raw(5));
    }

    #[test]
    fn empty_batch() {
        let src = DocText::new("abc");
        let map = SourceMap::new(src);
        assert!(map.byte_offsets_to_utf16(&[]).is_empty());
    }
}
