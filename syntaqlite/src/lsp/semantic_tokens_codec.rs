// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP semantic-tokens delta encoder.
//!
//! Converts a sorted slice of [`SemanticToken`] values plus the source text
//! into the flat `u32` array that `textDocument/semanticTokens/full` returns:
//! five values per token (`deltaLine`, `deltaStartChar`, `length`,
//! `legendIndex`, `modifiers`), with character offsets and lengths in UTF-16
//! code units per the LSP spec.

use syntaqlite_syntax::any::TokenCategory;
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

use crate::analysis::engine::tokens::SemanticToken;

use super::utf8_char_len;

/// Delta-encode semantic tokens to the LSP-wire flat `u32` array.
pub(super) fn encode_semantic_tokens(
    source: &str,
    semantic_tokens: &[SemanticToken],
    range: Option<DocRange>,
) -> Vec<u32> {
    let src = source.as_bytes();
    let src_end = DocOffset::from_raw(u32::try_from(src.len()).unwrap_or(u32::MAX));
    let DocRange {
        start: range_start,
        end: range_end,
    } = range.unwrap_or(DocRange {
        start: DocOffset::default(),
        end: src_end,
    });

    let mut result = Vec::with_capacity(semantic_tokens.len() * 5);
    let mut prev_line: u32 = 0;
    let mut prev_col: u32 = 0;
    let mut cur_line: u32 = 0;
    let mut cur_col: u32 = 0;
    let mut src_pos = DocOffset::default();

    for tok in semantic_tokens {
        while src_pos < tok.offset && src_pos < src_end {
            let i = src_pos.as_usize();
            if src[i] == b'\n' {
                cur_line += 1;
                cur_col = 0;
                src_pos += DocLen::from_raw(1);
            } else {
                let char_len = utf8_char_len(src[i]);
                cur_col += if char_len == 4 { 2 } else { 1 };
                src_pos += DocLen::from_raw(u32::try_from(char_len).unwrap_or(1));
            }
        }

        if tok.offset < range_start || tok.offset >= range_end {
            continue;
        }
        if matches!(
            tok.category,
            TokenCategory::Other | TokenCategory::Operator | TokenCategory::Punctuation
        ) {
            continue;
        }

        let legend_idx = tok.category as u32;
        let delta_line = cur_line - prev_line;
        let delta_start = if delta_line == 0 {
            cur_col - prev_col
        } else {
            cur_col
        };

        // Compute token length in UTF-16 code units.
        let tok_end = std::cmp::min(tok.offset + tok.length, src_end);
        let length_utf16 = utf16_len(&src[tok.offset.as_usize()..tok_end.as_usize()]);

        result.push(delta_line);
        result.push(delta_start);
        result.push(length_utf16);
        result.push(legend_idx);
        result.push(0);

        prev_line = cur_line;
        prev_col = cur_col;
    }

    result
}

/// Count the number of UTF-16 code units in a byte slice of valid UTF-8.
fn utf16_len(bytes: &[u8]) -> u32 {
    let mut n = 0u32;
    let mut i = 0;
    while i < bytes.len() {
        let char_len = utf8_char_len(bytes[i]);
        n += if char_len == 4 { 2 } else { 1 };
        i += char_len;
    }
    n
}
