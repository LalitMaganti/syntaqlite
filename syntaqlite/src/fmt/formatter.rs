// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{
    AnyNodeId, AnyParseError, AnyParseSession, AnyParsedStatement, AnyParser, AnyTokenizer,
    FieldValue, ParseOutcome,
};
use syntaqlite_syntax::source::{DocLen, DocRange, StmtLen, StmtOffset, StmtRange, StmtText};

use super::FormatConfig;
use super::FormatError;
use super::comment::{CommentCtx, CommentEntry, TokenEntry};
use super::doc::{DocArena, DocId, NIL_DOC, RenderBuffers};
use super::interpret::{FmtCtx, InterpretScratch, interpret_core};
use crate::dialect::AnyDialect;

/// Convert a parse error (statement-relative offsets) to a
/// [`FormatError`] with a document-absolute range.
fn parse_error_to_format_error(e: &AnyParseError<'_>) -> FormatError {
    let base = e.statement_base();
    let range = match (e.offset(), e.length()) {
        (Some(off), Some(len)) => Some(DocRange::from_offset_len(off.to_doc(base), len.into())),
        (Some(off), None) => Some(DocRange::from_offset_len(
            off.to_doc(base),
            DocLen::default(),
        )),
        _ => None,
    };
    FormatError::new(e.message().to_owned(), range)
}

/// High-level SQL formatter that pretty-prints SQL source text.
///
/// Created from a [`Dialect`](crate::Dialect) and a [`FormatConfig`], the
/// formatter is designed to be **reused** across many inputs. Internal
/// buffers (parser, arena, scratch space) are recycled between calls to
/// [`format`](Self::format), avoiding per-call allocation overhead.
///
/// # Quick start
///
/// ```rust
/// # use syntaqlite::Formatter;
/// let mut fmt = Formatter::new();   // SQLite dialect, default config
/// let output = fmt.format("select 1+2").unwrap();
/// assert_eq!(output, "SELECT 1 + 2;\n");
/// ```
///
/// # Custom configuration
///
/// ```rust
/// # use syntaqlite::fmt::KeywordCase;
/// # use syntaqlite::{Formatter, FormatConfig};
/// let config = FormatConfig::default()
///     .with_keyword_case(KeywordCase::Lower)
///     .with_semicolons(false);
///
/// let mut fmt = Formatter::with_config(&config);
/// let output = fmt.format("SELECT 1").unwrap();
/// assert_eq!(output, "select 1\n");
/// ```
pub struct Formatter {
    pub(super) dialect: AnyDialect,
    pub(super) parser: AnyParser,
    pub(super) config: FormatConfig,
    // Statement-scoped state cached on the formatter to avoid per-statement allocations.
    pub(super) arena: DocArena<'static>,
    pub(super) interpret_scratch: InterpretScratch,
    pub(super) render_bufs: RenderBuffers,
    /// Byte ranges (offset, length) of macro calls in the source.  The
    /// formatter only needs positions to decide when to emit a call
    /// verbatim; full `MacroRewrite` records would tie this buffer to
    /// the statement lifetime and prevent reuse across statements.
    pub(super) macro_rewrites: Vec<(StmtOffset, StmtLen)>,
    /// Mini-parses backing the structured pre-compute, one entry per
    /// entry in `macro_rewrites` whose call was successfully split into
    /// parseable argument subtrees.  Kept alive through render so the
    /// outer arena's borrows into their parser buffers remain valid,
    /// then cleared before the next statement.
    pub(super) mini_parses: Vec<MiniParse>,
    pub(super) comment_entries: Vec<CommentEntry>,
    pub(super) token_entries: Vec<TokenEntry>,
    pub(super) parts: Vec<DocId>,
    pub(super) consumed_regions: Vec<bool>,
    /// Reusable tokenizer for macro body re-indentation.
    pub(super) macro_tokenizer: AnyTokenizer,
}

#[cfg(feature = "sqlite")]
impl Default for Formatter {
    fn default() -> Self {
        Self::new()
    }
}

impl Formatter {
    /// Create a formatter for the built-in `SQLite` dialect with default configuration.
    #[cfg(feature = "sqlite")]
    pub fn new() -> Formatter {
        Formatter::with_config(&FormatConfig::default())
    }

    /// Create a formatter for the built-in `SQLite` dialect with custom configuration.
    #[cfg(feature = "sqlite")]
    pub fn with_config(format_config: &FormatConfig) -> Formatter {
        Formatter::with_dialect_config(crate::sqlite::dialect::dialect(), format_config)
    }

    /// Create a formatter bound to the given dialect with custom configuration.
    ///
    /// # Panics
    /// Panics if `dialect` has no formatter bytecode (i.e. the `.synq` definitions
    /// do not include `fmt` blocks).
    pub fn with_dialect_config(
        dialect: impl Into<AnyDialect>,
        format_config: &FormatConfig,
    ) -> Self {
        let dialect = dialect.into();
        assert!(
            dialect.has_fmt_data(),
            "dialect has no formatter bytecode — ensure .synq definitions include fmt blocks",
        );
        // Use the dialect handle embedded in the dialect — do NOT hardcode the
        // SQLite dialect here, as this method is called with external dialects too.
        let syntax = (*dialect).clone();
        let has_macros = syntax.has_macro_style();
        let parser = AnyParser::with_config(
            syntax,
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(has_macros)
                .with_collect_node_extents(has_macros),
        );
        let macro_tokenizer = AnyTokenizer::new((*dialect).clone());
        Formatter {
            dialect,
            parser,
            config: format_config.clone(),
            arena: DocArena::with_capacity(256),
            interpret_scratch: InterpretScratch::new(),
            render_bufs: RenderBuffers::new(),
            macro_rewrites: Vec::with_capacity(32),
            mini_parses: Vec::new(),
            comment_entries: Vec::with_capacity(64),
            token_entries: Vec::with_capacity(256),
            parts: Vec::with_capacity(64),
            consumed_regions: Vec::with_capacity(32),
            macro_tokenizer,
        }
    }

    /// Populate side-channel buffers (comments, tokens, macro regions) from an erased statement.
    fn collect_side_channels(&mut self, erased: &AnyParsedStatement<'_>) {
        self.macro_rewrites.clear();
        self.comment_entries.clear();
        self.comment_entries
            .extend(erased.comment_spans().map(|c| CommentEntry {
                offset: c.offset(),
                length: c.length(),
                kind: c.kind(),
                side: c.side(),
            }));
        self.token_entries.clear();
        self.token_entries
            .extend(erased.token_spans().map(|range| TokenEntry {
                offset: range.start,
                length: range.len(),
            }));
        // Only top-level rewrites are meaningful here: their `LayerOffset`
        // coincides with the statement-relative offset the formatter
        // compares against.  Nested rewrites measure into their parent's
        // expansion and belong to a different coordinate system.
        self.macro_rewrites.extend(
            erased
                .macro_rewrites()
                .filter(|r| r.parent().is_none())
                .map(|r| {
                    (
                        StmtOffset::from_raw(r.call_offset().as_u32()),
                        StmtLen::from(r.call_length()),
                    )
                }),
        );
    }

    /// Format SQL source text. Handles multiple statements and preserves comments.
    ///
    /// Pipeline overview per statement:
    /// 1. Parse and collect token/comment/macro metadata.
    /// 2. Interpret formatter bytecode into Doc fragments.
    /// 3. Render Doc fragments with a Wadler-style pretty-printer (`DocArena`).
    /// 4. Recycle temporary buffers for the next statement.
    ///
    /// # Errors
    /// Returns [`FormatError`] when parsing fails for any statement in `source`.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use syntaqlite::Formatter;
    /// let mut fmt = Formatter::new();
    ///
    /// // Single statement
    /// let out = fmt.format("select 1").unwrap();
    /// assert_eq!(out, "SELECT 1;\n");
    ///
    /// // Multiple statements (reuses the same formatter)
    /// let out = fmt.format("select 1; select 2").unwrap();
    /// assert!(out.contains("SELECT 1"));
    /// assert!(out.contains("SELECT 2"));
    /// ```
    pub fn format(&mut self, source: &str) -> Result<String, FormatError> {
        let mut session = self.parser.parse(source);
        let mut result = String::with_capacity(source.len());
        let mut stmt_num: usize = 0;

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(stmt) => stmt,
                ParseOutcome::Err(e) => {
                    return Err(parse_error_to_format_error(&e));
                }
            };

            let erased = stmt.erase();
            self.collect_side_channels(&erased);
            let stmt_source = erased.text();

            let root_id = erased.root_id();
            let semicolons = self.config.semicolons;
            let has_comments = !self.comment_entries.is_empty();
            let has_macros = !self.macro_rewrites.is_empty();
            let needs_token_ctx = has_comments || has_macros;

            let comment_ctx = if needs_token_ctx {
                // Move buffers into CommentCtx for this statement, then reclaim them after render.
                Some(CommentCtx::new(
                    std::mem::take(&mut self.comment_entries),
                    std::mem::take(&mut self.token_entries),
                ))
            } else {
                None
            };

            // Fresh arena for this statement — drops borrows from the previous iteration.
            let prev_arena = std::mem::replace(&mut self.arena, DocArena::new());
            let mut arena = DocArena::recycle(prev_arena);
            self.parts.clear();

            if stmt_num > 0 {
                emit_stmt_separator(
                    comment_ctx.as_ref(),
                    stmt_source,
                    &mut arena,
                    &mut self.parts,
                );
            } else if let Some(cctx) = comment_ctx.as_ref()
                && let Some((next_offset, _)) = cctx.peek_next_token()
            {
                drain_gap_comments(cctx, next_offset, stmt_source, &mut arena, &mut self.parts);
            }

            // Stage 1.5: Pre-compute a structured `DocId` for each
            // top-level macro call whose arguments we can parse in
            // isolation; triggering is conservative (see
            // `precompute_macro_docs`).  Calls with comments, string
            // literals, internal newlines, or parse failures fall
            // through to verbatim emission via the existing path.  The
            // `mini_parses` Vec is taken off `self` so we can also
            // borrow `&mut arena` during the pass.
            let mut mini_parses = std::mem::take(&mut self.mini_parses);
            mini_parses.clear();
            let macro_docs = precompute_macro_docs(
                &self.dialect,
                &self.macro_tokenizer,
                stmt_source,
                &self.macro_rewrites,
                &self.comment_entries,
                &mut mini_parses,
                &mut arena,
            );

            // Stage 2: Interpret bytecode for this AST into Doc fragments.
            let ctx = FmtCtx {
                dialect: self.dialect.clone(),
                reader: erased,
                comment_ctx,
                macro_rewrites: std::mem::take(&mut self.macro_rewrites),
            };
            let interpreted = self.interpret_node(&ctx, &macro_docs, root_id, &mut arena);
            self.parts.push(interpreted);

            // Emit this statement's terminator.  Trailing comments now
            // live on the SEMI's token attachment, so they belong in the
            // *same* render cycle as the SEMI; the previous design that
            // added the SEMI in the next statement's emit_stmt_separator
            // would render the trailing line_suffix in the wrong cycle.
            if semicolons && !root_id.is_null() {
                let semi = arena.text(";");
                self.parts.push(semi);
            }

            if let Some(cctx) = ctx.comment_ctx.as_ref() {
                self.parts
                    .push(cctx.drain_remaining(stmt_source, &mut arena));
            }

            // Stage 3: Render Docs via the Wadler-style group/flat/break algorithm.
            // Rendering happens here while `erased`/`ctx` still borrow parser session data.
            let doc = arena.cats(&self.parts);
            let mut bufs = std::mem::take(&mut self.render_bufs);
            bufs.clear();
            arena.render_into(doc, &self.config, &mut bufs);
            result.push_str(&bufs.out);
            self.render_bufs = bufs;

            // Stage 4: Recover and recycle statement-scoped buffers.
            if let Some(cctx) = ctx.comment_ctx {
                let (comments, tokens) = cctx.into_parts();
                self.comment_entries = comments;
                self.token_entries = tokens;
            }
            self.macro_rewrites = ctx.macro_rewrites;

            // Recycle the arena, releasing all Doc borrows from this iteration.
            self.arena = DocArena::recycle(arena);
            // Drop the mini-parses now that their span borrows are gone.
            mini_parses.clear();
            self.mini_parses = mini_parses;

            stmt_num += 1;
        }

        if stmt_num == 0 {
            return Ok(String::new());
        }

        result.push('\n');

        Ok(result)
    }

    /// Dump the raw interpreter bytecode for each statement.
    ///
    /// # Errors
    ///
    /// Returns `FormatError` if the source cannot be parsed.
    pub fn dump_bytecode(&mut self, source: &str) -> Result<String, FormatError> {
        use std::fmt::Write;
        use syntaqlite_common::fmt::bytecode::opcodes;

        let mut session = self.parser.parse(source);
        let mut result = String::new();

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(stmt) => stmt,
                ParseOutcome::Err(e) => {
                    return Err(parse_error_to_format_error(&e));
                }
            };

            let erased = stmt.erase();
            let root_id = erased.root_id();
            let Some((tag, _fields)) = erased.extract_fields(root_id) else {
                continue;
            };

            let node_name = self.dialect.syntax_dialect().node_name(tag);
            let _ = writeln!(result, "=== {node_name} (tag={}) ===", u32::from(tag));

            let Some((ops_bytes, ops_len)) = self.dialect.fmt_dispatch(tag) else {
                result.push_str("  <no fmt bytecode>\n");
                continue;
            };

            let mut depth: usize = 0;
            for ip in 0..ops_len {
                let base = ip * 6;
                let opcode = ops_bytes[base];
                let a = ops_bytes[base + 1];
                let b = u16::from_le_bytes([ops_bytes[base + 2], ops_bytes[base + 3]]);
                let c = u16::from_le_bytes([ops_bytes[base + 4], ops_bytes[base + 5]]);

                // Dedent closers before printing.
                match opcode {
                    opcodes::END_IF
                    | opcodes::ELSE_OP
                    | opcodes::GROUP_END
                    | opcodes::NEST_END
                    | opcodes::FOR_EACH_END => {
                        depth = depth.saturating_sub(1);
                    }
                    _ => {}
                }

                let indent_str = "  ".repeat(depth);
                let desc = match opcode {
                    opcodes::KEYWORD => {
                        let s = self.dialect.fmt_string(b);
                        format!("Keyword \"{s}\"")
                    }
                    opcodes::SPAN => format!("Span(field={a})"),
                    opcodes::CHILD => format!("Child(field={a})"),
                    opcodes::LINE => "Line".to_string(),
                    opcodes::SOFTLINE => "SoftLine".to_string(),
                    opcodes::HARDLINE => "HardLine".to_string(),
                    opcodes::GROUP_START => "Group {".to_string(),
                    opcodes::GROUP_END
                    | opcodes::NEST_END
                    | opcodes::END_IF
                    | opcodes::FOR_EACH_END => "}".to_string(),
                    opcodes::NEST_START => "Nest {".to_string(),
                    opcodes::IF_SET => format!("IfSet(field={a}) {{"),
                    opcodes::ELSE_OP => "} Else {".to_string(),
                    opcodes::FOR_EACH_START => format!("ForEach(field={a}) {{"),
                    opcodes::CHILD_ITEM => "ChildItem".to_string(),
                    opcodes::FOR_EACH_SEP => {
                        let s = self.dialect.fmt_string(b);
                        format!("Sep \"{s}\"")
                    }
                    opcodes::IF_BOOL => format!("IfBool(field={a}) {{"),
                    opcodes::IF_FLAG => format!("IfFlag(field={a}, mask={b:#x}) {{"),
                    opcodes::IF_ENUM => format!("IfEnum(field={a}, val={b}) {{"),
                    opcodes::IF_SPAN => format!("IfSpan(field={a}) {{"),
                    opcodes::ENUM_DISPLAY => format!("EnumDisplay(field={a}, base={b})"),
                    opcodes::FOR_EACH_SELF_START => "ForEachSelf {".to_string(),
                    opcodes::CHILD_PREC => format!("ChildPrec(field={a}, table={b}, packed={c})"),
                    opcodes::CHILD_PAREN_LIST => format!("ChildParenList(field={a})"),
                    opcodes::CHILD_PREC_FIXED => {
                        format!("ChildPrecFixed(field={a}, packed={b}, is_right={c})")
                    }
                    _ => format!("Unknown(opcode={opcode}, a={a}, b={b}, c={c})"),
                };

                let _ = writeln!(result, "  {ip:3}: {indent_str}{desc}");

                // Indent openers after printing.
                match opcode {
                    opcodes::IF_SET
                    | opcodes::IF_BOOL
                    | opcodes::IF_FLAG
                    | opcodes::IF_ENUM
                    | opcodes::IF_SPAN
                    | opcodes::ELSE_OP
                    | opcodes::GROUP_START
                    | opcodes::NEST_START
                    | opcodes::FOR_EACH_START
                    | opcodes::FOR_EACH_SELF_START => {
                        depth += 1;
                    }
                    _ => {}
                }
            }
        }

        Ok(result)
    }

    /// Dump the Wadler-Lindig document tree after bytecode interpretation.
    ///
    /// # Errors
    ///
    /// Returns `FormatError` if the source cannot be parsed.
    pub fn dump_doc_tree(&mut self, source: &str) -> Result<String, FormatError> {
        use std::fmt::Write;
        let mut session = self.parser.parse(source);
        let mut result = String::new();
        let mut stmt_num = 0usize;

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(stmt) => stmt,
                ParseOutcome::Err(e) => {
                    return Err(parse_error_to_format_error(&e));
                }
            };

            let erased = stmt.erase();
            self.collect_side_channels(&erased);
            let root_id = erased.root_id();

            if let Some((tag, _)) = erased.extract_fields(root_id) {
                let node_name = self.dialect.syntax_dialect().node_name(tag);
                let _ = writeln!(result, "=== {node_name} ===");
            }

            let has_comments = !self.comment_entries.is_empty();
            let has_macros = !self.macro_rewrites.is_empty();
            let needs_token_ctx = has_comments || has_macros;

            let comment_ctx = if needs_token_ctx {
                Some(CommentCtx::new(
                    std::mem::take(&mut self.comment_entries),
                    std::mem::take(&mut self.token_entries),
                ))
            } else {
                None
            };

            let prev_arena = std::mem::replace(&mut self.arena, DocArena::new());
            let mut arena = DocArena::recycle(prev_arena);
            self.parts.clear();

            let stmt_source = erased.text();
            if stmt_num > 0 {
                emit_stmt_separator(
                    comment_ctx.as_ref(),
                    stmt_source,
                    &mut arena,
                    &mut self.parts,
                );
            } else if let Some(cctx) = comment_ctx.as_ref()
                && let Some((next_offset, _)) = cctx.peek_next_token()
            {
                drain_gap_comments(cctx, next_offset, stmt_source, &mut arena, &mut self.parts);
            }

            let mut mini_parses = std::mem::take(&mut self.mini_parses);
            mini_parses.clear();
            let macro_docs = precompute_macro_docs(
                &self.dialect,
                &self.macro_tokenizer,
                stmt_source,
                &self.macro_rewrites,
                &self.comment_entries,
                &mut mini_parses,
                &mut arena,
            );

            let ctx = FmtCtx {
                dialect: self.dialect.clone(),
                reader: erased,
                comment_ctx,
                macro_rewrites: std::mem::take(&mut self.macro_rewrites),
            };
            let interpreted = self.interpret_node(&ctx, &macro_docs, root_id, &mut arena);
            self.parts.push(interpreted);

            if let Some(cctx) = ctx.comment_ctx.as_ref() {
                self.parts
                    .push(cctx.drain_remaining(stmt_source, &mut arena));
            }

            let doc = arena.cats(&self.parts);
            result.push_str(&arena.dump(doc));
            result.push('\n');

            // Recycle buffers.
            if let Some(cctx) = ctx.comment_ctx {
                let (comments, tokens) = cctx.into_parts();
                self.comment_entries = comments;
                self.token_entries = tokens;
            }
            self.macro_rewrites = ctx.macro_rewrites;
            self.arena = DocArena::recycle(arena);
            mini_parses.clear();
            self.mini_parses = mini_parses;

            stmt_num += 1;
        }

        Ok(result)
    }
}

// ── Multi-statement helpers ─────────────────────────────────────────────

fn emit_stmt_separator<'a>(
    comment_ctx: Option<&CommentCtx>,
    source: &'a StmtText,
    arena: &mut DocArena<'a>,
    parts: &mut Vec<DocId>,
) {
    parts.push(arena.hardline());
    parts.push(arena.hardline());
    if let Some(cctx) = comment_ctx
        && let Some((next_offset, _)) = cctx.peek_next_token()
    {
        drain_gap_comments(cctx, next_offset, source, arena, parts);
    }
}

fn drain_gap_comments<'a>(
    ctx: &CommentCtx,
    before: StmtOffset,
    source: &'a StmtText,
    arena: &mut DocArena<'a>,
    parts: &mut Vec<DocId>,
) {
    let mut prev_was_comment = false;
    let mut last_end = ctx.prev_token_end();
    let source_end = StmtOffset::default() + source.byte_len();
    while let Some(c) = ctx.peek_comment() {
        if c.offset >= before {
            break;
        }
        // Preserve blank lines between separate comment blocks
        // (but not between code tokens and the first comment).
        if prev_was_comment {
            let gap = StmtRange {
                start: last_end.min(source_end),
                end: c.offset.min(source_end),
            };
            if !gap.is_empty() && source[gap].contains("\n\n") {
                parts.push(arena.hardline());
            }
        }
        let text = &source[StmtRange::from_offset_len(c.offset, c.length)];
        parts.push(arena.text(text));
        parts.push(arena.hardline());
        last_end = c.offset + c.length;
        prev_was_comment = true;
        ctx.advance_comment();
    }
}

// ── Single-node formatting ──────────────────────────────────────────────

// ── Structured macro-argument formatting ────────────────────────────────
//
// When the outer parser runs in macro-fallback mode, a call like
// `foo!(a, b)` is opaque: it parses as a single `TK_ID` whose AST text
// spans the entire call.  The formatter falls back to emitting that
// source slice verbatim (see `try_macro_verbatim`).
//
// To reformat arguments, we run an independent "mini" parser on
// `SELECT a, b` — a synthetic expression list built from the call's
// comma-separated arg texts.  Each argument becomes an `Expr` subtree
// that the normal bytecode interpreter can format into the outer arena.
// We assemble `name + "!(" + arg_doc_0 + ", " + ... + ")"` as a
// `DocId` tree and stash it in `macro_docs[i]`, indexed by the same
// order as `macro_rewrites`.  `try_macro_structured` looks it up during
// the outer walk.
//
// Triggering is conservative on purpose.  We skip structured
// formatting when the call contains:
//
// - A comment (the mini-parser runs without a comment context, so any
//   comment inside the call text would be silently dropped).
// - A string literal (the envelope splitter doesn't respect quotes).
// - An internal newline (users who hand-wrapped the call want that
//   layout preserved; verbatim does the right thing).
// - Any parse or lookup failure.
//
// Each of these falls through to the existing verbatim path, so the
// feature is purely additive at this stage.

/// Mini-parser holder backing the structured argument path.  Owns an
/// `AnyParseSession` (which itself owns the copied source buffer and
/// parser state), so `&self` borrows can safely hand out
/// `AnyParsedStatement<'_>` views that outlive any individual
/// formatting call.
pub(super) struct MiniParse {
    session: AnyParseSession,
}

impl MiniParse {
    /// Parse `SELECT arg0, arg1, ...` as an expression list.  Returns
    /// `None` if the synthetic SELECT fails to parse (which happens,
    /// for instance, when an arg isn't expression-shaped).
    fn parse_args(dialect: &AnyDialect, args: &[&str]) -> Option<Self> {
        let mut source = String::from("SELECT ");
        for (i, a) in args.iter().enumerate() {
            if i > 0 {
                source.push_str(", ");
            }
            source.push_str(a);
        }
        let parser = AnyParser::with_config(
            (**dialect).clone(),
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_collect_node_extents(true)
                .with_macro_fallback(dialect.has_macro_style()),
        );
        let mut session = parser.parse(&source);
        match session.next() {
            ParseOutcome::Ok(_stmt) => Some(Self { session }),
            _ => None,
        }
    }

    fn statement(&self) -> AnyParsedStatement<'_> {
        self.session.arena_result()
    }
}

/// Byte offset at which the first argument sits inside a synthetic
/// `SELECT ` envelope.
const SELECT_PREFIX_LEN: u32 = 7;

/// Pre-compute a structured `DocId` for each top-level macro call in
/// `regions`, aligned by index.  Returns `None` at a given index when
/// the trigger guards or a parse fail out.  Newly created mini-parses
/// are appended to `mini_parses`, which the caller must keep alive
/// until the outer arena is rendered.
pub(super) fn precompute_macro_docs<'a>(
    dialect: &AnyDialect,
    tokenizer: &AnyTokenizer,
    source: &'a StmtText,
    regions: &[(StmtOffset, StmtLen)],
    comments: &[CommentEntry],
    mini_parses: &'a mut Vec<MiniParse>,
    arena: &mut DocArena<'a>,
) -> Vec<Option<DocId>> {
    let mut plans: Vec<Option<CallPlan>> = Vec::with_capacity(regions.len());
    for &(r_start, r_len) in regions {
        plans.push(plan_structured_call(
            dialect,
            tokenizer,
            source,
            r_start,
            r_len,
            comments,
            mini_parses,
        ));
    }

    let mut results: Vec<Option<DocId>> = Vec::with_capacity(regions.len());
    for plan in plans {
        let Some(plan) = plan else {
            results.push(None);
            continue;
        };
        let mini = &mini_parses[plan.mini_idx];
        let reader = mini.statement();
        let root = reader.root_id();
        if root.is_null() {
            results.push(None);
            continue;
        }
        let mut arg_docs: Vec<DocId> = Vec::with_capacity(plan.arg_ranges.len());
        let mut failed = false;
        for &(sub_off, sub_len) in &plan.arg_ranges {
            let Some(sub) = find_descendant_by_extent(&reader, root, sub_off, sub_len) else {
                failed = true;
                break;
            };
            let sub_ctx = FmtCtx {
                dialect: dialect.clone(),
                reader: reader.clone(),
                comment_ctx: None,
                macro_rewrites: Vec::new(),
            };
            let mut sub_scratch = InterpretScratch::new();
            let mut sub_consumed: Vec<bool> = Vec::new();
            let empty_macro_docs: Vec<Option<DocId>> = Vec::new();
            let doc = interpret_core(
                &sub_ctx,
                &empty_macro_docs,
                sub,
                arena,
                &mut sub_scratch,
                &mut sub_consumed,
                tokenizer,
            );
            arg_docs.push(doc);
        }
        if failed {
            results.push(None);
            continue;
        }
        let name_doc = arena.text(plan.name);
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
        d = arena.cat(d, close);
        results.push(Some(d));
    }
    results
}

/// Per-call metadata produced by the planning phase and consumed by
/// the DocId-assembly phase.  Splitting the two keeps the arena borrow
/// disjoint from the `&mut mini_parses` push borrow.
struct CallPlan<'src> {
    /// Macro name as a slice of the outer source.  No allocation; no
    /// leak.
    name: &'src str,
    /// `(offset, length)` of each argument's expected node inside the
    /// mini-parse's synthetic `SELECT a0, a1, ...` source.
    arg_ranges: Vec<(StmtOffset, StmtLen)>,
    /// Index into `mini_parses` where this call's parse lives.
    mini_idx: usize,
}

fn plan_structured_call<'src>(
    dialect: &AnyDialect,
    tokenizer: &AnyTokenizer,
    source: &'src StmtText,
    r_start: StmtOffset,
    r_len: StmtLen,
    comments: &[CommentEntry],
    mini_parses: &mut Vec<MiniParse>,
) -> Option<CallPlan<'src>> {
    let call_text = &source[StmtRange {
        start: r_start,
        end: r_start + r_len,
    }];
    // Trigger guards.  See the module-level comment block for why each
    // of these defers to verbatim.
    if call_contains_comment(comments, r_start, r_len) {
        return None;
    }
    let Envelope { name, inner } = split_envelope(tokenizer, call_text)?;
    // Bail on any literal newline in the call text: the user wrote a
    // hand-wrapped call and we preserve their layout by deferring to
    // verbatim.  A string literal that genuinely contains `\n` is
    // rare enough to treat the same way.
    if call_text.contains('\n') {
        return None;
    }
    let arg_texts = split_args(tokenizer, inner)?;
    let mini = MiniParse::parse_args(dialect, &arg_texts)?;
    let mini_idx = mini_parses.len();
    mini_parses.push(mini);
    // Compute each arg's `(offset, length)` inside the mini-parser's
    // synthetic `SELECT a0, a1, ...` source.
    let mut arg_ranges: Vec<(StmtOffset, StmtLen)> = Vec::with_capacity(arg_texts.len());
    let mut cursor: u32 = SELECT_PREFIX_LEN;
    for (i, a) in arg_texts.iter().enumerate() {
        if i > 0 {
            cursor += 2; // ", "
        }
        let len = u32::try_from(a.len()).ok()?;
        arg_ranges.push((StmtOffset::from_raw(cursor), StmtLen::from_raw(len)));
        cursor += len;
    }
    Some(CallPlan {
        name,
        arg_ranges,
        mini_idx,
    })
}

fn call_contains_comment(comments: &[CommentEntry], r_start: StmtOffset, r_len: StmtLen) -> bool {
    let r_end = r_start + r_len;
    comments
        .iter()
        .any(|c| c.offset >= r_start && c.offset < r_end)
}

/// `TK_*` token type ids used by the argument splitter.  These values
/// are stable across every dialect built on the `SQLite` token table;
/// the macro call syntax itself only needs the `!`, `(`, `)` and `,`
/// tokens.
const TK_LP_ID: u32 = 113;
const TK_RP_ID: u32 = 115;
const TK_COMMA_ID: u32 = 118;
const TK_SPACE_ID: u32 = 183;
const TK_BANG_ID: u32 = 188;

struct Envelope<'src> {
    name: &'src str,
    inner: &'src str,
}

/// Decompose `call_text` (`name!(…)`) using the dialect tokenizer.
/// Tokens are walked by text-length so we never have to search for
/// punctuation in the raw bytes.
fn split_envelope<'src>(tokenizer: &AnyTokenizer, call_text: &'src str) -> Option<Envelope<'src>> {
    // Expected token sequence: ID(name), `!`, `(`, ... args ..., `)`.
    // Anything else bails out.
    let mut iter = tokens_with_spans(tokenizer, call_text);
    let (name_tok, _) = iter.next()?;
    if !name_tok
        .text()
        .chars()
        .all(|c| c.is_alphanumeric() || c == '_')
        || name_tok.text().is_empty()
    {
        return None;
    }
    let (bang_tok, _) = iter.next()?;
    if u32::from(bang_tok.token_type()) != TK_BANG_ID || bang_tok.text() != "!" {
        return None;
    }
    let (lp_tok, lp_start) = iter.next()?;
    if u32::from(lp_tok.token_type()) != TK_LP_ID {
        return None;
    }
    let inner_start = lp_start + lp_tok.text().len();
    // Consume tokens until we find the RP that closes the envelope
    // at depth 0, recording its start offset so `inner` excludes it.
    let mut depth: i32 = 1;
    let mut inner_end: Option<usize> = None;
    for (tok, start) in iter {
        let tt = u32::from(tok.token_type());
        if tt == TK_LP_ID {
            depth += 1;
        } else if tt == TK_RP_ID {
            depth -= 1;
            if depth == 0 {
                inner_end = Some(start);
                break;
            }
        }
    }
    let inner_end = inner_end?;
    Some(Envelope {
        name: name_tok.text(),
        inner: &call_text[inner_start..inner_end],
    })
}

/// Iterator over non-whitespace tokens, paired with the start byte
/// offset of each token inside `text`.
fn tokens_with_spans<'a>(
    tokenizer: &AnyTokenizer,
    text: &'a str,
) -> impl Iterator<Item = (syntaqlite_syntax::any::AnyToken<'a>, usize)> + use<'a> {
    let base = text.as_ptr() as usize;
    tokenizer
        .tokenize(text)
        .filter(|t| u32::from(t.token_type()) != TK_SPACE_ID)
        .map(move |t| {
            let start = t.text().as_ptr() as usize - base;
            (t, start)
        })
}

/// Split `inner` on top-level commas using the dialect tokenizer.
/// Commas inside parens or string literals do not split.  Returns
/// `None` on unbalanced parens; an empty `inner` yields an empty
/// `Vec`, matching the `name!()` shape.
fn split_args<'src>(tokenizer: &AnyTokenizer, inner: &'src str) -> Option<Vec<&'src str>> {
    if inner.trim().is_empty() {
        return Some(Vec::new());
    }
    let mut args: Vec<&'src str> = Vec::new();
    let mut depth: i32 = 0;
    let mut seg_start: usize = 0;
    let base = inner.as_ptr() as usize;
    for tok in tokenizer.tokenize(inner) {
        let tt = u32::from(tok.token_type());
        let tok_start = tok.text().as_ptr() as usize - base;
        match tt {
            TK_LP_ID => depth += 1,
            TK_RP_ID => {
                depth -= 1;
                if depth < 0 {
                    return None;
                }
            }
            TK_COMMA_ID if depth == 0 => {
                args.push(inner[seg_start..tok_start].trim());
                seg_start = tok_start + tok.text().len();
            }
            _ => {}
        }
    }
    if depth != 0 {
        return None;
    }
    args.push(inner[seg_start..].trim());
    Some(args)
}

/// Depth-first search for the first descendant of `root` whose source
/// extent equals `(target_off, target_len)`.
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

/// Try structured emission via pre-computed `macro_docs` first; if no
/// region matches or that slot is `None`, fall through to verbatim.
pub(crate) fn try_macro_call<'a>(
    ctx: &FmtCtx<'a>,
    regions: &[(StmtOffset, StmtLen)],
    macro_docs: &[Option<DocId>],
    arena: &mut DocArena<'a>,
    consumed: &mut [bool],
    tokenizer: &AnyTokenizer,
    child_id: AnyNodeId,
) -> Option<DocId> {
    if let Some(doc) = try_macro_structured(ctx, regions, macro_docs, consumed, child_id) {
        return Some(doc);
    }
    try_macro_verbatim(ctx, regions, arena, consumed, tokenizer, child_id)
}

/// Look up a pre-computed structured `DocId` for a macro call
/// positioned at `child_id`.  The alignment rule is the same as
/// `try_macro_verbatim`'s: next unconsumed token starts at the
/// region's `r_start` and the node's extent ends at `r_end`.
/// Deliberately does NOT advance the cctx cursor — the child frame's
/// `Span` op advances it when the fallback `TK_ID` emits, matching
/// verbatim's timing so trailing-comment drains land in the outer
/// frame rather than the inner group.
fn try_macro_structured(
    ctx: &FmtCtx<'_>,
    regions: &[(StmtOffset, StmtLen)],
    macro_docs: &[Option<DocId>],
    consumed: &mut [bool],
    child_id: AnyNodeId,
) -> Option<DocId> {
    let cctx = ctx.comment_ctx.as_ref()?;
    let (tok_offset, _) = cctx.peek_next_token()?;
    let (node_text, node_off) = ctx.reader.node_text(child_id)?;
    let node_len = StmtLen::from_raw(u32::try_from(node_text.len()).ok()?);
    let node_end = node_off + node_len;

    for (i, &(r_start, r_len)) in regions.iter().enumerate() {
        if tok_offset == r_start && node_end == r_start + r_len {
            if consumed[i] {
                return Some(NIL_DOC);
            }
            let doc = (*macro_docs.get(i)?)?;
            consumed[i] = true;
            return Some(doc);
        }
    }
    None
}

/// Check if the next token falls within a macro region.
///
/// Requires `comment_ctx` to be populated on `ctx`. `format_parsed` satisfies
/// this precondition by building a `CommentCtx` from the statement's collected
/// tokens (which requires `collect_tokens: true` at parse time).
///
/// Emits verbatim only at a node whose bytecode-emitted content is exactly
/// the macro: any additional content the node would emit (keywords,
/// aliases, siblings) would be silently dropped by `ReturnAction::Discard`
/// in the caller.
///
/// The decision boils down to three position checks:
/// - `tok_offset == r_start`: the next unconsumed token *is* the macro's
///   first token.  Guards against *leading* content the node's bytecode
///   would emit — such a token would sit before `r_start`.
/// - `node_end == r_end`: the node's extent ends exactly where the macro
///   ends.  Guards against *trailing* content (e.g. a `ResultColumn`
///   alias).
/// - Node extent start is intentionally *not* checked: extents include
///   preceding keyword glue already consumed by the parent (e.g. `FROM`
///   before a `TableRef`, `AS` before an alias `IdentName`).
pub(crate) fn try_macro_verbatim<'a>(
    ctx: &FmtCtx<'a>,
    regions: &[(StmtOffset, StmtLen)],
    arena: &mut DocArena<'a>,
    consumed: &mut [bool],
    tokenizer: &AnyTokenizer,
    child_id: AnyNodeId,
) -> Option<DocId> {
    let cctx = ctx.comment_ctx.as_ref()?;
    let (tok_offset, _) = cctx.peek_next_token()?;
    let source = ctx.text();

    let (node_text, node_off) = ctx.reader.node_text(child_id)?;
    let node_len = StmtLen::from_raw(
        u32::try_from(node_text.len())
            .expect("node text length fits in u32; source buffer is addressed via u32 offsets"),
    );
    let node_end = node_off + node_len;

    for (i, &(r_start, r_len)) in regions.iter().enumerate() {
        let r_end = r_start + r_len;

        if tok_offset == r_start && node_end == r_end {
            if consumed[i] {
                return Some(NIL_DOC);
            }
            consumed[i] = true;
            let macro_text = &source[StmtRange {
                start: r_start,
                end: r_end,
            }];
            return Some(reindent_macro(macro_text, tokenizer, arena));
        }
    }
    None
}

/// Raw LP/RP token type values from the `SQLite` tokenizer. These are stable
/// across all dialects built on the `SQLite` dialect.
const TK_LP: u32 = 113;
const TK_RP: u32 = 115;

/// Re-indent a multiline macro call using tokenizer-based paren-depth tracking.
///
/// Single-line macros (e.g. `foo!(1 + 2)`) are returned verbatim.
/// Multiline macros get each line trimmed and re-indented based on
/// parenthesis nesting depth, using `hardline` + `nest()` so that
/// the base indentation adapts to the surrounding formatter context.
///
/// Paren depth is computed by tokenizing the macro body with the dialect's
/// tokenizer, so parentheses inside strings, comments, and quoted identifiers
/// are correctly ignored.
fn reindent_macro<'a>(
    macro_text: &'a str,
    tokenizer: &AnyTokenizer,
    arena: &mut DocArena<'a>,
) -> DocId {
    // Find "!(" to split name from body.
    let Some(bang_pos) = macro_text.find("!(") else {
        return arena.text(macro_text);
    };

    let prefix = &macro_text[..bang_pos + 2]; // "name!("
    let inner = &macro_text[bang_pos + 2..]; // everything after "!("

    // Single-line: return verbatim.
    if !inner.contains('\n') {
        return arena.text(macro_text);
    }

    // Step 1: Tokenize the inner body to compute paren depth at each newline.
    // depth_at_newline[i] = depth after processing all tokens up to and
    // including the (i+1)-th newline. We start at depth 1 because we're
    // inside the `!(` paren.
    let mut depth: i32 = 1;
    let mut depth_at_newline: Vec<i32> = Vec::new();

    for tok in tokenizer.tokenize(inner) {
        let tt: u32 = tok.token_type().into();
        let tok_text = tok.text();

        // LP/RP update depth. Tokens like strings and comments never produce
        // LP/RP, so parens inside them are automatically ignored.
        if tt == TK_LP {
            depth += 1;
        } else if tt == TK_RP {
            depth -= 1;
        }

        // Record depth at each newline boundary. Newlines appear in Space
        // tokens (and occasionally block-comment tokens).
        for _ in tok_text.bytes().filter(|&b| b == b'\n') {
            depth_at_newline.push(depth);
        }
    }
    // tokenizer cursor is dropped here

    // Step 2: Build doc from lines using pre-computed depths.
    let mut result = arena.text(prefix);
    let mut first = true;

    for (i, line) in inner.split('\n').enumerate() {
        let trimmed = line.trim();
        if trimmed.is_empty() {
            if first {
                first = false;
            }
            continue;
        }

        // Depth at the start of this line (before any tokens on this line).
        // Line 0 starts at depth 1 (inside `!(`).
        // Subsequent lines start at the depth recorded at the preceding newline.
        let line_depth = if i == 0 {
            1
        } else {
            depth_at_newline.get(i - 1).copied().unwrap_or(0)
        };

        // Leading `)` chars reduce indent for this line. Safe to count raw
        // characters here: a `)` at position 0 of trimmed text is always an
        // actual RP token (strings start with `'`, comments with `--`/`/*`).
        let leading_close =
            i32::try_from(trimmed.bytes().take_while(|&b| b == b')').count()).unwrap_or(i32::MAX);
        let indent = i16::try_from((line_depth - leading_close).max(0)).unwrap_or(i16::MAX);

        if first {
            // Content on same line as "!(" — keep inline.
            first = false;
            let txt = arena.text(trimmed);
            result = arena.cat(result, txt);
        } else {
            // Emit hardline + indent via nest wrappers.
            let hl = arena.hardline();
            let txt = arena.text(trimmed);
            let line_doc = arena.cat(hl, txt);
            let indented = if indent > 0 {
                arena.nest(indent, line_doc)
            } else {
                line_doc
            };
            result = arena.cat(result, indented);
        }
    }

    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use syntaqlite_syntax::{CommentKind, CommentSide};

    /// Verify that `Formatter` stores an `AnyParser` derived from the dialect,
    /// not a hardcoded `SQLite` `Parser`.
    ///
    /// This test FAILS TO COMPILE before the fix because `fmt.parser` is of
    /// type `syntaqlite_syntax::Parser` (SQLite-only), not `AnyParser`.
    /// After the fix, the field type changes to `AnyParser`.
    #[test]
    #[cfg(feature = "sqlite")]
    fn formatter_parser_is_any_dialect_based() {
        use syntaqlite_syntax::any::AnyParser;
        let dialect = crate::sqlite::dialect::dialect();
        let fmt = Formatter::with_dialect_config(dialect, &FormatConfig::default());
        // Type assertion: fails to compile if fmt.parser is Parser, not AnyParser.
        let _: &AnyParser = &fmt.parser;
    }

    fn render_parts(arena: &mut DocArena<'_>, parts: &[DocId]) -> String {
        let root = arena.cats(parts);
        arena.render(root, &FormatConfig::default())
    }

    #[test]
    fn emit_stmt_separator_without_comments_emits_blank_line() {
        let source = StmtText::new("SELECT 1");
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        emit_stmt_separator(None, source, &mut arena, &mut parts);
        assert_eq!(render_parts(&mut arena, &parts), "\n\n");
    }

    #[test]
    fn emit_stmt_separator_drains_leading_block_comment_after_break() {
        // emit_stmt_separator now only handles the inter-statement break
        // and drains LEADING comments of the next statement.  The
        // statement terminator (`;`) and any TRAILING comments on it are
        // emitted by per-statement processing in `format`, not here.
        let source = StmtText::new("/*x*/SELECT");
        let ctx = CommentCtx::new(
            vec![CommentEntry {
                offset: StmtOffset::from_raw(0),
                length: StmtLen::from_raw(5),
                kind: CommentKind::Block,
                side: CommentSide::Leading,
            }],
            vec![TokenEntry {
                offset: StmtOffset::from_raw(5),
                length: StmtLen::from_raw(6),
            }],
        );
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        emit_stmt_separator(Some(&ctx), source, &mut arena, &mut parts);
        assert_eq!(render_parts(&mut arena, &parts), "\n\n/*x*/\n");
    }

    #[test]
    fn drain_gap_comments_writes_each_comment_on_own_line() {
        let source = StmtText::new("--a\n/*b*/SELECT");
        let ctx = CommentCtx::new(
            vec![
                CommentEntry {
                    offset: StmtOffset::from_raw(0),
                    length: StmtLen::from_raw(3),
                    kind: CommentKind::Line,
                    side: CommentSide::Leading,
                },
                CommentEntry {
                    offset: StmtOffset::from_raw(4),
                    length: StmtLen::from_raw(5),
                    kind: CommentKind::Block,
                    side: CommentSide::Leading,
                },
            ],
            vec![TokenEntry {
                offset: StmtOffset::from_raw(9),
                length: StmtLen::from_raw(6),
            }],
        );
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        drain_gap_comments(
            &ctx,
            StmtOffset::from_raw(9),
            source,
            &mut arena,
            &mut parts,
        );
        assert_eq!(render_parts(&mut arena, &parts), "--a\n/*b*/\n");
    }
}
