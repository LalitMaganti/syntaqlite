// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::cell::RefCell;
use std::ffi::CStr;
use std::marker::PhantomData;
use std::ptr::NonNull;
use std::rc::Rc;

use crate::any::{AnyNodeTag, AnyTokenType};
use crate::ast::{AnyNodeId, ArenaNode, GrammarNodeType, GrammarTokenType, RawNodeList};
use crate::grammar::{AnyGrammar, TypedGrammar};

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
    AnyParserToken, Comment, CommentKind, CommentSpan, CompletionContext, MacroRegion,
    ParseOutcome, ParserTokenFlags, TypedParserToken,
};

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

/// Parser API parameterized by grammar type `G`.
///
/// Primarily for library/framework code over generated grammars.
///
/// - Use this when grammar type is known at compile time.
/// - Use top-level [`Parser`] for typical `SQLite` SQL app code.
pub struct TypedParser<G: TypedGrammar> {
    inner: Rc<RefCell<Option<ParserInner>>>,
    grammar: AnyGrammar,
    _marker: PhantomData<G>,
}

impl<G: TypedGrammar> TypedParser<G> {
    /// Create a parser for grammar `G` with default [`ParserConfig`].
    pub fn new(grammar: G) -> Self {
        Self::with_config(grammar, &ParserConfig::default())
    }

    /// Create a parser for grammar `G` with custom [`ParserConfig`].
    ///
    /// # Panics
    /// Panics if parser allocation fails (out of memory).
    pub fn with_config(grammar: G, config: &ParserConfig) -> Self {
        let grammar_raw: AnyGrammar = grammar.into();
        // SAFETY: create(NULL, grammar_raw.inner) allocates a new parser with
        // default malloc/free. The C side copies the grammar.
        let mut raw = NonNull::new(unsafe { CParser::create(std::ptr::null(), grammar_raw.inner) })
            .expect("parser allocation failed");

        // SAFETY: raw is freshly created (not sealed), so these calls always return 0.
        unsafe {
            raw.as_mut().set_trace(u32::from(config.trace()));
            raw.as_mut()
                .set_collect_tokens(u32::from(config.collect_tokens()));
            raw.as_mut()
                .set_macro_fallback(u32::from(config.macro_fallback()));
        }

        TypedParser {
            inner: Rc::new(RefCell::new(Some(ParserInner {
                raw,
                source_buf: Vec::new(),
            }))),
            grammar: grammar_raw,
            _marker: PhantomData,
        }
    }

    /// Parse a SQL script and return a typed statement session.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::typed::{grammar, TypedParser};
    /// use syntaqlite_syntax::ParseOutcome;
    ///
    /// let parser = TypedParser::new(grammar());
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
            grammar: self.grammar.clone(),
            inner: Some(inner),
            slot: Rc::clone(&self.inner),
            _marker: PhantomData,
        }
    }

    /// Start incremental parsing for grammar `G`.
    ///
    /// Use this when tokens arrive over time (editor completion, interactive
    /// parsing, macro-expansion pipelines).
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
    /// let _ = session.feed_token(TokenType::Select, 0..6);
    /// let _ = session.feed_token(TokenType::Integer, 7..8);
    /// let _ = session.finish();
    /// ```
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    /// Drop the previous session before starting a new one.
    /// Register a template macro with the parser.
    ///
    /// The macro `name` will be expanded when `name!(args)` is encountered
    /// during batch parsing (`parse()`). The `body` uses `$param` placeholders
    /// that are substituted with the corresponding arguments.
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    pub fn register_macro(&mut self, name: &str, params: &[&str], body: &str) {
        let mut inner_ref = self.inner.borrow_mut();
        let inner = inner_ref
            .as_mut()
            .expect("register_macro called while a session is still active");
        // The C side uses strlen() on param names, so they must be NUL-terminated.
        let param_cstrings: Vec<std::ffi::CString> = params
            .iter()
            .map(|p| std::ffi::CString::new(*p).expect("param name must not contain NUL"))
            .collect();
        let param_ptrs: Vec<*const std::ffi::c_char> =
            param_cstrings.iter().map(|c| c.as_ptr()).collect();
        // SAFETY: inner.raw is valid; all string pointers are valid for the
        // duration of the C call (which copies them).
        #[expect(clippy::cast_possible_truncation)]
        unsafe {
            inner.raw.as_mut().register_macro(
                name.as_ptr().cast(),
                name.len() as u32,
                param_ptrs.as_ptr(),
                params.len() as u32,
                body.as_ptr().cast(),
                body.len() as u32,
            );
        }
    }

    /// Deregister a macro by name.
    ///
    /// Returns `true` if the macro was found and removed.
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    pub fn deregister_macro(&mut self, name: &str) -> bool {
        let mut inner_ref = self.inner.borrow_mut();
        let inner = inner_ref
            .as_mut()
            .expect("deregister_macro called while a session is still active");
        #[expect(clippy::cast_possible_truncation)]
        // SAFETY: inner.raw is valid; name pointer is valid for the C call duration.
        let rc = unsafe {
            inner
                .raw
                .as_mut()
                .deregister_macro(name.as_ptr().cast(), name.len() as u32)
        };
        rc == 0
    }

    /// Start incremental parsing for grammar `G`.
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
        let c_source_ptr =
            NonNull::new(inner.source_buf.as_mut_ptr()).expect("source_buf is non-empty");
        TypedIncrementalParseSession::new(
            c_source_ptr,
            self.grammar.clone(),
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
pub struct TypedParseSession<G: TypedGrammar> {
    grammar: AnyGrammar,
    /// Checked-out parser state. Returned to `slot` on drop.
    inner: Option<ParserInner>,
    /// Slot to return `inner` to when this session is dropped.
    slot: Rc<RefCell<Option<ParserInner>>>,
    _marker: PhantomData<G>,
}

impl<G: TypedGrammar> Drop for TypedParseSession<G> {
    fn drop(&mut self) {
        if let Some(inner) = self.inner.take() {
            *self.slot.borrow_mut() = Some(inner);
        }
    }
}

impl<G: TypedGrammar> TypedParseSession<G> {
    /// Register a template macro with the parser during an active session.
    ///
    /// This is used by the formatter to auto-register macros defined by
    /// `CREATE PERFETTO MACRO` statements so that subsequent macro calls
    /// are expanded correctly.
    ///
    /// # Panics
    ///
    /// Panics if the session has already finished.
    pub fn register_macro(&mut self, name: &str, params: &[&str], body: &str) {
        let inner = self
            .inner
            .as_mut()
            .expect("register_macro called on finished session");
        let param_cstrings: Vec<std::ffi::CString> = params
            .iter()
            .map(|p| std::ffi::CString::new(*p).expect("param name must not contain NUL"))
            .collect();
        let param_ptrs: Vec<*const std::ffi::c_char> =
            param_cstrings.iter().map(|c| c.as_ptr()).collect();
        // SAFETY: inner.raw is valid; all string pointers are valid for the
        // duration of the C call (which copies them).
        #[expect(clippy::cast_possible_truncation)]
        unsafe {
            inner.raw.as_mut().register_macro(
                name.as_ptr().cast(),
                name.len() as u32,
                param_ptrs.as_ptr(),
                params.len() as u32,
                body.as_ptr().cast(),
                body.len() as u32,
            );
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
        let source_len = inner.source_buf.len().saturating_sub(1);
        // SAFETY: source_buf was populated from valid UTF-8 (&str) in
        // reset_parser. The first source_len bytes are the original source.
        let source = unsafe { std::str::from_utf8_unchecked(&inner.source_buf[..source_len]) };
        // SAFETY: inner.raw is valid (owned via ParserInner, not yet destroyed).
        let result =
            unsafe { TypedParsedStatement::new(inner.raw.as_ptr(), source, self.grammar.clone()) };
        if rc == ffi::PARSE_OK {
            ParseOutcome::Ok(result)
        } else {
            // ERROR (may still carry a recovery tree)
            ParseOutcome::Err(TypedParseError(result))
        }
    }

    /// Original SQL source bound to this session.
    ///
    /// # Panics
    ///
    /// Panics only if session invariants were violated.
    pub fn source(&self) -> &str {
        let inner = self
            .inner
            .as_ref()
            .expect("inner is Some while session is not finished");
        let source_len = inner.source_buf.len().saturating_sub(1);
        // SAFETY: source_buf was populated from valid UTF-8 (&str) in
        // reset_parser.
        unsafe { std::str::from_utf8_unchecked(&inner.source_buf[..source_len]) }
    }

    /// Get a grammar-agnostic view of this session's current arena state.
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
        let source_len = inner.source_buf.len().saturating_sub(1);
        // SAFETY: source_buf was populated from valid UTF-8 (&str) in
        // reset_parser; inner.raw is valid (owned via ParserInner).
        let source = unsafe { std::str::from_utf8_unchecked(&inner.source_buf[..source_len]) };
        // SAFETY: inner.raw is valid for 'self; source is valid UTF-8 for 'self.
        unsafe { AnyParsedStatement::new(inner.raw.as_ptr(), source, self.grammar.clone()) }
    }
}

/// Parser alias for grammar-independent code that picks grammar at runtime.
pub type AnyParser = TypedParser<AnyGrammar>;

/// Session alias paired with [`AnyParser`].
pub type AnyParseSession = TypedParseSession<AnyGrammar>;

/// Grammar-erased view of a parsed statement.
///
/// Cheap to borrow — holds a raw parser pointer, source reference, and grammar
/// handle. Nodes and lists store `&'a AnyParsedStatement<'a>` rather than an
/// owned copy, making them `Copy` and eliminating grammar-handle clones.
#[derive(Clone)]
pub struct AnyParsedStatement<'a> {
    pub(crate) raw: NonNull<CParser>,
    pub(crate) source: &'a str,
    pub(crate) grammar: AnyGrammar,
}

impl<'a> AnyParsedStatement<'a> {
    /// Construct from raw parts.
    ///
    /// # Safety
    /// `raw` must be a valid, non-null parser pointer that remains valid for `'a`.
    pub(crate) unsafe fn new(raw: *mut CParser, source: &'a str, grammar: AnyGrammar) -> Self {
        AnyParsedStatement {
            // SAFETY: caller guarantees raw is non-null.
            raw: unsafe { NonNull::new_unchecked(raw) },
            source,
            grammar,
        }
    }

    /// Root node ID for the current statement (`AnyNodeId::NULL` if absent).
    pub fn root_id(&self) -> AnyNodeId {
        // SAFETY: self.raw is a valid, non-null parser pointer for lifetime 'a.
        AnyNodeId(unsafe { self.raw.as_ref().result_root() })
    }

    /// Macro expansion call-site spans recorded during parsing.
    pub fn macro_regions(&self) -> impl Iterator<Item = MacroRegion> + use<'_> {
        // SAFETY: self.raw is valid for 'a; the slice lives for the parser lifetime.
        let raw: &[ffi::CMacroRegion] = unsafe { self.raw.as_ref().result_macros() };
        raw.iter().map(|r| MacroRegion {
            call_offset: r.call_offset,
            call_length: r.call_length,
        })
    }

    /// The source text bound to this result.
    pub fn source(&self) -> &'a str {
        self.source
    }

    /// Raw token spans `(offset, length)` for all collected tokens.
    ///
    /// Returns an empty iterator if `collect_tokens` was not enabled.
    /// Always non-empty when the result comes from [`TypedParser::incremental_parse`],
    /// which unconditionally enables token collection.
    pub fn token_spans(&self) -> impl Iterator<Item = (u32, u32)> + use<'_> {
        // SAFETY: self.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &[ffi::CParserToken] = unsafe { self.raw.as_ref().result_tokens() };
        raw.iter().map(|t| (t.offset, t.length))
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
            CommentSpan::new(c.offset, c.length, kind)
        })
    }

    /// Extract reflective node data (`tag` + field values) for `id`.
    pub fn extract_fields(
        &self,
        id: AnyNodeId,
    ) -> Option<(AnyNodeTag, crate::ast::NodeFields<'a>)> {
        let (ptr, tag) = self.node_ptr(id)?;
        let mut fields = crate::ast::NodeFields::new();
        for meta in self.grammar.field_meta(tag) {
            // SAFETY: ptr is a valid arena node pointer valid for 'a;
            // meta describes a field within that node's struct layout.
            let val = unsafe { extract_field_value(ptr, &meta, self.source) };
            fields.push(val);
        }
        Some((tag, fields))
    }

    /// Return child node IDs if `id` is a list node.
    pub fn list_children(&self, id: AnyNodeId) -> Option<&'a [AnyNodeId]> {
        let (_, tag) = self.node_ptr(id)?;
        if !self.grammar.is_list(tag) {
            return None;
        }
        #[expect(clippy::redundant_closure_for_method_calls)]
        self.resolve_list(id).map(|l| l.children())
    }

    /// Iterate direct child node IDs for the node at `id`.
    pub fn child_node_ids(&self, id: AnyNodeId) -> impl Iterator<Item = AnyNodeId> + '_ {
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
            fn free(ptr: *mut std::ffi::c_void);
        }
        // SAFETY: raw is valid; dump_node returns a malloc'd NUL-terminated string.
        #[expect(clippy::cast_possible_truncation)]
        unsafe {
            let ptr = self.raw.as_ref().dump_node(id.0, indent as u32);
            if !ptr.is_null() {
                out.push_str(&CStr::from_ptr(ptr).to_string_lossy());
                free(ptr.cast::<std::ffi::c_void>());
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
/// - Grammar-agnostic pipelines (`erase()`).
#[derive(Clone)]
pub struct TypedParsedStatement<'a, G: TypedGrammar> {
    pub(crate) any: AnyParsedStatement<'a>,
    _marker: PhantomData<G>,
}

impl<'a, G: TypedGrammar> TypedParsedStatement<'a, G> {
    /// Construct from raw parts.
    ///
    /// # Safety
    /// `raw` must be a valid, non-null parser pointer that remains valid for `'a`.
    pub(crate) unsafe fn new(raw: *mut CParser, source: &'a str, grammar: AnyGrammar) -> Self {
        TypedParsedStatement {
            any: AnyParsedStatement {
                // SAFETY: caller guarantees raw is non-null.
                raw: unsafe { NonNull::new_unchecked(raw) },
                source,
                grammar,
            },
            _marker: PhantomData,
        }
    }

    /// Convert to the grammar-agnostic [`AnyParsedStatement`] view.
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

    /// The source text bound to this result.
    pub fn source(&self) -> &'a str {
        self.any.source
    }

    /// Macro expansion call-site spans recorded during parsing.
    ///
    /// Each [`MacroRegion`] describes a byte range in the original source
    /// that was identified as a macro invocation (e.g. `name!(args)`).
    /// Populated automatically when the grammar's `macro_style` is set.
    pub fn macro_regions(&self) -> impl Iterator<Item = MacroRegion> + use<'_, 'a, G> {
        self.any.macro_regions()
    }

    /// Statement-local token stream for this parse result.
    ///
    /// Requires `collect_tokens: true` and skips unknown token ordinals for `G`.
    pub fn tokens(&self) -> impl Iterator<Item = TypedParserToken<'a, G>> {
        let source = self.any.source;
        // SAFETY: self.any.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &'a [ffi::CParserToken] = unsafe { self.any.raw.as_ref().result_tokens() };
        raw.iter().filter_map(move |t| {
            let token_type = G::Token::from_token_type(AnyTokenType(t.type_))?;
            let text = &source[t.offset as usize..(t.offset + t.length) as usize];
            Some(TypedParserToken::new(
                text,
                token_type,
                ParserTokenFlags::from_raw(t.flags),
                t.offset,
                t.length,
            ))
        })
    }

    /// Comments attached to this statement.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn comments(&self) -> impl Iterator<Item = Comment<'a>> {
        let source = self.any.source;
        // SAFETY: self.any.raw is valid for 'a; the returned slice lives for 'a.
        let raw: &'a [ffi::CComment] = unsafe { self.any.raw.as_ref().result_comments() };
        raw.iter().map(move |c| {
            let text = &source[c.offset as usize..(c.offset + c.length) as usize];
            let kind = match c.kind {
                ffi::CCommentKind::LineComment => CommentKind::Line,
                ffi::CCommentKind::BlockComment => CommentKind::Block,
            };
            Comment::new(text, kind, c.offset, c.length)
        })
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

    /// Byte offset of the error token, or `None` if unknown.
    pub(crate) fn error_offset(&self) -> Option<usize> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let v = unsafe { self.any.raw.as_ref().result_error_offset() };
        if v == 0xFFFF_FFFF {
            None
        } else {
            Some(v as usize)
        }
    }

    /// Byte length of the error token, or `None` if unknown.
    pub(crate) fn error_length(&self) -> Option<usize> {
        // SAFETY: self.any.raw is a valid, non-null parser pointer for lifetime 'a.
        let v = unsafe { self.any.raw.as_ref().result_error_length() };
        if v == 0 { None } else { Some(v as usize) }
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

/// Extract a single [`crate::ast::FieldValue`] from a raw arena node pointer.
///
/// # Safety
/// `ptr` must point to a valid arena node struct whose field at `meta.offset()`
/// has the type indicated by `meta.kind()`, and must be valid for lifetime `'a`.
#[expect(clippy::cast_ptr_alignment)]
unsafe fn extract_field_value<'a>(
    ptr: *const u8,
    meta: &crate::grammar::FieldMeta<'_>,
    source: &'a str,
) -> crate::ast::FieldValue<'a> {
    use crate::ast::{FieldValue, SourceSpan};
    use crate::grammar::FieldKind;
    // SAFETY: covered by function-level contract; ptr and meta are consistent.
    unsafe {
        let field_ptr = ptr.add(meta.offset() as usize);
        match meta.kind() {
            FieldKind::NodeId => FieldValue::NodeId(AnyNodeId(*(field_ptr.cast::<u32>()))),
            FieldKind::Span => {
                let span = &*(field_ptr.cast::<SourceSpan>());
                if span.is_empty() {
                    FieldValue::Span {
                        text: "",
                        quoted: false,
                    }
                } else {
                    FieldValue::Span {
                        text: span.as_str(source),
                        quoted: span.is_quoted(),
                    }
                }
            }
            FieldKind::Bool => FieldValue::Bool(*(field_ptr.cast::<u32>()) != 0),
            FieldKind::Flags => FieldValue::Flags(*field_ptr),
            FieldKind::Enum => FieldValue::Enum(*(field_ptr.cast::<u32>())),
        }
    }
}

/// Parse failure for a single statement in grammar `G`.
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
pub struct TypedParseError<'a, G: TypedGrammar>(TypedParsedStatement<'a, G>);

impl<'a, G: TypedGrammar> TypedParseError<'a, G> {
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
    /// Returns the byte offset of the error token, or `None` if unknown.
    pub fn offset(&self) -> Option<usize> {
        self.0.error_offset()
    }
    /// Returns the byte length of the error token, or `None` if unknown.
    pub fn length(&self) -> Option<usize> {
        self.0.error_length()
    }
    /// The partial recovery tree, if error recovery produced one.
    pub fn recovery_root(&'a self) -> Option<G::Node<'a>> {
        self.0.recovery_root()
    }

    /// The source text bound to this result.
    pub fn parse_source(&self) -> &'a str {
        self.0.source()
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

impl<G: TypedGrammar> std::fmt::Debug for TypedParseError<'_, G> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("TypedParseError")
            .field("kind", &self.kind())
            .field("message", &self.message())
            .field("offset", &self.offset())
            .field("length", &self.length())
            .finish()
    }
}

impl<G: TypedGrammar> std::fmt::Display for TypedParseError<'_, G> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.message())
    }
}

impl<G: TypedGrammar> std::error::Error for TypedParseError<'_, G> {}

/// Parse-error alias for grammar-independent pipelines.
pub type AnyParseError<'a> = TypedParseError<'a, AnyGrammar>;

// ── Crate-internal ───────────────────────────────────────────────────────────

/// Holds the C parser handle and mutable state. Checked out by sessions at
/// runtime and returned on [`Drop`].
pub(crate) struct ParserInner {
    pub(crate) raw: NonNull<CParser>,
    pub(crate) source_buf: Vec<u8>,
}

impl Drop for ParserInner {
    fn drop(&mut self) {
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
    let c_source_ptr = source_buf.as_ptr();
    // SAFETY: raw is valid (caller owns it); c_source_ptr points to
    // source_buf which is null-terminated.
    #[expect(clippy::cast_possible_truncation)]
    unsafe {
        (*raw).reset(c_source_ptr.cast(), source.len() as u32);
    }
}

pub(crate) use ffi::CParser;
