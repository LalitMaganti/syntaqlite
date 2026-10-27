// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![cfg_attr(test, expect(clippy::unwrap_used))]

//! C API for syntaqlite, built as `libsyntaqlite`.
//!
//! Exports the formatter, analyzer, and JSON-RPC entry points declared in
//! `include/syntaqlite/*.h`. Kept out of the `syntaqlite` crate so Rust users
//! of that crate do not also export these C symbols.

use std::ffi::c_char;

#[expect(unreachable_pub)]
mod analyzer;
#[cfg(feature = "sqlite")]
#[expect(unreachable_pub)]
mod formatter;
mod rpc;

/// Borrow a C `(source, len)` buffer as a `&str`, treating NULL as "".
///
/// # Safety
///
/// If `source` is non-NULL it must point to `len` bytes of valid UTF-8
/// that stay alive and unmodified for `'a`.
pub(crate) unsafe fn source_str<'a>(source: *const c_char, len: u32) -> &'a str {
    if source.is_null() {
        return "";
    }
    // SAFETY: caller guarantees non-NULL `source` points to `len` bytes
    // of valid UTF-8 that live for `'a`.
    unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(source.cast(), len as usize))
    }
}
