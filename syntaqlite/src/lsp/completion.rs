// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP-specific analysis helpers: semantic tokens, completion boundaries,
//! qualifier detection, and expected-token computation.
//!
//! These operate on an already-analyzed [`SemanticModel`] plus the dialect
//! and are not part of semantic validation — they answer editor queries.

use std::collections::HashSet;

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{AnyParser, AnyTokenType, TokenCategory};

use crate::dialect::AnyDialect;
use crate::semantic::model::{CompletionContext, CompletionInfo, SemanticModel, StoredToken};

/// Expected tokens and semantic context at `offset` (for completion).
pub(crate) fn completion_info(
    dialect: &AnyDialect,
    model: &SemanticModel,
    offset: usize,
) -> CompletionInfo {
    let source = model.source();
    let tokens = &model.tokens;
    let cursor = offset.min(source.len());
    let (boundary, backtracked) = completion_boundary(source, tokens, cursor);
    let start = statement_token_start(tokens, boundary);
    let stmt_tokens = &tokens[start..boundary];

    let syntax = (**dialect).clone();
    let parser = AnyParser::with_config(syntax, &ParserConfig::default());
    let mut cursor_p = parser.incremental_parse(source);
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
fn detect_qualifier(source: &str, tokens: &[StoredToken], dialect: &AnyDialect) -> Option<String> {
    if tokens.len() < 2 {
        return None;
    }
    let dot_tok = &tokens[tokens.len() - 1];
    let ident_tok = &tokens[tokens.len() - 2];

    if dot_tok.length != 1 || source.as_bytes().get(dot_tok.offset) != Some(&b'.') {
        return None;
    }

    let cat = dialect.token_category(ident_tok.token_type);
    if cat != TokenCategory::Identifier {
        return None;
    }

    let name = &source[ident_tok.offset..ident_tok.offset + ident_tok.length];
    Some(name.to_string())
}

fn completion_boundary(
    source: &str,
    tokens: &[StoredToken],
    cursor_offset: usize,
) -> (usize, bool) {
    let mut boundary = tokens.partition_point(|t| t.offset + t.length <= cursor_offset);

    while boundary > 0 {
        let tok = &tokens[boundary - 1];
        if tok.length == 0 && tok.offset == cursor_offset {
            boundary -= 1;
        } else {
            break;
        }
    }

    let mut backtracked = false;
    if boundary > 0
        && tokens[boundary - 1].offset + tokens[boundary - 1].length == cursor_offset
        && cursor_offset > 0
    {
        let prev = source.as_bytes()[cursor_offset - 1];
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

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::*;
    use syntaqlite_syntax::ParserTokenFlags;

    #[test]
    fn detect_qualifier_basic() {
        let dialect = crate::sqlite::dialect::dialect();
        let source = "SELECT t1.";
        let id_type = AnyTokenType::from(syntaqlite_syntax::TokenType::Id);
        let dot_type = AnyTokenType::from(syntaqlite_syntax::TokenType::Dot);

        let tokens = vec![
            StoredToken {
                offset: 7,
                length: 2,
                token_type: id_type,
                flags: ParserTokenFlags::default(),
            },
            StoredToken {
                offset: 9,
                length: 1,
                token_type: dot_type,
                flags: ParserTokenFlags::default(),
            },
        ];
        let result = detect_qualifier(source, &tokens, &dialect);
        assert_eq!(result.as_deref(), Some("t1"));
    }
}
