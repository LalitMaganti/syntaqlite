// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C FFI layer for the SQL formatter.
//!
//! Exposes [`Formatter`] to C via an opaque handle + accessor functions,
//! following the same pattern as the parser and validator FFI.

use std::ffi::{CString, c_char};

use super::formatter::Formatter;
use super::{FormatConfig, KeywordCase};

// ── C-compatible config struct ────────────────────────────────────────────

/// Mirrors `SyntaqliteFormatConfig` from the C header.
#[repr(C)]
pub struct SyntaqliteFormatConfig {
    pub line_width: u32,
    pub indent_width: u32,
    pub keyword_case: u32,
    pub semicolons: u32,
}

// ── Opaque handle ─────────────────────────────────────────────────────────

/// Internal state behind the opaque C handle.
struct FormatterState {
    formatter: Formatter,
    /// Formatted output from the last successful `format()` call.
    /// `None` when no successful call has been made yet (or after an error).
    last_output: Option<CString>,
    /// Error message from the last failed `format()` call.
    /// `None` when no error has occurred (or after a success).
    last_error: Option<CString>,
}

/// Opaque C handle — the pointer target of `SyntaqliteFormatter*`.
///
/// Zero-variant enum so Rust cannot construct it directly; all access goes
/// through raw pointer casts to `FormatterState`.
pub enum SyntaqliteFormatter {}

impl SyntaqliteFormatter {
    fn state(&self) -> &FormatterState {
        // SAFETY: `self` was created from a `Box<FormatterState>` via
        // `Box::into_raw` cast in a `syntaqlite_formatter_create_*` function.
        unsafe { &*std::ptr::from_ref::<Self>(self).cast::<FormatterState>() }
    }

    fn state_mut(&mut self) -> &mut FormatterState {
        // SAFETY: same as above.
        unsafe { &mut *std::ptr::from_mut::<Self>(self).cast::<FormatterState>() }
    }
}

// ── Config conversion ─────────────────────────────────────────────────────

fn config_from_c(c: &SyntaqliteFormatConfig) -> FormatConfig {
    FormatConfig {
        line_width: c.line_width as usize,
        indent_width: c.indent_width as usize,
        keyword_case: match c.keyword_case {
            1 => KeywordCase::Lower,
            _ => KeywordCase::Upper,
        },
        semicolons: c.semicolons != 0,
    }
}

fn new_state(formatter: Formatter) -> *mut SyntaqliteFormatter {
    let state = Box::new(FormatterState {
        formatter,
        last_output: None,
        last_error: None,
    });
    Box::into_raw(state).cast::<SyntaqliteFormatter>()
}

// ── Exported C functions ──────────────────────────────────────────────────

/// Free a formatter. No-op if `f` is NULL.
///
/// # Safety
///
/// `f` must be NULL or a valid pointer from `syntaqlite_formatter_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_formatter_destroy(f: *mut SyntaqliteFormatter) {
    if !f.is_null() {
        // SAFETY: `f` was created from `Box::into_raw` in a `create_*` function.
        drop(unsafe { Box::from_raw(f.cast::<FormatterState>()) });
    }
}

/// Format a SQL source string.
///
/// Returns 0 on success, -1 on error (parse failure).
///
/// On success, the formatted output is available via
/// `syntaqlite_formatter_output()` / `syntaqlite_formatter_output_len()`.
/// `syntaqlite_formatter_error_msg()` returns NULL.
///
/// On error, the error message is available via
/// `syntaqlite_formatter_error_msg()`.
/// `syntaqlite_formatter_output()` returns NULL and
/// `syntaqlite_formatter_output_len()` returns 0.
///
/// # Safety
///
/// - `f` must be a valid pointer from `syntaqlite_formatter_create_*`.
/// - `source` must point to `len` bytes of valid UTF-8.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_formatter_format(
    f: *mut SyntaqliteFormatter,
    source: *const c_char,
    len: u32,
) -> i32 {
    // SAFETY: caller guarantees `f` is valid.
    let f = unsafe { &mut *f };
    let state = f.state_mut();

    // SAFETY: caller guarantees `source` points to `len` bytes of valid UTF-8.
    let src = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(source.cast(), len as usize))
    };

    match state.formatter.format(src) {
        Ok(output) => {
            state.last_output = Some(CString::new(output).unwrap_or_default());
            state.last_error = None;
            0
        }
        Err(e) => {
            state.last_output = None;
            state.last_error = Some(CString::new(e.message).unwrap_or_default());
            -1
        }
    }
}

/// Pointer to the formatted output from the last successful `format()` call.
/// Returns a NUL-terminated UTF-8 string, or NULL if the last call failed
/// or `format()` has not been called.
///
/// The pointer is valid until the next `format()` or `destroy()` call.
///
/// # Safety
///
/// `f` must be a valid pointer from `syntaqlite_formatter_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_formatter_output(
    f: *const SyntaqliteFormatter,
) -> *const c_char {
    // SAFETY: caller guarantees `f` is valid.
    let f = unsafe { &*f };
    match &f.state().last_output {
        Some(s) => s.as_ptr(),
        None => std::ptr::null(),
    }
}

/// Length in bytes of the formatted output (excluding NUL terminator).
/// Returns 0 if the last call failed or `format()` has not been called.
///
/// # Safety
///
/// `f` must be a valid pointer from `syntaqlite_formatter_create_*`.
#[unsafe(no_mangle)]
#[expect(clippy::cast_possible_truncation)]
pub unsafe extern "C" fn syntaqlite_formatter_output_len(f: *const SyntaqliteFormatter) -> u32 {
    // SAFETY: caller guarantees `f` is valid.
    let f = unsafe { &*f };
    match &f.state().last_output {
        Some(s) => s.as_bytes().len() as u32,
        None => 0,
    }
}

/// Error message from the last failed `format()` call.
/// Returns a NUL-terminated UTF-8 string, or NULL if the last call succeeded
/// or `format()` has not been called.
///
/// The pointer is valid until the next `format()` or `destroy()` call.
///
/// # Safety
///
/// `f` must be a valid pointer from `syntaqlite_formatter_create_*`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_formatter_error_msg(
    f: *const SyntaqliteFormatter,
) -> *const c_char {
    // SAFETY: caller guarantees `f` is valid.
    let f = unsafe { &*f };
    match &f.state().last_error {
        Some(s) => s.as_ptr(),
        None => std::ptr::null(),
    }
}

// ── Dialect-generic constructor ────────────────────────────────────────────

/// Create a formatter for any dialect with optional config.
///
/// # Safety
///
/// - `dialect.tmpl` must point to a valid `SyntaqliteDialectTemplate`.
/// - `config` must be NULL or a valid pointer to a `SyntaqliteFormatConfig`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn syntaqlite_formatter_create_with_dialect(
    dialect: crate::dialect::ffi::CDialect,
    config: *const SyntaqliteFormatConfig,
) -> *mut SyntaqliteFormatter {
    // SAFETY: caller guarantees `dialect.tmpl` is valid.
    let any = unsafe { crate::dialect::AnyDialect::from_c_dialect(dialect) };
    let cfg = if config.is_null() {
        FormatConfig::default()
    } else {
        // SAFETY: caller guarantees `config` is valid.
        config_from_c(unsafe { &*config })
    };
    new_state(Formatter::with_dialect_config(any, &cfg))
}

// ── SQLite convenience (feature = "sqlite") ───────────────────────────────

/// Create a formatter for the built-in `SQLite` dialect with default config.
#[cfg(feature = "sqlite")]
#[unsafe(no_mangle)]
pub extern "C" fn syntaqlite_formatter_create_sqlite() -> *mut SyntaqliteFormatter {
    new_state(Formatter::new())
}

/// Create a formatter for the built-in `SQLite` dialect with custom config.
#[cfg(feature = "sqlite")]
#[unsafe(no_mangle)]
pub extern "C" fn syntaqlite_formatter_create_sqlite_with_config(
    config: *const SyntaqliteFormatConfig,
) -> *mut SyntaqliteFormatter {
    // SAFETY: caller guarantees `config` is a valid pointer.
    let cfg = config_from_c(unsafe { &*config });
    new_state(Formatter::with_config(&cfg))
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use super::*;
    use std::ffi::CStr;

    /// Helper: call format and return the output string (panics on error).
    unsafe fn format_ok(f: *mut SyntaqliteFormatter, sql: &str) -> String {
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe {
            let rc = syntaqlite_formatter_format(
                f,
                sql.as_ptr().cast(),
                u32::try_from(sql.len()).unwrap(),
            );
            assert_eq!(rc, 0, "format() returned error for: {sql}");
            let ptr = syntaqlite_formatter_output(f);
            assert!(!ptr.is_null());
            CStr::from_ptr(ptr).to_str().unwrap().to_owned()
        }
    }

    /// Helper: call format and return the error string (panics on success).
    unsafe fn format_err(f: *mut SyntaqliteFormatter, sql: &str) -> String {
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe {
            let rc = syntaqlite_formatter_format(
                f,
                sql.as_ptr().cast(),
                u32::try_from(sql.len()).unwrap(),
            );
            assert_eq!(rc, -1, "format() succeeded unexpectedly for: {sql}");
            let ptr = syntaqlite_formatter_error_msg(f);
            assert!(!ptr.is_null());
            CStr::from_ptr(ptr).to_str().unwrap().to_owned()
        }
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────

    #[test]
    fn null_destroy_is_noop() {
        // SAFETY: FFI test — passing null pointer, which is explicitly a no-op.
        unsafe { syntaqlite_formatter_destroy(std::ptr::null_mut()) };
    }

    #[test]
    fn before_first_format_output_and_error_are_null() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(unsafe { syntaqlite_formatter_output(f) }.is_null());
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(unsafe { syntaqlite_formatter_error_msg(f) }.is_null());
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert_eq!(unsafe { syntaqlite_formatter_output_len(f) }, 0);
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    // ── Success path ──────────────────────────────────────────────────────

    #[test]
    fn format_simple_select() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "select 1") };
        assert_eq!(out, "SELECT 1;\n");

        // error_msg must be NULL after success.
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(unsafe { syntaqlite_formatter_error_msg(f) }.is_null());
        // output_len must match.
        assert_eq!(
            // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
            usize::try_from(unsafe { syntaqlite_formatter_output_len(f) }).unwrap(),
            out.len()
        );

        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn format_multi_statement() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "select 1; select 2") };
        assert!(out.contains("SELECT 1;"), "missing first stmt: {out}");
        assert!(out.contains("SELECT 2;"), "missing second stmt: {out}");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn format_preserves_comments() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "-- hello\nselect 1") };
        assert!(out.contains("-- hello"), "comment lost: {out}");
        assert!(out.contains("SELECT"), "keyword not uppercased: {out}");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn format_empty_input() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "") };
        assert_eq!(out, "");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn format_whitespace_only() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "   \n\n  ") };
        assert_eq!(out, "");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    // ── Error path ────────────────────────────────────────────────────────

    #[test]
    fn format_error_returns_message_and_nulls_output() {
        let f = syntaqlite_formatter_create_sqlite();
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let err = unsafe { format_err(f, "SELECT FROM") };
        assert!(!err.is_empty());

        // output must be NULL on error.
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(unsafe { syntaqlite_formatter_output(f) }.is_null());
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert_eq!(unsafe { syntaqlite_formatter_output_len(f) }, 0);

        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    // ── State transitions ─────────────────────────────────────────────────

    #[test]
    fn success_after_error_clears_error() {
        let f = syntaqlite_formatter_create_sqlite();

        // First: error.
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { format_err(f, "SELECT FROM") };
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(!unsafe { syntaqlite_formatter_error_msg(f) }.is_null());

        // Second: success — error must be cleared.
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "SELECT 1") };
        assert_eq!(out, "SELECT 1;\n");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(unsafe { syntaqlite_formatter_error_msg(f) }.is_null());

        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn error_after_success_clears_output() {
        let f = syntaqlite_formatter_create_sqlite();

        // First: success.
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { format_ok(f, "SELECT 1") };
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(!unsafe { syntaqlite_formatter_output(f) }.is_null());

        // Second: error — output must be cleared.
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { format_err(f, "SELECT FROM") };
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert!(unsafe { syntaqlite_formatter_output(f) }.is_null());
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        assert_eq!(unsafe { syntaqlite_formatter_output_len(f) }, 0);

        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn reuse_across_multiple_successful_calls() {
        let f = syntaqlite_formatter_create_sqlite();
        for i in 1..=5 {
            let sql = format!("SELECT {i}");
            // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
            let out = unsafe { format_ok(f, &sql) };
            assert!(out.contains(&format!("SELECT {i};")), "iter {i}: {out}");
        }
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    // ── Config ────────────────────────────────────────────────────────────

    #[test]
    fn config_keyword_lower() {
        let config = SyntaqliteFormatConfig {
            line_width: 80,
            indent_width: 2,
            keyword_case: 1, // Lower
            semicolons: 1,
        };
        let f = syntaqlite_formatter_create_sqlite_with_config(&raw const config);
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "SELECT 1 FROM x") };
        assert!(out.contains("select"), "expected lowercase keywords: {out}");
        assert!(out.contains("from"), "expected lowercase FROM: {out}");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn config_no_semicolons() {
        let config = SyntaqliteFormatConfig {
            line_width: 80,
            indent_width: 2,
            keyword_case: 0, // Upper
            semicolons: 0,   // No semicolons
        };
        let f = syntaqlite_formatter_create_sqlite_with_config(&raw const config);
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "SELECT 1") };
        assert!(!out.contains(';'), "semicolon should be omitted: {out}");
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }

    #[test]
    fn config_narrow_line_width_forces_break() {
        let config = SyntaqliteFormatConfig {
            line_width: 10, // Very narrow
            indent_width: 2,
            keyword_case: 0,
            semicolons: 1,
        };
        let f = syntaqlite_formatter_create_sqlite_with_config(&raw const config);
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        let out = unsafe { format_ok(f, "SELECT column_a, column_b, column_c FROM my_table") };
        // With line_width=10, the formatter must break across multiple lines.
        let line_count = out.lines().count();
        assert!(
            line_count > 1,
            "expected line breaks with narrow width: {out}"
        );
        // SAFETY: FFI test — pointer obtained from `syntaqlite_formatter_create_sqlite`.
        unsafe { syntaqlite_formatter_destroy(f) };
    }
}
