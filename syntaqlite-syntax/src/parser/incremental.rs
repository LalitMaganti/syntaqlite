// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::cell::RefCell;
use std::marker::PhantomData;
use std::ops::Range;
use std::ptr::NonNull;
use std::rc::Rc;

use crate::ast::GrammarTokenType;
use crate::grammar::{AnyGrammar, TypedGrammar};

use super::{
    AnyParsedStatement, CParser, CompletionContext, ParserInner, TypedParseError,
    TypedParsedStatement, ffi,
};
#[cfg(feature = "sqlite")]
use super::{ParseError, ParsedStatement};

/// Incremental parser state machine for grammar `G`.
///
/// Use this for interactive/editor workflows where input arrives token by
/// token and you need expected-token or completion-context feedback.
///
/// Obtained from [`super::TypedParser::incremental_parse`].
pub struct TypedIncrementalParseSession<G: TypedGrammar> {
    /// Base pointer into the internal source buffer. `feed_token` uses this
    /// to compute the C-side token pointer from byte-offset spans.
    c_source_ptr: NonNull<u8>,
    grammar: AnyGrammar,
    /// Checked-out parser state. Returned to `slot` on drop.
    inner: Option<ParserInner>,
    /// Slot to return `inner` to when this session is dropped.
    slot: Rc<RefCell<Option<ParserInner>>>,
    finished: bool,
    _marker: PhantomData<G>,
}

impl<G: TypedGrammar> Drop for TypedIncrementalParseSession<G> {
    fn drop(&mut self) {
        if let Some(inner) = self.inner.take() {
            *self.slot.borrow_mut() = Some(inner);
        }
    }
}

impl<G: TypedGrammar> TypedIncrementalParseSession<G> {
    pub(crate) fn new(
        c_source_ptr: NonNull<u8>,
        grammar: AnyGrammar,
        inner: ParserInner,
        slot: Rc<RefCell<Option<ParserInner>>>,
    ) -> Self {
        TypedIncrementalParseSession {
            c_source_ptr,
            grammar,
            inner: Some(inner),
            slot,
            finished: false,
            _marker: PhantomData,
        }
    }

    fn assert_not_finished(&self) {
        assert!(
            !self.finished,
            "TypedIncrementalParseSession used after finish()"
        );
    }

    fn raw_ptr(&self) -> *mut CParser {
        self.inner
            .as_ref()
            .expect("inner taken after finish()")
            .raw
            .as_ptr()
    }

    fn typed_stmt_result(&self) -> TypedParsedStatement<'_, G> {
        let inner = self.inner.as_ref().expect("inner taken after finish()");
        let source_len = inner.source_buf.len().saturating_sub(1);
        // SAFETY: source_buf was populated from valid UTF-8 (&str) in
        // reset_parser. The first source_len bytes are the original source.
        let source = unsafe { std::str::from_utf8_unchecked(&inner.source_buf[..source_len]) };
        // SAFETY: inner.raw is valid (owned via ParserInner, not yet destroyed).
        unsafe { TypedParsedStatement::new(inner.raw.as_ptr(), source, self.grammar.clone()) }
    }

    fn result_from_rc(
        &self,
        rc: i32,
    ) -> Option<Result<TypedParsedStatement<'_, G>, TypedParseError<'_, G>>> {
        if rc == 0 {
            return None;
        }
        let result = self.typed_stmt_result();
        if rc == 1 {
            Some(Ok(result))
        } else {
            Some(Err(TypedParseError::new(result)))
        }
    }

    /// Feed one token from the bound source into the parser.
    ///
    /// Whitespace/comments are handled automatically; callers can focus on
    /// meaningful tokens and source spans.
    ///
    /// Returns:
    /// - `None` — keep going, statement not yet complete.
    /// - `Some(Ok(result))` — statement parsed cleanly; use
    ///   [`TypedParsedStatement::root`] to access the typed AST.
    /// - `Some(Err(err))` — parse error; `err.recovery_root()` may contain a partial
    ///   recovery tree.
    ///
    /// `span` is a byte range into the source text bound by this session.
    /// `token_type` is the grammar's typed token enum.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::typed::{grammar, TypedParser};
    /// use syntaqlite_syntax::TokenType;
    ///
    /// let parser = TypedParser::new(grammar());
    /// let mut session = parser.incremental_parse("SELECT 1");
    ///
    /// assert!(session.feed_token(TokenType::Select, 0..6).is_none());
    /// assert!(session.feed_token(TokenType::Integer, 7..8).is_none());
    /// assert!(session.finish().is_some());
    /// ```
    pub fn feed_token(
        &mut self,
        token_type: G::Token,
        span: Range<usize>,
    ) -> Option<Result<TypedParsedStatement<'_, G>, TypedParseError<'_, G>>> {
        self.assert_not_finished();
        // SAFETY: c_source_ptr is valid for the source length; raw is valid.
        let rc = unsafe {
            let c_text = self.c_source_ptr.as_ptr().add(span.start);
            let raw_token_type: u32 = token_type.into();
            #[expect(clippy::cast_possible_truncation)]
            (*self.raw_ptr()).feed_token(raw_token_type, c_text as *const _, span.len() as u32)
        };
        self.result_from_rc(rc)
    }

    /// Finalize parsing for the current input and flush any pending statement.
    ///
    /// Returns:
    /// - `None` — nothing was pending (empty input or bare semicolons only).
    /// - `Some(Ok(result))` — final statement parsed cleanly.
    /// - `Some(Err(err))` — parse error; `err.recovery_root()` may contain a partial
    ///   recovery tree.
    ///
    /// No further methods may be called after `finish()`.
    pub fn finish(
        &mut self,
    ) -> Option<Result<TypedParsedStatement<'_, G>, TypedParseError<'_, G>>> {
        self.assert_not_finished();
        self.finished = true;
        // SAFETY: raw is valid.
        let rc = unsafe { (*self.raw_ptr()).finish() };
        self.result_from_rc(rc)
    }

    /// Return token types that are currently valid next inputs.
    ///
    /// Useful for completion engines after feeding known prefix tokens.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::typed::{grammar, TypedParser};
    /// use syntaqlite_syntax::TokenType;
    ///
    /// let parser = TypedParser::new(grammar());
    /// let mut session = parser.incremental_parse("SELECT x FROM t");
    /// let _ = session.feed_token(TokenType::Select, 0..6);
    ///
    /// let expected: Vec<_> = session.expected_tokens().collect();
    /// assert!(!expected.is_empty());
    /// ```
    pub fn expected_tokens(&self) -> impl Iterator<Item = <G as TypedGrammar>::Token> {
        self.assert_not_finished();
        let raw = self.raw_ptr();
        let mut stack_buf = [0u32; 256];
        // SAFETY: raw is valid and exclusively borrowed via &self; stack_buf is
        // a valid output buffer.
        #[expect(clippy::cast_possible_truncation)]
        let total =
            unsafe { (*raw).expected_tokens(stack_buf.as_mut_ptr(), stack_buf.len() as u32) };
        let raw_tokens: Vec<u32> = if total == 0 {
            Vec::new()
        } else {
            let count = total as usize;
            if count <= stack_buf.len() {
                stack_buf[..count].to_vec()
            } else {
                let mut heap_buf = vec![0u32; count];
                // SAFETY: raw is valid; heap_buf is sized to hold `total` entries.
                let written = unsafe { (*raw).expected_tokens(heap_buf.as_mut_ptr(), total) };
                let len = written.clamp(0, total) as usize;
                heap_buf.truncate(len);
                heap_buf
            }
        };
        raw_tokens
            .into_iter()
            .map(crate::any::AnyTokenType)
            .filter_map(<G as TypedGrammar>::Token::from_token_type)
    }

    /// Return the semantic completion context for the current parser state.
    pub fn completion_context(&self) -> CompletionContext {
        self.assert_not_finished();
        // SAFETY: raw is valid and exclusively borrowed via &self.
        unsafe { (*self.raw_ptr()).completion_context() }
    }

    /// Return how many arena nodes have been built so far.
    pub fn node_count(&self) -> u32 {
        // SAFETY: raw is valid and exclusively borrowed via &self.
        unsafe { (*self.raw_ptr()).node_count() }
    }

    /// Mark subsequent fed tokens as originating from a macro expansion.
    ///
    /// `span` describes the macro call's byte range in the original source.
    /// Calls may nest (for nested macro expansions).
    ///
    /// # Panics
    ///
    /// Panics if `span.start` or `span.len()` does not fit in `u32`.
    pub fn begin_macro(&mut self, span: Range<usize>) {
        self.assert_not_finished();
        let call_offset = u32::try_from(span.start).expect("macro span start exceeds u32");
        let call_length = u32::try_from(span.len()).expect("macro span length exceeds u32");
        // SAFETY: raw is valid and exclusively borrowed via &mut self.
        unsafe { (*self.raw_ptr()).begin_macro(call_offset, call_length) }
    }

    /// End the innermost macro expansion region.
    pub fn end_macro(&mut self) {
        self.assert_not_finished();
        // SAFETY: raw is valid and exclusively borrowed via &mut self.
        unsafe { (*self.raw_ptr()).end_macro() }
    }

    pub(crate) fn stmt_result(&self) -> AnyParsedStatement<'_> {
        self.typed_stmt_result().erase()
    }

    pub(crate) fn comments(&self) -> &[ffi::CComment] {
        // SAFETY: raw is valid (owned via ParserInner, valid for &self).
        unsafe { (*self.raw_ptr()).result_comments() }
    }

    pub(crate) fn tokens(&self) -> &[ffi::CParserToken] {
        // SAFETY: raw is valid (owned via ParserInner, valid for &self).
        unsafe { (*self.raw_ptr()).result_tokens() }
    }

    pub(crate) fn macro_regions(&self) -> &[ffi::CMacroRegion] {
        // SAFETY: raw is valid (owned via ParserInner, valid for &self).
        unsafe { (*self.raw_ptr()).result_macros() }
    }
}

/// Type-erased incremental parser for runtime-selected grammars.
pub type AnyIncrementalParseSession = TypedIncrementalParseSession<AnyGrammar>;

/// Incremental parsing API for the built-in `SQLite` grammar.
///
/// Produced by [`super::Parser::incremental_parse`].
///
/// Feed tokens one at a time via [`feed_token`](Self::feed_token) and signal
/// end of input with [`finish`](Self::finish).
///
/// Ideal for editor-like flows that parse as the user types.
#[cfg(feature = "sqlite")]
pub struct IncrementalParseSession(TypedIncrementalParseSession<crate::sqlite::grammar::Grammar>);

#[cfg(feature = "sqlite")]
impl IncrementalParseSession {
    /// Feed one source token into the parser.
    ///
    /// Returns:
    /// - `None` — keep going, statement not yet complete.
    /// - `Some(Ok(result))` — statement parsed cleanly.
    /// - `Some(Err(e))` — parse error; `e.recovery_root()` may contain a partial
    ///   recovery tree.
    ///
    /// - `span` is a byte range into the source text bound by this session.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::{Parser, TokenType};
    ///
    /// let parser = Parser::new();
    /// let mut session = parser.incremental_parse("SELECT 1");
    ///
    /// assert!(session.feed_token(TokenType::Select, 0..6).is_none());
    /// assert!(session.feed_token(TokenType::Integer, 7..8).is_none());
    /// ```
    pub fn feed_token(
        &mut self,
        token_type: crate::sqlite::tokens::TokenType,
        span: Range<usize>,
    ) -> Option<Result<ParsedStatement<'_>, ParseError<'_>>> {
        Some(match self.0.feed_token(token_type, span)? {
            Ok(result) => Ok(ParsedStatement(result)),
            Err(err) => Err(ParseError(err)),
        })
    }

    /// Finalize parsing for the current input.
    ///
    /// Returns:
    /// - `None` — nothing was pending.
    /// - `Some(Ok(result))` — final statement parsed cleanly.
    /// - `Some(Err(e))` — parse error; `e.recovery_root()` may contain a partial
    ///   recovery tree.
    ///
    /// No further methods may be called after `finish()`.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::{Parser, TokenType};
    ///
    /// let parser = Parser::new();
    /// let mut session = parser.incremental_parse("SELECT 1");
    /// let _ = session.feed_token(TokenType::Select, 0..6);
    /// let _ = session.feed_token(TokenType::Integer, 7..8);
    ///
    /// let stmt = session.finish().and_then(Result::ok).unwrap();
    /// let _ = stmt.root();
    /// ```
    pub fn finish(&mut self) -> Option<Result<ParsedStatement<'_>, ParseError<'_>>> {
        Some(match self.0.finish()? {
            Ok(result) => Ok(ParsedStatement(result)),
            Err(err) => Err(ParseError(err)),
        })
    }

    /// Return token types that are currently valid lookaheads.
    pub fn expected_tokens(&self) -> impl Iterator<Item = crate::sqlite::tokens::TokenType> {
        self.0.expected_tokens()
    }

    /// Return the semantic completion context for the current parser state.
    pub fn completion_context(&self) -> CompletionContext {
        self.0.completion_context()
    }

    /// Return how many arena nodes have been built so far.
    pub fn node_count(&self) -> u32 {
        self.0.node_count()
    }

    /// Mark subsequent fed tokens as originating from a macro expansion.
    pub fn begin_macro(&mut self, span: Range<usize>) {
        self.0.begin_macro(span);
    }

    /// End the innermost macro expansion region.
    pub fn end_macro(&mut self) {
        self.0.end_macro();
    }

    #[expect(dead_code)]
    pub(crate) fn stmt_result(&self) -> AnyParsedStatement<'_> {
        self.0.stmt_result()
    }

    #[expect(dead_code)]
    pub(crate) fn comments(&self) -> &[ffi::CComment] {
        self.0.comments()
    }

    #[expect(dead_code)]
    pub(crate) fn tokens(&self) -> &[ffi::CParserToken] {
        self.0.tokens()
    }

    #[expect(dead_code)]
    pub(crate) fn macro_regions(&self) -> &[ffi::CMacroRegion] {
        self.0.macro_regions()
    }
}

#[cfg(feature = "sqlite")]
impl From<TypedIncrementalParseSession<crate::sqlite::grammar::Grammar>>
    for IncrementalParseSession
{
    fn from(inner: TypedIncrementalParseSession<crate::sqlite::grammar::Grammar>) -> Self {
        IncrementalParseSession(inner)
    }
}
