// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::cell::Cell;

use syntaqlite_syntax::source::{StmtLen, StmtOffset, StmtRange, StmtText};
use syntaqlite_syntax::{CommentKind, CommentSide};

use super::doc::{DocArena, DocId, NIL_DOC};

/// A collected comment entry with pre-computed byte offset, length, and
/// parser-supplied attachment (`side` + `token_idx`).
#[derive(Clone, Copy)]
pub(crate) struct CommentEntry {
    pub offset: StmtOffset,
    pub length: StmtLen,
    pub kind: CommentKind,
    pub side: CommentSide,
}

impl CommentEntry {
    fn range(self) -> StmtRange {
        StmtRange::from_offset_len(self.offset, self.length)
    }

    fn end(self) -> StmtOffset {
        self.offset + self.length
    }
}

/// A collected token entry with pre-computed byte offset and length.
#[derive(Clone, Copy)]
pub(crate) struct TokenEntry {
    pub offset: StmtOffset,
    pub length: StmtLen,
}

impl TokenEntry {
    fn range(self) -> StmtRange {
        StmtRange::from_offset_len(self.offset, self.length)
    }

    fn end(self) -> StmtOffset {
        self.offset + self.length
    }
}

/// Result of draining comment items. Trailing docs (e.g. `LineSuffix` for
/// end-of-line comments) go BEFORE any pending line break. Leading docs
/// (comments on their own line) go AFTER any pending line break.
pub(crate) struct DrainResult {
    pub trailing: DocId,
    pub leading: DocId,
}

/// Two cursors advancing monotonically through sorted comment and token arrays.
/// Shared via `&` across iterative formatting traversal; interior mutability is
/// required because interpreter state carries a shared `&CommentCtx`.
///
/// Owns its comment and token data (no lifetime parameter).
pub(crate) struct CommentCtx {
    comments: Vec<CommentEntry>,
    tokens: Vec<TokenEntry>,
    cursor: Cell<usize>,
    token_cursor: Cell<usize>,
}

impl CommentCtx {
    pub(crate) fn new(comments: Vec<CommentEntry>, tokens: Vec<TokenEntry>) -> Self {
        CommentCtx {
            comments,
            tokens,
            cursor: Cell::new(0),
            token_cursor: Cell::new(0),
        }
    }

    /// Return owned storage so callers can recycle vector allocations.
    pub(crate) fn into_parts(self) -> (Vec<CommentEntry>, Vec<TokenEntry>) {
        (self.comments, self.tokens)
    }

    /// Borrow the comment entries. The slice stays valid for the life
    /// of `self`; callers that need to pass the comments to a helper
    /// (e.g. `compute_macro_docs`) while still owning the `CommentCtx`
    /// use this instead of re-moving the vec out.
    pub(crate) fn comments(&self) -> &[CommentEntry] {
        &self.comments
    }

    /// End offset of the token just before the current token cursor position.
    /// Returns 0 if the cursor is at the start.
    pub(crate) fn prev_token_end(&self) -> StmtOffset {
        let idx = self.token_cursor.get();
        if idx > 0 {
            self.tokens[idx - 1].end()
        } else {
            StmtOffset::default()
        }
    }

    /// Drain all comments with offset < `before`.
    ///
    /// Stops early if there is non-whitespace source text (i.e. a keyword)
    /// between a comment and `before`.
    pub(crate) fn drain_before<'a>(
        &self,
        before: StmtOffset,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DrainResult {
        self.drain_impl(before, source, arena, false)
    }

    #[expect(clippy::too_many_lines)]
    fn drain_impl<'a>(
        &self,
        before: StmtOffset,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
        skip_text_check: bool,
    ) -> DrainResult {
        let mut trailing = NIL_DOC;
        let mut leading = NIL_DOC;
        let mut cursor = self.cursor.get();
        let mut last_end = self.prev_token_end();
        let source_end = StmtOffset::default() + source.byte_len();
        while cursor < self.comments.len() && self.comments[cursor].offset < before {
            let t = &self.comments[cursor];

            if !skip_text_check {
                let scan_end = before.min(source_end);
                if t.end() < scan_end
                    && has_intervening_emitted_token(source, t.end(), scan_end, &self.tokens)
                {
                    break;
                }
            }

            let text = &source[t.range()];

            // Leading vs trailing is fixed at parse time
            // (see synq_parser_record_comment).  The gap text is still
            // scanned below for blank-line preservation between adjacent
            // leading comments.
            let gap = StmtRange {
                start: last_end.min(source_end),
                end: t.offset.min(source_end),
            };
            let is_leading = matches!(t.side, CommentSide::Leading);

            match t.kind {
                CommentKind::Line => {
                    if is_leading {
                        // Source gaps on both sides of this comment —
                        // used to detect blank lines that must survive.
                        let next_offset = self
                            .comments
                            .get(cursor + 1)
                            .filter(|n| n.offset < before)
                            .map_or(before, |n| n.offset);
                        let tail_gap = StmtRange {
                            start: t.end().min(source_end),
                            end: next_offset.min(source_end),
                        };
                        let has_blank_before = !gap.is_empty() && source[gap].contains("\n\n");
                        let has_blank_after =
                            !tail_gap.is_empty() && source[tail_gap].contains("\n\n");
                        let next_is_contiguous_comment = !has_blank_after
                            && next_offset != before
                            && !tail_gap.is_empty()
                            && source[tail_gap].contains('\n');

                        // Prefix: `CommentBreak` so it elides when a
                        // surrounding fmt op already emitted a break.
                        // Chunks after the first prepend a `HardLine` to
                        // preserve a blank line between comment blocks —
                        // the `HardLine` elides against the prior chunk's
                        // trailing `CommentBreak`, clearing both render
                        // flags so the `CommentBreak` here still fires.
                        let cb_prefix = arena.comment_break();
                        let prefix = if has_blank_before && leading != NIL_DOC {
                            let hl = arena.hardline();
                            arena.cat(hl, cb_prefix)
                        } else {
                            cb_prefix
                        };

                        // Trailing: depends on what follows in source.
                        //   contiguous next comment → no trailing (its
                        //     own prefix provides the separator)
                        //   blank line before the next event → `HardLine`
                        //     always emits the blank line, `CommentBreak`
                        //     still silences any break from the next op
                        //   otherwise → plain `CommentBreak`
                        let trailing = if next_is_contiguous_comment {
                            NIL_DOC
                        } else if has_blank_after {
                            let hl = arena.hardline();
                            let cb = arena.comment_break();
                            arena.cat(hl, cb)
                        } else {
                            arena.comment_break()
                        };

                        let comment_doc = arena.text(text);
                        let chunk = arena.cats(&[prefix, comment_doc, trailing]);
                        leading = arena.cat(leading, chunk);
                    } else {
                        let space = arena.text(" ");
                        let comment = arena.text(text);
                        let inner = arena.cat(space, comment);
                        let ls = arena.line_suffix(inner);
                        let bp = arena.break_parent();
                        let chunk = arena.cat(ls, bp);
                        trailing = if trailing == NIL_DOC {
                            chunk
                        } else {
                            arena.cat(trailing, chunk)
                        };
                    }
                }
                CommentKind::Block => {
                    if is_leading {
                        let hl = arena.hardline();
                        let comment_doc = arena.text(text);
                        let sp = arena.text(" ");
                        let chunk = arena.cats(&[hl, comment_doc, sp]);
                        leading = if leading == NIL_DOC {
                            chunk
                        } else {
                            arena.cat(leading, chunk)
                        };
                    } else {
                        let sp = arena.text(" ");
                        let comment_doc = arena.text(text);
                        let chunk = arena.cat(sp, comment_doc);
                        trailing = if trailing == NIL_DOC {
                            chunk
                        } else {
                            arena.cat(trailing, chunk)
                        };
                    }
                }
            }

            last_end = t.end();
            cursor += 1;
        }

        self.cursor.set(cursor);

        DrainResult { trailing, leading }
    }

    /// Find the next occurrence of a keyword in the token stream, starting
    /// from the current token cursor.
    ///
    /// Verifies each token's text matches the corresponding keyword word
    /// (case-insensitive). If the keyword starts at the current cursor
    /// position, returns immediately. Otherwise, scans forward up to
    /// `MAX_SCAN` tokens to handle untracked tokens (e.g. `(` and `)` from
    /// dialect-level syntax that no fmt opcode covers).
    ///
    /// On match, the token cursor is advanced past any skipped tokens so it
    /// points to the first word of the keyword. Returns `None` if the
    /// keyword is not present in the source (e.g., an inserted `AS`).
    pub(crate) fn peek_keyword_tokens(
        &self,
        kw_text: &str,
        source: &StmtText,
    ) -> Option<(StmtOffset, usize)> {
        const MAX_SCAN: usize = 8;
        let start_idx = self.token_cursor.get();

        for scan in 0..MAX_SCAN {
            let first_idx = start_idx + scan;
            if first_idx >= self.tokens.len() {
                return None;
            }
            let mut word_count = 0usize;
            let mut matched = true;
            for word in kw_text.split_whitespace() {
                let tok_idx = first_idx + word_count;
                if tok_idx >= self.tokens.len() {
                    matched = false;
                    break;
                }
                let tok = &self.tokens[tok_idx];
                let tok_text = &source[tok.range()];
                if !tok_text.eq_ignore_ascii_case(word) {
                    matched = false;
                    break;
                }
                word_count += 1;
            }
            if matched && word_count > 0 {
                // Advance past any skipped tokens.
                if scan > 0 {
                    self.token_cursor.set(first_idx);
                }
                let first_offset = self.tokens[first_idx].offset;
                return Some((first_offset, word_count));
            }
        }
        None
    }

    /// Advance the token cursor by `n` positions.
    pub(crate) fn advance_token_cursor(&self, n: usize) {
        self.token_cursor.set(self.token_cursor.get() + n);
    }

    /// Advance the token cursor past all tokens whose offset is `< end_offset`.
    pub(crate) fn advance_past(&self, end_offset: StmtOffset) {
        let mut idx = self.token_cursor.get();
        while idx < self.tokens.len() && self.tokens[idx].offset < end_offset {
            idx += 1;
        }
        self.token_cursor.set(idx);
    }

    /// Mark comments with offset `< end_offset` as consumed. Use after
    /// emitting a verbatim source range (e.g. the body of a `span()` op)
    /// that already contains the comment text — otherwise the comments
    /// stay in the queue and a later `drain_remaining` will slice a
    /// reversed `[prev_token_end, comment_offset)` range and panic.
    pub(crate) fn discard_comments_before(&self, end_offset: StmtOffset) {
        let mut idx = self.cursor.get();
        while idx < self.comments.len() && self.comments[idx].offset < end_offset {
            idx += 1;
        }
        self.cursor.set(idx);
    }

    /// Peek at the next undrained comment without advancing the cursor.
    pub(crate) fn peek_comment(&self) -> Option<&CommentEntry> {
        let idx = self.cursor.get();
        self.comments.get(idx)
    }

    /// Advance the comment cursor by one.
    pub(crate) fn advance_comment(&self) {
        let idx = self.cursor.get();
        if idx < self.comments.len() {
            self.cursor.set(idx + 1);
        }
    }

    /// Peek at the next token's offset and length without advancing.
    pub(crate) fn peek_next_token(&self) -> Option<(StmtOffset, StmtLen)> {
        let idx = self.token_cursor.get();
        self.tokens.get(idx).map(|tp| (tp.offset, tp.length))
    }

    /// Flush all remaining comments.  Bypasses the `has_non_comment_text`
    /// guard because, at end-of-statement drain, every remaining comment
    /// is a trailing comment that this statement owns; the guard's check
    /// for "syntax text past the comment" would spuriously fire when the
    /// source text continues into the next statement.
    pub(crate) fn drain_remaining<'a>(
        &self,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DocId {
        let drain = self.drain_impl(StmtOffset::from_raw(u32::MAX), source, arena, true);
        arena.cat(drain.trailing, drain.leading)
    }
}

/// Returns true if the token stream contains a token in `[start, end)` that
/// the formatter will emit — i.e. any token other than a vestigial `(` / `)`.
///
/// The parser rule `expr ::= LP expr RP` is erased via `synq_pass`
/// (parser-actions/expressions.y:30), so the inner `(` and `)` tokens remain
/// in the token stream without any corresponding fmt opcode consuming them.
/// They're not obstacles between a comment and its drain target; the drain
/// must be allowed to step over them. All other token kinds (keywords,
/// identifiers, operators, and paren tokens in tracked positions like
/// function calls / IN / CAST) either sit at the drain target or will be
/// emitted by some fmt opcode, so they *do* block a cross-drain.
fn has_intervening_emitted_token(
    source: &StmtText,
    start: StmtOffset,
    end: StmtOffset,
    tokens: &[TokenEntry],
) -> bool {
    // Tokens are sorted by offset; binary-search for the first one that could
    // overlap [start, end) to keep this O(log n + k) per call.
    let first = tokens.partition_point(|t| t.end() <= start);
    for tok in &tokens[first..] {
        if tok.offset >= end {
            break;
        }
        let text = &source[tok.range()];
        if text != "(" && text != ")" {
            return true;
        }
    }
    false
}
