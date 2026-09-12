// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{
    AnyNodeId, AnyParseError, AnyParsedStatement, AnyParser, AnyTokenizer, ParseOutcome,
};
use syntaqlite_syntax::source::{DocLen, DocRange, StmtLen, StmtOffset, StmtRange};

use super::FormatConfig;
use super::FormatError;
use super::comment::CommentEntry;
use super::doc::{DocArena, DocId, NIL_DOC, RenderBuffers};
use super::interpret::FmtCtx;
use crate::dialect::AnyDialect;

/// Convert a parse error (statement-relative offsets) to a
/// [`FormatError`] with a document-absolute range.
pub(super) fn parse_error_to_format_error(e: &AnyParseError<'_>) -> FormatError {
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
    /// Dedicated parser for structured macro-arg mini-parses.  Kept
    /// separate from `parser` because each parser instance holds a
    /// single `ParserInner`: while the outer render is walking a
    /// statement, `parser`'s inner is still owned by the outer
    /// session and cannot serve a second `parse()` call.
    pub(super) mini_parser: AnyParser,
    pub(super) config: FormatConfig,
    // Statement-scoped state cached on the formatter to avoid per-statement allocations.
    pub(super) layout: Option<super::token_layout::State<'static>>,
    pub(super) render_bufs: RenderBuffers,
    /// Byte ranges (offset, length) of macro calls in the source.  The
    /// formatter only needs positions to decide when to emit a call
    /// verbatim; full `MacroRewrite` records would tie this buffer to
    /// the statement lifetime and prevent reuse across statements.
    pub(super) macro_rewrites: Vec<(StmtOffset, StmtLen)>,
    pub(super) comment_entries: Vec<CommentEntry>,
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
            syntax.clone(),
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(has_macros)
                .with_collect_node_extents(has_macros),
        );
        // The mini-parser always needs node extents (used by
        // `find_descendant_by_extent` to locate each arg's expression
        // subtree) and macro fallback (so nested `foo!(...)` inside
        // an arg parses as a TK_ID rather than a syntax error).
        let mini_parser = AnyParser::with_config(
            syntax,
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_collect_node_extents(true)
                .with_macro_fallback(has_macros),
        );
        let macro_tokenizer = AnyTokenizer::new((*dialect).clone());
        Formatter {
            layout: Some(super::token_layout::State::new(AnyTokenizer::new(
                (*dialect).clone(),
            ))),
            dialect,
            parser,
            mini_parser,
            config: format_config.clone(),
            render_bufs: RenderBuffers::new(),
            macro_rewrites: Vec::with_capacity(32),
            comment_entries: Vec::with_capacity(64),
            macro_tokenizer,
        }
    }

    /// Populate side-channel buffers (comments, tokens, macro regions) from an erased statement.
    pub(super) fn collect_side_channels(&mut self, erased: &AnyParsedStatement<'_>) {
        self.macro_rewrites.clear();
        self.comment_entries.clear();
        self.comment_entries
            .extend(erased.comment_spans().map(|c| CommentEntry {
                offset: c.offset(),
                length: c.length(),
                kind: c.kind(),
                side: c.side(),
            }));
        // Only top-level fallback rewrites are meaningful here:
        // - `parent().is_none()` keeps offsets in the statement
        //   coordinate system the formatter compares against.
        // - `is_fallback()` keeps only calls kept verbatim as a
        //   `TK_ID` — those are the ones the formatter sees as a
        //   single token range and may restructure or emit verbatim.
        //   Expanded macros don't appear at the call site's byte
        //   range; their tokens come from the expansion buffer.
        self.macro_rewrites.extend(
            erased
                .macro_rewrites()
                .filter(|r| r.parent().is_none() && r.is_fallback())
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
        super::token_layout::format(self, source, false)
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

    /// Dump the document tree used by the production formatter.
    ///
    /// # Errors
    ///
    /// Returns `FormatError` if the source cannot be parsed.
    pub fn dump_doc_tree(&mut self, source: &str) -> Result<String, FormatError> {
        super::token_layout::format(self, source, true)
    }
}

// ── Macro-call emission ─────────────────────────────────────────────────

/// Emit a macro call at `child_id`, preferring pre-computed structured
/// formatting when available and falling back to verbatim reindent.
///
/// Requires `ctx.comment_ctx` populated (`format_parsed` satisfies
/// this).  Emits only at a node whose bytecode-emitted content is
/// exactly the macro call: any additional content the node would emit
/// (keywords, aliases, siblings) would be silently dropped by
/// `ReturnAction::Discard` in the caller.
///
/// The position check boils down to:
/// - `tok_offset == r_start`: the next unconsumed token *is* the
///   macro's first token.  Guards against *leading* content the node's
///   bytecode would emit (such a token would sit before `r_start`).
/// - `node_end == r_end`: the node's extent ends exactly where the
///   macro ends.  Guards against *trailing* content (e.g. a
///   `ResultColumn` alias).
/// - Node extent start is deliberately *not* checked: extents include
///   preceding keyword glue consumed by the parent (`FROM` before a
///   `TableRef`, `AS` before an alias `IdentName`).
///
/// Deliberately does NOT advance the cctx cursor — the child frame's
/// `Span` op advances it when the fallback `TK_ID` emits, so trailing
/// comments drain against the outer frame rather than the inner group.
pub(crate) fn try_macro<'a>(
    ctx: &FmtCtx<'a>,
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

    for (i, &(r_start, r_len)) in ctx.macro_rewrites.iter().enumerate() {
        let r_end = r_start + r_len;
        if tok_offset != r_start || node_end != r_end {
            continue;
        }
        if consumed[i] {
            return Some(NIL_DOC);
        }
        consumed[i] = true;
        if let Some(Some(doc)) = ctx.macro_docs.get(i) {
            return Some(*doc);
        }
        let macro_text = &source[StmtRange {
            start: r_start,
            end: r_end,
        }];
        return Some(reindent_macro(macro_text, tokenizer, arena));
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
pub(super) fn reindent_macro<'a>(
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
    fn formatter_parser_is_any_dialect_based() {
        use syntaqlite_syntax::any::AnyParser;
        let dialect = crate::sqlite::dialect::dialect();
        let fmt = Formatter::with_dialect_config(dialect, &FormatConfig::default());
        // Type assertion: fails to compile if fmt.parser is Parser, not AnyParser.
        let _: &AnyParser = &fmt.parser;
    }
}
