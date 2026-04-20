// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Parse-level probes for interactive tooling: completion and signature help.
//!
//! These are neutral queries over a captured token stream + a source string.
//! They do not depend on any LSP protocol machinery — the LSP layer assembles
//! [`CompletionEntry`](crate::lsp::CompletionEntry) values from
//! [`CompletionInfo`] separately.

use std::collections::HashSet;

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{AnyParser, AnyTokenType, TokenCategory};
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange, DocText};

use crate::analysis::catalog::AritySpec;
use crate::analysis::engine::tokens::StoredToken;
use crate::dialect::AnyDialect;

// ── Completion ───────────────────────────────────────────────────────────────

/// Semantic completion context derived from parser stack state.
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum CompletionContext {
    Unknown = 0,
    Expression = 1,
    TableRef = 2,
}

impl CompletionContext {
    pub(crate) fn from_parser(v: syntaqlite_syntax::CompletionContext) -> Self {
        match v {
            syntaqlite_syntax::CompletionContext::Expression => Self::Expression,
            syntaqlite_syntax::CompletionContext::TableRef => Self::TableRef,
            syntaqlite_syntax::CompletionContext::Unknown => Self::Unknown,
        }
    }
}

/// Expected tokens and semantic context at a cursor position.
#[derive(Debug)]
pub(crate) struct CompletionInfo {
    pub(crate) tokens: Vec<AnyTokenType>,
    pub(crate) context: CompletionContext,
    pub(crate) qualifier: Option<String>,
}

/// Expected tokens and semantic context at `offset`, derived by feeding the
/// statement's tokens through an incremental parser up to the cursor.
pub(crate) fn completion_info(
    dialect: &AnyDialect,
    source_text: &str,
    tokens: &[StoredToken],
    offset: DocOffset,
) -> CompletionInfo {
    let source = DocText::new(source_text);
    let end_of_doc = source.byte_len();
    let doc_end = DocOffset::default() + end_of_doc;
    let cursor = if offset > doc_end { doc_end } else { offset };
    let (boundary, backtracked) = completion_boundary(source, tokens, cursor);
    let start = statement_token_start(tokens, boundary);
    let stmt_tokens = &tokens[start..boundary];

    let syntax = (**dialect).clone();
    let parser = AnyParser::with_config(syntax, &ParserConfig::default());
    let mut cursor_p = parser.incremental_parse(source.as_str());
    // Do not call expected_tokens() before feeding any tokens: the C parser
    // returns a garbage `total` count when no tokens have been fed yet,
    // which would trigger a multi-GiB allocation and SIGKILL.
    let mut last_expected: Vec<AnyTokenType> = Vec::new();

    for (i, tok) in stmt_tokens.iter().enumerate() {
        let span = tok.offset..(tok.offset + tok.length);
        if cursor_p.feed_token(tok.token_type, span).is_some() {
            let qualifier = detect_qualifier(source, &stmt_tokens[..=i], dialect);
            return CompletionInfo {
                tokens: last_expected,
                context: CompletionContext::from_parser(cursor_p.completion_context()),
                qualifier,
            };
        }
        last_expected.clear();
        last_expected.extend(cursor_p.expected_tokens());
    }

    let context = CompletionContext::from_parser(cursor_p.completion_context());

    if backtracked && let Some(extra) = tokens.get(boundary) {
        let span = extra.offset..(extra.offset + extra.length);
        if cursor_p.feed_token(extra.token_type, span).is_none() {
            merge_expected_tokens(
                &mut last_expected,
                cursor_p.expected_tokens().collect::<Vec<AnyTokenType>>(),
            );
        }
    }

    let qualifier = detect_qualifier(source, stmt_tokens, dialect);

    CompletionInfo {
        tokens: last_expected,
        context,
        qualifier,
    }
}

/// If the last two tokens are `Identifier` then `.`, return the identifier
/// text as the qualifier (used to detect `table.` prefixes for qualified
/// column completion).
fn detect_qualifier(
    source: &DocText,
    tokens: &[StoredToken],
    dialect: &AnyDialect,
) -> Option<String> {
    if tokens.len() < 2 {
        return None;
    }
    let dot_tok = &tokens[tokens.len() - 1];
    let ident_tok = &tokens[tokens.len() - 2];

    if dot_tok.length != DocLen::from_raw(1)
        || &source[DocRange::from_offset_len(dot_tok.offset, dot_tok.length)] != "."
    {
        return None;
    }

    let cat = dialect.token_category(ident_tok.token_type);
    if cat != TokenCategory::Identifier {
        return None;
    }

    let name = &source[DocRange::from_offset_len(ident_tok.offset, ident_tok.length)];
    Some(name.to_string())
}

fn completion_boundary(
    source: &DocText,
    tokens: &[StoredToken],
    cursor_offset: DocOffset,
) -> (usize, bool) {
    let mut boundary = tokens.partition_point(|t| t.offset + t.length <= cursor_offset);

    while boundary > 0 {
        let tok = &tokens[boundary - 1];
        if tok.length == DocLen::default() && tok.offset == cursor_offset {
            boundary -= 1;
        } else {
            break;
        }
    }

    let mut backtracked = false;
    if boundary > 0
        && tokens[boundary - 1].offset + tokens[boundary - 1].length == cursor_offset
        && cursor_offset > DocOffset::default()
    {
        let prev = source.as_str().as_bytes()[cursor_offset.as_usize() - 1];
        if prev.is_ascii_alphanumeric() || prev == b'_' {
            boundary -= 1;
            backtracked = true;
        }
    }
    (boundary, backtracked)
}

/// Find the index of the first token in the statement that contains `offset`.
///
/// Uses `TokenType::Semi` — safe across all dialects because `SQLite` token
/// ordinals are stable and equal to `AnyTokenType` ordinals.
fn statement_token_start(tokens: &[StoredToken], boundary: usize) -> usize {
    let semi = AnyTokenType::from(syntaqlite_syntax::TokenType::Semi);
    tokens[..boundary]
        .iter()
        .rposition(|t| t.token_type == semi)
        .map_or(0, |idx| idx + 1)
}

fn merge_expected_tokens(into: &mut Vec<AnyTokenType>, extra: Vec<AnyTokenType>) {
    let mut seen: HashSet<AnyTokenType> = into.iter().copied().collect();
    for token in extra {
        if seen.insert(token) {
            into.push(token);
        }
    }
}

// ── Signature help ───────────────────────────────────────────────────────────

/// Signature-help probe result: the enclosing function name, its arity
/// overloads, and the active parameter index.
pub(crate) struct SignatureHelpInfo {
    pub(crate) name: String,
    pub(crate) arities: Vec<AritySpec>,
    pub(crate) active_parameter: u32,
}

/// Walk backwards from `before.len()` to find the enclosing `func_name(` and
/// count commas at the outermost depth to determine the active parameter
/// index. Returns `(function_name, active_parameter)` when the cursor is
/// inside a call whose callee was classified as a `Function` token.
pub(crate) fn find_enclosing_call(
    before: &str,
    tokens: &[StoredToken],
    dialect: &AnyDialect,
) -> Option<(String, u32)> {
    let before_doc = DocText::new(before);
    let bytes = before.as_bytes();
    let mut depth: i32 = 0;
    let mut commas: u32 = 0;
    let mut pos = bytes.len();

    // Scan backwards to find the matching `(`.
    while pos > 0 {
        pos -= 1;
        match bytes[pos] {
            b')' => depth += 1,
            b'(' => {
                if depth == 0 {
                    // Found the opening paren — look for the function name token before it.
                    let paren_offset = DocOffset::from_raw(u32::try_from(pos).unwrap_or(u32::MAX));
                    let func_token = tokens.iter().rev().find(|t| {
                        t.offset + t.length <= paren_offset
                            && dialect.classify_token(t.token_type, t.flags)
                                == TokenCategory::Function
                    })?;
                    // Make sure the function token is immediately before the paren
                    // (only whitespace between).
                    let tok_end = func_token.offset + func_token.length;
                    let between = &before_doc[DocRange {
                        start: tok_end,
                        end: paren_offset,
                    }];
                    if between.trim().is_empty() {
                        let name = before_doc
                            [DocRange::from_offset_len(func_token.offset, func_token.length)]
                        .to_string();
                        return Some((name, commas));
                    }
                    return None;
                }
                depth -= 1;
            }
            b',' if depth == 0 => commas += 1,
            _ => {}
        }
    }
    None
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::*;
    use syntaqlite_syntax::ParserTokenFlags;

    #[test]
    fn detect_qualifier_basic() {
        let dialect = crate::sqlite::dialect::dialect();
        let source = DocText::new("SELECT t1.");
        let id_type = AnyTokenType::from(syntaqlite_syntax::TokenType::Id);
        let dot_type = AnyTokenType::from(syntaqlite_syntax::TokenType::Dot);

        let tokens = vec![
            StoredToken {
                offset: DocOffset::from_raw(7),
                length: DocLen::from_raw(2),
                token_type: id_type,
                flags: ParserTokenFlags::default(),
            },
            StoredToken {
                offset: DocOffset::from_raw(9),
                length: DocLen::from_raw(1),
                token_type: dot_type,
                flags: ParserTokenFlags::default(),
            },
        ];
        let result = detect_qualifier(source, &tokens, &dialect);
        assert_eq!(result.as_deref(), Some("t1"));
    }
}
