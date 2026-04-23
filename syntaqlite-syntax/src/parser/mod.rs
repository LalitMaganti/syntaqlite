// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::cell::RefCell;
use std::ffi::CStr;
use std::marker::PhantomData;
use std::ptr::NonNull;
use std::rc::Rc;

use std::ffi::c_void;

use crate::source::{
    ColumnNumber, DocLen, DocOffset, DocRange, DocText, LayerLen, LayerOffset, LayerText,
    LineNumber, RewriteIdx, StatementBase, StmtLen, StmtOffset, StmtRange, StmtText, TokenIdx,
};

use crate::any::{AnyNodeTag, AnyTokenType};
use crate::ast::{AnyNodeId, ArenaNode, GrammarNodeType, GrammarTokenType, RawNodeList};
use crate::dialect::{AnyDialect, TypedDialect};

use ffi::{CParser, CToken};

mod config;
mod ffi;
mod incremental;
#[cfg(feature = "sqlite")]
mod session;
mod types;

pub use config::ParserConfig;
#[cfg(feature = "sqlite")]
pub use incremental::IncrementalParseSession;
pub use incremental::{AnyIncrementalParseSession, TypedIncrementalParseSession};
#[cfg(feature = "sqlite")]
pub use session::{ParseError, ParseSession, ParsedStatement, Parser, ParserToken};
pub use types::{
    AnyParserToken, ArgOrigin, Comment, CommentKind, CommentSide, CommentSpan, CompletionContext,
    MACRO_BODY_CALL_ARG_INTERNAL, MacroArgSegment, MacroCallArg, MacroRewrite, ParseOutcome,
    ParserTokenFlags, TracebackFrame, TypedParserToken,
};

/// A single macro argument as presented to the lookup callback.
///
/// Each argument spans the raw tokens between commas in the invocation
/// `name!(arg1, arg2, ...)`.
#[derive(Debug)]
pub struct MacroArg<'a> {
    /// The raw text of this argument.
    pub text: &'a str,
}

/// Handle for writing macro expansion results from inside a lookup callback.
///
/// Wraps a `*mut CParser` — the callback calls [`write`](Self::write)
/// to set the expanded body, and optionally
/// [`set_definition`](Self::set_definition) for traceback provenance.
pub struct MacroOutput {
    parser: *mut CParser,
}

impl MacroOutput {
    fn new(parser: *mut CParser) -> Self {
        Self { parser }
    }

    /// Write the expanded body text as the macro result.
    #[expect(clippy::cast_possible_truncation)]
    pub fn write(&mut self, body: &str) {
        // SAFETY: parser is valid for the duration of the callback.
        unsafe {
            ffi::syntaqlite_macro_expansion_set_result(
                self.parser,
                body.as_ptr().cast(),
                body.len() as u32,
                0,
                0,
            );
        }
    }

    /// Set the 1-based line/column of the macro definition for tracebacks.
    /// Must be called *after* [`write`](Self::write).  Use
    /// [`LineNumber::from_raw`] / [`ColumnNumber::from_raw`] to construct
    /// the inputs; `0` means "unknown".
    pub fn set_definition(&mut self, line: LineNumber, col: ColumnNumber) {
        // SAFETY: parser is valid for the duration of the callback.
        unsafe {
            ffi::syntaqlite_macro_expansion_set_result(
                self.parser,
                std::ptr::null(),
                0,
                line.as_u32(),
                col.as_u32(),
            );
        }
    }

    /// Expand a template body by substituting `$param` placeholders and
    /// set the result.
    ///
    /// Uses the SQL tokenizer to correctly skip `$param` inside string
    /// literals and comments. Returns `true` on success, `false` if arg
    /// count doesn't match or a placeholder references an unknown param.
    pub fn expand_template(&mut self, body: &str, params: &[String]) -> bool {
        self.expand_template_inner(body, params, 0)
    }

    /// Like [`expand_template`](Self::expand_template), but unknown `$param`
    /// tokens (those not in `params`) are copied verbatim into the expansion
    /// buffer instead of causing a failure. This is useful for macros whose
    /// bodies contain `$placeholders` intended for a nested macro.
    pub fn expand_template_permissive(&mut self, body: &str, params: &[String]) -> bool {
        self.expand_template_inner(body, params, EXPAND_PASSTHROUGH_UNKNOWN)
    }

    #[expect(clippy::cast_possible_truncation)]
    fn expand_template_inner(&mut self, body: &str, params: &[String], flags: u32) -> bool {
        let param_ptrs: Vec<*const std::ffi::c_char> =
            params.iter().map(|p| p.as_ptr().cast()).collect();
        let param_lens: Vec<u32> = params.iter().map(|p| p.len() as u32).collect();

        // SAFETY: All pointers are valid for the duration of the call.
        let rc = unsafe {
            ffi::syntaqlite_macro_expansion_expand_and_set_result(
                self.parser,
                body.as_ptr().cast(),
                body.len() as u32,
                param_ptrs.as_ptr(),
                param_lens.as_ptr(),
                params.len() as u32,
                flags,
            )
        };
        rc == ffi::SYNTAQLITE_OK
    }
}

/// Flag: copy unknown `$param` tokens verbatim instead of failing.
const EXPAND_PASSTHROUGH_UNKNOWN: u32 = 0x1;

/// Trait for macro lookup callbacks.
///
/// Implement this to provide custom macro expansion logic. The parser
/// calls [`lookup`](MacroLookup::lookup) when it encounters a `name!(args)`
/// invocation.
///
/// For the common case of template macros (`$param` substitution), use
/// [`TemplateMacroRegistry`] which implements this trait.
pub trait MacroLookup {
    /// Look up a macro by name and expand it.
    ///
    /// On success, write the expanded body into `out` and return `true`.
    /// Return `false` if the macro is not found (the parser will fall
    /// back to `TK_ID` when `macro_fallback` is enabled).
    fn lookup(&mut self, name: &str, args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool;
}

/// Internal state for the macro lookup trampoline.
struct MacroLookupState {
    handler: Box<dyn MacroLookup>,
}

/// C trampoline for the macro lookup callback.
///
/// # Safety
///
/// `user_data` must be a valid pointer to a `MacroLookupState`.
/// `parser` must be a valid pointer to the active `SyntaqliteParser`.
unsafe extern "C" fn macro_lookup_trampoline(
    user_data: *mut c_void,
    parser: *mut CParser,
    name: *const std::ffi::c_char,
    name_len: u32,
    args: *const CToken,
    arg_count: u32,
) -> i32 {
    // SAFETY: `user_data` is a `Box<MacroLookupState>` pointer created in
    // `set_macro_lookup` and valid for the lifetime of the parser.
    let state: &mut MacroLookupState = unsafe { &mut *(user_data.cast::<MacroLookupState>()) };

    // SAFETY: `name` points to `name_len` bytes of valid UTF-8 from the C parser.
    let name_str = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(name.cast(), name_len as usize))
    };

    let macro_args: Vec<MacroArg<'_>> = (0..arg_count as usize)
        .map(|i| {
            // SAFETY: `args` points to `arg_count` contiguous CToken structs.
            let tok = unsafe { &*args.add(i) };
            // SAFETY: `tok.text` points to `tok.length` bytes of valid UTF-8.
            let text = unsafe {
                std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                    tok.text.cast(),
                    tok.length as usize,
                ))
            };
            MacroArg { text }
        })
        .collect();

    let mut macro_out = MacroOutput::new(parser);
    if state.handler.lookup(name_str, &macro_args, &mut macro_out) {
        ffi::MACRO_LOOKUP_OK
    } else {
        ffi::MACRO_LOOKUP_NOT_FOUND
    }
}

/// Indicates whether parsing can continue after an error.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum ParseErrorKind {
    /// Parsing recovered to the next statement boundary.
    ///
    /// In plain terms: this statement had a syntax error, but the parser was
    /// still able to skip forward (usually to the next `;`) and continue with
    /// later statements.
    ///
    /// The current statement can include `Error` AST nodes where invalid input
    /// was skipped.
    ///
    /// A partial AST may still be available for diagnostics.
    Recovered = 1,
    /// Parsing could not recover for this statement/input.
    ///
    /// In plain terms: the parser hit a syntax error and could not find a safe
    /// point to continue from.
    ///
    /// No reliable tree is available, and callers should usually stop reading
    /// further results from this session.
    Fatal = 2,
}

/// Parser API parameterized by dialect type `G`.
///
/// Primarily for library/framework code over generated dialects.
///
/// - Use this when dialect type is known at compile time.
/// - Use top-level [`Parser`] for typical `SQLite` SQL app code.
pub struct TypedParser<G: TypedDialect> {
    inner: Rc<RefCell<Option<ParserInner>>>,
    dialect: AnyDialect,
    _marker: PhantomData<G>,
}

impl<G: TypedDialect> TypedParser<G> {
    /// Create a parser for dialect `G` with default [`ParserConfig`].
    pub fn new(dialect: G) -> Self {
        Self::with_config(dialect, &ParserConfig::default())
    }

    /// Create a parser for dialect `G` with custom [`ParserConfig`].
    ///
    /// # Panics
    /// Panics if parser allocation fails (out of memory).
    pub fn with_config(dialect: G, config: &ParserConfig) -> Self {
        let dialect_raw: AnyDialect = dialect.into();
        // SAFETY: create(NULL, dialect_raw.inner) allocates a new parser with
        // default malloc/free. The C side copies the dialect handle.
        let mut raw = NonNull::new(unsafe { CParser::create(std::ptr::null(), dialect_raw.inner) })
            .expect("parser allocation failed");

        // SAFETY: raw is freshly created (not sealed), so these calls always return 0.
        unsafe {
            raw.as_mut().set_trace(u32::from(config.trace()));
            raw.as_mut()
                .set_collect_tokens(u32::from(config.collect_tokens()));
            raw.as_mut()
                .set_macro_fallback(u32::from(config.macro_fallback()));
            raw.as_mut()
                .set_collect_node_extents(u32::from(config.collect_node_extents()));
        }

        TypedParser {
            inner: Rc::new(RefCell::new(Some(ParserInner {
                raw,
                source_buf: Vec::new(),
                macro_handler: None,
            }))),
            dialect: dialect_raw,
            _marker: PhantomData,
        }
    }

    /// Parse a SQL script and return a typed statement session.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::typed::{dialect, TypedParser};
    /// use syntaqlite_syntax::ParseOutcome;
    ///
    /// let parser = TypedParser::new(dialect());
    /// let mut session = parser.parse("SELECT 1;");
    /// let stmt = match session.next() {
    ///     ParseOutcome::Ok(stmt) => stmt,
    ///     ParseOutcome::Done => panic!("expected statement"),
    ///     ParseOutcome::Err(err) => panic!("unexpected parse error: {err}"),
    /// };
    /// assert!(stmt.root().is_some());
    /// ```
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    /// Drop the previous session before starting a new one.
    pub fn parse(&self, source: &str) -> TypedParseSession<G> {
        let mut inner = self
            .inner
            .borrow_mut()
            .take()
            .expect("TypedParser::parse called while a session is still active");
        // SAFETY: inner.raw is valid (owned via ParserInner); source is
        // copied into source_buf which will be owned by the session.
        unsafe { reset_parser(inner.raw.as_ptr(), &mut inner.source_buf, source) };
        TypedParseSession {
            dialect: self.dialect.clone(),
            inner: Some(inner),
            slot: Rc::clone(&self.inner),
            _marker: PhantomData,
        }
    }

    /// Install a macro lookup callback.
    ///
    /// When the parser encounters `name!(args)`, it calls `handler` to resolve
    /// the macro. Pass `None` to disable macro expansion.
    ///
    /// The handler is called with the macro name and its arguments. It should
    /// return `Some(MacroExpansion)` on success, or `None` if the macro is not
    /// found (fallback to `TK_ID` when `macro_fallback` is enabled).
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    pub fn set_macro_lookup(&mut self, handler: Option<Box<dyn MacroLookup>>) {
        let mut inner_ref = self.inner.borrow_mut();
        let inner = inner_ref
            .as_mut()
            .expect("set_macro_lookup called while a session is still active");
        // Drop old handler if any.
        if let Some(old) = inner.macro_handler.take() {
            // SAFETY: ptr was created by Box::into_raw in a previous call.
            let _: Box<MacroLookupState> = unsafe { Box::from_raw(old.cast()) };
        }
        match handler {
            Some(handler) => {
                let state = Box::new(MacroLookupState { handler });
                let user_data = Box::into_raw(state).cast::<c_void>();
                // SAFETY: user_data is a valid pointer to MacroLookupState.
                unsafe {
                    inner
                        .raw
                        .as_mut()
                        .set_macro_lookup(Some(macro_lookup_trampoline), user_data);
                }
                inner.macro_handler = Some(user_data);
            }
            None => {
                // SAFETY: passing NULL disables macro expansion.
                unsafe {
                    inner
                        .raw
                        .as_mut()
                        .set_macro_lookup(None, std::ptr::null_mut());
                }
            }
        }
    }

    /// Start incremental parsing for dialect `G`.
    ///
    /// Use this when tokens arrive over time (editor completion, interactive
    /// parsing, macro-expansion pipelines).
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    /// Drop the previous session before starting a new one.
    pub fn incremental_parse(&self, source: &str) -> TypedIncrementalParseSession<G> {
        let mut inner = self
            .inner
            .borrow_mut()
            .take()
            .expect("TypedParser::incremental_parse called while a session is still active");
        // SAFETY: inner.raw is valid (owned via ParserInner); source is
        // copied into source_buf.
        unsafe { reset_parser(inner.raw.as_ptr(), &mut inner.source_buf, source) };
        let c_text_ptr =
            NonNull::new(inner.source_buf.as_mut_ptr()).expect("source_buf is non-empty");
        TypedIncrementalParseSession::new(
            c_text_ptr,
            self.dialect.clone(),
            inner,
            Rc::clone(&self.inner),
        )
    }
}

/// Cursor over statements parsed by a [`TypedParser`].
///
/// Designed for multi-statement SQL input.
///
/// - Iterates statement-by-statement.
/// - Surfaces failures per statement.
/// - Can continue after recoverable errors.
pub struct TypedParseSession<G: TypedDialect> {
    dialect: AnyDialect,
    /// Checked-out parser state. Returned to `slot` on drop.
    inner: Option<ParserInner>,
    /// Slot to return `inner` to when this session is dropped.
    slot: Rc<RefCell<Option<ParserInner>>>,
    _marker: PhantomData<G>,
}

impl<G: TypedDialect> Drop for TypedParseSession<G> {
    fn drop(&mut self) {
        if let Some(inner) = self.inner.take() {
            *self.slot.borrow_mut() = Some(inner);
        }
    }
}

impl<G: TypedDialect> TypedParseSession<G> {
    /// Install a macro lookup callback during an active session.
    ///
    /// This is used by the analyzer to install a callback that resolves
    /// macro calls defined by `CREATE PERFETTO MACRO` statements during
    /// parsing.
    ///
    /// # Panics
    ///
    /// Panics if the session has already finished.
    pub fn set_macro_lookup(&mut self, handler: Option<Box<dyn MacroLookup>>) {
        let inner = self
            .inner
            .as_mut()
            .expect("set_macro_lookup called on finished session");
        // Drop old handler if any.
        if let Some(old) = inner.macro_handler.take() {
            // SAFETY: `old` was created via `Box::into_raw` in a previous call.
            let _: Box<MacroLookupState> = unsafe { Box::from_raw(old.cast()) };
        }
        match handler {
            Some(handler) => {
                let state = Box::new(MacroLookupState { handler });
                let user_data = Box::into_raw(state).cast::<c_void>();
                // SAFETY: `inner.raw` is a valid parser pointer; the trampoline
                // and user_data are compatible and outlive the parser.
                unsafe {
                    inner
                        .raw
                        .as_mut()
                        .set_macro_lookup(Some(macro_lookup_trampoline), user_data);
                }
                inner.macro_handler = Some(user_data);
            }
            // SAFETY: passing null disables the callback; no dangling pointer.
            None => unsafe {
                inner
                    .raw
                    .as_mut()
                    .set_macro_lookup(None, std::ptr::null_mut());
            },
        }
    }

    /// Parse and return the next statement as a tri-state outcome.
    ///
    /// Mirrors C parser return codes directly:
    /// - [`ParseOutcome::Done`]  -> `SYNTAQLITE_PARSE_DONE`
    /// - [`ParseOutcome::Ok`]    -> `SYNTAQLITE_PARSE_OK`
    /// - [`ParseOutcome::Err`]   -> `SYNTAQLITE_PARSE_ERROR`
    ///
    /// Use [`ParseOutcome::transpose`] for `?`-friendly
    /// `Result<Option<_>, _>` control flow.
    ///
    /// # Panics
    ///
    /// Panics if called after the session is finished.
    #[expect(clippy::should_implement_trait)]
    pub fn next(&mut self) -> ParseOutcome<TypedParsedStatement<'_, G>, TypedParseError<'_, G>> {
        // SAFETY: raw is valid and exclusively borrowed via &mut self.
        let rc = unsafe {
            self.inner
                .as_mut()
                .expect("inner is Some while session is not finished")
                .raw
                .as_mut()
                .next()
        };

        if rc == ffi::PARSE_DONE {
            return ParseOutcome::Done;
        }

        let inner = self
            .inner
            .as_ref()
            .expect("inner is Some while session is not finished");
        // SAFETY: inner.raw is valid (owned via ParserInner, not yet
        // destroyed); its bound source buffer lives in ParserInner and
        // outlives `&self`.
        let result = unsafe { TypedParsedStatement::new(inner.raw.as_ptr(), self.dialect.clone()) };
        if rc == ffi::PARSE_OK {
            ParseOutcome::Ok(result)
        } else {
            // ERROR (may still carry a recovery tree)
            ParseOutcome::Err(TypedParseError(result))
        }
    }

    /// Full SQL source bound to this session.
    ///
    /// # Panics
    ///
    /// Panics only if session invariants were violated.
    pub fn full_text(&self) -> &str {
        let inner = self
            .inner
            .as_ref()
            .expect("inner is Some while session is not finished");
        // SAFETY: inner.raw is valid for `&self`; the returned slice
        // borrows from the parser's source buffer.
        unsafe { inner.raw.as_ref().full_text() }
    }

    /// Post-expansion source — the bound source with every
    /// currently-active macro call replaced by its expansion.
    /// Materialized into a parser-owned scratch buffer; the returned
    /// slice is invalidated by the next `*_expanded_text` call or
    /// when the session advances to the next statement.
    ///
    /// # Panics
    ///
    /// Panics only if session invariants were violated.
    pub fn expanded_text(&self) -> &str {
        let inner = self
            .inner
            .as_ref()
            .expect("inner is Some while session is not finished");
        // SAFETY: inner.raw is valid for `&self`; the returned slice
        // borrows from the parser's scratch buffer.
        unsafe { inner.raw.as_ref().expanded_text() }
    }

    /// Get a dialect-agnostic view of this session's current arena state.
    ///
    /// Allows reading node data and source text after all statements have been
    /// consumed via [`next`](Self::next). The returned
    /// result borrows from `&self` and is valid as long as this session is alive.
    ///
    /// # Panics
    /// Panics only if session invariants were violated.
    pub fn arena_result(&self) -> AnyParsedStatement<'_> {
        let inner = self
            .inner
            .as_ref()
            .expect("inner is Some while session is alive");
        // SAFETY: inner.raw is a valid parser pointer whose source_buf
        // outlives `&self`.
        unsafe { AnyParsedStatement::new(inner.raw.as_ptr(), self.dialect.clone()) }
    }
}

/// Parser alias for dialect-independent code that picks dialect at runtime.
pub type AnyParser = TypedParser<AnyDialect>;

/// Session alias paired with [`AnyParser`].
pub type AnyParseSession = TypedParseSession<AnyDialect>;

/// Dialect-erased view of a parsed statement.
///
/// Cheap to borrow — holds a raw parser pointer and dialect handle.  Nodes
/// and lists store `&'a AnyParsedStatement<'a>` rather than an owned copy,
/// making them `Copy` and eliminating dialect-handle clones.
#[derive(Clone, Debug)]
pub struct AnyParsedStatement<'a> {
    pub(crate) raw: NonNull<CParser>,
    pub(crate) dialect: AnyDialect,
    _marker: PhantomData<&'a str>,
}

impl<'a> AnyParsedStatement<'a> {
    /// Construct from raw parts.
    ///
    /// # Safety
    /// `raw` must be a valid, non-null parser pointer whose bound source
    /// buffer remains valid for `'a`.
    pub(crate) unsafe fn new(raw: *mut CParser, dialect: AnyDialect) -> Self {
        AnyParsedStatement {
            // SAFETY: caller guarantees raw is non-null.
            raw: unsafe { NonNull::new_unchecked(raw) },
            dialect,
            _marker: PhantomData,
        }
    }

    /// Root node ID for the current statement (`AnyNodeId::NULL` if absent).
    pub fn root_id(&self) -> AnyNodeId {
        // SAFETY: self.raw is a valid, non-null parser pointer for lifetime 'a.
        AnyNodeId(unsafe { self.raw.as_ref().result_root() })
    }

    /// Source text of AST node `id` as `(text, offset)`, or `None` when
    /// extent tracking is disabled or no extent was recorded for this
    /// node.  `offset` is statement-relative.
    ///
    /// Requires [`ParserConfig::with_collect_node_extents`].
    pub fn node_text(&self, id: AnyNodeId) -> Option<(&'a str, StmtOffset)> {
        if id.is_null() {
            return None;
        }
        // SAFETY: self.raw is valid for 'a; the returned slice borrows
        // from the parser's source buffer which outlives 'a.
        unsafe { self.raw.as_ref().node_text(id.0) }
    }

    /// Post-expansion text of AST node `id`.
    ///
    /// For nodes whose tokens all live in a single layer, returns a
    /// direct slice of that layer's buffer (input source or macro
    /// expansion).  For mixed-layer nodes, the text is materialized
    /// into a parser-owned scratch buffer by inlining each enclosed
    /// macro call's expansion; the returned slice is valid until the
    /// next `*_expanded_text` call on the same parser or until the
    /// parser advances to the next statement.  Requires
    /// [`ParserConfig::with_collect_node_extents`].
    pub fn node_expanded_text(&self, id: AnyNodeId) -> Option<&'a str> {
        if id.is_null() {
            return None;
        }
        // SAFETY: self.raw is valid for 'a; the returned slice borrows
        // from either a layer buffer or the parser's scratch buffer,
        // both of which outlive 'a.
        unsafe { self.raw.as_ref().node_expanded_text(id.0) }
    }

    /// Returns `true` if all tokens of AST node `id` live in layer 0
    /// (the original source), `false` if any came from a macro
    /// expansion.  Returns `false` when extent tracking is disabled,
    /// the node id is unknown, or the node is null.
    ///
    /// Requires [`ParserConfig::with_collect_node_extents`].
    pub fn node_is_macro_free(&self, id: AnyNodeId) -> bool {
        if id.is_null() {
            return false;
        }
        // SAFETY: self.raw is valid for 'a.
        unsafe { self.raw.as_ref().node_is_macro_free(id.0) }
    }

    /// Post-expansion text for the whole bound source — the
    /// parser-level analogue of [`Self::node_expanded_text`].
    /// Materializes the source with every currently-active macro call
    /// replaced by its expansion into a parser-owned scratch buffer;
    /// the returned slice is valid until the next `*_expanded_text`
    /// call on the same parser or until the parser advances.
    pub fn expanded_text(&self) -> &'a str {
        // SAFETY: self.raw is valid for 'a; the returned slice borrows
        // from the parser's scratch buffer which outlives 'a.
        unsafe { self.raw.as_ref().expanded_text() }
    }

    /// Returns `true` if the statement contains no macro expansions —
    /// all tokens came from the original source text.
    pub fn is_macro_free(&self) -> bool {
        // SAFETY: self.raw is valid for 'a.
        unsafe { self.raw.as_ref().result_macro_count() == 0 }
    }

    /// Macro rewrites recorded during parsing.  See [`MacroRewrite`] for
    /// the shape of each entry.
    pub fn macro_rewrites(&self) -> impl Iterator<Item = MacroRewrite<'a>> + use<'_, 'a> {
        // SAFETY: self.raw is valid for 'a; the indexed accessor is stable
        // until the next parser_next / reset / destroy call.
        let count = unsafe { self.raw.as_ref().result_macro_count() };
        (0..count).map(move |i| {
            // SAFETY: i < count, so the C side returns a valid rewrite.
            let r = unsafe { self.raw.as_ref().result_macro_rewrite_at(i) };
            // `expansion` and `name` borrow from parser memory valid for
            // 'a (until the next parser_next / reset / destroy, which
            // requires ending the 'a-tied statement borrow).
            let expansion: &'a LayerText = if r.expansion.is_null() {
                LayerText::new("")
            } else {
                // SAFETY: `expansion` points to `expansion_len` bytes
                // owned by the parser, valid for 'a.  Parser buffers are
                // UTF-8 by construction (source text or expansion text
                // produced by the callback, which accepts a &str body).
                let s = unsafe {
                    std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                        r.expansion,
                        r.expansion_len as usize,
                    ))
                };
                LayerText::new(s)
            };
            let name: &'a str = if r.name.is_null() {
                ""
            } else {
                // SAFETY: `name` points to `name_len` bytes owned by the
                // parser, valid for 'a; ASCII identifier by construction.
                unsafe {
                    std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                        r.name,
                        r.name_len as usize,
                    ))
                }
            };
            let parent = if r.parent_idx == u32::MAX {
                None
            } else {
                Some(RewriteIdx::from_raw(r.parent_idx))
            };
            let parent_buffer: &'a LayerText = if r.parent_buffer.is_null() {
                LayerText::new("")
            } else {
                // SAFETY: `parent_buffer` points to `parent_buffer_len`
                // bytes owned by the parser (statement source or parent
                // layer's expansion buffer), valid for 'a.  UTF-8 by
                // construction — both sources are UTF-8.
                let s = unsafe {
                    std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                        r.parent_buffer,
                        r.parent_buffer_len as usize,
                    ))
                };
                LayerText::new(s)
            };
            MacroRewrite {
                parent,
                rewrite_idx: RewriteIdx::from_raw(i),
                call_offset: LayerOffset::from_raw(r.call_offset),
                call_length: LayerLen::from_raw(r.call_length),
                expansion,
                name,
                def_line: LineNumber::from_raw(r.def_line),
                def_col: ColumnNumber::from_raw(r.def_col),
                body_call_offset: LayerOffset::from_raw(r.body_call_offset),
                body_call_length: LayerLen::from_raw(r.body_call_length),
                parent_buffer,
                is_fallback: r.is_fallback != 0,
                parser: self.raw,
                _lifetime: PhantomData,
            }
        })
    }

    /// Build a traceback for a span field.
    ///
    /// Yields [`TracebackFrame`]s in outermost-to-innermost order —
    /// frame 0 is the root source, and the final frame is the position
    /// inside the deepest macro expansion layer.  For macro-free spans,
    /// yields exactly one root frame.
    ///
    /// When a span was tokenized inside a substituted macro argument,
    /// the walk drills through the substitution: the innermost frame
    /// points at the user's authored arg text rather than at the
    /// `foo!(…)` call site.
    ///
    /// Yields no frames for invalid or non-span fields.
    ///
    /// Frames live in a parser-owned scratch buffer that is overwritten
    /// on every call, so this method takes `&mut self`; `.collect()` the
    /// iterator before calling `traceback` again if you need to retain
    /// frames across calls.
    pub fn traceback(
        &mut self,
        node_id: AnyNodeId,
        field_idx: u8,
    ) -> impl Iterator<Item = TracebackFrame<'a>> + use<'_, 'a> {
        let sp = self.field_span(node_id, field_idx);
        // The returned slice borrows from the parser's internal
        // `traceback_buf` vec.  The `&mut self` receiver on this method
        // ensures no other traceback call can overwrite that buffer
        // while the returned iterator is live.
        let raw_frames: &[ffi::CTracebackFrame] = match sp {
            // SAFETY: self.raw is valid for 'a; sp is a copy of an arena value.
            Some(sp) => unsafe { self.raw.as_ref().traceback(sp) },
            None => &[],
        };
        raw_frames.iter().map(|f| TracebackFrame {
            name: if f.name.is_null() || f.name_len == 0 {
                None
            } else {
                // SAFETY: C guarantees name points to name_len bytes of
                // valid UTF-8 in a parser-owned buffer valid for 'a.
                Some(unsafe {
                    std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                        f.name,
                        f.name_len as usize,
                    ))
                })
            },
            line: LineNumber::from_raw(f.line),
            col: ColumnNumber::from_raw(f.col),
            snippet: if f.snippet.is_null() || f.snippet_len == 0 {
                LayerText::new("")
            } else {
                // SAFETY: C guarantees snippet points to snippet_len bytes
                // of valid UTF-8 in a parser-owned buffer valid for 'a.
                let s = unsafe {
                    std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                        f.snippet,
                        f.snippet_len as usize,
                    ))
                };
                LayerText::new(s)
            },
            offset_in_snippet: LayerOffset::from_raw(f.offset_in_snippet),
            length_in_snippet: LayerLen::from_raw(f.length_in_snippet),
        })
    }

    /// Post-expansion text for an arena span — the bytes the tokenizer
    /// actually saw.
    ///
    /// For direct (macro-free) spans, returns a slice of the original
    /// source.  For spans inside a macro expansion, returns the slice
    /// from the appropriate expansion layer's buffer (e.g. `"a"` for
    /// `$name` expanded with arg `a`).  Always a direct slice — no
    /// allocation.
    pub fn span_expanded_text(&self, span: crate::ast::TextSpan) -> &'a str {
        let mut out_len: u32 = 0;
        // SAFETY: self.raw is valid for 'a; span is a copy of an arena value.
        let ptr = unsafe { self.raw.as_ref().span_expanded_text(span, &raw mut out_len) };
        if ptr.is_null() || out_len == 0 {
            return "";
        }
        // SAFETY: C guarantees ptr points to out_len bytes of valid UTF-8
        // in a parser-owned buffer valid for 'a.
        unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize)) }
    }

    /// Resolve a [`TextSpan`](crate::ast::TextSpan) to `(authored_text,
    /// offset)` where `authored_text` is a direct slice of the user's
    /// input source and `offset` is its byte offset relative to
    /// [`text`](Self::text).
    ///
    /// For direct (macro-free) spans, this is the span's own bytes and
    /// position.  For spans inside a macro expansion, this walks the
    /// expansion layer chain: if the span was tokenized inside a
    /// substituted `$param`, it drills back to the arg's origin text in
    /// the caller's source; otherwise it collapses to the outermost
    /// `name!(...)` call site.  Always a direct slice — no allocation.
    ///
    /// Returns `("", 0)` for invalid spans (null, out-of-range).  For a
    /// *valid* in-range span of zero length — either an absent field
    /// (zero-initialized) or a genuine empty-but-quoted token like
    /// `""` — returns `("", real_offset)`; the offset is always
    /// meaningful independent of length.  Callers that need to
    /// distinguish absent from empty-token inspect the span's quote
    /// flag (see [`TextSpan::is_quoted`](crate::ast::TextSpan)).
    pub fn span_text(&self, span: crate::ast::TextSpan) -> (&'a str, StmtOffset) {
        let mut out_len: u32 = 0;
        let mut out_offset: u32 = 0;
        // SAFETY: self.raw is valid for 'a; span is a copy of an arena value.
        let ptr = unsafe {
            self.raw
                .as_ref()
                .span_text(span, &raw mut out_len, &raw mut out_offset)
        };
        if ptr.is_null() {
            return ("", StmtOffset::default());
        }
        // SAFETY: C guarantees ptr points to out_len bytes of valid UTF-8
        // in a parser-owned buffer valid for 'a.  `out_len` may be zero
        // (empty quoted token, or absent field) — `from_raw_parts` is
        // sound for len=0 as long as ptr is non-null, which we've
        // checked.
        let text = unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize))
        };
        (text, StmtOffset::from_raw(out_offset))
    }

    pub(crate) fn field_span(
        &self,
        node_id: AnyNodeId,
        field_idx: u8,
    ) -> Option<crate::ast::TextSpan> {
        let (ptr, tag) = self.node_ptr(node_id)?;
        let meta = self.dialect.field_meta(tag).nth(field_idx as usize)?;
        if !matches!(meta.kind(), crate::dialect::FieldKind::Span) {
            return None;
        }
        // SAFETY: ptr is a valid arena node pointer for 'a; meta describes
        // a Span field at the indicated byte offset within the node struct.
        // TextSpan is `#[repr(C)]` Copy with 1-byte alignment; we use
        // read_unaligned to avoid alignment assumptions on the raw pointer.
        Some(unsafe {
            ptr.add(meta.offset() as usize)
                .cast::<crate::ast::TextSpan>()
                .read_unaligned()
        })
    }

    /// Full SQL source bound to the parse session — the whole input
    /// for multi-statement parses.  Use [`Self::text`] for just this
    /// statement.
    ///
    /// Returned as a [`DocText`], which can be indexed by [`DocRange`]
    /// for type-safe slicing.  Call [`DocText::as_str`] for interop with
    /// `std::str` APIs.
    pub fn full_text(&self) -> &'a DocText {
        // SAFETY: self.raw is valid for 'a; the returned slice borrows
        // from the parser's source buffer which outlives 'a.
        DocText::new(unsafe { self.raw.as_ref().full_text() })
    }

    /// Source slice for just this statement, including any attached
    /// leading/trailing comments.  Every offset the parser emits for
    /// this statement — tokens, comments, spans, node extents, error
    /// offsets, macro rewrite call offsets — is relative to this slice.
    /// Empty if no statement was produced.
    ///
    /// Returned as a [`StmtText`], which can be indexed by [`StmtRange`]
    /// for type-safe slicing.  Call [`StmtText::as_str`] for interop
    /// with `std::str` APIs.
    pub fn text(&self) -> &'a StmtText {
        // SAFETY: self.raw is valid for 'a; the returned slice borrows
        // from the parser's source buffer which outlives 'a.
        let (s, _) = unsafe { self.raw.as_ref().text() };
        StmtText::new(s)
    }

    /// Document-absolute offset of this statement's start within
    /// [`Self::full_text`].  Use [`StatementBase::to_doc`] to convert any
    /// statement-relative offset into an absolute one.
    pub fn statement_base(&self) -> StatementBase {
        // SAFETY: self.raw is valid for 'a.
        let (_, off) = unsafe { self.raw.as_ref().text() };
        StatementBase::new(DocOffset::from_raw(off))
    }

    /// Resolve a span to its document-absolute `(text, range)` in
    /// [`Self::full_text`].  Convenience for diagnostic emission sites
    /// that need full-source positions.
    pub fn span_text_abs(&self, span: crate::ast::TextSpan) -> (&'a str, DocRange) {
        let (text, off) = self.span_text(span);
        let start = off.to_doc(self.statement_base());
        let end = start + DocLen::from_raw(u32::try_from(text.len()).unwrap_or(u32::MAX));
        (text, DocRange { start, end })
    }

    /// Token stream for the current statement.
    ///
    /// Yields every token the parser fed to Lemon while reducing the
    /// statement — including tokens produced by macro expansion.
    /// Each token's [`stmt_range`](AnyParserToken::stmt_range) gives
    /// its authored-source range (layer-N tokens drill up to the
    /// enclosing macro call site, just like `span_text` does for AST
    /// spans), so most consumers can ignore layer identity entirely.
    /// Layer-local info is still available via
    /// [`layer_id`](AnyParserToken::layer_id) /
    /// [`offset`](AnyParserToken::offset) /
    /// [`length`](AnyParserToken::length) for advanced uses.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn tokens(&self) -> impl Iterator<Item = AnyParserToken<'a>> + use<'_, 'a> {
        // SAFETY: self.raw is valid for 'a; the returned slices live for 'a.
        let raw: &'a [ffi::CParserToken] = unsafe { self.raw.as_ref().result_tokens() };
        raw.iter()
            .map(move |t| build_parser_token(self.raw, t, AnyTokenType(t.type_)))
    }

    /// Comments attached to this statement with full per-comment data.
    ///
    /// Requires `collect_tokens: true`.
    pub fn comments(&self) -> impl Iterator<Item = Comment<'a>> + use<'_, 'a> {
        let source = self.text();
        let parser = self.raw;
        // SAFETY: self.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &'a [ffi::CComment] = unsafe { self.raw.as_ref().result_comments() };
        raw.iter().map(move |c| ffi_comment(parser, source, c))
    }

    /// Lightweight comment descriptors without source text borrows.
    ///
    /// Returns an empty iterator if `collect_tokens` was not enabled.
    pub fn comment_spans(&self) -> impl Iterator<Item = CommentSpan> + use<'_> {
        // SAFETY: self.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &[ffi::CComment] = unsafe { self.raw.as_ref().result_comments() };
        raw.iter().map(|c| {
            let kind = match c.kind {
                ffi::CCommentKind::LineComment => CommentKind::Line,
                ffi::CCommentKind::BlockComment => CommentKind::Block,
            };
            let side = match c.side {
                ffi::CCommentSide::Leading => CommentSide::Leading,
                ffi::CCommentSide::Trailing => CommentSide::Trailing,
            };
            CommentSpan::new(
                StmtOffset::from_raw(c.offset),
                StmtLen::from_raw(c.length),
                kind,
                TokenIdx::from_raw(c.token_idx),
                side,
                c.layer_id,
            )
        })
    }

    /// The inclusive `[first, last]` token indices the parser fed to
    /// Lemon while reducing AST node `id`.  Returns `None` when
    /// [`ParserConfig::with_collect_node_extents`] was not enabled,
    /// `id` is null / out-of-range, or the node reduced over zero
    /// tokens (pure epsilon).
    ///
    /// O(1): the range is recorded in a side table during reduction.
    ///
    /// For macro-expanded nodes, the indices may point at layer-N
    /// tokens.  Use [`Comment::layer_id`] to filter expansion-body
    /// comments from authored-source comments when composing with
    /// [`Self::leading_comments`] / [`Self::trailing_comments`].
    pub fn node_token_range(&self, id: AnyNodeId) -> Option<(TokenIdx, TokenIdx)> {
        if id.is_null() {
            return None;
        }
        // SAFETY: self.raw is valid for 'a.
        unsafe { self.raw.as_ref().node_token_range(id.0) }
    }

    /// Comments attached to the first token of AST node `id`.  Thin
    /// wrapper over [`Self::node_token_range`] +
    /// [`Self::leading_comments`].  Returns an empty iterator when
    /// the node has no token range or no leading comments.
    ///
    /// Interior comments (on keywords or between children inside the
    /// node) are NOT surfaced here — walk
    /// [`Self::node_token_range`] and call
    /// [`Self::leading_comments`] / [`Self::trailing_comments`] on
    /// interior token indices for those.
    pub fn node_leading_comments(
        &self,
        id: AnyNodeId,
    ) -> impl Iterator<Item = Comment<'a>> + use<'_, 'a> {
        let source = self.text();
        let parser = self.raw;
        let raw: &'a [ffi::CComment] = if id.is_null() {
            &[]
        } else {
            // SAFETY: self.raw is valid for 'a; the returned slice lives for 'a.
            unsafe { self.raw.as_ref().node_leading_comments(id.0) }
        };
        raw.iter().map(move |c| ffi_comment(parser, source, c))
    }

    /// Comments trailing the last token of AST node `id`.  See
    /// [`Self::node_leading_comments`] for the symmetric case and the
    /// caveat about interior comments.
    pub fn node_trailing_comments(
        &self,
        id: AnyNodeId,
    ) -> impl Iterator<Item = Comment<'a>> + use<'_, 'a> {
        let source = self.text();
        let parser = self.raw;
        let raw: &'a [ffi::CComment] = if id.is_null() {
            &[]
        } else {
            // SAFETY: self.raw is valid for 'a; the returned slice lives for 'a.
            unsafe { self.raw.as_ref().node_trailing_comments(id.0) }
        };
        raw.iter().map(move |c| ffi_comment(parser, source, c))
    }

    /// Extract reflective node data (`tag` + field values) for `id`.
    pub fn extract_fields(&self, id: AnyNodeId) -> Option<(AnyNodeTag, crate::ast::NodeFields)> {
        let (ptr, tag) = self.node_ptr(id)?;
        let mut fields = crate::ast::NodeFields::new();
        for meta in self.dialect.field_meta(tag) {
            // SAFETY: ptr is a valid arena node pointer; meta describes a
            // field within that node's struct layout.
            let val = unsafe { extract_field_value(ptr, &meta) };
            fields.push(val);
        }
        Some((tag, fields))
    }

    // ── Field-shaped convenience accessors ──────────────────────────────
    //
    // These wrap the common patterns of "extract fields, then look at one
    // field" so callers don't repeat the empty/null/wrong-kind boilerplate
    // every time. They handle the [`crate::ast::FIELD_ABSENT`] sentinel as
    // a None hit so callers can pass field indices read from role tables
    // without pre-checking.

    /// Expanded text of a span field, or `None` if the index is
    /// [`crate::ast::FIELD_ABSENT`], the field is empty, or the field is
    /// not a span.
    pub fn span_field_text(&self, fields: &crate::ast::NodeFields, idx: u8) -> Option<&'a str> {
        if idx == crate::ast::FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            crate::ast::FieldValue::Span(sp) if !sp.is_empty() => Some(self.span_expanded_text(sp)),
            _ => None,
        }
    }

    /// `(text, range)` of a span field. `range` is the document-absolute
    /// extent. `None` under the same conditions as
    /// [`Self::span_field_text`].
    pub fn span_field_range(
        &self,
        fields: &crate::ast::NodeFields,
        idx: u8,
    ) -> Option<(&'a str, DocRange)> {
        if idx == crate::ast::FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            crate::ast::FieldValue::Span(sp) if !sp.is_empty() => {
                let text = self.span_expanded_text(sp);
                let (_, range) = self.span_text_abs(sp);
                Some((text, range))
            }
            _ => None,
        }
    }

    /// `(text, range)` of a Name node's field-0 span — used by
    /// `IdentName`-shaped and `Error`-shaped nodes where the identifier
    /// always sits at field 0. Returns empty strings when the node is
    /// `None`, null, or shaped differently.
    pub fn name_text(&self, node_id: Option<AnyNodeId>) -> (&'a str, DocRange) {
        let Some(node_id) = node_id else {
            return ("", DocRange::default());
        };
        let Some((_, fields)) = self.extract_fields(node_id) else {
            return ("", DocRange::default());
        };
        if fields.is_empty() {
            return ("", DocRange::default());
        }
        match fields[0] {
            crate::ast::FieldValue::Span(sp) => {
                let text = self.span_expanded_text(sp);
                let (_, range) = self.span_text_abs(sp);
                (text, range)
            }
            _ => ("", DocRange::default()),
        }
    }

    /// First non-empty span anywhere in `node_id`'s fields. Used as a
    /// generic "give me whatever identifier this node carries" probe.
    pub fn first_span_text(&self, node_id: AnyNodeId) -> Option<&'a str> {
        if node_id.is_null() {
            return None;
        }
        let (_, fields) = self.extract_fields(node_id)?;
        for i in 0..fields.len() {
            if let crate::ast::FieldValue::Span(sp) = fields[i]
                && !sp.is_empty()
            {
                return Some(self.span_expanded_text(sp));
            }
        }
        None
    }

    /// Text of a "name-shaped" field that may be either a direct `Span`
    /// or a `NodeId` pointing at a Name node. Mirrors the dual
    /// representation used by `SourceRef.alias`, CTE names, and similar
    /// fields where the codegen sometimes emits a span and sometimes
    /// emits a child node.
    pub fn name_field_text(&self, fields: &crate::ast::NodeFields, idx: u8) -> Option<&'a str> {
        if idx == crate::ast::FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            crate::ast::FieldValue::Span(sp) if !sp.is_empty() => Some(self.span_expanded_text(sp)),
            crate::ast::FieldValue::NodeId(id) if !id.is_null() => self.first_span_text(id),
            _ => None,
        }
    }

    /// Return child node IDs if `id` is a list node.
    pub fn list_children(&self, id: AnyNodeId) -> Option<&'a [AnyNodeId]> {
        let (_, tag) = self.node_ptr(id)?;
        if !self.dialect.is_list(tag) {
            return None;
        }
        #[expect(clippy::redundant_closure_for_method_calls)]
        self.resolve_list(id).map(|l| l.children())
    }

    /// Iterate direct child node IDs for the node at `id`.
    ///
    /// The returned iterator owns its data and does not borrow from
    /// `self`, so it can be held across `&mut self` method calls on the
    /// statement (e.g. while recursively invoking a visitor that itself
    /// takes `&mut AnyParsedStatement`).
    pub fn child_node_ids(&self, id: AnyNodeId) -> impl Iterator<Item = AnyNodeId> + use<> {
        let mut out = Vec::new();
        if let Some((_, fields)) = self.extract_fields(id) {
            for i in 0..fields.len() {
                if let crate::ast::FieldValue::NodeId(child_id) = fields[i] {
                    if child_id.is_null() {
                        continue;
                    }
                    if let Some(children) = self.list_children(child_id) {
                        out.extend(children.iter().copied().filter(|id| !id.is_null()));
                    } else {
                        out.push(child_id);
                    }
                }
            }
        }
        out.into_iter()
    }

    /// Resolve a `AnyNodeId` to a typed reference, validating the tag.
    pub(crate) fn resolve_as<T: ArenaNode>(&self, id: AnyNodeId) -> Option<&'a T> {
        let (ptr, tag) = self.node_ptr(id)?;
        if tag.0 != T::TAG {
            return None;
        }
        // SAFETY: tag matches T::TAG, confirming the arena node has type T.
        // ptr is valid for 'a. T is #[repr(C)] with a u32 tag as its first
        // field, matching the arena layout.
        Some(unsafe { &*ptr.cast::<T>() })
    }

    /// Resolve a `AnyNodeId` as a [`RawNodeList`] (for list nodes).
    pub(crate) fn resolve_list(&self, id: AnyNodeId) -> Option<&'a RawNodeList> {
        let (ptr, _) = self.node_ptr(id)?;
        // SAFETY: ptr is valid for 'a. List nodes have RawNodeList layout.
        #[expect(clippy::cast_ptr_alignment)]
        Some(unsafe { &*ptr.cast::<RawNodeList>() })
    }

    /// Get a raw pointer to a node in the arena. Returns `(pointer, tag)`.
    pub(crate) fn node_ptr(&self, id: AnyNodeId) -> Option<(*const u8, AnyNodeTag)> {
        if id.is_null() {
            return None;
        }
        // SAFETY: self.raw is valid for 'a. The returned pointer is
        // null-checked; all arena nodes start with a u32 tag.
        unsafe {
            let ptr = self.raw.as_ref().node(id.0);
            if ptr.is_null() {
                return None;
            }
            let tag = AnyNodeTag(*ptr);
            Some((ptr.cast::<u8>(), tag))
        }
    }

    /// Return the root node as an [`AnyNode`](crate::ast::AnyNode), or `None`
    /// if the parse result has no root (e.g. empty input or fatal parse error).
    ///
    /// When the `serde` feature is enabled, the returned
    /// [`AnyNode`](crate::ast::AnyNode) implements `serde::Serialize` using
    /// the same structure as `dump_node`.
    pub fn root_node(&self) -> Option<crate::ast::AnyNode<'_>> {
        let id = self.root_id();
        if id.is_null() {
            return None;
        }
        Some(crate::ast::AnyNode {
            id,
            stmt_result: self,
        })
    }

    /// Dump an AST node tree as indented text into `out`.
    pub(crate) fn dump_node(&self, id: AnyNodeId, out: &mut String, indent: usize) {
        unsafe extern "C" {
            fn free(ptr: *mut c_void);
        }
        // SAFETY: raw is valid; dump_node returns a malloc'd NUL-terminated string.
        #[expect(clippy::cast_possible_truncation)]
        unsafe {
            let ptr = self.raw.as_ref().dump_node(id.0, indent as u32);
            if !ptr.is_null() {
                out.push_str(&CStr::from_ptr(ptr).to_string_lossy());
                free(ptr.cast::<c_void>());
            }
        }
    }
}

/// Parse result for one statement from a [`TypedParseSession`].
///
/// Main hand-off point to:
///
/// - AST traversal (`root()`).
/// - Token/comment-aware tooling (`tokens()`, `comments()`).
/// - Dialect-agnostic pipelines (`erase()`).
#[derive(Clone, Debug)]
pub struct TypedParsedStatement<'a, G: TypedDialect> {
    pub(crate) any: AnyParsedStatement<'a>,
    _marker: PhantomData<G>,
}

impl<'a, G: TypedDialect> TypedParsedStatement<'a, G> {
    /// Construct from raw parts.
    ///
    /// # Safety
    /// `raw` must be a valid, non-null parser pointer whose bound source
    /// buffer remains valid for `'a`.
    pub(crate) unsafe fn new(raw: *mut CParser, dialect: AnyDialect) -> Self {
        TypedParsedStatement {
            // SAFETY: caller guarantees raw is non-null and its bound
            // source buffer outlives 'a.
            any: unsafe { AnyParsedStatement::new(raw, dialect) },
            _marker: PhantomData,
        }
    }

    /// Convert to the dialect-agnostic [`AnyParsedStatement`] view.
    pub fn erase(self) -> AnyParsedStatement<'a> {
        self.any
    }

    /// Typed AST root for this statement, if available.
    ///
    /// Borrows `self` for `'a` so that returned nodes can hold `&'a AnyParsedStatement<'a>`
    /// without cloning. Drop the returned node to release the borrow.
    pub fn root(&'a self) -> Option<G::Node<'a>> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let id = AnyNodeId(unsafe { self.any.raw.as_ref().result_root() });
        if id.is_null() {
            return None;
        }
        G::Node::from_result(&self.any, id)
    }

    /// Dump the AST as indented text into `out`.
    pub fn dump(&self, out: &mut String, indent: usize) {
        self.any.dump_node(self.any.root_id(), out, indent);
    }

    /// Serialize the AST to a JSON string using the `serde-json` feature.
    ///
    /// The JSON structure mirrors the text dump format: nodes become
    /// `{"type":"NodeName","field":value,...}` and lists become
    /// `{"type":"ListName","count":N,"children":[...]}`.
    ///
    /// # Errors
    /// Returns `Err` if JSON serialization fails.
    #[cfg(feature = "serde-json")]
    pub fn dump_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string(&self.any.root_node())
    }

    /// See [`AnyParsedStatement::text`].
    pub fn text(&self) -> &'a StmtText {
        self.any.text()
    }

    /// See [`AnyParsedStatement::full_text`].
    pub fn full_text(&self) -> &'a DocText {
        self.any.full_text()
    }

    /// See [`AnyParsedStatement::statement_base`].
    pub fn statement_base(&self) -> StatementBase {
        self.any.statement_base()
    }

    /// Post-expansion source — the bound source with every currently-
    /// active macro call replaced by its expansion.  See
    /// [`AnyParsedStatement::expanded_text`] for lifetime semantics.
    pub fn expanded_text(&self) -> &'a str {
        self.any.expanded_text()
    }

    /// Macro rewrites recorded during parsing.
    ///
    /// Each [`MacroRewrite`] describes a macro invocation and its
    /// expansion — enough to reconstruct a source-to-expanded rewrite
    /// tree (see [`MacroRewrite`] for details).  Populated automatically
    /// when the dialect's `macro_style` is set.
    pub fn macro_rewrites(&self) -> impl Iterator<Item = MacroRewrite<'a>> + use<'_, 'a, G> {
        self.any.macro_rewrites()
    }

    /// Token stream for this parse result.  Yields every token fed
    /// to Lemon across all expansion layers; see
    /// [`AnyParsedStatement::tokens`] for the layer semantics.
    ///
    /// Requires `collect_tokens: true` and skips unknown token ordinals for `G`.
    pub fn tokens(&self) -> impl Iterator<Item = TypedParserToken<'a, G>> {
        // SAFETY: self.any.raw is valid for 'a; the returned slices live for 'a.
        let raw: &'a [ffi::CParserToken] = unsafe { self.any.raw.as_ref().result_tokens() };
        raw.iter().filter_map(move |t| {
            let token_type = G::Token::from_token_type(AnyTokenType(t.type_))?;
            Some(build_parser_token(self.any.raw, t, token_type))
        })
    }

    /// Comments attached to this statement.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn comments(&self) -> impl Iterator<Item = Comment<'a>> {
        let source = self.any.text();
        let parser = self.any.raw;
        // SAFETY: self.any.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &'a [ffi::CComment] = unsafe { self.any.raw.as_ref().result_comments() };
        raw.iter().map(move |c| ffi_comment(parser, source, c))
    }

    /// Comments that appear immediately before token `token_idx`, in source
    /// order.  See [`Comment`] for attachment semantics.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn leading_comments(&self, token_idx: TokenIdx) -> impl Iterator<Item = Comment<'a>> {
        let source = self.any.text();
        let parser = self.any.raw;
        // SAFETY: self.any.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &'a [ffi::CComment] =
            unsafe { self.any.raw.as_ref().token_leading_comments(token_idx) };
        raw.iter().map(move |c| ffi_comment(parser, source, c))
    }

    /// Comments that appear on the same source line as token `token_idx`,
    /// after it, in source order.  See [`Comment`] for attachment semantics.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn trailing_comments(&self, token_idx: TokenIdx) -> impl Iterator<Item = Comment<'a>> {
        let source = self.any.text();
        let parser = self.any.raw;
        // SAFETY: self.any.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &'a [ffi::CComment] =
            unsafe { self.any.raw.as_ref().token_trailing_comments(token_idx) };
        raw.iter().map(move |c| ffi_comment(parser, source, c))
    }

    /// Inclusive `[first, last]` token indices covering AST node `id`.
    /// See [`AnyParsedStatement::node_token_range`].
    pub fn node_token_range(&self, id: AnyNodeId) -> Option<(TokenIdx, TokenIdx)> {
        self.any.node_token_range(id)
    }

    /// Comments attached to the first token of AST node `id`.
    /// See [`AnyParsedStatement::node_leading_comments`].
    pub fn node_leading_comments(
        &self,
        id: AnyNodeId,
    ) -> impl Iterator<Item = Comment<'a>> + use<'_, 'a, G> {
        self.any.node_leading_comments(id)
    }

    /// Comments trailing the last token of AST node `id`.
    /// See [`AnyParsedStatement::node_trailing_comments`].
    pub fn node_trailing_comments(
        &self,
        id: AnyNodeId,
    ) -> impl Iterator<Item = Comment<'a>> + use<'_, 'a, G> {
        self.any.node_trailing_comments(id)
    }

    // ── Result accessors (mirror syntaqlite_result_*) ──────────────────────

    /// Human-readable error message, or `None`.
    pub(crate) fn error_msg(&self) -> Option<&str> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        unsafe {
            let ptr = self.any.raw.as_ref().result_error_msg();
            if ptr.is_null() {
                None
            } else {
                Some(CStr::from_ptr(ptr).to_str().unwrap_or("parse error"))
            }
        }
    }

    /// Statement-relative byte offset of the error token, or `None` if unknown.
    pub(crate) fn error_offset(&self) -> Option<StmtOffset> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let v = unsafe { self.any.raw.as_ref().result_error_offset() };
        if v == 0xFFFF_FFFF {
            None
        } else {
            Some(StmtOffset::from_raw(v))
        }
    }

    /// Byte length of the error token, or `None` if unknown.
    pub(crate) fn error_length(&self) -> Option<StmtLen> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let v = unsafe { self.any.raw.as_ref().result_error_length() };
        if v == 0 {
            None
        } else {
            Some(StmtLen::from_raw(v))
        }
    }

    /// Error classification for the current result.
    pub(crate) fn error_kind(&self) -> ParseErrorKind {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let recovery_root = AnyNodeId(unsafe { self.any.raw.as_ref().result_recovery_root() });
        if recovery_root.is_null() {
            ParseErrorKind::Fatal
        } else {
            ParseErrorKind::Recovered
        }
    }

    /// Typed recovery AST root for this statement, if available.
    pub(crate) fn recovery_root(&'a self) -> Option<G::Node<'a>> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let id = AnyNodeId(unsafe { self.any.raw.as_ref().result_recovery_root() });
        if id.is_null() {
            return None;
        }
        G::Node::from_result(&self.any, id)
    }
}

/// Build a public [`Comment`] from an FFI [`ffi::CComment`] borrowing into
/// the layer buffer the comment was tokenized from.
///
/// `c.offset` is interpreted layer-locally (matches `CParserToken.offset`):
/// the authored source for `layer_id == 0`, the expansion buffer for
/// `layer_id > 0`.  The returned `Comment::text` is a slice into the
/// owning layer's buffer.
fn ffi_comment<'a>(raw: NonNull<CParser>, source: &'a StmtText, c: &ffi::CComment) -> Comment<'a> {
    let offset = StmtOffset::from_raw(c.offset);
    let length = StmtLen::from_raw(c.length);
    let text: &'a str = if c.layer_id == 0 {
        &source[StmtRange::from_offset_len(offset, length)]
    } else {
        // SAFETY: raw is valid for 'a; layer_text returns a slice into
        // a parser-owned expansion buffer that lives for 'a.
        let buf: &'a str = unsafe { raw.as_ref().layer_text(u32::from(c.layer_id)) };
        let start = c.offset as usize;
        let end = start.saturating_add(c.length as usize);
        buf.get(start..end).unwrap_or("")
    };
    let kind = match c.kind {
        ffi::CCommentKind::LineComment => CommentKind::Line,
        ffi::CCommentKind::BlockComment => CommentKind::Block,
    };
    let side = match c.side {
        ffi::CCommentSide::Leading => CommentSide::Leading,
        ffi::CCommentSide::Trailing => CommentSide::Trailing,
    };
    Comment::new(
        text,
        kind,
        offset,
        length,
        TokenIdx::from_raw(c.token_idx),
        side,
        c.layer_id,
    )
}

/// Build a [`TypedParserToken`] from a raw FFI token entry, resolving
/// its byte text against the owning layer's buffer and drilling up to
/// the authored-source range via `span_text`.  Shared between the
/// [`AnyParsedStatement`] and [`TypedParsedStatement`] token iterators.
// `CParserToken._layer_id` and `TextSpan._layer_id` mirror the C ABI
// (where the underscore signals "internal — use span APIs to resolve"),
// so Rust reads of the field need the allow.
#[expect(clippy::used_underscore_binding)]
fn build_parser_token<'a, G: TypedDialect>(
    raw: NonNull<CParser>,
    t: &ffi::CParserToken,
    token_type: G::Token,
) -> TypedParserToken<'a, G> {
    // SAFETY: raw is valid for 'a.  layer_text and span_text each
    // borrow from parser-owned buffers that remain valid for 'a.
    let buffer: &'a str = unsafe { raw.as_ref().layer_text(t._layer_id) };
    let start = t.offset as usize;
    let end = start.saturating_add(t.length as usize);
    let text = buffer.get(start..end).unwrap_or("");
    let layer_id = u8::try_from(t._layer_id).unwrap_or(u8::MAX);

    let stmt_range = if t._layer_id == 0 {
        StmtRange::from_offset_len(StmtOffset::from_raw(t.offset), StmtLen::from_raw(t.length))
    } else {
        // Drill up through the expansion-layer chain to the authored
        // source range (same rule `span_text` uses for AST spans).
        let span = crate::ast::TextSpan {
            offset: t.offset,
            length: t.length,
            flags: 0,
            _layer_id: t._layer_id,
        };
        let mut out_len: u32 = 0;
        let mut out_offset: u32 = 0;
        // SAFETY: raw is valid; span mirrors the token's layer
        // position; the out pointers are live stack slots.
        unsafe {
            raw.as_ref()
                .span_text(span, &raw mut out_len, &raw mut out_offset);
        }
        StmtRange::from_offset_len(StmtOffset::from_raw(out_offset), StmtLen::from_raw(out_len))
    };

    TypedParserToken::new(
        text,
        token_type,
        ParserTokenFlags::from_raw(t.flags),
        LayerOffset::from_raw(t.offset),
        LayerLen::from_raw(t.length),
        layer_id,
        stmt_range,
    )
}

/// Extract a single [`crate::ast::FieldValue`] from a raw arena node pointer.
///
/// Just copies raw bytes into the corresponding variant — callers pull
/// text / offset / expanded-buffer views out via the accessor methods on
/// [`AnyParsedStatement`] (e.g. [`span_text`](AnyParsedStatement::span_text)).
///
/// # Safety
/// `ptr` must point to a valid arena node struct whose field at `meta.offset()`
/// has the type indicated by `meta.kind()`.
#[expect(clippy::cast_ptr_alignment)]
unsafe fn extract_field_value(
    ptr: *const u8,
    meta: &crate::dialect::FieldMeta<'_>,
) -> crate::ast::FieldValue {
    use crate::ast::{FieldValue, TextSpan};
    use crate::dialect::FieldKind;
    // SAFETY: covered by function-level contract; ptr and meta are consistent.
    unsafe {
        let field_ptr = ptr.add(meta.offset() as usize);
        match meta.kind() {
            FieldKind::NodeId => FieldValue::NodeId(AnyNodeId(*(field_ptr.cast::<u32>()))),
            FieldKind::Span => {
                // TextSpan is `#[repr(C)]` Copy with 1-byte alignment;
                // use read_unaligned to avoid alignment assumptions.
                let span = field_ptr.cast::<TextSpan>().read_unaligned();
                FieldValue::Span(span)
            }
            FieldKind::Bool => FieldValue::Bool(*(field_ptr.cast::<u32>()) != 0),
            FieldKind::Flags => FieldValue::Flags(*field_ptr),
            FieldKind::Enum => FieldValue::Enum(*(field_ptr.cast::<u32>())),
        }
    }
}

/// Parse failure for a single statement in dialect `G`.
///
/// Designed for diagnostics:
///
/// - Message text (`message()`).
/// - Optional source location (`offset()`, `length()`).
/// - Severity/recovery status (`kind()`).
/// - Optional recovery tree (`recovery_root()`).
///
/// Recovery model:
///
/// - `Recovered`: this statement is invalid, but the parser skipped ahead
///   (usually to the next `;`) so it can continue with later statements.
/// - The returned `recovery_root()` can still be useful for diagnostics, but may
///   contain error placeholders where input was skipped.
/// - `Fatal`: the parser could not find a safe point to continue from.
pub struct TypedParseError<'a, G: TypedDialect>(TypedParsedStatement<'a, G>);

impl<'a, G: TypedDialect> TypedParseError<'a, G> {
    pub(crate) fn new(result: TypedParsedStatement<'a, G>) -> Self {
        TypedParseError(result)
    }

    /// Whether parsing recovered to a statement boundary.
    pub fn kind(&self) -> ParseErrorKind {
        self.0.error_kind()
    }

    /// True if this error was recovered and yielded a partial tree.
    pub fn is_recovered(&self) -> bool {
        self.kind() == ParseErrorKind::Recovered
    }

    /// True if this error is fatal (unrecoverable).
    pub fn is_fatal(&self) -> bool {
        self.kind() == ParseErrorKind::Fatal
    }

    /// Human-readable diagnostic text.
    pub fn message(&self) -> &str {
        self.0.error_msg().unwrap_or("parse error")
    }
    /// Statement-relative byte offset of the error token, or `None` if unknown.
    pub fn offset(&self) -> Option<StmtOffset> {
        self.0.error_offset()
    }
    /// Byte length of the error token, or `None` if unknown.
    pub fn length(&self) -> Option<StmtLen> {
        self.0.error_length()
    }
    /// The partial recovery tree, if error recovery produced one.
    pub fn recovery_root(&'a self) -> Option<G::Node<'a>> {
        self.0.recovery_root()
    }

    /// See [`AnyParsedStatement::statement_base`].
    pub fn statement_base(&self) -> StatementBase {
        self.0.any.statement_base()
    }

    /// The source text bound to this result.
    pub fn text(&self) -> &'a StmtText {
        self.0.text()
    }

    /// Tokens collected during the (partial) parse, if `collect_tokens` was enabled.
    pub fn tokens(&self) -> impl Iterator<Item = TypedParserToken<'a, G>> {
        self.0.tokens()
    }

    /// Comments collected during the (partial) parse, if `collect_tokens` was enabled.
    pub fn comments(&self) -> impl Iterator<Item = Comment<'a>> {
        self.0.comments()
    }
}

impl<G: TypedDialect> std::fmt::Debug for TypedParseError<'_, G> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("TypedParseError")
            .field("kind", &self.kind())
            .field("message", &self.message())
            .field("offset", &self.offset())
            .field("length", &self.length())
            .finish()
    }
}

impl<G: TypedDialect> std::fmt::Display for TypedParseError<'_, G> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.message())
    }
}

impl<G: TypedDialect> std::error::Error for TypedParseError<'_, G> {}

/// Parse-error alias for dialect-independent pipelines.
pub type AnyParseError<'a> = TypedParseError<'a, AnyDialect>;

// ── Crate-internal ───────────────────────────────────────────────────────────

/// Holds the C parser handle and mutable state. Checked out by sessions at
/// runtime and returned on [`Drop`].
pub(crate) struct ParserInner {
    pub(crate) raw: NonNull<CParser>,
    pub(crate) source_buf: Vec<u8>,
    /// Raw pointer to the boxed macro handler closure, if installed.
    /// Freed on drop.
    pub(crate) macro_handler: Option<*mut c_void>,
}

impl Drop for ParserInner {
    fn drop(&mut self) {
        // Free the macro lookup handler if set.
        if let Some(ptr) = self.macro_handler.take() {
            // SAFETY: ptr was created by Box::into_raw in set_macro_lookup.
            let _: Box<MacroLookupState> = unsafe { Box::from_raw(ptr.cast()) };
        }
        // SAFETY: self.raw was allocated by CParser::create and has not been
        // freed (Drop runs exactly once).
        unsafe { CParser::destroy(self.raw.as_ptr()) }
    }
}

/// Copy source into `source_buf` (with null terminator) and reset the C parser.
///
/// # Safety
/// `raw` must be a valid parser pointer owned by the caller.
pub(crate) unsafe fn reset_parser(raw: *mut CParser, source_buf: &mut Vec<u8>, source: &str) {
    source_buf.clear();
    source_buf.reserve(source.len() + 1);
    source_buf.extend_from_slice(source.as_bytes());
    source_buf.push(0);

    // source_buf has at least one byte (the null terminator just pushed).
    let c_text_ptr = source_buf.as_ptr();
    // SAFETY: raw is valid (caller owns it); c_text_ptr points to
    // source_buf which is null-terminated.
    #[expect(clippy::cast_possible_truncation)]
    unsafe {
        (*raw).reset(c_text_ptr.cast(), source.len() as u32);
    }
}
