// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::ffi::{c_char, c_void};

/// Opaque C parser type.
pub(crate) enum CParser {}

/// Return code: no statement / done.
pub(crate) const PARSE_DONE: i32 = 0;
/// Return code: statement parsed cleanly.
pub(crate) const PARSE_OK: i32 = 1;
/// Return code: statement has parse/runtime error.
#[cfg(test)]
pub(crate) const PARSE_ERROR: i32 = -1;

/// Mirrors C `SyntaqliteMemMethods` (xMalloc, xRealloc, xFree).
#[repr(C)]
#[expect(clippy::struct_field_names)]
pub(crate) struct CMemMethods {
    pub x_malloc: unsafe extern "C" fn(usize) -> *mut c_void,
    pub x_realloc: unsafe extern "C" fn(*mut c_void, usize) -> *mut c_void,
    pub x_free: unsafe extern "C" fn(*mut c_void),
}

/// The kind of a comment.
#[expect(dead_code)] // C FFI mirror — variants match the C enum values
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub(crate) enum CCommentKind {
    LineComment = 0,
    BlockComment = 1,
}

/// Mirrors C `SyntaqliteComment`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CComment {
    pub offset: u32,
    pub length: u32,
    pub kind: CCommentKind,
}

#[expect(dead_code)] // C FFI mirrors — not yet consumed on the Rust side
pub(super) const TOKEN_FLAG_AS_ID: u32 = 1;
#[expect(dead_code)]
pub(super) const TOKEN_FLAG_AS_FUNCTION: u32 = 2;
#[expect(dead_code)]
pub(super) const TOKEN_FLAG_AS_TYPE: u32 = 4;

/// Mirrors C `SyntaqliteCompletionContext` (`typedef uint32_t`).
pub(crate) type CCompletionContext = u32;

/// Mirrors C `SyntaqliteParserTokenFlags` (`typedef uint32_t`).
pub(crate) type CParserTokenFlags = u32;

/// Mirrors C `SyntaqliteParserToken`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CParserToken {
    pub offset: u32,
    pub length: u32,
    pub type_: u32,
    pub flags: CParserTokenFlags,
    /// Internal: 0 = original source, >0 = expansion layer id.
    /// Read only by C-side span accessors; the Rust side does not
    /// inspect it directly.
    pub _layer_id: u8,
    pub _pad: [u8; 3],
}

/// A recorded macro invocation region.
///
/// Mirrors C `SyntaqliteMacroRegion` from `include/syntaqlite/parser.h`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CMacroRegion {
    /// Byte offset of the macro call in the original source.
    pub(crate) call_offset: u32,
    /// Byte length of the entire macro call.
    pub(crate) call_length: u32,
}

/// One frame in a traceback produced by the span traceback API.
///
/// Mirrors C `SyntaqliteTracebackFrame` from `include/syntaqlite/parser.h`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CTracebackFrame {
    /// Macro name pointer (borrowed from the macro registry), or null for
    /// the root source frame.
    pub(crate) name: *const u8,
    /// Length of `name`, or 0 for the root frame.
    pub(crate) name_len: u32,
    /// 1-based line number of `offset_in_snippet` within `snippet`.
    pub(crate) line: u32,
    /// 1-based column number of `offset_in_snippet` within `snippet`.
    pub(crate) col: u32,
    /// Buffer to render the frame against — the original source for the
    /// root frame, or an expansion layer's buffer for macro frames.
    pub(crate) snippet: *const u8,
    pub(crate) snippet_len: u32,
    /// Byte offset of this frame's position within `snippet`.
    pub(crate) offset_in_snippet: u32,
    /// Byte length of this frame's position within `snippet`.
    pub(crate) length_in_snippet: u32,
}

impl CParser {
    // Lifecycle
    pub(crate) unsafe fn create(
        mem: *const CMemMethods,
        dialect: crate::dialect::ffi::CDialect,
    ) -> *mut Self {
        // SAFETY: mem may be null (use default allocator); dialect is a
        // valid dialect handle passed by the caller.
        unsafe { syntaqlite_parser_create_with_dialect(mem, dialect) }
    }

    pub(crate) unsafe fn set_trace(&mut self, enable: u32) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_set_trace(self, enable) }
    }

    pub(crate) unsafe fn set_collect_tokens(&mut self, enable: u32) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_set_collect_tokens(self, enable) }
    }

    pub(crate) unsafe fn set_macro_fallback(&mut self, enable: u32) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_set_macro_fallback(self, enable) }
    }

    pub(crate) unsafe fn set_collect_node_extents(&mut self, enable: u32) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_set_collect_node_extents(self, enable) }
    }

    /// Source text bound by the last `reset()` call.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn text<'a>(&self) -> &'a str {
        let mut out_len: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                &raw mut out_len,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return "";
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the parser's source buffer.
        unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize))
        }
    }

    /// Post-expansion source text — the bound source with every
    /// currently-active macro call replaced by its expansion.
    /// Materialized into the parser's scratch buffer; the returned
    /// slice is valid until the next call to `expanded_text` /
    /// `node_expanded_text` or until the parser advances.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying
    /// `self` and is invalidated by the next call.
    pub(crate) unsafe fn expanded_text<'a>(&self) -> &'a str {
        let mut out_len: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_expanded_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                &raw mut out_len,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return "";
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the parser's scratch buffer.
        unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize))
        }
    }

    /// Source text of AST node `node_id`, returned as `(slice, offset)`
    /// where `slice` borrows from the parser's source buffer.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn node_text<'a>(&self, node_id: u32) -> Option<(&'a str, u32)> {
        let mut out_len: u32 = 0;
        let mut out_offset: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_node_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                node_id,
                &raw mut out_len,
                &raw mut out_offset,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return None;
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the parser's source buffer.
        let text = unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize))
        };
        Some((text, out_offset))
    }

    /// Post-expansion text of AST node `node_id` — a slice of whichever
    /// layer buffer (source or macro expansion) contains the node's
    /// tokens.  `None` for mixed-layer nodes.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn node_expanded_text<'a>(&self, node_id: u32) -> Option<&'a str> {
        let mut out_len: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_node_expanded_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                node_id,
                &raw mut out_len,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return None;
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the layer buffer it belongs to.
        let text = unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize))
        };
        Some(text)
    }

    pub(crate) unsafe fn reset(&mut self, source: *const c_char, len: u32) {
        // SAFETY: self is a valid, non-null CParser pointer; source is a
        // null-terminated C string of at least `len` bytes.
        unsafe { syntaqlite_parser_reset(self, source, len) }
    }

    pub(crate) unsafe fn next(&mut self) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_next(self) }
    }

    pub(crate) unsafe fn destroy(this: *mut Self) {
        // SAFETY: this is a valid CParser pointer previously created by
        // `syntaqlite_parser_create_with_dialect` and not yet destroyed.
        unsafe { syntaqlite_parser_destroy(this) }
    }

    // Result accessors (valid after `next()` returns non-DONE)
    pub(crate) unsafe fn result_root(&self) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        unsafe { syntaqlite_result_root(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    pub(crate) unsafe fn result_recovery_root(&self) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        unsafe { syntaqlite_result_recovery_root(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    pub(crate) unsafe fn result_error_msg(&self) -> *const c_char {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        unsafe { syntaqlite_result_error_msg(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    pub(crate) unsafe fn result_error_offset(&self) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        unsafe { syntaqlite_result_error_offset(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    pub(crate) unsafe fn result_error_length(&self) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        unsafe { syntaqlite_result_error_length(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    pub(crate) unsafe fn result_comments(&self) -> &[CComment] {
        let mut count: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        let ptr = unsafe {
            syntaqlite_result_comments(std::ptr::from_ref::<Self>(self).cast_mut(), &raw mut count)
        };
        if count == 0 || ptr.is_null() {
            return &[];
        }
        // SAFETY: ptr is a valid pointer to `count` CComment values owned
        // by the parser arena; the slice is valid for the parser's lifetime.
        unsafe { std::slice::from_raw_parts(ptr, count as usize) }
    }

    pub(crate) unsafe fn result_tokens(&self) -> &[CParserToken] {
        let mut count: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        let ptr = unsafe {
            syntaqlite_result_tokens(std::ptr::from_ref::<Self>(self).cast_mut(), &raw mut count)
        };
        if count == 0 || ptr.is_null() {
            return &[];
        }
        // SAFETY: ptr is a valid pointer to `count` CParserToken values owned
        // by the parser arena; the slice is valid for the parser's lifetime.
        unsafe { std::slice::from_raw_parts(ptr, count as usize) }
    }

    pub(crate) unsafe fn span_expanded_text(
        &self,
        span: crate::ast::TextSpan,
        out_len: *mut u32,
    ) -> *const u8 {
        // SAFETY: self is a valid, non-null CParser pointer; span is a copy
        // of an arena value with the SyntaqliteTextSpan layout; out_len is
        // a valid pointer owned by the caller.
        unsafe {
            syntaqlite_parser_span_expanded_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                std::ptr::from_ref(&span).cast(),
                out_len,
            )
        }
    }

    pub(crate) unsafe fn span_text(
        &self,
        span: crate::ast::TextSpan,
        out_len: *mut u32,
        out_offset: *mut u32,
    ) -> *const u8 {
        // SAFETY: self is a valid, non-null CParser pointer; span is a copy
        // of an arena value with the SyntaqliteTextSpan layout; out_len and
        // out_offset are valid pointers (or null).
        unsafe {
            syntaqlite_parser_span_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                std::ptr::from_ref(&span).cast(),
                out_len,
                out_offset,
            )
        }
    }

    pub(crate) unsafe fn traceback(&self, span: crate::ast::TextSpan) -> &[CTracebackFrame] {
        let mut count: u32 = 0;
        // SAFETY: self is a valid CParser pointer; span is a copy of an
        // arena value with the SyntaqliteTextSpan layout.  The returned
        // pointer is backed by the parser's owned `traceback_buf` vec and
        // remains valid until the next call to this function or until the
        // parser is mutated through another `&mut` method.
        let ptr = unsafe {
            syntaqlite_parser_traceback(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                std::ptr::from_ref(&span).cast(),
                &raw mut count,
            )
        };
        if ptr.is_null() || count == 0 {
            return &[];
        }
        // SAFETY: ptr + count describe a valid slice of CTracebackFrame
        // values owned by the parser for the duration of the next call.
        unsafe { std::slice::from_raw_parts(ptr, count as usize) }
    }

    pub(crate) unsafe fn result_macro_count(&self) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        unsafe { syntaqlite_result_macro_count(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    pub(crate) unsafe fn result_macro_at(&self, idx: u32) -> CMacroRegion {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        // The C side clamps out-of-range indices to {0, 0}.
        unsafe { syntaqlite_result_macro_at(std::ptr::from_ref::<Self>(self).cast_mut(), idx) }
    }

    // Arena accessors
    pub(crate) unsafe fn node(&self, node_id: u32) -> *const u32 {
        // SAFETY: self is a valid, non-null CParser pointer; node_id is a
        // raw node ID from the arena (null is handled by the C side).
        unsafe { syntaqlite_parser_node(std::ptr::from_ref::<Self>(self).cast_mut(), node_id) }
    }

    pub(crate) unsafe fn node_count(&self) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_node_count(std::ptr::from_ref::<Self>(self).cast_mut()) }
    }

    // AST dump
    pub(crate) unsafe fn dump_node(&self, node_id: u32, indent: u32) -> *mut c_char {
        // SAFETY: self is a valid, non-null CParser pointer; node_id is a
        // raw node ID from the arena. Returns a malloc'd string or null.
        unsafe {
            syntaqlite_dump_node(std::ptr::from_ref::<Self>(self).cast_mut(), node_id, indent)
        }
    }

    // Incremental (token-feeding) API
    pub(crate) unsafe fn feed_token(
        &mut self,
        token_type: u32,
        text: *const c_char,
        len: u32,
    ) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer; text is a
        // valid pointer to at least `len` bytes of token text.
        unsafe { syntaqlite_parser_feed_token(self, token_type, text, len) }
    }

    pub(crate) unsafe fn finish(&mut self) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_finish(self) }
    }

    pub(crate) unsafe fn expected_tokens(&self, out_tokens: *mut u32, out_cap: u32) -> u32 {
        // SAFETY: self is a valid, non-null CParser pointer; out_tokens
        // is a valid pointer to at least `out_cap` u32 values.
        unsafe {
            syntaqlite_parser_expected_tokens(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                out_tokens,
                out_cap,
            )
        }
    }

    pub(crate) unsafe fn completion_context(&self) -> super::CompletionContext {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe {
            super::CompletionContext::from_raw(syntaqlite_parser_completion_context(
                std::ptr::from_ref::<Self>(self).cast_mut(),
            ))
        }
    }

    pub(crate) unsafe fn begin_macro(&mut self, call_offset: u32, call_length: u32) {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_begin_macro(self, call_offset, call_length) }
    }

    pub(crate) unsafe fn end_macro(&mut self) {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe { syntaqlite_parser_end_macro(self) }
    }

    #[expect(clippy::too_many_arguments, reason = "mirrors the C API surface 1:1")]
    pub(crate) unsafe fn register_macro(
        &mut self,
        name: *const c_char,
        name_len: u32,
        param_names: *const *const c_char,
        param_count: u32,
        body: *const c_char,
        body_len: u32,
        def_line: u32,
        def_col: u32,
    ) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer; name, param_names,
        // and body are valid pointers with the specified lengths.
        unsafe {
            syntaqlite_parser_register_macro(
                self,
                name,
                name_len,
                param_names,
                param_count,
                body,
                body_len,
                def_line,
                def_col,
            )
        }
    }

    pub(crate) unsafe fn deregister_macro(&mut self, name: *const c_char, name_len: u32) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer; name is valid.
        unsafe { syntaqlite_parser_deregister_macro(self, name, name_len) }
    }
}

unsafe extern "C" {
    // Parser lifecycle
    fn syntaqlite_parser_create_with_dialect(
        mem: *const CMemMethods,
        dialect: crate::dialect::ffi::CDialect,
    ) -> *mut CParser;
    fn syntaqlite_parser_reset(p: *mut CParser, source: *const c_char, len: u32);
    fn syntaqlite_parser_next(p: *mut CParser) -> i32;
    fn syntaqlite_parser_destroy(p: *mut CParser);

    // Result accessors
    fn syntaqlite_result_root(p: *mut CParser) -> u32;
    fn syntaqlite_result_recovery_root(p: *mut CParser) -> u32;
    fn syntaqlite_result_error_msg(p: *mut CParser) -> *const c_char;
    fn syntaqlite_result_error_offset(p: *mut CParser) -> u32;
    fn syntaqlite_result_error_length(p: *mut CParser) -> u32;
    fn syntaqlite_result_comments(p: *mut CParser, count: *mut u32) -> *const CComment;
    fn syntaqlite_result_tokens(p: *mut CParser, count: *mut u32) -> *const CParserToken;
    fn syntaqlite_result_macro_count(p: *mut CParser) -> u32;
    fn syntaqlite_result_macro_at(p: *mut CParser, idx: u32) -> CMacroRegion;

    // Arena accessors
    fn syntaqlite_parser_node(p: *mut CParser, node_id: u32) -> *const u32;
    fn syntaqlite_parser_node_count(p: *mut CParser) -> u32;
    // Span accessors
    fn syntaqlite_parser_span_expanded_text(
        p: *mut CParser,
        span: *const c_void,
        out_len: *mut u32,
    ) -> *const u8;
    fn syntaqlite_parser_span_text(
        p: *mut CParser,
        span: *const c_void,
        out_len: *mut u32,
        out_offset: *mut u32,
    ) -> *const u8;
    fn syntaqlite_parser_traceback(
        p: *mut CParser,
        span: *const c_void,
        out_count: *mut u32,
    ) -> *const CTracebackFrame;

    // Configuration
    fn syntaqlite_parser_set_trace(p: *mut CParser, enable: u32) -> i32;
    fn syntaqlite_parser_set_collect_tokens(p: *mut CParser, enable: u32) -> i32;
    fn syntaqlite_parser_set_macro_fallback(p: *mut CParser, enable: u32) -> i32;
    fn syntaqlite_parser_set_collect_node_extents(p: *mut CParser, enable: u32) -> i32;
    fn syntaqlite_parser_text(p: *mut CParser, out_len: *mut u32) -> *const u8;
    fn syntaqlite_parser_expanded_text(p: *mut CParser, out_len: *mut u32) -> *const u8;
    fn syntaqlite_parser_node_text(
        p: *mut CParser,
        node_id: u32,
        out_len: *mut u32,
        out_offset: *mut u32,
    ) -> *const u8;
    fn syntaqlite_parser_node_expanded_text(
        p: *mut CParser,
        node_id: u32,
        out_len: *mut u32,
    ) -> *const u8;

    // AST dump
    fn syntaqlite_dump_node(p: *mut CParser, node_id: u32, indent: u32) -> *mut c_char;

    // Incremental (token-feeding) API (from incremental.h)
    fn syntaqlite_parser_feed_token(
        p: *mut CParser,
        token_type: u32,
        text: *const c_char,
        len: u32,
    ) -> i32;
    fn syntaqlite_parser_finish(p: *mut CParser) -> i32;
    fn syntaqlite_parser_expected_tokens(
        p: *mut CParser,
        out_tokens: *mut u32,
        out_cap: u32,
    ) -> u32;
    fn syntaqlite_parser_completion_context(p: *mut CParser) -> CCompletionContext;
    fn syntaqlite_parser_begin_macro(p: *mut CParser, call_offset: u32, call_length: u32);
    fn syntaqlite_parser_end_macro(p: *mut CParser);

    // Macro registration
    fn syntaqlite_parser_register_macro(
        p: *mut CParser,
        name: *const c_char,
        name_len: u32,
        param_names: *const *const c_char,
        param_count: u32,
        body: *const c_char,
        body_len: u32,
        def_line: u32,
        def_col: u32,
    ) -> i32;
    fn syntaqlite_parser_deregister_macro(
        p: *mut CParser,
        name: *const c_char,
        name_len: u32,
    ) -> i32;
}

#[cfg(all(test, feature = "sqlite"))]
mod tests {
    use std::ffi::CString;
    use std::ptr::NonNull;

    use super::{CParser, PARSE_DONE, PARSE_ERROR, PARSE_OK};
    use crate::any::AnyDialect;
    use crate::ast::{AnyNodeId, GrammarNodeType};
    use crate::sqlite::ast::{Expr, Name, Stmt};

    const NULL_NODE: u32 = u32::MAX;

    struct ParserHandle {
        raw: NonNull<CParser>,
    }

    impl ParserHandle {
        fn new() -> Self {
            let dialect: AnyDialect = crate::sqlite::dialect::dialect().into();
            // SAFETY: SQLite dialect handle is valid static dialect metadata.
            let raw = unsafe { CParser::create(std::ptr::null(), dialect.inner) };
            let raw = NonNull::new(raw).expect("parser allocation failed");
            Self { raw }
        }

        fn parser_mut(&mut self) -> &mut CParser {
            // SAFETY: `raw` is owned by this handle and remains valid until drop.
            unsafe { self.raw.as_mut() }
        }
    }

    impl Drop for ParserHandle {
        fn drop(&mut self) {
            // SAFETY: pointer was created by CParser::create and not yet destroyed.
            unsafe { CParser::destroy(self.raw.as_ptr()) };
        }
    }

    fn reset_with_source(parser: &mut CParser, sql: &str) -> CString {
        let sql_c = CString::new(sql).expect("SQL test input must not contain NUL bytes");
        // SAFETY: sql_c is NUL-terminated and lives until caller drops it.
        unsafe {
            parser.reset(
                sql_c.as_ptr(),
                u32::try_from(sql.len()).expect("SQL test input too large"),
            );
        }
        sql_c
    }

    fn with_recovery_stmt<F, R>(parser: *mut CParser, _source: &str, recovery_root: u32, f: F) -> R
    where
        F: FnOnce(Stmt<'_>) -> R,
    {
        let dialect: AnyDialect = crate::sqlite::dialect::dialect().into();
        // SAFETY: parser pointer is valid for the test scope.
        let result = unsafe { crate::parser::AnyParsedStatement::new(parser, dialect) };
        let stmt = Stmt::from_result(&result, AnyNodeId(recovery_root))
            .expect("recovery root should resolve to typed Stmt");
        f(stmt)
    }

    #[test]
    fn c_parser_ok_statement_sets_root_only() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        let _sql = reset_with_source(parser, "SELECT 1;");

        // SAFETY: parser was reset before next().
        let rc = unsafe { parser.next() };
        assert_eq!(rc, PARSE_OK);

        // SAFETY: result accessors are valid after non-DONE return.
        let root = unsafe { parser.result_root() };
        assert_ne!(root, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        assert_eq!(unsafe { parser.result_recovery_root() }, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        assert!(unsafe { parser.result_error_msg().is_null() });

        // SAFETY: parser remains valid.
        assert_eq!(unsafe { parser.next() }, PARSE_DONE);
    }

    #[test]
    fn c_parser_expr_error_sets_recovery_root_with_error_node() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        let _sql = reset_with_source(parser, "SELECT");

        // SAFETY: parser was reset before next().
        let rc = unsafe { parser.next() };
        assert_eq!(rc, PARSE_ERROR);

        // Statement errors should not expose a success root.
        // SAFETY: result accessors are valid after non-DONE return.
        assert_eq!(unsafe { parser.result_root() }, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        let recovery_root = unsafe { parser.result_recovery_root() };
        assert_ne!(recovery_root, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        assert!(!unsafe { parser.result_error_msg().is_null() });

        with_recovery_stmt(
            std::ptr::from_mut::<CParser>(parser),
            "SELECT",
            recovery_root,
            |stmt| {
                let Stmt::SelectStmt(select) = stmt else {
                    panic!("expected recovery root to be SelectStmt")
                };
                let columns = select
                    .columns()
                    .expect("recovery select should keep result columns");
                assert_eq!(columns.len(), 1);
                let col = columns.get(0).expect("first result column should exist");
                assert!(
                    matches!(col.expr(), Some(Expr::Error(_))),
                    "expected recovered expr hole at result column expr"
                );
            },
        );
    }

    #[test]
    fn c_parser_name_error_sets_recovery_root_with_error_node() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        let _sql = reset_with_source(parser, "SELECT 1 AS");

        // SAFETY: parser was reset before next().
        let rc = unsafe { parser.next() };
        assert_eq!(rc, PARSE_ERROR);

        // SAFETY: result accessors are valid after non-DONE return.
        assert_eq!(unsafe { parser.result_root() }, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        let recovery_root = unsafe { parser.result_recovery_root() };
        assert_ne!(recovery_root, NULL_NODE);

        with_recovery_stmt(
            std::ptr::from_mut::<CParser>(parser),
            "SELECT 1 AS",
            recovery_root,
            |stmt| {
                let Stmt::SelectStmt(select) = stmt else {
                    panic!("expected recovery root to be SelectStmt")
                };
                let columns = select
                    .columns()
                    .expect("recovery select should keep result columns");
                assert_eq!(columns.len(), 1);
                let col = columns.get(0).expect("first result column should exist");
                assert!(
                    matches!(col.alias(), Some(Name::Error(_))),
                    "expected recovered name hole at result column alias"
                );
                assert!(
                    matches!(col.expr(), Some(Expr::Literal(_))),
                    "expected original expression to remain intact"
                );
            },
        );
    }

    #[test]
    fn c_parser_fatal_error_has_no_recovery_root() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        let _sql = reset_with_source(parser, "abc");

        // SAFETY: parser was reset before next().
        let rc = unsafe { parser.next() };
        assert_eq!(rc, PARSE_ERROR);

        // SAFETY: result accessors are valid after non-DONE return.
        assert_eq!(unsafe { parser.result_root() }, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        assert_eq!(unsafe { parser.result_recovery_root() }, NULL_NODE);
        // SAFETY: result accessors are valid after non-DONE return.
        assert!(!unsafe { parser.result_error_msg().is_null() });
    }

    // ── Macro registry / hashmap tests ──────────────────────────────────

    /// Helper: create a parser with `macro_fallback` enabled (needed for macro
    /// registration tests since `SQLite`'s dialect has `macro_style` = NONE).
    fn new_macro_parser() -> ParserHandle {
        let mut handle = ParserHandle::new();
        // SAFETY: CParser wraps a valid C parser handle.
        let rc = unsafe { handle.parser_mut().set_macro_fallback(1) };
        assert_eq!(rc, 0);
        handle
    }

    /// Helper: register a template macro via the C API.
    #[expect(clippy::cast_possible_truncation)]
    fn register_macro(parser: &mut CParser, name: &str, params: &[&str], body: &str) {
        let param_cstrings: Vec<CString> =
            params.iter().map(|p| CString::new(*p).unwrap()).collect();
        let param_ptrs: Vec<*const std::ffi::c_char> =
            param_cstrings.iter().map(|c| c.as_ptr()).collect();
        // SAFETY: All pointers point to valid Rust-owned data that outlives
        // the FFI call. Lengths are small test values that fit in u32.
        let rc = unsafe {
            parser.register_macro(
                name.as_ptr().cast(),
                name.len() as u32,
                param_ptrs.as_ptr(),
                params.len() as u32,
                body.as_ptr().cast(),
                body.len() as u32,
                0,
                0,
            )
        };
        assert_eq!(rc, 0, "register_macro failed for '{name}'");
    }

    /// Helper: parse a single statement and return its status.
    fn parse_one(parser: &mut CParser, sql: &str) -> (i32, CString) {
        let sql_c = reset_with_source(parser, sql);
        // SAFETY: parser has been reset with a valid NUL-terminated source.
        let rc = unsafe { parser.next() };
        (rc, sql_c)
    }

    #[test]
    fn macro_register_and_expand() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();
        register_macro(parser, "double", &["x"], "($x + $x)");

        let (rc, _sql) = parse_one(parser, "SELECT double!(1);");
        assert_eq!(
            rc, PARSE_OK,
            "macro expansion should produce a valid statement"
        );
        // SAFETY: parser has valid state after parse_one.
        assert_ne!(unsafe { parser.result_root() }, NULL_NODE);
    }

    #[test]
    fn macro_deregister_removes_entry() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();
        register_macro(parser, "foo", &["x"], "$x");

        // SAFETY: pointer and length match the literal "foo".
        let rc = unsafe { parser.deregister_macro(b"foo".as_ptr().cast(), 3) };
        assert_eq!(rc, 0);

        // After deregistering, the name is no longer a macro — a second
        // deregister should fail (entry not found).
        // SAFETY: pointer and length match the literal "foo".
        let rc = unsafe { parser.deregister_macro(b"foo".as_ptr().cast(), 3) };
        assert_eq!(rc, -1, "deregistering again should fail");
    }

    #[test]
    fn macro_deregister_nonexistent_returns_error() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();

        // SAFETY: pointer and length match the literal "nope".
        let rc = unsafe { parser.deregister_macro(b"nope".as_ptr().cast(), 4) };
        assert_eq!(rc, -1);
    }

    #[test]
    fn macro_case_insensitive_lookup() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();
        register_macro(parser, "MyMacro", &["x"], "$x");

        // SQL identifiers are case-insensitive; "mymacro" should match "MyMacro".
        let (rc, _sql) = parse_one(parser, "SELECT mymacro!(42);");
        assert_eq!(rc, PARSE_OK);
    }

    #[test]
    fn macro_overwrite_existing() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();
        register_macro(parser, "m", &["x"], "$x");
        // Re-register with different body — should overwrite.
        register_macro(parser, "m", &["x"], "($x + 1)");

        let (rc, _sql) = parse_one(parser, "SELECT m!(5);");
        assert_eq!(rc, PARSE_OK);
    }

    #[test]
    fn macro_register_after_deregister_reuses_tombstone() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();
        register_macro(parser, "tmp", &["x"], "$x");

        // SAFETY: pointer and length match the literal "tmp".
        let rc = unsafe { parser.deregister_macro(b"tmp".as_ptr().cast(), 3) };
        assert_eq!(rc, 0);

        // Re-register the same name — should reuse the tombstone slot.
        register_macro(parser, "tmp", &["x"], "($x)");

        let (rc, _sql) = parse_one(parser, "SELECT tmp!(7);");
        assert_eq!(rc, PARSE_OK);
    }

    #[test]
    fn macro_many_entries_forces_grow() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();

        // Register enough macros to trigger at least one table resize.
        // Initial capacity is 16, load factor threshold is 70% → grows at 12.
        let names: Vec<String> = (0..20).map(|i| format!("m{i}")).collect();
        for name in &names {
            register_macro(parser, name, &["x"], "$x");
        }

        // Verify all 20 macros are still reachable after growth.
        for name in &names {
            let sql = format!("SELECT {name}!(1);");
            let (rc, _sql) = parse_one(parser, &sql);
            assert_eq!(
                rc, PARSE_OK,
                "macro '{name}' should expand after table grow"
            );
        }
    }

    #[test]
    fn macro_deregister_then_grow_drops_tombstones() {
        let mut handle = new_macro_parser();
        let parser = handle.parser_mut();

        // Fill table, then delete half, then add more to force a grow.
        // After grow, tombstones should be gone and all live entries reachable.
        for i in 0..10 {
            let name = format!("a{i}");
            register_macro(parser, &name, &["x"], "$x");
        }
        for i in 0..5 {
            let name = format!("a{i}");
            // SAFETY: pointer and length refer to the valid `name` string.
            #[expect(clippy::cast_possible_truncation)]
            let rc = unsafe { parser.deregister_macro(name.as_ptr().cast(), name.len() as u32) };
            assert_eq!(rc, 0);
        }
        // Add more to force a grow (5 live + new entries past 70% of 16).
        for i in 10..20 {
            let name = format!("a{i}");
            register_macro(parser, &name, &["x"], "$x");
        }

        // Verify surviving entries (a5..a19) all work.
        for i in 5..20 {
            let name = format!("a{i}");
            let sql = format!("SELECT {name}!(1);");
            let (rc, _sql) = parse_one(parser, &sql);
            assert_eq!(
                rc, PARSE_OK,
                "macro '{name}' should be reachable after grow"
            );
        }

        // Verify deleted entries (a0..a4) are gone.
        for i in 0..5 {
            let name = format!("a{i}");
            // SAFETY: pointer and length refer to the valid `name` string.
            #[expect(clippy::cast_possible_truncation)]
            let rc = unsafe { parser.deregister_macro(name.as_ptr().cast(), name.len() as u32) };
            assert_eq!(rc, -1, "deleted macro 'a{i}' should not be found");
        }
    }

    // ── Macro fallback tests ─────────────────────────────────────────────

    /// Helper: enable macro fallback + token collection on a parser.
    fn enable_fallback(parser: &mut CParser) {
        // SAFETY: CParser wraps a valid C parser handle.
        let rc = unsafe { parser.set_macro_fallback(1) };
        assert_eq!(rc, 0, "set_macro_fallback should succeed");
        // SAFETY: CParser wraps a valid C parser handle.
        let rc = unsafe { parser.set_collect_tokens(1) };
        assert_eq!(rc, 0, "set_collect_tokens should succeed");
    }

    #[test]
    fn macro_fallback_unregistered_parses_as_id() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);

        // Without any macros registered, foo!(1, 2) should parse OK as an identifier.
        let (rc, _sql) = parse_one(parser, "SELECT foo!(1, 2);");
        assert_eq!(
            rc, PARSE_OK,
            "unregistered macro call should parse OK with fallback enabled"
        );
        // SAFETY: CParser wraps a valid C parser handle.
        assert_ne!(unsafe { parser.result_root() }, NULL_NODE);
    }

    #[test]
    fn macro_fallback_without_flag_still_errors() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        // Do NOT enable fallback.

        let (rc, _sql) = parse_one(parser, "SELECT foo!(1, 2);");
        assert_eq!(
            rc, PARSE_ERROR,
            "unregistered macro call should error without fallback"
        );
    }

    #[test]
    fn macro_fallback_records_macro_region() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);

        let sql = "SELECT foo!(1, 2);";
        let (rc, _sql) = parse_one(parser, sql);
        assert_eq!(rc, PARSE_OK);

        // SAFETY: CParser wraps a valid C parser handle.
        let count = unsafe { parser.result_macro_count() };
        assert_eq!(count, 1, "expected one macro region");
        // SAFETY: idx < count.
        let r = unsafe { parser.result_macro_at(0) };
        #[expect(clippy::cast_possible_truncation)]
        let call_start = sql.find("foo!").unwrap() as u32;
        assert_eq!(r.call_offset, call_start);
        // "foo!(1, 2)" is 10 bytes.
        assert_eq!(r.call_length, 10);
    }

    #[test]
    fn macro_fallback_registered_still_expands() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);
        register_macro(parser, "double", &["x"], "($x + $x)");

        // Registered macro should still expand normally even with fallback on.
        let (rc, _sql) = parse_one(parser, "SELECT double!(3);");
        assert_eq!(rc, PARSE_OK);
    }

    #[test]
    fn macro_fallback_unbalanced_parens_errors() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);

        // Unbalanced parens should still cause parse error.
        let (rc, _sql) = parse_one(parser, "SELECT foo!(1, 2;");
        assert_eq!(
            rc, PARSE_ERROR,
            "unbalanced parens should error even with fallback"
        );
    }

    #[test]
    fn macro_fallback_empty_args() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);

        let (rc, _sql) = parse_one(parser, "SELECT foo!();");
        assert_eq!(
            rc, PARSE_OK,
            "empty-args macro call should parse OK with fallback"
        );
    }

    #[test]
    fn macro_fallback_in_from_clause() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);

        let (rc, _sql) = parse_one(parser, "SELECT * FROM my_table!(t1);");
        assert_eq!(rc, PARSE_OK, "macro fallback should work in FROM clause");
    }

    #[test]
    fn macro_fallback_nested_parens() {
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        enable_fallback(parser);

        let sql = "SELECT * FROM graph!(( SELECT id FROM t ), ( SELECT id FROM s ));";
        let (rc, _sql) = parse_one(parser, sql);
        assert_eq!(
            rc, PARSE_OK,
            "nested parens in macro args should parse OK with fallback"
        );

        // SAFETY: CParser wraps a valid C parser handle.
        let count = unsafe { parser.result_macro_count() };
        assert_eq!(count, 1);
        // SAFETY: idx < count.
        let r = unsafe { parser.result_macro_at(0) };
        let call_text = &sql[r.call_offset as usize..(r.call_offset + r.call_length) as usize];
        assert!(
            call_text.starts_with("graph!(") && call_text.ends_with(')'),
            "macro region should cover full call, got: '{call_text}'"
        );
    }

    #[test]
    fn macro_expanded_extract_fields_does_not_panic() {
        // Regression: synq_span() computes `tok.z - ctx->source` for ALL
        // tokens, but during macro expansion tok.z points into the expansion
        // buffer (a different allocation). This makes the offset garbage when
        // layer_id == 0 is not corrected. extract_fields then panics with
        // "byte index N is out of bounds".
        use crate::ParserConfig;
        use crate::any::{AnyParser, ParseOutcome};

        let dialect = crate::sqlite::dialect::dialect();
        let parser = AnyParser::with_config(
            dialect.into(),
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(true),
        );
        // Register a macro whose body is a valid expression that produces
        // span fields (identifiers) in the expansion buffer.
        {
            let mut setup = parser.parse("SELECT 1;");
            while let ParseOutcome::Ok(_) = setup.next() {}
            setup.register_macro("my_expr", &["x"], "$x + 1");
        }

        // Parse a statement that invokes the macro in expression position.
        // The macro body "$x + 1" expands with x=42, producing "42 + 1".
        // The "1" token comes from the expansion buffer — its span offset
        // must be relative to the expansion buffer, not ctx->source.
        let mut session2 = parser.parse("SELECT my_expr!(42);");
        match session2.next() {
            ParseOutcome::Ok(stmt) => {
                let erased = stmt.erase();
                let root = erased.root_id();
                // This is the call that panics if span offsets are wrong.
                let result = erased.extract_fields(root);
                assert!(result.is_some(), "root should have extractable fields");
                // Also walk all children to exercise all spans.
                for child in erased.child_node_ids(root) {
                    let _ = erased.extract_fields(child);
                }
            }
            ParseOutcome::Err(_) => {
                // Parse error is acceptable — the macro may not produce
                // valid SQL. But it must not panic.
            }
            ParseOutcome::Done => panic!("expected a statement"),
        }
    }
}
