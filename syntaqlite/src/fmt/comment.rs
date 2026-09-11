// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::cell::{Cell, RefCell};

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
    pub inside_token: bool,
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
    pub source_index: u32,
    pub offset: StmtOffset,
    pub length: StmtLen,
    /// End of the comment batch attached before this surviving token.
    pub comment_end: u32,
}

impl TokenEntry {
    fn range(self) -> StmtRange {
        StmtRange::from_offset_len(self.offset, self.length)
    }

    fn end(self) -> StmtOffset {
        self.offset + self.length
    }
}

/// Document layout for an explicit batch of parser attachments. Trailing docs (e.g. `LineSuffix` for
/// end-of-line comments) go BEFORE any pending line break. Leading docs
/// (comments on their own line) go AFTER any pending line break.
pub(crate) struct CommentDocs {
    pub trailing: DocId,
    pub leading: DocId,
    pub force_break: bool,
}

/// Parser attachments indexed at surviving token boundaries.
/// Shared via `&` across iterative formatting traversal; interior mutability is
/// required because interpreter state carries a shared `&CommentCtx`.
///
/// Owns its comment and token data (no lifetime parameter).
pub(crate) struct CommentCtx {
    comments: Vec<CommentEntry>,
    tokens: Vec<TokenEntry>,
    boundary_claimed: Cell<bool>,
    verbatim_end: Cell<StmtOffset>,
    verbatim_start: Cell<StmtOffset>,
    token_cursor: Cell<usize>,
    bindings: RefCell<Vec<std::ops::Range<usize>>>,
    error: RefCell<Option<String>>,
    after_block: Cell<bool>,
}

impl CommentCtx {
    pub(crate) fn new(comments: Vec<CommentEntry>, tokens: Vec<TokenEntry>) -> Self {
        CommentCtx {
            comments,
            tokens,
            boundary_claimed: Cell::new(false),
            verbatim_end: Cell::new(StmtOffset::default()),
            verbatim_start: Cell::new(StmtOffset::default()),
            token_cursor: Cell::new(0),
            bindings: RefCell::new(Vec::new()),
            error: RefCell::new(None),
            after_block: Cell::new(false),
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

    fn boundary_range(&self) -> std::ops::Range<usize> {
        let index = self.token_cursor.get();
        let end = self
            .tokens
            .get(index)
            .map_or(self.comments.len(), |t| t.comment_end as usize);
        let start = index
            .checked_sub(1)
            .map_or(0, |i| self.tokens[i].comment_end as usize);
        start..end
    }

    fn take_boundary(&self) -> std::ops::Range<usize> {
        if self.boundary_claimed.replace(true) {
            return 0..0;
        }
        self.boundary_range()
    }

    /// Emit the batch already assigned to this token boundary. No source-offset
    /// search or ownership decisions are made during document interpretation.
    pub(crate) fn attachments<'a>(
        &self,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> CommentDocs {
        self.layout(self.take_boundary(), source, arena)
    }

    pub(crate) fn verbatim_attachments<'a>(
        &self,
        start: StmtOffset,
        end: StmtOffset,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> CommentDocs {
        self.verbatim_start.set(start);
        self.verbatim_end.set(end);
        self.attachments(source, arena)
    }

    #[expect(clippy::too_many_lines)]
    fn layout<'a>(
        &self,
        range: std::ops::Range<usize>,
        source: &'a StmtText,
        arena: &mut DocArena<'a>,
    ) -> CommentDocs {
        if range.is_empty() {
            return CommentDocs {
                trailing: NIL_DOC,
                leading: NIL_DOC,
                force_break: false,
            };
        }
        let mut force_break = false;
        let mut trailing = NIL_DOC;
        let mut leading = NIL_DOC;
        let before = self
            .tokens
            .get(self.token_cursor.get())
            .map_or(StmtOffset::default() + source.byte_len(), |t| t.offset);
        let mut last_end = self.prev_token_end();
        let source_end = StmtOffset::default() + source.byte_len();
        for cursor in range.clone() {
            let t = &self.comments[cursor];
            // A verbatim atom already contains these comments in its text.
            if t.inside_token
                || (t.offset >= self.verbatim_start.get() && t.offset < self.verbatim_end.get())
            {
                continue;
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
                            .filter(|_| cursor + 1 < range.end)
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
                        force_break = true;
                        let space = arena.text(" ");
                        let comment = arena.text(text);
                        let inner = arena.cat(space, comment);
                        let ls = arena.line_suffix(inner);
                        let bp = arena.break_parent();
                        let chunk = arena.cat(ls, bp);
                        trailing = if trailing == NIL_DOC {
                            chunk
                        } else if cursor > 0 && self.comments[cursor - 1].kind == CommentKind::Line
                        {
                            // Continuations occupy their own physical lines; a
                            // second LineSuffix would otherwise migrate to SQL
                            // after an elided adjacent document break.
                            let boundary = arena.comment_break();
                            let text = arena.text(text);
                            arena.cats(&[trailing, boundary, text, boundary])
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

            self.after_block
                .set(t.kind == CommentKind::Block && t.side == CommentSide::Trailing);
            last_end = t.end();
        }

        CommentDocs {
            trailing,
            leading,
            force_break,
        }
    }

    /// Block comments separate atoms, except before closing punctuation.
    /// Template whitespace already supplies the separator when present.
    pub(crate) fn block_separator(&self, next: &str) -> bool {
        self.after_block.replace(false)
            && next.as_bytes().first().is_some_and(|c| {
                !c.is_ascii_whitespace() && !matches!(c, b',' | b';' | b')' | b']')
            })
    }

    pub(crate) fn push_binding(&self, range: Option<std::ops::Range<usize>>) {
        if let Some(range) = range {
            self.bindings.borrow_mut().push(range);
        } else {
            self.fail("formatter syntax role is missing".into());
            self.bindings.borrow_mut().push(0..0);
        }
    }

    pub(crate) fn pop_binding(&self) {
        self.bindings
            .borrow_mut()
            .pop()
            .expect("balanced source bytecode");
    }

    fn fail(&self, message: String) {
        self.error.borrow_mut().get_or_insert(message);
    }

    pub(crate) fn take_error(&self) -> Option<String> {
        self.error.borrow_mut().take()
    }

    /// Resolve exactly the next authored terminal. A declared source role
    /// permits canonical spelling; an empty role denotes inserted syntax.
    /// There is no forward search or skipped-token recovery.
    pub(crate) fn terminal(&self, text: &str, source: &StmtText) -> Option<StmtOffset> {
        let bindings = self.bindings.borrow();
        let bound = bindings.last();
        if bound.is_some_and(std::ops::Range::is_empty) {
            return None;
        }
        let Some(token) = self.tokens.get(self.token_cursor.get()) else {
            self.fail(format!("formatter introduced an unbound terminal: {text}"));
            return None;
        };
        if bound.is_some_and(|range| !range.contains(&(token.source_index as usize)))
            || (bound.is_none() && !source[token.range()].eq_ignore_ascii_case(text))
        {
            self.fail(format!(
                "formatter source mismatch: expected {text}, got {}",
                &source[token.range()]
            ));
            return None;
        }
        Some(token.offset)
    }

    /// A source span may cover several tokens, but must not skip a live anchor.
    pub(crate) fn check_span_start(&self, start: StmtOffset) {
        if self
            .tokens
            .get(self.token_cursor.get())
            .is_some_and(|t| t.offset < start)
        {
            self.fail("formatter span skipped an unconsumed source anchor".into());
        }
    }

    /// Statement terminators are emitted by Formatter, outside AST bytecode.
    pub(crate) fn finish(&self, source: &StmtText) {
        let remaining = &self.tokens[self.token_cursor.get()..];
        if !(remaining.is_empty() || remaining.len() == 1 && &source[remaining[0].range()] == ";") {
            self.fail("formatter left unconsumed source anchors".into());
        }
    }

    /// Advance the token cursor by `n` positions.
    pub(crate) fn advance_token_cursor(&self, n: usize) {
        self.token_cursor.set(self.token_cursor.get() + n);
        self.boundary_claimed.set(false);
    }

    /// Advance the token cursor past all tokens whose offset is `< end_offset`.
    pub(crate) fn advance_past(&self, end_offset: StmtOffset) {
        let mut idx = self.token_cursor.get();
        while idx < self.tokens.len() && self.tokens[idx].offset < end_offset {
            idx += 1;
        }
        if idx != self.token_cursor.get() {
            self.boundary_claimed.set(false);
            self.token_cursor.set(idx);
        }
    }

    /// Verbatim atoms consume whole token ranges, including their interior
    /// attachments. Those batches are skipped with the tokens, not searched.
    pub(crate) fn consume_verbatim(&self, end: StmtOffset) {
        self.advance_past(end);
        self.verbatim_end.set(end);
    }

    /// Statement-leading attachments use the surrounding statement layout.
    pub(crate) fn header<'a>(&self, source: &'a StmtText, arena: &mut DocArena<'a>) -> DocId {
        let range = self.take_boundary();
        let next_token = self
            .tokens
            .first()
            .map_or(StmtOffset::default() + source.byte_len(), |t| t.offset);
        let mut doc = NIL_DOC;
        for index in range.clone() {
            let c = self.comments[index];
            if c.inside_token {
                continue;
            }
            let text = arena.text(&source[c.range()]);
            let line = arena.hardline();
            doc = arena.cats(&[doc, text, line]);
            let next = self
                .comments
                .get(index + 1)
                .filter(|_| index + 1 < range.end)
                .map_or(next_token, |c| c.offset);
            if c.end() < next
                && source[StmtRange {
                    start: c.end(),
                    end: next,
                }]
                .contains("\n\n")
            {
                doc = arena.cat(doc, line);
            }
        }
        doc
    }

    /// Peek at the next token's offset and length without advancing.
    pub(crate) fn peek_next_token(&self) -> Option<(StmtOffset, StmtLen)> {
        let idx = self.token_cursor.get();
        self.tokens.get(idx).map(|tp| (tp.offset, tp.length))
    }

    /// Emit the statement's final attachments, including those on its terminator.
    pub(crate) fn footer<'a>(&self, source: &'a StmtText, arena: &mut DocArena<'a>) -> DocId {
        let boundary = self.boundary_range();
        let start = if self.boundary_claimed.get() {
            boundary.end
        } else {
            boundary.start
        };
        let docs = self.layout(start..self.comments.len(), source, arena);
        arena.cat(docs.trailing, docs.leading)
    }
}
