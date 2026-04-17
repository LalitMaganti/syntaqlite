// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

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
    offset: u32,
    length: u32,
    token_idx: u32,
    side: CommentSide,
}

impl<'a> Comment<'a> {
    pub(super) fn new(
        text: &'a str,
        kind: CommentKind,
        offset: u32,
        length: u32,
        token_idx: u32,
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

    /// Byte offset of the comment start within the statement source.
    pub fn offset(&self) -> u32 {
        self.offset
    }

    /// Byte length of the comment text.
    pub fn length(&self) -> u32 {
        self.length
    }

    /// Index of the owning token in the statement's token stream.
    /// May equal the token count when the comment trails the last token
    /// of the statement and no following statement exists.
    pub fn token_idx(&self) -> u32 {
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
    offset: u32,
    length: u32,
    kind: CommentKind,
    token_idx: u32,
    side: CommentSide,
}

impl CommentSpan {
    /// Byte offset of the comment start within the statement source.
    pub fn offset(&self) -> u32 {
        self.offset
    }

    /// Byte length of the comment text.
    pub fn length(&self) -> u32 {
        self.length
    }

    /// Whether this is a line (`--`) or block (`/* */`) comment.
    pub fn kind(&self) -> CommentKind {
        self.kind
    }

    /// Index of the owning token in the statement's token stream.
    /// See [`Comment::token_idx`] for attachment semantics.
    pub fn token_idx(&self) -> u32 {
        self.token_idx
    }

    /// Whether this comment leads or trails its owning token.
    pub fn side(&self) -> CommentSide {
        self.side
    }

    pub(super) fn new(
        offset: u32,
        length: u32,
        kind: CommentKind,
        token_idx: u32,
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
#[derive(Debug, Clone, Copy)]
pub struct TypedParserToken<'a, G: TypedDialect> {
    text: &'a str,
    token_type: G::Token,
    flags: ParserTokenFlags,
    offset: u32,
    length: u32,
}

impl<'a, G: TypedDialect> TypedParserToken<'a, G> {
    pub(super) fn new(
        text: &'a str,
        token_type: G::Token,
        flags: ParserTokenFlags,
        offset: u32,
        length: u32,
    ) -> Self {
        TypedParserToken {
            text,
            token_type,
            flags,
            offset,
            length,
        }
    }

    /// The source text slice covered by this token.
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

    /// Byte offset of the token start within the statement source.
    pub fn offset(&self) -> u32 {
        self.offset
    }

    /// Byte length of the token text.
    pub fn length(&self) -> u32 {
        self.length
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
    pub(crate) parent: Option<u32>,
    pub(crate) rewrite_idx: u32,
    pub(crate) call_offset: u32,
    pub(crate) call_length: u32,
    pub(crate) expansion: &'a str,
    pub(crate) name: &'a str,
    pub(crate) def_line: u32,
    pub(crate) def_col: u32,
    pub(crate) body_call_offset: u32,
    pub(crate) body_call_length: u32,
    pub(crate) parser: std::ptr::NonNull<crate::parser::ffi::CParser>,
    pub(crate) _lifetime: std::marker::PhantomData<&'a ()>,
}

impl<'a> MacroRewrite<'a> {
    /// Index of the parent rewrite, or `None` if this rewrite applies
    /// directly to the authored source.
    pub fn parent(&self) -> Option<u32> {
        self.parent
    }
    /// Byte offset of the macro call in the parent's text.
    pub fn call_offset(&self) -> u32 {
        self.call_offset
    }
    /// Byte length of the entire macro call in the parent's text.
    pub fn call_length(&self) -> u32 {
        self.call_length
    }
    /// The replacement text for the macro call.  Nested macro calls that
    /// appear in this buffer are reported as separate [`MacroRewrite`]
    /// entries whose [`parent`](Self::parent) refers back to this one.
    pub fn expansion(&self) -> &'a str {
        self.expansion
    }
    /// The macro name as it appears at the call site.
    pub fn name(&self) -> &'a str {
        self.name
    }
    /// 1-based line of the macro definition (0 if unknown).
    pub fn def_line(&self) -> u32 {
        self.def_line
    }
    /// 1-based column of the macro definition (0 if unknown).
    pub fn def_col(&self) -> u32 {
        self.def_col
    }
    /// Byte offset of this call in the parent's *authored* body, computed
    /// by inverting the length shifts the parent's `$param` substitutions
    /// introduced.  For top-level rewrites the parent is the authored
    /// source, so this equals [`call_offset`](Self::call_offset).
    ///
    /// Returns [`u32::MAX`] (the arg-internal sentinel) when the call was
    /// tokenized from a `$param` substitution — consumers should descend
    /// through the matching arg segment instead of rewriting in the body.
    /// [`body_call_length`](Self::body_call_length) mirrors the sentinel.
    pub fn body_call_offset(&self) -> u32 {
        self.body_call_offset
    }
    /// Length counterpart of [`body_call_offset`](Self::body_call_offset).
    pub fn body_call_length(&self) -> u32 {
        self.body_call_length
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
                ArgOrigin::Rewrite(s.origin_parent_idx)
            };
            MacroArgSegment {
                body_offset: s.body_offset,
                body_length: s.body_length,
                expansion_offset: s.expansion_offset,
                expansion_length: s.expansion_length,
                origin,
                origin_offset: s.origin_offset,
                origin_length: s.origin_length,
                _lifetime: std::marker::PhantomData,
            }
        })
    }
}

/// Where a macro argument's text was authored.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ArgOrigin {
    /// The arg text lives in the authored source.
    Source,
    /// The arg text lives in the expansion buffer of the referenced
    /// rewrite (which in turn may have its own arg segments to descend).
    Rewrite(u32),
}

/// One `$param` substitution recorded on a [`MacroRewrite`].
///
/// Enables downstream tracebacks to anchor each substitution back to the
/// authored source, possibly via a chain of earlier substitutions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MacroArgSegment<'a> {
    pub(crate) body_offset: u32,
    pub(crate) body_length: u32,
    pub(crate) expansion_offset: u32,
    pub(crate) expansion_length: u32,
    pub(crate) origin: ArgOrigin,
    pub(crate) origin_offset: u32,
    pub(crate) origin_length: u32,
    pub(crate) _lifetime: std::marker::PhantomData<&'a ()>,
}

impl MacroArgSegment<'_> {
    /// Byte offset of the `$param` token in the macro's authored body.
    /// Zero when the rewrite was registered via the raw arg-map API
    /// (which doesn't supply authored-body positions).
    pub fn body_offset(&self) -> u32 {
        self.body_offset
    }
    /// Byte length of the `$param` token in the authored body.
    pub fn body_length(&self) -> u32 {
        self.body_length
    }
    /// Byte offset of the substituted arg text in the rewrite's
    /// [`expansion`](MacroRewrite::expansion) buffer.
    pub fn expansion_offset(&self) -> u32 {
        self.expansion_offset
    }
    /// Byte length of the substituted arg text in the expansion buffer.
    pub fn expansion_length(&self) -> u32 {
        self.expansion_length
    }
    /// Where the arg text was authored — the source, or another rewrite's
    /// expansion buffer.
    pub fn origin(&self) -> ArgOrigin {
        self.origin
    }
    /// Convenience: parent rewrite index if the origin is a rewrite,
    /// `None` if the origin is the authored source.
    pub fn origin_parent(&self) -> Option<u32> {
        match self.origin {
            ArgOrigin::Source => None,
            ArgOrigin::Rewrite(i) => Some(i),
        }
    }
    /// Byte offset of the arg text in its origin.
    pub fn origin_offset(&self) -> u32 {
        self.origin_offset
    }
    /// Byte length of the arg text in its origin.
    pub fn origin_length(&self) -> u32 {
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
    pub line: u32,
    /// 1-based column number of `offset_in_snippet` within `snippet`.
    pub col: u32,
    /// Buffer to render the caret against.
    pub snippet: &'a str,
    /// Byte offset of the frame's position within `snippet`.
    pub offset_in_snippet: usize,
    /// Byte length of the frame's position within `snippet`.
    pub length_in_snippet: usize,
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
