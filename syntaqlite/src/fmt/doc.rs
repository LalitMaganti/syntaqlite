// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use super::{FormatConfig, KeywordCase};

/// A handle into the `DocArena`. `NIL_DOC` represents an empty/absent document.
pub(super) type DocId = u32;

/// Sentinel value meaning "no document". Builder methods treat `NIL_DOC` operands
/// as identity elements (e.g. `cat(NIL_DOC, x) == x`).
pub(super) const NIL_DOC: DocId = u32::MAX;

/// A node in the Wadler-style document algebra. Lifetime `'a` covers borrowed text slices.
#[derive(Debug, Clone)]
enum Doc<'a> {
    /// Source text (identifiers, literals). Never case-transformed.
    Text(&'a str),
    /// SQL keyword. Subject to `KeywordCase` at render time.
    Keyword(&'a str),
    /// Flat mode: space. Break mode: newline + indent.
    Line,
    /// Flat mode: empty. Break mode: newline + indent.
    SoftLine,
    /// Always newline + indent.
    HardLine,
    /// Concatenation of two documents.
    Cat { left: DocId, right: DocId },
    /// Increase indent by `indent` for `child`.
    Nest { indent: i16, child: DocId },
    /// Try to fit `child` on one line; break if it doesn't fit.
    Group { child: DocId },
    /// Defer `child` to end of current line (for trailing comments).
    LineSuffix { child: DocId },
    /// Force the enclosing group to break.
    BreakParent,
}

/// Arena-based storage for `Doc` nodes. Push-to-allocate, indexed by `DocId`.
#[derive(Debug)]
pub(crate) struct DocArena<'a> {
    docs: Vec<Doc<'a>>,
}

impl<'a> DocArena<'a> {
    pub(crate) fn new() -> Self {
        DocArena { docs: Vec::new() }
    }

    /// Create a new arena with pre-allocated capacity.
    pub(crate) fn with_capacity(cap: usize) -> Self {
        DocArena {
            docs: Vec::with_capacity(cap),
        }
    }

    /// Create a new arena, reusing the allocation from a previous arena.
    ///
    /// The old arena is consumed. The new arena has the same backing
    /// memory but a fresh (possibly different) lifetime parameter.
    pub(crate) fn recycle(mut old: DocArena<'_>) -> Self {
        old.docs.clear();
        let mut old_docs = std::mem::ManuallyDrop::new(old.docs);
        let ptr = old_docs.as_mut_ptr();
        let cap = old_docs.capacity();
        // SAFETY: Doc<'old> and Doc<'new> have identical layout (enum of
        // pointers/ids). Clearing removed all elements, so no stale
        // borrows exist. We reuse the raw allocation with a new lifetime.
        let docs = unsafe { Vec::from_raw_parts(ptr.cast::<Doc<'a>>(), 0, cap) };
        DocArena { docs }
    }

    fn push(&mut self, doc: Doc<'a>) -> DocId {
        let id = usize_to_doc_id(self.docs.len());
        debug_assert!(id != NIL_DOC, "DocArena overflow");
        self.docs.push(doc);
        id
    }

    fn get(&self, id: DocId) -> &Doc<'a> {
        &self.docs[id as usize]
    }

    // -- Builder methods --

    pub(crate) fn text(&mut self, s: &'a str) -> DocId {
        self.push(Doc::Text(s))
    }

    pub(crate) fn keyword(&mut self, s: &'a str) -> DocId {
        self.push(Doc::Keyword(s))
    }

    pub(crate) fn line(&mut self) -> DocId {
        self.push(Doc::Line)
    }

    pub(crate) fn softline(&mut self) -> DocId {
        self.push(Doc::SoftLine)
    }

    pub(crate) fn hardline(&mut self) -> DocId {
        self.push(Doc::HardLine)
    }

    /// Concatenate two documents. If either operand is `NIL_DOC`, returns the other.
    pub(crate) fn cat(&mut self, left: DocId, right: DocId) -> DocId {
        if left == NIL_DOC {
            return right;
        }
        if right == NIL_DOC {
            return left;
        }
        self.push(Doc::Cat { left, right })
    }

    /// Concatenate a slice of documents left-to-right. Skips `NIL_DOC` entries.
    pub(crate) fn cats(&mut self, ids: &[DocId]) -> DocId {
        let mut result = NIL_DOC;
        for &id in ids {
            result = self.cat(result, id);
        }
        result
    }

    /// Nest (indent) `child` by `indent` spaces. Returns `NIL_DOC` if child is nil.
    pub(crate) fn nest(&mut self, indent: i16, child: DocId) -> DocId {
        if child == NIL_DOC {
            return NIL_DOC;
        }
        self.push(Doc::Nest { indent, child })
    }

    /// Group `child` — try flat first, break if it doesn't fit. Returns `NIL_DOC` if child is nil.
    pub(crate) fn group(&mut self, child: DocId) -> DocId {
        if child == NIL_DOC {
            return NIL_DOC;
        }
        self.push(Doc::Group { child })
    }

    pub(crate) fn line_suffix(&mut self, child: DocId) -> DocId {
        if child == NIL_DOC {
            return NIL_DOC;
        }
        self.push(Doc::LineSuffix { child })
    }

    pub(crate) fn break_parent(&mut self) -> DocId {
        self.push(Doc::BreakParent)
    }

    // -- Render --

    /// Render the document tree rooted at `root` to a string.
    #[cfg(test)]
    pub(crate) fn render(&self, root: DocId, config: &FormatConfig) -> String {
        let mut bufs = RenderBuffers::new();
        self.render_into(root, config, &mut bufs);
        bufs.out
    }

    /// Render into caller-provided buffers, reusing their allocations.
    ///
    /// Uses a Wadler-style pretty-printing strategy:
    /// - `Group` probes whether content fits in flat mode
    /// - if it does not fit, rendering switches to break mode
    /// - `Line`/`SoftLine` choose space vs newline based on that mode
    ///
    /// This avoids re-allocating the render stack, fits stack, and output
    /// string on every format call.
    pub(crate) fn render_into(&self, root: DocId, config: &FormatConfig, bufs: &mut RenderBuffers) {
        let RenderBuffers {
            out,
            stack,
            fits_stack,
            line_suffix_buf,
        } = bufs;
        if root == NIL_DOC {
            return;
        }

        let keyword_case = config.keyword_case();
        let indent_width = usize_to_i32(config.indent_width());

        out.reserve(self.docs.len() * 4);
        let mut pos: usize = 0;
        let line_width_i32 = usize_to_i32(config.line_width());
        stack.push((0, Mode::Break, root));

        while let Some((indent, mode, doc_id)) = stack.pop() {
            if doc_id == NIL_DOC {
                continue;
            }

            match self.get(doc_id) {
                Doc::Text(s) => {
                    out.push_str(s);
                    pos += s.len();
                }

                Doc::Keyword(s) => {
                    push_keyword(s, keyword_case, out);
                    pos += s.len();
                }

                Doc::Line => match mode {
                    Mode::Flat => {
                        out.push(' ');
                        pos += 1;
                    }
                    Mode::Break => {
                        flush_line_suffixes(self, line_suffix_buf, config, out, &mut pos);
                        emit_newline(indent, out, &mut pos);
                    }
                },

                Doc::SoftLine => match mode {
                    Mode::Flat => {}
                    Mode::Break => {
                        flush_line_suffixes(self, line_suffix_buf, config, out, &mut pos);
                        emit_newline(indent, out, &mut pos);
                    }
                },

                Doc::HardLine => {
                    flush_line_suffixes(self, line_suffix_buf, config, out, &mut pos);
                    emit_newline(indent, out, &mut pos);
                }

                Doc::Cat { left, right } => {
                    stack.push((indent, mode, *right));
                    stack.push((indent, mode, *left));
                }

                Doc::Nest { indent: di, child } => {
                    let spaces = i32::from(*di) * indent_width;
                    stack.push((indent + spaces, mode, *child));
                }

                Doc::Group { child } => {
                    if self.fits_with(
                        *child,
                        indent,
                        line_width_i32 - usize_to_i32(pos),
                        indent_width,
                        fits_stack,
                    ) {
                        stack.push((indent, Mode::Flat, *child));
                    } else {
                        stack.push((indent, Mode::Break, *child));
                    }
                }

                Doc::LineSuffix { child } => {
                    line_suffix_buf.push((indent, mode, *child));
                }

                Doc::BreakParent => {}
            }
        }

        flush_line_suffixes(self, line_suffix_buf, config, out, &mut pos);
    }

    /// Check whether a document fits within `remaining` columns when rendered flat.
    /// Reuses `scratch` to avoid per-call allocation.
    fn fits_with(
        &self,
        doc_id: DocId,
        indent: i32,
        remaining: i32,
        indent_width: i32,
        scratch: &mut Vec<(i32, DocId)>,
    ) -> bool {
        if remaining < 0 {
            return false;
        }

        let mut remaining = remaining;
        scratch.clear();
        scratch.push((indent, doc_id));

        while let Some((indent, doc_id)) = scratch.pop() {
            if remaining < 0 {
                return false;
            }
            if doc_id == NIL_DOC {
                continue;
            }

            match self.get(doc_id) {
                Doc::Text(s) | Doc::Keyword(s) => {
                    remaining -= usize_to_i32(s.len());
                }
                Doc::Line => {
                    remaining -= 1;
                }
                Doc::SoftLine | Doc::LineSuffix { .. } => {}
                Doc::HardLine | Doc::BreakParent => {
                    return false;
                }
                Doc::Cat { left, right } => {
                    scratch.push((indent, *right));
                    scratch.push((indent, *left));
                }
                Doc::Nest { indent: di, child } => {
                    let spaces = i32::from(*di) * indent_width;
                    scratch.push((indent + spaces, *child));
                }
                Doc::Group { child } => {
                    scratch.push((indent, *child));
                }
            }
        }

        remaining >= 0
    }

    /// Dump the document tree as an indented text representation (for debugging).
    pub(crate) fn dump(&self, root: DocId) -> String {
        use std::fmt::Write;
        let mut out = String::new();
        let mut stack: Vec<(DocId, usize)> = vec![(root, 0)];

        while let Some((id, depth)) = stack.pop() {
            if id == NIL_DOC {
                continue;
            }
            let indent_str: String = "  ".repeat(depth);
            match self.get(id) {
                Doc::Text(s) => {
                    let _ = writeln!(out, "{indent_str}Text({s:?})");
                }
                Doc::Keyword(s) => {
                    let _ = writeln!(out, "{indent_str}Keyword({s:?})");
                }
                Doc::Line => {
                    let _ = writeln!(out, "{indent_str}Line");
                }
                Doc::SoftLine => {
                    let _ = writeln!(out, "{indent_str}SoftLine");
                }
                Doc::HardLine => {
                    let _ = writeln!(out, "{indent_str}HardLine");
                }
                Doc::BreakParent => {
                    let _ = writeln!(out, "{indent_str}BreakParent");
                }
                Doc::LineSuffix { child } => {
                    let _ = writeln!(out, "{indent_str}LineSuffix");
                    stack.push((*child, depth + 1));
                }
                Doc::Cat { left, right } => {
                    // Push right first so left is processed first (stack LIFO).
                    stack.push((*right, depth));
                    stack.push((*left, depth));
                }
                Doc::Nest { indent, child } => {
                    let _ = writeln!(out, "{indent_str}Nest({indent})");
                    stack.push((*child, depth + 1));
                }
                Doc::Group { child } => {
                    let _ = writeln!(out, "{indent_str}Group");
                    stack.push((*child, depth + 1));
                }
            }
        }

        out
    }
}

// ── Private helpers ──────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Mode {
    Flat,
    Break,
}

/// Reusable buffers for `DocArena::render_into`. Keeping these across
/// format calls avoids re-allocating the render stack, fits stack, and
/// output string on every invocation.
pub(crate) struct RenderBuffers {
    pub out: String,
    pub stack: Vec<(i32, Mode, DocId)>,
    pub fits_stack: Vec<(i32, DocId)>,
    pub line_suffix_buf: Vec<(i32, Mode, DocId)>,
}

impl RenderBuffers {
    pub(crate) fn new() -> Self {
        Self {
            out: String::new(),
            stack: Vec::new(),
            fits_stack: Vec::new(),
            line_suffix_buf: Vec::new(),
        }
    }

    pub(crate) fn clear(&mut self) {
        self.out.clear();
        self.stack.clear();
        self.fits_stack.clear();
        self.line_suffix_buf.clear();
    }
}

impl Default for RenderBuffers {
    fn default() -> Self {
        Self::new()
    }
}

/// Static buffer of spaces for indent emission (avoids per-char push).
const SPACES: &str = "                                                                                                                                ";

fn emit_newline(indent: i32, out: &mut String, pos: &mut usize) {
    out.push('\n');
    let spaces = non_negative_i32_to_usize(indent.max(0));
    let mut remaining = spaces;
    while remaining > 0 {
        let chunk = remaining.min(SPACES.len());
        out.push_str(&SPACES[..chunk]);
        remaining -= chunk;
    }
    *pos = spaces;
}

/// Push a keyword string with the appropriate casing to the output.
#[inline]
fn push_keyword(s: &str, case: KeywordCase, out: &mut String) {
    match case {
        KeywordCase::Upper => {
            for &b in s.as_bytes() {
                out.push(b.to_ascii_uppercase() as char);
            }
        }
        KeywordCase::Lower => {
            for &b in s.as_bytes() {
                out.push(b.to_ascii_lowercase() as char);
            }
        }
    }
}

/// Render buffered line suffixes directly to output (before the next newline).
fn flush_line_suffixes(
    arena: &DocArena,
    buf: &mut Vec<(i32, Mode, DocId)>,
    config: &FormatConfig,
    out: &mut String,
    pos: &mut usize,
) {
    if buf.is_empty() {
        return;
    }
    for (indent, _mode, doc_id) in buf.drain(..) {
        render_inline(arena, doc_id, indent, config, out, pos);
    }
}

/// Render a doc tree directly into the output string (used for line suffixes).
fn render_inline(
    arena: &DocArena,
    doc_id: DocId,
    indent: i32,
    config: &FormatConfig,
    out: &mut String,
    pos: &mut usize,
) {
    let keyword_case = config.keyword_case();
    let indent_width = usize_to_i32(config.indent_width());
    if doc_id == NIL_DOC {
        return;
    }
    let mut stack: Vec<(i32, DocId)> = vec![(indent, doc_id)];
    while let Some((indent, doc_id)) = stack.pop() {
        if doc_id == NIL_DOC {
            continue;
        }
        match arena.get(doc_id) {
            Doc::Text(s) => {
                out.push_str(s);
                *pos += s.len();
            }
            Doc::Keyword(s) => {
                push_keyword(s, keyword_case, out);
                *pos += s.len();
            }
            Doc::Cat { left, right } => {
                stack.push((indent, *right));
                stack.push((indent, *left));
            }
            Doc::Nest { indent: di, child } => {
                let spaces = i32::from(*di) * indent_width;
                stack.push((indent + spaces, *child));
            }
            _ => {}
        }
    }
}

#[inline]
fn usize_to_doc_id(value: usize) -> DocId {
    u32::try_from(value).expect("DocArena index must fit in DocId")
}

#[inline]
fn usize_to_i32(value: usize) -> i32 {
    i32::try_from(value).expect("value must fit in i32")
}

#[inline]
fn non_negative_i32_to_usize(value: i32) -> usize {
    usize::try_from(value).expect("value must be non-negative")
}

#[cfg(test)]
mod tests {
    use super::*;

    fn cfg() -> FormatConfig {
        FormatConfig::default()
    }

    fn cfg_width(line_width: usize) -> FormatConfig {
        FormatConfig::default().with_line_width(line_width)
    }

    #[test]
    fn arena_alloc_and_get() {
        let mut arena = DocArena::new();
        let id = arena.text("hello");
        assert_eq!(id, 0);
        match arena.get(id) {
            Doc::Text(s) => assert_eq!(*s, "hello"),
            _ => panic!("expected Text"),
        }
    }

    #[test]
    fn nil_doc_identity_in_cat() {
        let mut arena = DocArena::new();
        let a = arena.text("a");
        assert_eq!(arena.cat(NIL_DOC, a), a);
        assert_eq!(arena.cat(a, NIL_DOC), a);
        assert_eq!(arena.cat(NIL_DOC, NIL_DOC), NIL_DOC);
    }

    #[test]
    fn nil_doc_identity_in_nest_and_group() {
        let mut arena = DocArena::new();
        assert_eq!(arena.nest(4, NIL_DOC), NIL_DOC);
        assert_eq!(arena.group(NIL_DOC), NIL_DOC);
        assert_eq!(arena.line_suffix(NIL_DOC), NIL_DOC);
    }

    #[test]
    fn cats_skips_nil() {
        let mut arena = DocArena::new();
        let a = arena.text("a");
        let b = arena.text("b");
        let result = arena.cats(&[NIL_DOC, a, NIL_DOC, b, NIL_DOC]);
        match arena.get(result) {
            Doc::Cat { left, right } => {
                assert_eq!(*left, a);
                assert_eq!(*right, b);
            }
            _ => panic!("expected Cat"),
        }
    }

    #[test]
    fn cats_single_element() {
        let mut arena = DocArena::new();
        let a = arena.text("a");
        assert_eq!(arena.cats(&[a]), a);
    }

    #[test]
    fn cats_empty() {
        let mut arena = DocArena::new();
        assert_eq!(arena.cats(&[]), NIL_DOC);
    }

    #[test]
    fn plain_text() {
        let mut arena = DocArena::new();
        let doc = arena.text("hello world");
        assert_eq!(arena.render(doc, &cfg()), "hello world");
    }

    #[test]
    fn cat_two_texts() {
        let mut arena = DocArena::new();
        let a = arena.text("hello");
        let b = arena.text(" world");
        let doc = arena.cat(a, b);
        assert_eq!(arena.render(doc, &cfg()), "hello world");
    }

    #[test]
    fn group_fits_flat() {
        let mut arena = DocArena::new();
        let a = arena.text("a");
        let sp = arena.line();
        let b = arena.text("b");
        let inner = arena.cats(&[a, sp, b]);
        let doc = arena.group(inner);
        assert_eq!(arena.render(doc, &cfg()), "a b");
    }

    #[test]
    fn group_breaks() {
        let mut arena = DocArena::new();
        let a = arena.text("aaaa");
        let sp = arena.line();
        let b = arena.text("bbbb");
        let inner = arena.cats(&[a, sp, b]);
        let doc = arena.group(inner);
        assert_eq!(arena.render(doc, &cfg_width(6)), "aaaa\nbbbb");
    }

    #[test]
    fn nest_indentation() {
        let mut arena = DocArena::new();
        let a = arena.text("a");
        let sp = arena.line();
        let b = arena.text("b");
        let inner = arena.cats(&[a, sp, b]);
        // nest(1, ..) = 1 indent level; default indent_width=2 → 2 spaces
        let nested = arena.nest(1, inner);
        let doc = arena.group(nested);
        assert_eq!(arena.render(doc, &cfg_width(3)), "a b");
        assert_eq!(arena.render(doc, &cfg_width(2)), "a\n  b");
    }

    #[test]
    fn nil_doc_renders_empty() {
        let arena = DocArena::new();
        assert_eq!(arena.render(NIL_DOC, &cfg()), "");
    }

    #[test]
    fn nest_respects_indent_width() {
        use crate::fmt::FormatConfig;

        let mut arena = DocArena::new();
        let a = arena.text("a");
        let sp = arena.line();
        let b = arena.text("b");
        let inner = arena.cats(&[a, sp, b]);
        // nest(1, ..) = 1 indent level
        let nested = arena.nest(1, inner);
        let doc = arena.group(nested);

        // With indent_width=4, one level should produce 4 spaces.
        let config = FormatConfig::default()
            .with_line_width(2)
            .with_indent_width(4);
        assert_eq!(arena.render(doc, &config), "a\n    b");

        // With indent_width=8, one level should produce 8 spaces.
        let config = FormatConfig::default()
            .with_line_width(2)
            .with_indent_width(8);
        assert_eq!(arena.render(doc, &config), "a\n        b");
    }
}
