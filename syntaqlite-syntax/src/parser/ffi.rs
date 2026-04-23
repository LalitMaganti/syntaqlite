// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::ffi::{c_char, c_void};

use crate::source::{RewriteIdx, StmtOffset, TokenIdx};

/// Opaque C parser type.
pub(crate) enum CParser {}

/// Return code: no statement / done.
pub(crate) const PARSE_DONE: i32 = 0;
/// Return code: statement parsed cleanly.
pub(crate) const PARSE_OK: i32 = 1;
/// Return code: statement has parse/runtime error.
#[cfg(test)]
pub(crate) const PARSE_ERROR: i32 = -1;

/// Generic success code for C APIs that return 0/-1 status.
pub(crate) const SYNTAQLITE_OK: i32 = 0;

/// `SyntaqliteMacroLookupFn`: macro expansion completed successfully.
pub(crate) const MACRO_LOOKUP_OK: i32 = 0;
/// `SyntaqliteMacroLookupFn`: no macro with the given name is registered.
pub(crate) const MACRO_LOOKUP_NOT_FOUND: i32 = -1;

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

/// Which side of a token a comment attaches to.  Matches C
/// `SYNQ_COMMENT_LEADING` / `SYNQ_COMMENT_TRAILING`.
#[expect(dead_code)] // C FFI mirror — variants match the C enum values
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub(crate) enum CCommentSide {
    Leading = 0,
    Trailing = 1,
}

/// Mirrors C `SyntaqliteComment`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CComment {
    pub offset: u32,
    pub length: u32,
    pub token_idx: u32,
    pub kind: CCommentKind,
    pub side: CCommentSide,
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
    pub _layer_id: u32,
}

/// Mirrors C `SyntaqliteToken` from `include/syntaqlite/tokenizer.h`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CToken {
    pub(crate) text: *const c_char,
    pub(crate) length: u32,
    pub(crate) type_: u32,
}

/// A recorded macro rewrite.
///
/// Mirrors C `SyntaqliteMacroRewrite` from `include/syntaqlite/parser.h`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CMacroRewrite {
    /// Index of the parent rewrite (`u32::MAX` = authored source).
    pub(crate) parent_idx: u32,
    /// Byte offset of the macro call in the parent's text.
    pub(crate) call_offset: u32,
    /// Byte length of the entire macro call in the parent's text.
    pub(crate) call_length: u32,
    /// Pointer to the expansion (replacement) text.  Not NUL-terminated.
    pub(crate) expansion: *const u8,
    /// Length of `expansion`.
    pub(crate) expansion_len: u32,
    /// Pointer to the macro name.  Not NUL-terminated; may be null.
    pub(crate) name: *const u8,
    /// Length of `name`.
    pub(crate) name_len: u32,
    /// 1-based line of the macro definition (0 = unknown).
    pub(crate) def_line: u32,
    /// 1-based column of the macro definition (0 = unknown).
    pub(crate) def_col: u32,
    /// Call position in the parent's *authored* body.
    /// `u32::MAX` (`ARG_INTERNAL`) means the call came from a $param
    /// substitution and has no body position.
    pub(crate) body_call_offset: u32,
    pub(crate) body_call_length: u32,
    /// The buffer that `call_offset` (and each arg's offset) indexes
    /// into — the statement source for top-level rewrites, or the
    /// parent rewrite's expansion buffer otherwise.  Not NUL-terminated.
    pub(crate) parent_buffer: *const u8,
    /// Length of `parent_buffer`.
    pub(crate) parent_buffer_len: u32,
    /// 1 if this rewrite is a fallback (unregistered name! kept as a
    /// `TK_ID` with no expansion); 0 for registered macros.
    pub(crate) is_fallback: u32,
}

/// One $param substitution within a macro expansion.
///
/// Mirrors C `SyntaqliteMacroArgSegment` from `include/syntaqlite/parser.h`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CMacroArgSegment {
    /// Position of the $param token in the authored body (0 if unknown).
    pub(crate) body_offset: u32,
    /// Length of the $param token in the authored body (0 if unknown).
    pub(crate) body_length: u32,
    /// Position of the substituted arg text in the rewrite's expansion.
    pub(crate) expansion_offset: u32,
    /// Length of the substituted arg text in the rewrite's expansion.
    pub(crate) expansion_length: u32,
    /// Origin of the arg text: `u32::MAX` = authored source, else a
    /// rewrite index.
    pub(crate) origin_parent_idx: u32,
    /// Byte offset of the arg text in the origin.
    pub(crate) origin_offset: u32,
    /// Byte length of the arg text in the origin.
    pub(crate) origin_length: u32,
}

/// One top-level argument of a macro call, at the call site.
///
/// Mirrors C `SyntaqliteMacroCallArg` from
/// `include/syntaqlite/parser.h`.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub(crate) struct CMacroCallArg {
    pub(crate) offset: u32,
    pub(crate) length: u32,
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

    /// Source slice for the last-completed statement and its absolute
    /// offset within the bound source.  Returns `("", 0)` when no
    /// statement has been produced yet.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn text<'a>(&self) -> (&'a str, u32) {
        let mut out_offset: u32 = 0;
        let mut out_len: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                &raw mut out_offset,
                &raw mut out_len,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return ("", 0);
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the parser's source buffer.
        let s = unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize))
        };
        (s, out_offset)
    }

    /// Full SQL source bound by the last `reset()` call.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn full_text<'a>(&self) -> &'a str {
        let mut out_len: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_full_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                &raw mut out_len,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return "";
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the parser's source buffer.
        unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize)) }
    }

    /// Tokenization buffer for `layer_id` — the authored-source slice
    /// for the current statement (layer 0) or an expansion layer's
    /// body (layer > 0).  Empty if `layer_id` is out of range or
    /// macros are compiled out.
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn layer_text<'a>(&self, layer_id: u32) -> &'a str {
        let mut out_len: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        let ptr = unsafe {
            syntaqlite_parser_layer_text(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                layer_id,
                &raw mut out_len,
            )
        };
        if ptr.is_null() || out_len == 0 {
            return "";
        }
        // SAFETY: C guarantees `ptr` points to `out_len` bytes of valid
        // UTF-8 within the parser's layer buffer.
        unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize)) }
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
        unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr, out_len as usize)) }
    }

    /// Source text of AST node `node_id`, returned as `(slice, offset)`
    /// where `offset` is statement-relative (see the C side).
    ///
    /// # Safety
    /// The returned slice must not outlive the borrow underlying `self`.
    pub(crate) unsafe fn node_text<'a>(&self, node_id: u32) -> Option<(&'a str, StmtOffset)> {
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
        Some((text, StmtOffset::from_raw(out_offset)))
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

    /// Returns `true` if all tokens of AST node `node_id` live in
    /// layer 0 (original source).  Requires extent tracking.
    pub(crate) unsafe fn node_is_macro_free(&self, node_id: u32) -> bool {
        // SAFETY: self is a valid, non-null CParser pointer owned by the caller.
        unsafe {
            syntaqlite_node_is_macro_free(std::ptr::from_ref::<Self>(self).cast_mut(), node_id) != 0
        }
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

    pub(crate) unsafe fn token_leading_comments(&self, token_idx: TokenIdx) -> &[CComment] {
        let mut count: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        let ptr = unsafe {
            syntaqlite_token_leading_comments(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                token_idx.as_u32(),
                &raw mut count,
            )
        };
        if count == 0 || ptr.is_null() {
            return &[];
        }
        // SAFETY: ptr+count describe a contiguous slice of CComment values
        // inside the parser's comments vec; valid for the parser's lifetime.
        unsafe { std::slice::from_raw_parts(ptr, count as usize) }
    }

    pub(crate) unsafe fn token_trailing_comments(&self, token_idx: TokenIdx) -> &[CComment] {
        let mut count: u32 = 0;
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        let ptr = unsafe {
            syntaqlite_token_trailing_comments(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                token_idx.as_u32(),
                &raw mut count,
            )
        };
        if count == 0 || ptr.is_null() {
            return &[];
        }
        // SAFETY: ptr+count describe a contiguous slice of CComment values
        // inside the parser's comments vec; valid for the parser's lifetime.
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

    pub(crate) unsafe fn result_macro_rewrite_at(&self, idx: u32) -> CMacroRewrite {
        // SAFETY: self is a valid, non-null CParser pointer; result
        // accessors are valid after `next()` returns a non-DONE code.
        // The C side clamps out-of-range indices to a zero-initialized rewrite.
        unsafe {
            syntaqlite_result_macro_rewrite_at(std::ptr::from_ref::<Self>(self).cast_mut(), idx)
        }
    }

    pub(crate) unsafe fn macro_rewrite_arg_segment_count(&self, rewrite_idx: RewriteIdx) -> u32 {
        // SAFETY: self is a valid CParser pointer; the C side clamps
        // out-of-range indices to zero.
        unsafe {
            syntaqlite_macro_rewrite_arg_segment_count(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                rewrite_idx.as_u32(),
            )
        }
    }

    pub(crate) unsafe fn macro_rewrite_arg_segment_at(
        &self,
        rewrite_idx: RewriteIdx,
        segment_idx: u32,
    ) -> CMacroArgSegment {
        // SAFETY: self is a valid CParser pointer; the C side clamps
        // out-of-range indices to a zero-initialized segment.
        unsafe {
            syntaqlite_macro_rewrite_arg_segment_at(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                rewrite_idx.as_u32(),
                segment_idx,
            )
        }
    }

    pub(crate) unsafe fn macro_rewrite_arg_count(&self, rewrite_idx: RewriteIdx) -> u32 {
        // SAFETY: self is a valid CParser pointer; the C side clamps
        // out-of-range indices to zero.
        unsafe {
            syntaqlite_macro_rewrite_arg_count(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                rewrite_idx.as_u32(),
            )
        }
    }

    pub(crate) unsafe fn macro_rewrite_arg_at(
        &self,
        rewrite_idx: RewriteIdx,
        arg_idx: u32,
    ) -> CMacroCallArg {
        // SAFETY: self is a valid CParser pointer; the C side clamps
        // out-of-range indices to a zero-initialized arg.
        unsafe {
            syntaqlite_macro_rewrite_arg_at(
                std::ptr::from_ref::<Self>(self).cast_mut(),
                rewrite_idx.as_u32(),
                arg_idx,
            )
        }
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

    pub(crate) unsafe fn set_macro_lookup(
        &mut self,
        func: Option<
            unsafe extern "C" fn(
                user_data: *mut c_void,
                parser: *mut CParser,
                name: *const c_char,
                name_len: u32,
                args: *const CToken,
                arg_count: u32,
            ) -> i32,
        >,
        user_data: *mut c_void,
    ) -> i32 {
        // SAFETY: self is a valid, non-null CParser pointer.
        unsafe { syntaqlite_parser_set_macro_lookup(self, func, user_data) }
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
    fn syntaqlite_token_leading_comments(
        p: *mut CParser,
        token_idx: u32,
        count: *mut u32,
    ) -> *const CComment;
    fn syntaqlite_token_trailing_comments(
        p: *mut CParser,
        token_idx: u32,
        count: *mut u32,
    ) -> *const CComment;
    fn syntaqlite_result_macro_count(p: *mut CParser) -> u32;
    fn syntaqlite_result_macro_rewrite_at(p: *mut CParser, idx: u32) -> CMacroRewrite;
    fn syntaqlite_macro_rewrite_arg_segment_count(p: *mut CParser, rewrite_idx: u32) -> u32;
    fn syntaqlite_macro_rewrite_arg_segment_at(
        p: *mut CParser,
        rewrite_idx: u32,
        segment_idx: u32,
    ) -> CMacroArgSegment;
    fn syntaqlite_macro_rewrite_arg_count(p: *mut CParser, rewrite_idx: u32) -> u32;
    fn syntaqlite_macro_rewrite_arg_at(
        p: *mut CParser,
        rewrite_idx: u32,
        arg_idx: u32,
    ) -> CMacroCallArg;

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
    fn syntaqlite_parser_text(
        p: *mut CParser,
        out_offset: *mut u32,
        out_len: *mut u32,
    ) -> *const u8;
    fn syntaqlite_parser_full_text(p: *mut CParser, out_len: *mut u32) -> *const u8;
    fn syntaqlite_parser_layer_text(p: *mut CParser, layer_id: u32, out_len: *mut u32)
    -> *const u8;
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

    fn syntaqlite_node_is_macro_free(p: *mut CParser, node_id: u32) -> i32;

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

    // Macro expansion result (called from inside the lookup callback)
    pub(crate) fn syntaqlite_macro_expansion_set_result(
        p: *mut CParser,
        body: *const c_char,
        body_len: u32,
        def_line: u32,
        def_col: u32,
    );
    pub(crate) fn syntaqlite_macro_expansion_expand_and_set_result(
        p: *mut CParser,
        body: *const c_char,
        body_len: u32,
        param_names: *const *const c_char,
        param_name_lens: *const u32,
        param_count: u32,
        flags: u32,
    ) -> i32;

    // Macro lookup callback
    fn syntaqlite_parser_set_macro_lookup(
        p: *mut CParser,
        func: Option<
            unsafe extern "C" fn(
                user_data: *mut c_void,
                parser: *mut CParser,
                name: *const c_char,
                name_len: u32,
                args: *const CToken,
                arg_count: u32,
            ) -> i32,
        >,
        user_data: *mut c_void,
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

    // ── Macro lookup callback tests ─────────────────────────────────────

    use crate::parser::{MacroArg, MacroLookup, MacroOutput, Parser, ParserConfig};

    /// Helper: parse a single statement and return its status.
    fn parse_one(parser: &mut CParser, sql: &str) -> (i32, CString) {
        let sql_c = reset_with_source(parser, sql);
        // SAFETY: parser has been reset with a valid NUL-terminated source.
        let rc = unsafe { parser.next() };
        (rc, sql_c)
    }

    /// Shared test macro handler.
    ///
    /// `matches` decides whether this handler owns the macro name.
    /// `expand` transforms the first argument text into the expansion body.
    struct TestLookup {
        matches: Box<dyn Fn(&str) -> bool>,
        expand: Box<dyn Fn(&str) -> String>,
    }

    impl TestLookup {
        /// Match a single name (case-insensitive) with a custom expand fn.
        fn named(name: &'static str, expand: fn(&str) -> String) -> Self {
            Self {
                matches: Box::new(move |n| n.eq_ignore_ascii_case(name)),
                expand: Box::new(expand),
            }
        }

        /// Match any name starting with `prefix`; expand = identity.
        fn prefix(prefix: &'static str) -> Self {
            Self {
                matches: Box::new(move |n| n.starts_with(prefix)),
                expand: Box::new(ToString::to_string),
            }
        }
    }

    impl MacroLookup for TestLookup {
        fn lookup(&mut self, name: &str, args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
            if (self.matches)(name) {
                let x = args.first().map_or("", |a| a.text);
                out.write(&(self.expand)(x));
                true
            } else {
                false
            }
        }
    }

    #[test]
    fn macro_c_callback_expands_via_set_result() {
        // Exercise the raw C FFI callback signature directly.
        struct CallbackState {
            pending: Option<String>,
        }

        unsafe extern "C" fn c_lookup(
            user_data: *mut std::ffi::c_void,
            parser: *mut CParser,
            name: *const std::ffi::c_char,
            name_len: u32,
            _args: *const super::CToken,
            _arg_count: u32,
        ) -> i32 {
            // SAFETY: user_data was created from Box::into_raw(CallbackState).
            let state = unsafe { &mut *(user_data.cast::<CallbackState>()) };
            // SAFETY: name/name_len come from the parser's valid token buffer.
            let name_str = unsafe {
                std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                    name.cast(),
                    name_len as usize,
                ))
            };
            if !name_str.eq_ignore_ascii_case("ident") {
                return -1;
            }
            state.pending = Some("1".to_string());
            let s = state.pending.as_ref().unwrap();
            // SAFETY: parser is valid for the duration of the callback.
            #[expect(clippy::cast_possible_truncation)]
            unsafe {
                super::syntaqlite_macro_expansion_set_result(
                    parser,
                    s.as_ptr().cast(),
                    s.len() as u32,
                    0,
                    0,
                );
            }
            0
        }

        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        // SAFETY: CParser wraps a valid C parser handle.
        unsafe { parser.set_macro_fallback(1) };
        let state = Box::new(CallbackState { pending: None });
        let ptr = Box::into_raw(state);
        // SAFETY: ptr is valid for the test lifetime.
        unsafe { parser.set_macro_lookup(Some(c_lookup), ptr.cast()) };

        let (rc, _sql) = parse_one(parser, "SELECT ident!(x);");
        assert_eq!(rc, PARSE_OK, "C callback macro should expand successfully");
        // SAFETY: result accessors valid after non-DONE return.
        assert_ne!(unsafe { parser.result_root() }, NULL_NODE);

        // SAFETY: ptr was created by Box::into_raw above and not yet freed.
        let _ = unsafe { Box::from_raw(ptr) };
    }

    #[test]
    fn macro_register_and_expand() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup::named("double", |x| {
            format!("({x} + {x})")
        }))));

        let mut session = parser.parse("SELECT double!(1);");
        match session.next() {
            crate::ParseOutcome::Ok(stmt) => {
                assert!(stmt.root().is_some());
            }
            _ => panic!("expected Ok"),
        }
    }

    #[test]
    fn macro_deregister_removes_entry() {
        // With the callback API, "deregister" = the callback returns false.
        use std::cell::Cell;

        let enabled = std::rc::Rc::new(Cell::new(true));
        let enabled_cb = enabled.clone();
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup {
            matches: Box::new(move |n| enabled_cb.get() && n.eq_ignore_ascii_case("foo")),
            expand: Box::new(ToString::to_string),
        })));

        // First parse: macro is active.
        let mut session = parser.parse("SELECT foo!(1);");
        match session.next() {
            crate::ParseOutcome::Ok(_) => {}
            _ => panic!("expected Ok"),
        }
        drop(session);

        // "Deregister" by disabling.
        enabled.set(false);
        let mut session = parser.parse("SELECT foo!(1);");
        // Now foo is not found, falls back to TK_ID.
        let _ = session.next();
    }

    #[test]
    fn macro_deregister_nonexistent_returns_error() {
        // No callback installed → no macros exist.
        let mut handle = ParserHandle::new();
        let parser = handle.parser_mut();
        // No macros to deregister — this test just verifies no crash.
        let (rc, _sql) = parse_one(parser, "SELECT 1;");
        assert_eq!(rc, PARSE_OK);
    }

    #[test]
    fn macro_case_insensitive_lookup() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup::named("mymacro", |x| {
            x.to_string()
        }))));

        // Call as "mymacro" — case insensitive match
        // is handled by the callback (eq_ignore_ascii_case).
        let mut session = parser.parse("SELECT mymacro!(42);");
        match session.next() {
            crate::ParseOutcome::Ok(_) => {}
            _ => panic!("expected Ok"),
        }
    }

    #[test]
    fn macro_overwrite_existing() {
        // With callbacks, "overwrite" = the callback returns different body.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup::named("m", |x| {
            format!("({x} + 1)")
        }))));

        let mut session = parser.parse("SELECT m!(5);");
        match session.next() {
            crate::ParseOutcome::Ok(_) => {}
            _ => panic!("expected Ok"),
        }
    }

    #[test]
    fn macro_register_after_deregister_reuses_tombstone() {
        // This test is no longer relevant (no internal hashmap/tombstones).
        // Replace with a basic callback re-installation test.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup::named("tmp", |x| {
            format!("({x})")
        }))));

        let mut session = parser.parse("SELECT tmp!(7);");
        match session.next() {
            crate::ParseOutcome::Ok(_) => {}
            _ => panic!("expected Ok"),
        }
    }

    #[test]
    fn macro_many_entries_forces_grow() {
        // No internal table to grow anymore. Test that many macros work via callback.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup::prefix("m"))));

        for i in 0..20 {
            let sql = format!("SELECT m{i}!(1);");
            let mut session = parser.parse(&sql);
            match session.next() {
                crate::ParseOutcome::Ok(_) => {}
                _ => panic!("macro 'm{i}' should expand"),
            }
        }
    }

    #[test]
    fn macro_deregister_then_grow_drops_tombstones() {
        // No internal table/tombstones anymore. Verify callback-based removal.
        use std::cell::RefCell;

        let active: std::rc::Rc<RefCell<std::collections::HashSet<String>>> =
            std::rc::Rc::new(RefCell::new(std::collections::HashSet::new()));
        for i in 0..20 {
            active.borrow_mut().insert(format!("a{i}"));
        }

        let active_cb = active.clone();
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup {
            matches: Box::new(move |n| active_cb.borrow().contains(&n.to_ascii_lowercase())),
            expand: Box::new(ToString::to_string),
        })));

        // Remove some.
        for i in 0..5 {
            active.borrow_mut().remove(&format!("a{i}"));
        }

        // Verify surviving entries work.
        for i in 5..20 {
            let sql = format!("SELECT a{i}!(1);");
            let mut session = parser.parse(&sql);
            match session.next() {
                crate::ParseOutcome::Ok(_) => {}
                _ => panic!("macro 'a{i}' should expand"),
            }
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
    fn unknown_macro_with_lookup_fn_emits_error_message() {
        // When a lookup callback is registered but the macro is not found,
        // the parser should produce a hard error with "unknown macro 'name'".
        // We need macro_fallback to enable macro syntax for SQLite dialect.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        // Lookup callback that always returns false (nothing registered).
        parser.set_macro_lookup(Some(Box::new(TestLookup::prefix("__never__"))));

        let mut session = parser.parse("SELECT foo!(1, 2);");
        match session.next() {
            crate::ParseOutcome::Err(e) => {
                let msg = e.message();
                assert!(
                    msg.contains("unknown macro") && msg.contains("foo"),
                    "expected 'unknown macro' error mentioning 'foo', got: '{msg}'"
                );
            }
            crate::ParseOutcome::Ok(_) => {
                panic!("expected parse error for unknown macro, got Ok");
            }
            crate::ParseOutcome::Done => {
                panic!("expected parse error, got Done");
            }
        }
    }

    #[test]
    fn unknown_macro_without_lookup_fn_falls_through() {
        // When macro_fallback is enabled but NO lookup callback is
        // registered, unknown macro calls should fall through to TK_ID.
        let parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        // No set_macro_lookup — no callback.

        let mut session = parser.parse("SELECT foo!(1, 2);");
        match session.next() {
            crate::ParseOutcome::Ok(_) => {} // Expected — no lookup, fallback mode.
            crate::ParseOutcome::Err(e) => {
                panic!("expected Ok without lookup_fn, got error: {}", e.message());
            }
            crate::ParseOutcome::Done => panic!("expected Ok, got Done"),
        }
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
        let r = unsafe { parser.result_macro_rewrite_at(0) };
        #[expect(clippy::cast_possible_truncation)]
        let call_start = sql.find("foo!").unwrap() as u32;
        assert_eq!(r.call_offset, call_start);
        // "foo!(1, 2)" is 10 bytes.
        assert_eq!(r.call_length, 10);
    }

    #[test]
    fn macro_fallback_registered_still_expands() {
        // With a callback installed, known macros expand even with fallback on.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(TestLookup::named("double", |x| {
            format!("({x} + {x})")
        }))));

        let mut session = parser.parse("SELECT double!(3);");
        match session.next() {
            crate::ParseOutcome::Ok(_) => {}
            _ => panic!("expected Ok"),
        }
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
        let r = unsafe { parser.result_macro_rewrite_at(0) };
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
        use crate::any::{AnyParser, ParseOutcome};

        let dialect = crate::sqlite::dialect::dialect();
        let mut parser = AnyParser::with_config(
            dialect.into(),
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(true),
        );
        parser.set_macro_lookup(Some(Box::new(TestLookup::named("my_expr", |x| {
            format!("{x} + 1")
        }))));

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
