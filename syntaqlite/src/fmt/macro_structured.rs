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

use syntaqlite_syntax::any::{
    AnyNodeId, AnyParsedStatement, AnyParser, AnyTokenizer, FieldValue, MacroRewrite, ParseOutcome,
};
use syntaqlite_syntax::source::{StmtLen, StmtOffset};

use super::comment::CommentEntry;
use super::doc::{DocArena, DocId, NIL_DOC};
use super::interpret::{FmtCtx, InterpretScratch, interpret_core};
use crate::dialect::AnyDialect;

/// Compute structured `DocId`s for each top-level fallback macro call
/// in `erased`.  The returned vector is index-aligned with the same
/// `parent().is_none() && is_fallback()` filter applied by
/// `Formatter::collect_side_channels`.
///
/// `mini_parser` is the caller's cached parser used for per-arg
/// `SELECT arg;` mini-parses.  It must be distinct from the parser
/// driving the outer render (each parser instance holds a single
/// `ParserInner`, and the outer session still owns it at this point).
pub(super) fn compute_macro_docs<'a>(
    mini_parser: &AnyParser,
    dialect: &AnyDialect,
    erased: &AnyParsedStatement<'a>,
    tokenizer: &AnyTokenizer,
    comments: &[CommentEntry],
    arena: &mut DocArena<'a>,
) -> Vec<Option<DocId>> {
    erased
        .macro_rewrites()
        .filter(|r| r.parent().is_none() && r.is_fallback())
        .map(|r| compute_one(mini_parser, dialect, &r, tokenizer, comments, arena))
        .collect()
}

fn compute_one(
    mini_parser: &AnyParser,
    dialect: &AnyDialect,
    r: &MacroRewrite<'_>,
    tokenizer: &AnyTokenizer,
    comments: &[CommentEntry],
    arena: &mut DocArena<'_>,
) -> Option<DocId> {
    let call_off = StmtOffset::from_raw(r.call_offset().as_u32());
    let call_end = call_off + StmtLen::from(r.call_length());

    // Defer to verbatim when the call has interior comments — the mini
    // parser has no comment context, so comments inside an arg would
    // silently disappear. The parser records comments encountered while
    // scanning a fallback macro call into the same statement comment
    // list (see `synq_parser_scan_macro_args` in parser_macros.c), so
    // this single check covers both comments in the outer gaps and
    // comments nested inside the macro's body.
    if comments
        .iter()
        .any(|c| c.offset >= call_off && c.offset < call_end)
    {
        return None;
    }

    let arg_texts: Vec<&str> = r.args().map(|a| a.text()).collect();
    if arg_texts.is_empty() {
        return None;
    }

    let mut arg_docs: Vec<DocId> = Vec::with_capacity(arg_texts.len());
    for arg_text in &arg_texts {
        arg_docs.push(format_arg(
            mini_parser,
            dialect,
            arg_text,
            tokenizer,
            arena,
        )?);
    }

    // Assemble `name!(arg0, arg1, ...)` in the outer arena.  `name`
    // is borrowed from the parent buffer (stable across render), but
    // we own-copy it anyway to keep `macro_structured` independent of
    // the caller's buffer-lifetime story.
    //
    // The arg run is wrapped in `nest(1)` so that a break inside an
    // arg (e.g. a binary expression that won't fit flat) indents by
    // one level relative to the `!(` opener. This matches the
    // `reindent_macro` verbatim path — which sees depth-1 content
    // after the opening `!(` and emits it at `nest(1)` — so a
    // formatter-broken macro call round-trips idempotently whether
    // the next pass runs the structured or verbatim path.
    let name_doc = arena.own_text(r.name());
    let open = arena.text("!(");
    let close = arena.text(")");
    let sep = arena.text(", ");
    let mut body = NIL_DOC;
    for (i, ad) in arg_docs.iter().enumerate() {
        if i > 0 {
            body = arena.cat(body, sep);
        }
        body = arena.cat(body, *ad);
    }
    let nested = arena.nest(1, body);
    let prefix = arena.cat(name_doc, open);
    let with_body = arena.cat(prefix, nested);
    Some(arena.cat(with_body, close))
}

/// Parse `arg_text` as an expression via a synthetic `SELECT arg;`
/// mini-parse, format the expression subtree into a throwaway arena
/// borrowing the mini-parse, then copy-own the result into `arena`.
/// Returns `None` if the mini-parse fails or the expression subtree
/// cannot be located.
///
/// On return, the mini-parse's session has been dropped (returning
/// its `ParserInner` to `mini_parser`), so the next call can reuse
/// the same parser without re-allocating its C-side state.
fn format_arg(
    mini_parser: &AnyParser,
    dialect: &AnyDialect,
    arg_text: &str,
    tokenizer: &AnyTokenizer,
    arena: &mut DocArena<'_>,
) -> Option<DocId> {
    let mini_source = format!("SELECT {arg_text};");
    let mut session = mini_parser.parse(&mini_source);
    match session.next() {
        ParseOutcome::Ok(_) => {}
        _ => return None,
    }
    let stmt = session.arena_result();
    let expr_id = select_wrapper_arg_node(&stmt)?;

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

/// Navigate a `SELECT <arg>;` mini-parse to the `ResultColumn`
/// wrapping `<arg>`.  We format at the `ResultColumn` level (not
/// its `expr` child) so that row-value args like `(a, b)` get the
/// paren-wrap emitted by `child_paren_list(expr)` in the
/// `ResultColumn` bytecode.
///
/// Relies on the grammar shape defined in
/// `syntaqlite-buildtools/parser-nodes/select.synq`: `SelectStmt`'s
/// first `NodeId` field is `columns: ResultColumnList`, and the list
/// has exactly one child since we emit exactly one expression inside
/// the wrapper.
fn select_wrapper_arg_node(reader: &AnyParsedStatement<'_>) -> Option<AnyNodeId> {
    let columns_list = first_node_field(reader, reader.root_id())?;
    reader.list_children(columns_list)?.first().copied()
}

/// Return the first non-null `NodeId` field of `node`.  Used to walk
/// the fixed-shape SELECT wrapper without depending on specific field
/// indices.
fn first_node_field(reader: &AnyParsedStatement<'_>, node: AnyNodeId) -> Option<AnyNodeId> {
    let (_, fields) = reader.extract_fields(node)?;
    for i in 0..fields.len() {
        if let FieldValue::NodeId(id) = fields[i]
            && !id.is_null()
        {
            return Some(id);
        }
    }
    None
}
