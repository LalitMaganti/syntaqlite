// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::cell::Cell;

use syntaqlite_syntax::source::{StmtLen, StmtOffset, StmtRange, StmtText};
use syntaqlite_syntax::{CommentKind, CommentSide};

use super::doc::{DocArena, DocId, NIL_DOC};

/// A comment, with the side the parser attached it to.
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

/// One source token's extent.
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

/// One walk over a statement's tokens, emitting the comments it passes.
///
/// The interpreter holds this shared, hence the `Cell` cursors.
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

    /// Give the vectors back so the formatter can reuse the allocations.
    pub(crate) fn into_parts(self) -> (Vec<CommentEntry>, Vec<TokenEntry>) {
        (self.comments, self.tokens)
    }

    /// Every comment, emitted or not.
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
        let mut out = DrainResult::default();
        let mut cursor = self.cursor.get();
        let mut last_end = self.prev_token_end();

        while cursor < self.comments.len() && self.comments[cursor].offset < before {
            let c = self.comments[cursor];
            let text = &source[c.range()];

            // Which side a comment sits on is fixed at parse time; the source
            // around it is read only to decide which blank lines survive.
            match (c.kind, c.side) {
                (CommentKind::Line, CommentSide::Leading) => {
                    let next = self
                        .comments
                        .get(cursor + 1)
                        .filter(|n| n.offset < before)
                        .map_or(before, |n| n.offset);
                    let after = gap(source, c.end(), next);
                    let blank_after = after.contains("\n\n");
                    // A comment on the next line brings its own separator.
                    let joined = next != before && !blank_after && after.contains('\n');
                    let blank_before = gap(source, last_end, c.offset).contains("\n\n");

                    // `CommentBreak` elides against a break a surrounding op
                    // already emitted, on either side, so the chunk carries one
                    // at each end without stacking into a blank line.
                    let prefix = if blank_before && out.leading != NIL_DOC {
                        arena.blank_line()
                    } else {
                        arena.comment_break()
                    };
                    let suffix = if joined {
                        NIL_DOC
                    } else if blank_after {
                        arena.blank_line()
                    } else {
                        arena.comment_break()
                    };
                    let doc = arena.text(text);
                    let chunk = arena.cats(&[prefix, doc, suffix]);
                    out.leading = arena.cat(out.leading, chunk);
                }
                (CommentKind::Line, CommentSide::Trailing) => {
                    let sp = arena.text(" ");
                    let doc = arena.text(text);
                    let inner = arena.cat(sp, doc);
                    let ls = arena.line_suffix(inner);
                    let bp = arena.break_parent();
                    let chunk = arena.cat(ls, bp);
                    out.trailing = arena.cat(out.trailing, chunk);
                }
                (CommentKind::Block, CommentSide::Leading) => {
                    let hl = arena.hardline();
                    let doc = arena.text(text);
                    let sp = arena.text(" ");
                    let chunk = arena.cats(&[hl, doc, sp]);
                    out.leading = arena.cat(out.leading, chunk);
                }
                (CommentKind::Block, CommentSide::Trailing) => {
                    let sp = arena.text(" ");
                    let doc = arena.text(text);
                    let chunk = arena.cat(sp, doc);
                    out.trailing = arena.cat(out.trailing, chunk);
                }
            }

            last_end = c.end();
            cursor += 1;
        }

        self.cursor.set(cursor);
        out
    }

    /// The tokens one keyword atom covers, as `(start offset, count)`.
    ///
    /// One word is one token, so the comparison is not a search: it checks
    /// that the atom stream and the token stream are still in step. `None`
    /// when they are not, and for whitespace atoms, which cover no token.
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

    /// Step over a keyword atom, emitting the comments the step passes.
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

    /// Step past everything before `offset` without emitting: the caller has
    /// already written that source range out verbatim, comments included.
    pub(crate) fn skip_upto(&self, offset: StmtOffset) {
        self.seek_tokens(offset);
        let mut idx = self.cursor.get();
        while self.comments.get(idx).is_some_and(|c| c.offset < offset) {
            idx += 1;
        }
        self.cursor.set(idx);
    }

    /// Motion is forward-only: a target behind the cursor moves nothing.
    fn seek_tokens(&self, offset: StmtOffset) {
        let mut idx = self.token_cursor.get();
        while self.tokens.get(idx).is_some_and(|t| t.offset < offset) {
            idx += 1;
        }
        self.token_cursor.set(idx);
    }

    /// The next unemitted comment.
    pub(crate) fn peek_comment(&self) -> Option<&CommentEntry> {
        self.comments.get(self.cursor.get())
    }

    /// Mark the next unemitted comment as emitted.
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

/// Source text between two offsets, empty if they do not span anything.
fn gap(source: &StmtText, from: StmtOffset, to: StmtOffset) -> &str {
    let limit = StmtOffset::default() + source.byte_len();
    let (start, end) = (from.min(limit), to.min(limit));
    if start >= end {
        return "";
    }
    &source[StmtRange { start, end }]
}
