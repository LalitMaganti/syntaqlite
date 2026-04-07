// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use syntaqlite_syntax::any::{
    AnyNodeId, AnyParsedStatement, AnyParser, AnyTokenizer, FieldValue, MacroRegion, ParseOutcome,
};
use syntaqlite_syntax::{CommentKind, ParserConfig};

use super::FormatConfig;
use super::FormatError;
use super::comment::{CommentCtx, CommentEntry, TokenEntry};
use super::doc::{DocArena, DocId, NIL_DOC, RenderBuffers};
use super::interpret::{FmtCtx, InterpretScratch};
use crate::dialect::AnyDialect;

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
    pub(super) macro_regions: Vec<MacroRegion>,
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
        // Use the grammar embedded in the dialect — do NOT hardcode the SQLite
        // grammar here, as this method is called with external dialects too.
        let grammar = (*dialect).clone();
        let has_macros = grammar.has_macro_style();
        let parser = AnyParser::with_config(
            grammar,
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(has_macros),
        );
        let macro_tokenizer = AnyTokenizer::new((*dialect).clone());
        Formatter {
            dialect,
            parser,
            config: format_config.clone(),
            arena: DocArena::with_capacity(256),
            interpret_scratch: InterpretScratch::new(),
            render_bufs: RenderBuffers::new(),
            macro_regions: Vec::with_capacity(32),
            comment_entries: Vec::with_capacity(64),
            token_entries: Vec::with_capacity(256),
            parts: Vec::with_capacity(64),
            consumed_regions: Vec::with_capacity(32),
            macro_tokenizer,
        }
    }

    /// Populate side-channel buffers (comments, tokens, macro regions) from an erased statement.
    fn collect_side_channels(&mut self, erased: &AnyParsedStatement<'_>) {
        self.macro_regions.clear();
        self.comment_entries.clear();
        self.comment_entries
            .extend(erased.comment_spans().map(|c| CommentEntry {
                offset: c.offset(),
                length: c.length(),
                kind: c.kind(),
            }));
        self.token_entries.clear();
        self.token_entries.extend(
            erased
                .token_spans()
                .map(|(offset, length)| TokenEntry { offset, length }),
        );
        self.macro_regions.extend(erased.macro_regions());
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
        let mut last_has_root = false;

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(stmt) => stmt,
                ParseOutcome::Err(e) => {
                    return Err(FormatError {
                        message: e.message().to_owned(),
                        offset: e.offset(),
                        length: e.length(),
                    });
                }
            };

            let erased = stmt.erase();
            self.collect_side_channels(&erased);
            let stmt_source = erased.source();

            let root_id = erased.root_id();
            let prev_has_root = last_has_root;
            last_has_root = !root_id.is_null();
            let semicolons = self.config.semicolons;
            let has_comments = !self.comment_entries.is_empty();
            let has_macros = !self.macro_regions.is_empty();
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
                    semicolons && prev_has_root,
                    stmt_source,
                    &mut arena,
                    &mut self.parts,
                );
            } else if let Some(cctx) = comment_ctx.as_ref()
                && let Some((next_offset, _)) = cctx.peek_next_token()
            {
                drain_gap_comments(cctx, next_offset, stmt_source, &mut arena, &mut self.parts);
            }

            // Stage 2: Interpret bytecode for this AST into Doc fragments.
            let ctx = FmtCtx {
                dialect: self.dialect.clone(),
                reader: erased,
                comment_ctx,
                macro_regions: std::mem::take(&mut self.macro_regions),
            };
            let interpreted = self.interpret_node(&ctx, root_id, &mut arena);
            self.parts.push(interpreted);

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
            self.macro_regions = ctx.macro_regions;

            // Recycle the arena, releasing all Doc borrows from this iteration.
            self.arena = DocArena::recycle(arena);

            stmt_num += 1;
        }

        if stmt_num == 0 {
            return Ok(String::new());
        }

        if self.config.semicolons && last_has_root {
            result.push(';');
        }
        result.push('\n');

        Ok(result)
    }

    /// Dump the raw interpreter bytecode for each statement.
    ///
    /// # Errors
    ///
    /// Returns `FormatError` if the source cannot be parsed.
    #[expect(clippy::too_many_lines)]
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
                    return Err(FormatError {
                        message: e.message().to_owned(),
                        offset: e.offset(),
                        length: e.length(),
                    });
                }
            };

            let erased = stmt.erase();
            let root_id = erased.root_id();
            let Some((tag, _fields)) = erased.extract_fields(root_id) else {
                continue;
            };

            let node_name = self.dialect.grammar().node_name(tag);
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
                    return Err(FormatError {
                        message: e.message().to_owned(),
                        offset: e.offset(),
                        length: e.length(),
                    });
                }
            };

            let erased = stmt.erase();
            self.collect_side_channels(&erased);
            let root_id = erased.root_id();

            if let Some((tag, _)) = erased.extract_fields(root_id) {
                let node_name = self.dialect.grammar().node_name(tag);
                let _ = writeln!(result, "=== {node_name} ===");
            }

            let has_comments = !self.comment_entries.is_empty();
            let has_macros = !self.macro_regions.is_empty();
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

            let stmt_source = erased.source();
            if stmt_num > 0 {
                emit_stmt_separator(
                    comment_ctx.as_ref(),
                    false, // no semicolons in debug output
                    stmt_source,
                    &mut arena,
                    &mut self.parts,
                );
            } else if let Some(cctx) = comment_ctx.as_ref()
                && let Some((next_offset, _)) = cctx.peek_next_token()
            {
                drain_gap_comments(cctx, next_offset, stmt_source, &mut arena, &mut self.parts);
            }

            let ctx = FmtCtx {
                dialect: self.dialect.clone(),
                reader: erased,
                comment_ctx,
                macro_regions: std::mem::take(&mut self.macro_regions),
            };
            let interpreted = self.interpret_node(&ctx, root_id, &mut arena);
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
            self.macro_regions = ctx.macro_regions;
            self.arena = DocArena::recycle(arena);

            stmt_num += 1;
        }

        Ok(result)
    }
}

// ── Multi-statement helpers ─────────────────────────────────────────────

fn emit_stmt_separator<'a>(
    comment_ctx: Option<&CommentCtx>,
    semicolons: bool,
    source: &'a str,
    arena: &mut DocArena<'a>,
    parts: &mut Vec<DocId>,
) {
    if let Some(cctx) = comment_ctx
        && let Some((next_offset, _)) = cctx.peek_next_token()
    {
        if semicolons {
            parts.push(arena.text(";"));
        }
        drain_trailing_gap(cctx, next_offset, source, arena, parts);
        parts.push(arena.hardline());
        parts.push(arena.hardline());
        drain_gap_comments(cctx, next_offset, source, arena, parts);
        return;
    }
    if semicolons {
        parts.push(arena.text(";"));
    }
    parts.push(arena.hardline());
    parts.push(arena.hardline());
}

fn drain_trailing_gap<'a>(
    ctx: &CommentCtx,
    before: u32,
    source: &'a str,
    arena: &mut DocArena<'a>,
    parts: &mut Vec<DocId>,
) {
    let mut last_end = ctx.prev_token_end();
    while let Some(c) = ctx.peek_comment() {
        if c.offset >= before {
            break;
        }
        let gap_start = (last_end as usize).min(source.len());
        let gap_end = (c.offset as usize).min(source.len());
        if gap_start < gap_end && source[gap_start..gap_end].contains('\n') {
            break;
        }
        let text = &source[c.offset as usize..(c.offset + c.length) as usize];
        match c.kind {
            CommentKind::Line => {
                let space = arena.text(" ");
                let comment = arena.text(text);
                let inner = arena.cat(space, comment);
                parts.push(arena.line_suffix(inner));
                parts.push(arena.break_parent());
            }
            CommentKind::Block => {
                parts.push(arena.text(" "));
                parts.push(arena.text(text));
            }
        }
        last_end = c.offset + c.length;
        ctx.advance_comment();
    }
}

fn drain_gap_comments<'a>(
    ctx: &CommentCtx,
    before: u32,
    source: &'a str,
    arena: &mut DocArena<'a>,
    parts: &mut Vec<DocId>,
) {
    let mut prev_was_comment = false;
    let mut last_end = ctx.prev_token_end();
    while let Some(c) = ctx.peek_comment() {
        if c.offset >= before {
            break;
        }
        // Preserve blank lines between separate comment blocks
        // (but not between code tokens and the first comment).
        if prev_was_comment {
            let gap_start = (last_end as usize).min(source.len());
            let gap_end = (c.offset as usize).min(source.len());
            if gap_start < gap_end && source[gap_start..gap_end].contains("\n\n") {
                parts.push(arena.hardline());
            }
        }
        let text = &source[c.offset as usize..(c.offset + c.length) as usize];
        parts.push(arena.text(text));
        parts.push(arena.hardline());
        last_end = c.offset + c.length;
        prev_was_comment = true;
        ctx.advance_comment();
    }
}

// ── Single-node formatting ──────────────────────────────────────────────

/// Check if the next token falls within a macro region.
///
/// Requires `comment_ctx` to be populated on `ctx`. `format_parsed` satisfies
/// this precondition by building a `CommentCtx` from the statement's collected
/// tokens (which requires `collect_tokens: true` at parse time).
///
/// Only matches at the *innermost* node that fully contains the macro.
/// If the child node has inline enum/flag fields with non-default values,
/// it likely carries additional keywords (e.g. `FOLLOWING` in a `FrameBound`)
/// and should format normally — the inner expression handler will emit
/// the verbatim text at the appropriate level.
pub(crate) fn try_macro_verbatim<'a>(
    ctx: &FmtCtx<'a>,
    regions: &[MacroRegion],
    arena: &mut DocArena<'a>,
    consumed: &mut [bool],
    tokenizer: &AnyTokenizer,
    child_id: AnyNodeId,
) -> Option<DocId> {
    let cctx = ctx.comment_ctx.as_ref()?;
    let (tok_offset, _) = cctx.peek_next_token()?;
    let source = ctx.source();

    for (i, r) in regions.iter().enumerate() {
        let r_start = r.call_offset();
        let r_end = r_start + r.call_length();

        if tok_offset >= r_start && tok_offset < r_end {
            // Check if this child node extends beyond the macro region
            // by examining its fields. If it has enum fields with non-zero
            // values or spans beyond the region, it's an intermediate node
            // (like FrameBound with EXPR_FOLLOWING) — skip and let the
            // inner expression handler emit verbatim at the right level.
            if let Some((_, child_fields)) = ctx.reader.extract_fields(child_id) {
                for i in 0..child_fields.len() {
                    match child_fields[i] {
                        // Non-zero enum → node has keyword variants
                        // (e.g. EXPR_FOLLOWING)
                        FieldValue::Enum(v) if v != 0 => return None,
                        // Non-zero flags → node has keyword modifiers
                        FieldValue::Flags(f) if f != 0 => return None,
                        _ => {}
                    }
                }
            }

            if consumed[i] {
                return Some(NIL_DOC);
            }
            consumed[i] = true;
            let macro_text = &source[r_start as usize..r_end as usize];
            return Some(reindent_macro(macro_text, tokenizer, arena));
        }
    }
    None
}

/// Raw LP/RP token type values from the `SQLite` tokenizer. These are stable
/// across all dialects built on the `SQLite` grammar.
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

    /// Verify that `Formatter` stores an `AnyParser` derived from the dialect,
    /// not a hardcoded `SQLite` `Parser`.
    ///
    /// This test FAILS TO COMPILE before the fix because `fmt.parser` is of
    /// type `syntaqlite_syntax::Parser` (SQLite-only), not `AnyParser`.
    /// After the fix, the field type changes to `AnyParser`.
    #[test]
    #[cfg(feature = "sqlite")]
    fn formatter_parser_is_any_grammar_based() {
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
    fn emit_stmt_separator_without_comments_with_semicolon() {
        let source = "SELECT 1";
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        emit_stmt_separator(None, true, source, &mut arena, &mut parts);
        assert_eq!(render_parts(&mut arena, &parts), ";\n\n");
    }

    #[test]
    fn emit_stmt_separator_without_comments_without_semicolon() {
        let source = "SELECT 1";
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        emit_stmt_separator(None, false, source, &mut arena, &mut parts);
        assert_eq!(render_parts(&mut arena, &parts), "\n\n");
    }

    #[test]
    fn emit_stmt_separator_emits_inline_block_comment_before_break() {
        let source = "/*x*/SELECT";
        let ctx = CommentCtx::new(
            vec![CommentEntry {
                offset: 0,
                length: 5,
                kind: CommentKind::Block,
            }],
            vec![TokenEntry {
                offset: 5,
                length: 6,
            }],
        );
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        emit_stmt_separator(Some(&ctx), true, source, &mut arena, &mut parts);
        assert_eq!(render_parts(&mut arena, &parts), "; /*x*/\n\n");
    }

    #[test]
    fn drain_gap_comments_writes_each_comment_on_own_line() {
        let source = "--a\n/*b*/SELECT";
        let ctx = CommentCtx::new(
            vec![
                CommentEntry {
                    offset: 0,
                    length: 3,
                    kind: CommentKind::Line,
                },
                CommentEntry {
                    offset: 4,
                    length: 5,
                    kind: CommentKind::Block,
                },
            ],
            vec![TokenEntry {
                offset: 9,
                length: 6,
            }],
        );
        let mut arena = DocArena::new();
        let mut parts = Vec::new();
        drain_gap_comments(&ctx, 9, source, &mut arena, &mut parts);
        assert_eq!(render_parts(&mut arena, &parts), "--a\n/*b*/\n");
    }
}
