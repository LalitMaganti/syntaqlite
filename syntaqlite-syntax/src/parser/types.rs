// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use crate::source::{
    ColumnNumber, LayerLen, LayerOffset, LayerText, LineNumber, RewriteIdx, StmtLen, StmtOffset,
    StmtRange, TokenIdx,
};

use crate::dialect::TypedDialect;

/// Tri-state parse result for statement-oriented parser APIs.
///
/// Mirrors C parser return codes:
/// - [`ParseOutcome::Done`]  -> `SYNTAQLITE_PARSE_DONE`
/// - [`ParseOutcome::Ok`]    -> `SYNTAQLITE_PARSE_OK`
/// - [`ParseOutcome::Err`]   -> `SYNTAQLITE_PARSE_ERROR`
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ParseOutcome<T, E> {
    /// No more statements/results are available.
    Done,
    /// A statement parsed successfully.
    Ok(T),
    /// A statement parsed with an error.
    Err(E),
}

impl<T, E> ParseOutcome<T, E> {
    /// Convert into `Result<Option<T>, E>` for `?`-friendly control flow.
    ///
    /// # Errors
    ///
    /// Returns `Err(e)` when the outcome is [`ParseOutcome::Err`].
    pub fn transpose(self) -> Result<Option<T>, E> {
        match self {
            ParseOutcome::Done => Ok(None),
            ParseOutcome::Ok(v) => Ok(Some(v)),
            ParseOutcome::Err(e) => Err(e),
        }
    }

    /// Map the `Ok(T)` payload.
    pub fn map<U>(self, f: impl FnOnce(T) -> U) -> ParseOutcome<U, E> {
        match self {
            ParseOutcome::Done => ParseOutcome::Done,
            ParseOutcome::Ok(v) => ParseOutcome::Ok(f(v)),
            ParseOutcome::Err(e) => ParseOutcome::Err(e),
        }
    }

    /// Map the `Err(E)` payload.
    pub fn map_err<F>(self, f: impl FnOnce(E) -> F) -> ParseOutcome<T, F> {
        match self {
            ParseOutcome::Done => ParseOutcome::Done,
            ParseOutcome::Ok(v) => ParseOutcome::Ok(v),
            ParseOutcome::Err(e) => ParseOutcome::Err(f(e)),
        }
    }
}

/// SQL comment style.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CommentKind {
    /// A line comment starting with `--`.
    Line,
    /// A block comment delimited by `/* ... */`.
    Block,
}

/// Which token a comment is attached to, and on which side.
///
/// Set at parse time so consumers can ask "what comments belong to
/// token N?" without walking the source.  See
/// [`super::TypedParsedStatement::leading_comments`] /
/// [`super::TypedParsedStatement::trailing_comments`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CommentSide {
    /// The comment appears on its own line (or before any token of the
    /// statement) and immediately precedes the owning token.
    Leading,
    /// The comment appears on the same source line as the owning token,
    /// after it.
    Trailing,
}

/// Comment captured from source during parsing.
///
/// Returned by [`super::TypedParsedStatement::comments`]. Requires
/// `collect_tokens: true` in [`super::ParserConfig`].
#[derive(Debug, Clone, Copy)]
pub struct Comment<'a> {
    text: &'a str,
    kind: CommentKind,
    offset: StmtOffset,
    length: StmtLen,
    token_idx: TokenIdx,
    side: CommentSide,
}

impl<'a> Comment<'a> {
    pub(super) fn new(
        text: &'a str,
        kind: CommentKind,
        offset: StmtOffset,
        length: StmtLen,
        token_idx: TokenIdx,
        side: CommentSide,
    ) -> Self {
        Comment {
            text,
            kind,
            offset,
            length,
            token_idx,
            side,
        }
    }

    /// The full comment text, including delimiters.
    pub fn text(&self) -> &'a str {
        self.text
    }

    /// Whether this is a line (`--`) or block (`/* */`) comment.
    pub fn kind(&self) -> CommentKind {
        self.kind
    }

    /// Statement-relative byte offset of the comment start.
    pub fn offset(&self) -> StmtOffset {
        self.offset
    }

    /// Byte length of the comment text.
    pub fn length(&self) -> StmtLen {
        self.length
    }

    /// Index of the owning token in the statement's token stream.
    /// May equal the token count when the comment trails the last token
    /// of the statement and no following statement exists.
    pub fn token_idx(&self) -> TokenIdx {
        self.token_idx
    }

    /// Whether this comment leads or trails its owning token.
    pub fn side(&self) -> CommentSide {
        self.side
    }
}

/// Lightweight comment descriptor without a source text borrow.
///
/// Returned by [`super::AnyParsedStatement::comment_spans`].
/// Use this when you only need position and kind, not the text.
#[derive(Debug, Clone, Copy)]
pub struct CommentSpan {
    offset: StmtOffset,
    length: StmtLen,
    kind: CommentKind,
    token_idx: TokenIdx,
    side: CommentSide,
}

impl CommentSpan {
    /// Statement-relative byte offset of the comment start.
    pub fn offset(&self) -> StmtOffset {
        self.offset
    }

    /// Byte length of the comment text.
    pub fn length(&self) -> StmtLen {
        self.length
    }

    /// Whether this is a line (`--`) or block (`/* */`) comment.
    pub fn kind(&self) -> CommentKind {
        self.kind
    }

    /// Index of the owning token in the statement's token stream.
    /// See [`Comment::token_idx`] for attachment semantics.
    pub fn token_idx(&self) -> TokenIdx {
        self.token_idx
    }

    /// Whether this comment leads or trails its owning token.
    pub fn side(&self) -> CommentSide {
        self.side
    }

    pub(super) fn new(
        offset: StmtOffset,
        length: StmtLen,
        kind: CommentKind,
        token_idx: TokenIdx,
        side: CommentSide,
    ) -> Self {
        CommentSpan {
            offset,
            length,
            kind,
            token_idx,
            side,
        }
    }
}

pub use crate::dialect::ParserTokenFlags;

/// Token captured from a parsed statement, typed by dialect `G`.
///
/// Returned by [`super::TypedParsedStatement::tokens`]. Requires
/// `collect_tokens: true` in [`super::ParserConfig`].
///
/// A token records both its *layer-local* position (where the parser
/// actually consumed it — in the authored source for layer 0, or in a
/// macro expansion buffer for layer > 0) and its *statement-relative*
/// range (the authored-source range the user would point at when
/// asking "where is this in my SQL?"):
///
/// - [`text`](Self::text) returns the exact byte slice for this token,
///   from the authored source for layer-0 tokens or from the
///   expansion-layer buffer for layer-N tokens.
/// - [`layer_id`](Self::layer_id), [`offset`](Self::offset), and
///   [`length`](Self::length) describe the token's position inside its
///   owning layer — in [`LayerOffset`] / [`LayerLen`] coordinates.
/// - [`stmt_range`](Self::stmt_range) collapses layer-N tokens up to
///   their authored call-site range (the same drill-up rule used by
///   the span-text APIs), so every token has a meaningful statement
///   range regardless of layer.
#[derive(Debug, Clone, Copy)]
pub struct TypedParserToken<'a, G: TypedDialect> {
    text: &'a str,
    token_type: G::Token,
    flags: ParserTokenFlags,
    offset: LayerOffset,
    length: LayerLen,
    layer_id: u8,
    stmt_range: StmtRange,
}

impl<'a, G: TypedDialect> TypedParserToken<'a, G> {
    pub(super) fn new(
        text: &'a str,
        token_type: G::Token,
        flags: ParserTokenFlags,
        offset: LayerOffset,
        length: LayerLen,
        layer_id: u8,
        stmt_range: StmtRange,
    ) -> Self {
        TypedParserToken {
            text,
            token_type,
            flags,
            offset,
            length,
            layer_id,
            stmt_range,
        }
    }

    /// The byte text of this token — a slice of the authored source
    /// (layer 0) or of the owning expansion layer's buffer (layer > 0).
    pub fn text(&self) -> &'a str {
        self.text
    }

    /// Dialect-typed token variant.
    pub fn token_type(&self) -> G::Token {
        self.token_type
    }

    /// Semantic usage flags inferred by the parser.
    pub fn flags(&self) -> ParserTokenFlags {
        self.flags
    }

    /// Byte offset of the token within its owning layer's buffer.
    ///
    /// For layer-0 tokens this equals the statement-relative offset.
    /// For layer-N tokens it is a position in the expansion layer's
    /// buffer; use [`stmt_range`](Self::stmt_range) for authored-source
    /// coordinates.
    pub fn offset(&self) -> LayerOffset {
        self.offset
    }

    /// Byte length of the token text in its owning layer.
    pub fn length(&self) -> LayerLen {
        self.length
    }

    /// The layer the token lives in.  `0` = authored source;
    /// `>0` = an expansion layer.  Most consumers do not need this;
    /// prefer [`stmt_range`](Self::stmt_range) for authored-source
    /// positions.
    pub fn layer_id(&self) -> u8 {
        self.layer_id
    }

    /// Authored-source byte range for this token.
    ///
    /// For layer-0 tokens this is the token's own source position.
    /// For tokens produced by macro expansion, this collapses up the
    /// expansion chain to the enclosing macro call site (or, for
    /// tokens substituted from a `$param`, to the caller's authored
    /// argument text).  This mirrors the drill-up rule used by
    /// `AnyParsedStatement::span_text`, so every token reports the
    /// statement range the user would see in their source — multiple
    /// expansion tokens from the same call may share the call site's
    /// range.
    pub fn stmt_range(&self) -> StmtRange {
        self.stmt_range
    }
}

/// Parser-token alias for dialect-independent pipelines.
pub type AnyParserToken<'a> = TypedParserToken<'a, crate::dialect::AnyDialect>;

/// A macro rewrite recorded during parsing.
///
/// Carries enough information to reconstruct a source-to-expanded rewrite
/// tree (e.g. to drive Perfetto's `SqlSource::Rewriter` or an equivalent).
///
/// Entries are reported in insertion order: outer macros appear before the
/// nested macros they contain, and macros at the same nesting level appear
/// in source order.  Nesting is expressed via [`parent`](Self::parent): a
/// rewrite with `parent() == None` replaces a range in the authored
/// source; a rewrite with `parent() == Some(i)` replaces a range in the
/// `i`-th entry's [`expansion`](Self::expansion) buffer.
///
/// [`expansion`](Self::expansion) and [`name`](Self::name) borrow from
/// parser-owned memory — they are valid for the lifetime of the
/// originating parsed statement.
///
/// Returned by [`super::AnyParsedStatement::macro_rewrites`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MacroRewrite<'a> {
    pub(crate) parent: Option<RewriteIdx>,
    pub(crate) rewrite_idx: RewriteIdx,
    pub(crate) call_offset: LayerOffset,
    pub(crate) call_length: LayerLen,
    pub(crate) expansion: &'a LayerText,
    pub(crate) name: &'a str,
    pub(crate) def_line: LineNumber,
    pub(crate) def_col: ColumnNumber,
    pub(crate) body_call_offset: LayerOffset,
    pub(crate) body_call_length: LayerLen,
    /// Buffer that [`call_offset`](Self::call_offset) and every
    /// [`MacroCallArg::offset`] indexes into — statement source for
    /// top-level rewrites, parent rewrite's expansion otherwise.
    pub(crate) parent_buffer: &'a LayerText,
    pub(crate) is_fallback: bool,
    pub(crate) parser: std::ptr::NonNull<crate::parser::ffi::CParser>,
    pub(crate) _lifetime: std::marker::PhantomData<&'a ()>,
}

/// Sentinel value for [`MacroRewrite::body_call_offset`] /
/// [`MacroRewrite::body_call_length`] meaning "this call was tokenized
/// from a `$param` substitution; descend through the matching arg
/// segment instead of indexing into the parent's body."
pub const MACRO_BODY_CALL_ARG_INTERNAL: LayerOffset = LayerOffset::from_raw(u32::MAX);

impl<'a> MacroRewrite<'a> {
    /// Index of the parent rewrite, or `None` if this rewrite applies
    /// directly to the authored source.
    pub fn parent(&self) -> Option<RewriteIdx> {
        self.parent
    }
    /// Byte offset of the macro call in the parent's text.
    pub fn call_offset(&self) -> LayerOffset {
        self.call_offset
    }
    /// Byte length of the entire macro call in the parent's text.
    pub fn call_length(&self) -> LayerLen {
        self.call_length
    }
    /// The replacement text for the macro call.  Nested macro calls that
    /// appear in this buffer are reported as separate [`MacroRewrite`]
    /// entries whose [`parent`](Self::parent) refers back to this one.
    ///
    /// Returned as a [`LayerText`], slicable by [`LayerRange`] for
    /// type-safe slicing.  Offsets from nested rewrites ([`call_offset`],
    /// [`call_length`]) are measured into this buffer.
    ///
    /// [`LayerRange`]: crate::source::LayerRange
    /// [`call_offset`]: Self::call_offset
    /// [`call_length`]: Self::call_length
    pub fn expansion(&self) -> &'a LayerText {
        self.expansion
    }
    /// The macro name as it appears at the call site.
    pub fn name(&self) -> &'a str {
        self.name
    }
    /// 1-based line of the macro definition (0 if unknown).
    pub fn def_line(&self) -> LineNumber {
        self.def_line
    }
    /// 1-based column of the macro definition (0 if unknown).
    pub fn def_col(&self) -> ColumnNumber {
        self.def_col
    }
    /// Byte offset of this call in the parent's *authored* body, computed
    /// by inverting the length shifts the parent's `$param` substitutions
    /// introduced.  For top-level rewrites the parent is the authored
    /// source, so this equals [`call_offset`](Self::call_offset).
    ///
    /// Returns [`MACRO_BODY_CALL_ARG_INTERNAL`] when the call was
    /// tokenized from a `$param` substitution — consumers should descend
    /// through the matching arg segment instead of rewriting in the body.
    /// [`body_call_length`](Self::body_call_length) mirrors the sentinel.
    pub fn body_call_offset(&self) -> LayerOffset {
        self.body_call_offset
    }
    /// Length counterpart of [`body_call_offset`](Self::body_call_offset).
    pub fn body_call_length(&self) -> LayerLen {
        self.body_call_length
    }
    /// 1 if this rewrite is a fallback call — an unregistered
    /// `name!(args)` the parser kept verbatim as a `TK_ID` — and 0
    /// if it was expanded by a registered macro.  Distinct from
    /// "`expansion()` is empty", since a registered macro *could*
    /// theoretically expand to the empty string.
    pub fn is_fallback(&self) -> bool {
        self.is_fallback
    }

    /// The buffer that [`call_offset`](Self::call_offset) and every
    /// argument offset from [`args`](Self::args) indexes into.  For
    /// top-level rewrites this is the current statement's source
    /// slice; for nested rewrites it is the parent rewrite's
    /// [`expansion`](Self::expansion) buffer.  Returned so consumers
    /// can slice arbitrary sub-ranges without walking
    /// [`parent`](Self::parent) themselves.
    pub fn parent_buffer(&self) -> &'a LayerText {
        self.parent_buffer
    }

    /// The text of the full `name!(...)` call, sliced from
    /// [`parent_buffer`](Self::parent_buffer).  Equivalent to
    /// `&parent_buffer()[call_offset..call_offset + call_length]`.
    pub fn call_text(&self) -> &'a str {
        let end = LayerOffset::from_raw(self.call_offset.as_u32() + self.call_length.as_u32());
        &self.parent_buffer[crate::source::LayerRange {
            start: self.call_offset,
            end,
        }]
    }

    /// Iterator over the top-level argument spans of this macro call
    /// at the call site.  Populated for both registered and fallback
    /// calls — the parser scans `name!(a, b, c)` the same way in
    /// either path.  Each arg's offset indexes into the same
    /// [`parent_buffer`](Self::parent_buffer) as
    /// [`call_offset`](Self::call_offset); call
    /// [`text()`](MacroCallArg::text) on the arg to slice directly.
    /// Leading and trailing whitespace / comments are trimmed.
    ///
    /// Yields an empty iterator for `name!()` calls with zero args
    /// and for calls whose arity exceeded the parser's scan buffer
    /// (rare; >64 args, falls through gracefully).
    pub fn args(&self) -> impl Iterator<Item = MacroCallArg<'a>> + use<'_, 'a> {
        // SAFETY: the parser pointer is live for 'a (the parsed
        // statement's lifetime); the C accessors clamp out-of-range
        // indices so count is authoritative.
        let count = unsafe {
            self.parser
                .as_ref()
                .macro_rewrite_arg_count(self.rewrite_idx)
        };
        let rewrite_idx = self.rewrite_idx;
        let parser = self.parser;
        let buffer = self.parent_buffer;
        (0..count).map(move |i| {
            // SAFETY: i < count; the C side returns a valid arg.
            let a = unsafe { parser.as_ref().macro_rewrite_arg_at(rewrite_idx, i) };
            MacroCallArg {
                buffer,
                offset: LayerOffset::from_raw(a.offset),
                length: LayerLen::from_raw(a.length),
            }
        })
    }

    /// Iterator over the `$param` substitutions recorded on this rewrite.
    pub fn arg_segments(&self) -> impl Iterator<Item = MacroArgSegment<'a>> + use<'_, 'a> {
        // SAFETY: the parser pointer is live for 'a (the parsed
        // statement's lifetime); the C accessors clamp out-of-range
        // indices so count is authoritative.
        let count = unsafe {
            self.parser
                .as_ref()
                .macro_rewrite_arg_segment_count(self.rewrite_idx)
        };
        let rewrite_idx = self.rewrite_idx;
        let parser = self.parser;
        (0..count).map(move |i| {
            // SAFETY: i < count; the C side returns a valid segment.
            let s = unsafe { parser.as_ref().macro_rewrite_arg_segment_at(rewrite_idx, i) };
            let origin = if s.origin_parent_idx == u32::MAX {
                ArgOrigin::Source
            } else {
                ArgOrigin::Rewrite(RewriteIdx::from_raw(s.origin_parent_idx))
            };
            MacroArgSegment {
                body_offset: LayerOffset::from_raw(s.body_offset),
                body_length: LayerLen::from_raw(s.body_length),
                expansion_offset: LayerOffset::from_raw(s.expansion_offset),
                expansion_length: LayerLen::from_raw(s.expansion_length),
                origin,
                origin_offset: LayerOffset::from_raw(s.origin_offset),
                origin_length: LayerLen::from_raw(s.origin_length),
                _lifetime: std::marker::PhantomData,
            }
        })
    }
}

/// One top-level argument of a macro call, at the call site.
///
/// Produced by [`MacroRewrite::args`] for both registered (expanded)
/// and fallback calls — the parser scans `name!(a, b, c)` the same
/// way regardless of whether `name` resolved to a registered macro.
/// Leading and trailing whitespace and comments are trimmed from the
/// range.
///
/// The arg carries the buffer it indexes into ([`buffer`](Self::buffer),
/// always equal to the enclosing rewrite's
/// [`parent_buffer`](MacroRewrite::parent_buffer)), so callers can
/// slice the text directly via [`text`](Self::text) without having
/// to walk the rewrite's parent chain.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MacroCallArg<'a> {
    pub(crate) buffer: &'a LayerText,
    pub(crate) offset: LayerOffset,
    pub(crate) length: LayerLen,
}

impl<'a> MacroCallArg<'a> {
    /// Byte offset of the arg text, measured into
    /// [`buffer`](Self::buffer).
    pub fn offset(&self) -> LayerOffset {
        self.offset
    }
    /// Byte length of the arg text.
    pub fn length(&self) -> LayerLen {
        self.length
    }
    /// The buffer the arg's offset indexes into — the enclosing
    /// rewrite's [`parent_buffer`](MacroRewrite::parent_buffer).
    pub fn buffer(&self) -> &'a LayerText {
        self.buffer
    }
    /// The arg's source text, sliced from [`buffer`](Self::buffer).
    pub fn text(&self) -> &'a str {
        let end = LayerOffset::from_raw(self.offset.as_u32() + self.length.as_u32());
        &self.buffer[crate::source::LayerRange {
            start: self.offset,
            end,
        }]
    }
}

/// Where a macro argument's text was authored.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ArgOrigin {
    /// The arg text lives in the authored source.
    Source,
    /// The arg text lives in the expansion buffer of the referenced
    /// rewrite (which in turn may have its own arg segments to descend).
    Rewrite(RewriteIdx),
}

/// One `$param` substitution recorded on a [`MacroRewrite`].
///
/// Enables downstream tracebacks to anchor each substitution back to the
/// authored source, possibly via a chain of earlier substitutions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MacroArgSegment<'a> {
    pub(crate) body_offset: LayerOffset,
    pub(crate) body_length: LayerLen,
    pub(crate) expansion_offset: LayerOffset,
    pub(crate) expansion_length: LayerLen,
    pub(crate) origin: ArgOrigin,
    pub(crate) origin_offset: LayerOffset,
    pub(crate) origin_length: LayerLen,
    pub(crate) _lifetime: std::marker::PhantomData<&'a ()>,
}

impl MacroArgSegment<'_> {
    /// Byte offset of the `$param` token in the macro's authored body.
    /// Zero when the rewrite was registered via the raw arg-map API
    /// (which doesn't supply authored-body positions).
    pub fn body_offset(&self) -> LayerOffset {
        self.body_offset
    }
    /// Byte length of the `$param` token in the authored body.
    pub fn body_length(&self) -> LayerLen {
        self.body_length
    }
    /// Byte offset of the substituted arg text in the rewrite's
    /// [`expansion`](MacroRewrite::expansion) buffer.
    pub fn expansion_offset(&self) -> LayerOffset {
        self.expansion_offset
    }
    /// Byte length of the substituted arg text in the expansion buffer.
    pub fn expansion_length(&self) -> LayerLen {
        self.expansion_length
    }
    /// Where the arg text was authored — the source, or another rewrite's
    /// expansion buffer.
    pub fn origin(&self) -> ArgOrigin {
        self.origin
    }
    /// Convenience: parent rewrite index if the origin is a rewrite,
    /// `None` if the origin is the authored source.
    pub fn origin_parent(&self) -> Option<RewriteIdx> {
        match self.origin {
            ArgOrigin::Source => None,
            ArgOrigin::Rewrite(i) => Some(i),
        }
    }
    /// Byte offset of the arg text in its origin.
    pub fn origin_offset(&self) -> LayerOffset {
        self.origin_offset
    }
    /// Byte length of the arg text in its origin.
    pub fn origin_length(&self) -> LayerLen {
        self.origin_length
    }
}

/// One frame in a span traceback.
///
/// Each frame describes a position inside either the original authored
/// source (root frame, `name` is `None`) or a macro expansion layer
/// (inner frame, `name` carries the macro name).
///
/// Frame 0 is the outermost (root source); the last frame is the
/// innermost expansion layer.
///
/// `snippet` is the buffer to render the caret against, and
/// `offset_in_snippet` is the byte offset within that snippet.
///
/// Returned by [`super::AnyParsedStatement::traceback`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TracebackFrame<'a> {
    /// Frame name — `None` for the root source frame, or `Some(name)`
    /// for a macro expansion frame, where `name` is the macro's
    /// registered name.
    pub name: Option<&'a str>,
    /// 1-based line number of `offset_in_snippet` within `snippet`.
    pub line: LineNumber,
    /// 1-based column number of `offset_in_snippet` within `snippet`.
    pub col: ColumnNumber,
    /// Buffer to render the caret against — the original source for the
    /// root frame, or an expansion layer's buffer for macro frames.
    /// Typed as [`LayerText`] so it can be sliced directly by a
    /// [`LayerRange`] built from `offset_in_snippet` / `length_in_snippet`.
    ///
    /// [`LayerRange`]: crate::source::LayerRange
    pub snippet: &'a LayerText,
    /// Byte offset of the frame's position within `snippet`.
    pub offset_in_snippet: LayerOffset,
    /// Byte length of the frame's position within `snippet`.
    pub length_in_snippet: LayerLen,
}

/// Parser's best guess about what kind of token fits next.
///
/// Returned by incremental parse sessions for completion engines.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
#[repr(u32)]
pub enum CompletionContext {
    /// Could not determine context.
    #[default]
    Unknown = 0,
    /// Parser expects an expression.
    Expression = 1,
    /// Parser expects a table reference.
    TableRef = 2,
}

impl CompletionContext {
    /// Convert from a numeric completion-context code.
    ///
    /// Mostly useful for FFI and serialization boundaries.
    pub fn from_raw(v: u32) -> Self {
        match v {
            1 => Self::Expression,
            2 => Self::TableRef,
            _ => Self::Unknown,
        }
    }

    /// Return the numeric completion-context code.
    ///
    /// Mostly useful for FFI and serialization boundaries.
    pub fn raw(self) -> u32 {
        self as u32
    }
}

impl From<CompletionContext> for u32 {
    fn from(v: CompletionContext) -> u32 {
        v.raw()
    }
}
