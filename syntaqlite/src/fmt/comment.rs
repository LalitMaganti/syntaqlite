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

impl Default for DrainResult {
    fn default() -> Self {
        DrainResult {
            trailing: NIL_DOC,
            leading: NIL_DOC,
        }
    }
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

    /// End offset of the token just before the cursor, or 0 at the start.
    fn prev_token_end(&self) -> StmtOffset {
        let idx = self.token_cursor.get();
        if idx > 0 {
            self.tokens[idx - 1].end()
        } else {
            StmtOffset::default()
        }
    }

    /// Emit every comment lying before `before` that is still unemitted.
    fn emit_comments_before<'a>(
        &self,
        before: StmtOffset,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DrainResult {
        let mut trailing = NIL_DOC;
        let mut leading = NIL_DOC;
        let mut cursor = self.cursor.get();
        let mut last_end = self.prev_token_end();
        let source_end = StmtOffset::default() + source.byte_len();
        while cursor < self.comments.len() && self.comments[cursor].offset < before {
            let t = &self.comments[cursor];

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

    /// The tokens one keyword atom covers, as `(start offset, count)`.
    ///
    /// Every fmt atom corresponds to one source token, so the atom stream and
    /// the token stream step together: the atom's word count is how many
    /// tokens it covers, and the cursor is where they are. The comparison is
    /// not a search, it is the check that the two are still in step.
    ///
    /// `None` for an atom that covers no token: one the source does not
    /// contain, and whitespace atoms, which are pure layout.
    fn keyword_at_cursor(&self, kw_text: &str, source: &StmtText) -> Option<(StmtOffset, usize)> {
        let first = self.token_cursor.get();
        let mut words = 0usize;
        for word in kw_text.split_whitespace() {
            let tok = self.tokens.get(first + words)?;
            if !source[tok.range()].eq_ignore_ascii_case(word) {
                return None;
            }
            words += 1;
        }
        (words > 0).then(|| (self.tokens[first].offset, words))
    }

    /// Emit the comments sitting before the token at the cursor.
    pub(crate) fn take_comments<'a>(
        &self,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DrainResult {
        match self.tokens.get(self.token_cursor.get()) {
            Some(tok) => self.emit_comments_before(tok.offset, source, arena),
            None => DrainResult::default(),
        }
    }

    /// Step over the tokens one keyword atom covers, emitting the comments
    /// the step passes. Emitting reads the cursor, so it happens first.
    pub(crate) fn take_keyword<'a>(
        &self,
        kw_text: &str,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DrainResult {
        let Some((offset, words)) = self.keyword_at_cursor(kw_text, source) else {
            return DrainResult::default();
        };
        let drain = self.emit_comments_before(offset, source, arena);
        self.token_cursor.set(self.token_cursor.get() + words);
        drain
    }

    /// Step over the tokens one keyword atom covers without emitting anything.
    pub(crate) fn skip_keyword(&self, kw_text: &str, source: &StmtText) {
        if let Some((_, words)) = self.keyword_at_cursor(kw_text, source) {
            self.token_cursor.set(self.token_cursor.get() + words);
        }
    }

    /// Step onto the token at `offset`, emitting the comments the step passes.
    pub(crate) fn take_upto<'a>(
        &self,
        offset: StmtOffset,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DrainResult {
        let drain = self.emit_comments_before(offset, source, arena);
        self.seek_tokens(offset);
        drain
    }

    /// Step past everything before `offset`, emitting nothing. For a caller
    /// that has already written that source range out verbatim, comments
    /// included.
    pub(crate) fn skip_upto(&self, offset: StmtOffset) {
        self.seek_tokens(offset);
        self.cursor
            .set(advance_to(&self.comments, self.cursor.get(), offset, |c| {
                c.offset
            }));
    }

    /// Move the token cursor forward to `offset`. Motion is forward-only, so
    /// a target behind the cursor leaves the cursor where it is.
    fn seek_tokens(&self, offset: StmtOffset) {
        self.token_cursor.set(advance_to(
            &self.tokens,
            self.token_cursor.get(),
            offset,
            |t| t.offset,
        ));
    }

    /// Peek at the next unemitted comment without advancing the cursor.
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

    /// Where the cursor is, or `None` once it is past the last token.
    pub(crate) fn cursor_offset(&self) -> Option<StmtOffset> {
        self.tokens.get(self.token_cursor.get()).map(|t| t.offset)
    }

    /// Flush every comment this statement still owns.
    pub(crate) fn drain_remaining<'a>(
        &self,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> DocId {
        let drain = self.emit_comments_before(StmtOffset::from_raw(u32::MAX), source, arena);
        arena.cat(drain.trailing, drain.leading)
    }
}

/// First index at or after `from` whose entry starts at or after `offset`.
fn advance_to<T>(
    items: &[T],
    from: usize,
    offset: StmtOffset,
    start: impl Fn(&T) -> StmtOffset,
) -> usize {
    let mut idx = from;
    while idx < items.len() && start(&items[idx]) < offset {
        idx += 1;
    }
    idx
}
