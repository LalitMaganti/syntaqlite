// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Structured formatting of fallback macro-call arguments.
//!
//! In macro-fallback mode the outer parser keeps `foo!(a, b)` as a
//! single `TK_ID` whose AST text spans the whole call.  The default
//! formatter emits that source slice verbatim.  This module re-parses
//! each argument as its own `SELECT arg;` expression and returns a
//! pre-formatted `DocId` that the bytecode interpreter can splice in
//! place of the verbatim slice.
//!
//! Each call is processed in isolation: parse an arg, run the
//! interpreter against a throwaway arena that borrows the mini-parse,
//! copy-own the result into the caller's arena, then drop the
//! mini-parse.  No per-statement mini-parse state leaks out.

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{
    AnyNodeId, AnyParsedStatement, AnyParser, AnyTokenizer, FieldValue, MacroRewrite, ParseOutcome,
};
use syntaqlite_syntax::source::{StmtLen, StmtOffset, StmtRange, StmtText};

use super::comment::CommentEntry;
use super::doc::{DocArena, DocId};
use super::interpret::{FmtCtx, InterpretScratch, interpret_core};
use crate::dialect::AnyDialect;

/// Compute structured `DocId`s for each top-level fallback macro call
/// in `erased`.  The returned vector is index-aligned with the same
/// `parent().is_none() && is_fallback()` filter applied by
/// `Formatter::collect_side_channels`.
pub(super) fn compute_macro_docs<'a>(
    dialect: &AnyDialect,
    erased: &AnyParsedStatement<'a>,
    source: &'a StmtText,
    tokenizer: &AnyTokenizer,
    comments: &[CommentEntry],
    arena: &mut DocArena<'a>,
) -> Vec<Option<DocId>> {
    erased
        .macro_rewrites()
        .filter(|r| r.parent().is_none() && r.is_fallback())
        .map(|r| compute_one(dialect, &r, source, tokenizer, comments, arena))
        .collect()
}

fn compute_one<'a>(
    dialect: &AnyDialect,
    r: &MacroRewrite<'_>,
    source: &'a StmtText,
    tokenizer: &AnyTokenizer,
    comments: &[CommentEntry],
    arena: &mut DocArena<'a>,
) -> Option<DocId> {
    let call_off = StmtOffset::from_raw(r.call_offset().as_u32());
    let call_len = StmtLen::from(r.call_length());
    let call_end = call_off + call_len;

    // Defer to verbatim when the call has interior comments (the mini
    // parser has no comment context, so they'd silently disappear),
    // was hand-wrapped across multiple lines (authored layout wins),
    // or has no args (nothing to restructure).
    if comments
        .iter()
        .any(|c| c.offset >= call_off && c.offset < call_end)
    {
        return None;
    }
    let call_text = &source[StmtRange {
        start: call_off,
        end: call_end,
    }];
    if call_text.contains('\n') {
        return None;
    }

    let arg_texts: Vec<&str> = r.args().map(|a| a.text()).collect();
    if arg_texts.is_empty() {
        return None;
    }

    let mut arg_docs: Vec<DocId> = Vec::with_capacity(arg_texts.len());
    for arg_text in &arg_texts {
        arg_docs.push(format_arg(dialect, arg_text, tokenizer, arena)?);
    }

    // Assemble `name!(arg0, arg1, ...)` in the outer arena.  `name`
    // is borrowed from the parent buffer (stable across render), but
    // we own-copy it anyway to keep `macro_structured` independent of
    // the caller's buffer-lifetime story.
    let name_doc = arena.own_text(r.name());
    let open = arena.text("!(");
    let close = arena.text(")");
    let sep = arena.text(", ");
    let mut d = arena.cat(name_doc, open);
    for (i, ad) in arg_docs.iter().enumerate() {
        if i > 0 {
            d = arena.cat(d, sep);
        }
        d = arena.cat(d, *ad);
    }
    Some(arena.cat(d, close))
}

/// Byte offset of the first arg inside a synthetic `SELECT ` envelope.
const SELECT_PREFIX_LEN: u32 = 7;
const _: () = assert!(b"SELECT ".len() == SELECT_PREFIX_LEN as usize);

/// Parse `arg_text` as an expression via a synthetic `SELECT arg;`
/// mini-parse, format the expression subtree into a throwaway arena
/// borrowing the mini-parse, then copy-own the result into `arena`.
/// Returns `None` if the mini-parse fails or the expression subtree
/// cannot be located.
///
/// On return, the mini-parse and its scratch arena have been dropped;
/// the returned `DocId` is self-contained in `arena`.
fn format_arg(
    dialect: &AnyDialect,
    arg_text: &str,
    tokenizer: &AnyTokenizer,
    arena: &mut DocArena<'_>,
) -> Option<DocId> {
    let mini_source = format!("SELECT {arg_text};");
    let parser = AnyParser::with_config(
        (**dialect).clone(),
        &ParserConfig::default()
            .with_collect_tokens(true)
            .with_collect_node_extents(true)
            .with_macro_fallback(dialect.has_macro_style()),
    );
    let mut session = parser.parse(&mini_source);
    match session.next() {
        ParseOutcome::Ok(_) => {}
        _ => return None,
    }
    let stmt = session.arena_result();

    // Expression subtree has extent `(SELECT_PREFIX_LEN, arg_text.len())`
    // inside the synthetic source; the first AST node with that
    // extent is the outermost expression node we want to format.
    let target_off = StmtOffset::from_raw(SELECT_PREFIX_LEN);
    let target_len = StmtLen::from_raw(u32::try_from(arg_text.len()).ok()?);
    let expr_id = find_descendant_by_extent(&stmt, stmt.root_id(), target_off, target_len)?;

    let mut sub_arena: DocArena<'_> = DocArena::new();
    let sub_ctx = FmtCtx {
        dialect: dialect.clone(),
        reader: stmt,
        comment_ctx: None,
        macro_rewrites: Vec::new(),
        macro_docs: Vec::new(),
    };
    let mut scratch = InterpretScratch::new();
    let mut consumed: Vec<bool> = Vec::new();
    let sub_doc = interpret_core(
        &sub_ctx,
        expr_id,
        &mut sub_arena,
        &mut scratch,
        &mut consumed,
        tokenizer,
    );
    Some(arena.copy_owned_from(&sub_arena, sub_doc))
}

/// Depth-first search for the first descendant of `root` whose source
/// extent matches `(target_off, target_len)`.  First-match order
/// yields the outermost matching node, which is what we want for
/// formatting (an expression wrapper, not its inner leaf).
fn find_descendant_by_extent(
    reader: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    target_off: StmtOffset,
    target_len: StmtLen,
) -> Option<AnyNodeId> {
    if let Some((text, off)) = reader.node_text(root) {
        let len = StmtLen::from_raw(u32::try_from(text.len()).ok()?);
        if off == target_off && len == target_len {
            return Some(root);
        }
    }
    if let Some((_, fields)) = reader.extract_fields(root) {
        for i in 0..fields.len() {
            if let FieldValue::NodeId(child) = fields[i]
                && !child.is_null()
                && let Some(found) =
                    find_descendant_by_extent(reader, child, target_off, target_len)
            {
                return Some(found);
            }
        }
    }
    if let Some(children) = reader.list_children(root) {
        for &c in children {
            if let Some(found) = find_descendant_by_extent(reader, c, target_off, target_len) {
                return Some(found);
            }
        }
    }
    None
}
