// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.
#![allow(missing_docs)] // ABI exports don't need rustdoc
#![cfg_attr(test, expect(clippy::unwrap_used))]

//! WASM ABI for syntaqlite.
//!
//! All stateful entry points operate on a *session* identified by an opaque
//! `u32` handle from [`wasm_session_new`]. A session owns its dialect,
//! `SQLite` version/cflag overrides, and LSP analysis state, so multiple
//! independent sessions (e.g. two editors with different schemas) can
//! coexist in one instance. Handle `0` is never valid.
//!
//! Calls return a status (`0` ok, negative on error; query calls return a
//! non-negative count on success) and write their payload — or the error
//! message — to a shared result buffer read via [`wasm_result_ptr`] /
//! [`wasm_result_len`] and released with [`wasm_result_free`].
//!
//! All editor features (diagnostics, completions, semantic tokens, schema
//! session context, ...) are served over LSP JSON-RPC via
//! [`wasm_lsp_message`]. The remaining direct calls are one-shot utilities
//! (format, AST dump, cflag list) and the experimental embedded analyzers.

use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::slice;

use serde::Serialize;

use syntaqlite::any::AnyDialect;
use syntaqlite::fmt::KeywordCase;
use syntaqlite::lsp::LspDispatcher;
use syntaqlite::util::{SqliteFlag, SqliteFlags, SqliteVersion};
use syntaqlite::{FormatConfig, Formatter};

// ── Session ──────────────────────────────────────────────────────────

/// Per-session state: dialect (with version/cflag overrides layered on the
/// loaded template) and the lazily built language server derived from it.
struct Session {
    /// Raw pointer to the active `SyntaqliteDialectTemplate`, retained so
    /// version/cflag overrides can rebuild the dialect without reloading
    /// the side module. `0` = no dialect loaded.
    dialect_template: u32,
    version: SqliteVersion,
    cflags: SqliteFlags,
    dialect: Option<AnyDialect>,
    /// Language server behind [`wasm_lsp_message`]; lazily created from
    /// `dialect` and dropped whenever the dialect changes.
    lsp: Option<LspDispatcher>,
}

impl Session {
    fn new() -> Self {
        Session {
            dialect_template: 0,
            version: SqliteVersion::Latest,
            cflags: SqliteFlags::default(),
            dialect: None,
            lsp: None,
        }
    }

    /// Rebuild the dialect from the stored template pointer, applying the
    /// session's version/cflag overrides. Drops the language server, which
    /// is bound to the old dialect. No-op when no side module is loaded yet.
    fn rebuild_dialect(&mut self) {
        self.lsp = None;
        if self.dialect_template == 0 {
            self.dialect = None;
            return;
        }
        // SAFETY: the pointer was validated in run_set_dialect when stored.
        let dialect = unsafe {
            AnyDialect::from_c_dialect_ptr(
                self.dialect_template as *const syntaqlite::any::ffi::CDialectTemplate,
            )
        }
        .with_version(self.version)
        .with_cflags(self.cflags);
        self.dialect = Some(dialect);
    }

    fn dialect(&self) -> Result<AnyDialect, String> {
        self.dialect
            .clone()
            .ok_or_else(|| "no dialect loaded: call wasm_set_dialect first".to_string())
    }

    fn lsp(&mut self) -> Result<&mut LspDispatcher, String> {
        if self.lsp.is_none() {
            self.lsp = Some(LspDispatcher::new(self.dialect()?));
        }
        Ok(self.lsp.as_mut().expect("lsp dispatcher just created"))
    }
}

thread_local! {
    static RESULT_BUF: RefCell<Vec<u8>> = const { RefCell::new(Vec::new()) };
    static SESSIONS: RefCell<HashMap<u32, Session>> = RefCell::new(HashMap::new());
    static NEXT_SESSION: Cell<u32> = const { Cell::new(1) };
}

/// Run `f` against the session for `handle`, or report an invalid handle.
/// Returns `-1` on a bad handle, which is an error for every call convention
/// (setters expect `0`, queries expect a non-negative count).
fn with_session(handle: u32, f: impl FnOnce(&mut Session) -> i32) -> i32 {
    SESSIONS.with(|cell| {
        let mut sessions = cell.borrow_mut();
        if let Some(session) = sessions.get_mut(&handle) {
            f(session)
        } else {
            set_result("invalid session handle");
            -1
        }
    })
}

fn session_new() -> u32 {
    let handle = NEXT_SESSION.with(|next| {
        let handle = next.get();
        next.set(handle.wrapping_add(1).max(1));
        handle
    });
    SESSIONS.with(|cell| cell.borrow_mut().insert(handle, Session::new()));
    handle
}

fn session_free(handle: u32) {
    SESSIONS.with(|cell| cell.borrow_mut().remove(&handle));
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_session_new() -> u32 {
    catch_unwind_u32(session_new, "wasm_session_new panicked")
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_session_free(handle: u32) {
    session_free(handle);
}

// ── Result buffer / input marshalling ────────────────────────────────

fn set_result(text: &str) {
    RESULT_BUF.with(|buf| {
        let mut buf = buf.borrow_mut();
        buf.clear();
        buf.extend_from_slice(text.as_bytes());
    });
}

fn set_result_u32s(data: &[u32]) {
    RESULT_BUF.with(|buf| {
        let mut buf = buf.borrow_mut();
        buf.clear();
        // SAFETY: u32 has no invalid bit patterns; reinterpreting as bytes is safe.
        let bytes = unsafe { slice::from_raw_parts(data.as_ptr().cast::<u8>(), data.len() * 4) };
        buf.extend_from_slice(bytes);
    });
}

fn decode_input(ptr: u32, len: u32) -> Result<String, String> {
    if len == 0 {
        return Ok(String::new());
    }
    if ptr == 0 {
        return Err("null input pointer".to_string());
    }
    // SAFETY: caller provides pointer/length in this module's linear memory.
    let bytes = unsafe { slice::from_raw_parts(ptr as *const u8, len as usize) };
    let source = std::str::from_utf8(bytes).map_err(|e| format!("invalid UTF-8 input: {e}"))?;
    Ok(source.to_string())
}

/// Installs a panic hook that prints the panic location and message to stderr
/// (which emscripten routes to `console.error`). Without this the default hook
/// runs, but the message can get swallowed by the nounwind guard inserted at
/// `extern "C"` boundaries, leaving the JS side with only a generic wasm trap.
/// `main` is not called by the emscripten loader for `MAIN_MODULE` builds, so
/// we install lazily on first export call.
fn install_panic_hook() {
    use std::sync::Once;
    static ONCE: Once = Once::new();
    ONCE.call_once(|| {
        std::panic::set_hook(Box::new(|info| {
            let loc = info.location().map_or_else(
                || "unknown location".to_string(),
                |l| format!("{}:{}:{}", l.file(), l.line(), l.column()),
            );
            let msg = info
                .payload()
                .downcast_ref::<&str>()
                .copied()
                .or_else(|| info.payload().downcast_ref::<String>().map(String::as_str))
                .unwrap_or("<non-string panic payload>");
            eprintln!("syntaqlite-wasm panic at {loc}: {msg}");
        }));
    });
}

/// Runs `f`, catching any panic and writing `msg` to the result buffer on failure.
fn catch_unwind<F: FnOnce() -> i32>(f: F, msg: &'static str) -> i32 {
    install_panic_hook();
    if let Ok(result) = std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        result
    } else {
        set_result(msg);
        -1
    }
}

/// Like [`catch_unwind`] but for exports returning `u32`; `0` on panic.
fn catch_unwind_u32<F: FnOnce() -> u32>(f: F, msg: &'static str) -> u32 {
    install_panic_hook();
    if let Ok(result) = std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        result
    } else {
        set_result(msg);
        0
    }
}

/// Unwraps a `Result`, writing the error to the result buffer and returning `$code` on failure.
macro_rules! try_wasm {
    ($expr:expr) => {
        try_wasm!($expr, -1)
    };
    ($expr:expr, $code:expr) => {
        match $expr {
            Ok(val) => val,
            Err(e) => {
                set_result(&e.to_string());
                return $code;
            }
        }
    };
}

// ── Memory management ────────────────────────────────────────────────

fn alloc(len: u32) -> u32 {
    if len == 0 {
        return 0;
    }
    let mut buf = Vec::<u8>::with_capacity(len as usize);
    let ptr = buf.as_mut_ptr();
    std::mem::forget(buf);
    ptr as u32
}

fn free(ptr: u32, len: u32) {
    if ptr == 0 || len == 0 {
        return;
    }
    #[expect(
        clippy::same_length_and_capacity,
        reason = "intentional: capacity equals length for dealloc"
    )]
    // SAFETY: pointer/capacity pair must come from alloc(). len == cap since alloc
    // allocates exactly `len` bytes and we use it as both length and capacity here.
    unsafe {
        let _ = Vec::<u8>::from_raw_parts(ptr as *mut u8, len as usize, len as usize);
    }
}

fn result_ptr() -> u32 {
    RESULT_BUF.with(|buf| {
        let buf = buf.borrow();
        if buf.is_empty() {
            0
        } else {
            buf.as_ptr() as u32
        }
    })
}

fn result_len() -> u32 {
    RESULT_BUF.with(|buf| u32::try_from(buf.borrow().len()).expect("result length fits u32"))
}

fn result_free() {
    RESULT_BUF.with(|buf| {
        let mut buf = buf.borrow_mut();
        buf.clear();
        buf.shrink_to_fit();
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_alloc(len: u32) -> u32 {
    alloc(len)
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_free(ptr: u32, len: u32) {
    free(ptr, len);
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_result_ptr() -> u32 {
    result_ptr()
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_result_len() -> u32 {
    result_len()
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_result_free() {
    result_free();
}

// ── AST JSON ─────────────────────────────────────────────────────────

fn run_ast_json(session: &mut Session, ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let dialect = try_wasm!(session.dialect());
    let grammar = (*dialect).clone();
    let parser = syntaqlite::any::AnyParser::with_config(
        grammar,
        &syntaqlite::parse::ParserConfig::default(),
    );
    let mut session = parser.parse(&source);
    let mut nodes: Vec<serde_json::Value> = Vec::new();
    loop {
        match session.next() {
            syntaqlite::any::ParseOutcome::Done => break,
            syntaqlite::any::ParseOutcome::Ok(stmt) => {
                let val = stmt
                    .erase()
                    .root_node()
                    .map_or(serde_json::Value::Null, |n| {
                        serde_json::to_value(n).unwrap_or(serde_json::Value::Null)
                    });
                nodes.push(val);
            }
            syntaqlite::any::ParseOutcome::Err(_) => {}
        }
    }
    let count = i32::try_from(nodes.len()).expect("node count fits i32");
    set_result(&serde_json::to_string(&nodes).expect("ast json serialization failed"));
    count
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_ast_json(handle: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_ast_json(s, ptr, len)),
        "wasm_ast_json panicked",
    )
}

// ── Formatter ────────────────────────────────────────────────────────

fn run_fmt(
    session: &mut Session,
    ptr: u32,
    len: u32,
    line_width: u32,
    indent_width: u32,
    keyword_case: u32,
    semicolons: u32,
) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let config = FormatConfig::default()
        .with_line_width(if line_width == 0 {
            80
        } else {
            line_width as usize
        })
        .with_indent_width(if indent_width == 0 {
            2
        } else {
            indent_width as usize
        })
        .with_keyword_case(match keyword_case {
            2 => KeywordCase::Lower,
            _ => KeywordCase::Upper,
        })
        .with_semicolons(semicolons != 0);
    let dialect = try_wasm!(session.dialect());
    let mut formatter = Formatter::with_dialect_config(dialect, &config);
    let sql = try_wasm!(formatter.format(&source).map_err(|e| e.to_string()));
    set_result(&sql);
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_fmt(
    handle: u32,
    ptr: u32,
    len: u32,
    line_width: u32,
    indent_width: u32,
    keyword_case: u32,
    semicolons: u32,
) -> i32 {
    catch_unwind(
        || {
            with_session(handle, |s| {
                run_fmt(
                    s,
                    ptr,
                    len,
                    line_width,
                    indent_width,
                    keyword_case,
                    semicolons,
                )
            })
        },
        "wasm_fmt panicked",
    )
}

// ── LSP JSON-RPC ─────────────────────────────────────────────────────

/// Join already-serialized JSON-RPC messages into one JSON array.
fn join_messages(messages: &[String]) -> String {
    format!("[{}]", messages.join(","))
}

fn run_lsp_message(session: &mut Session, ptr: u32, len: u32) -> i32 {
    let input = try_wasm!(decode_input(ptr, len));
    let lsp = try_wasm!(session.lsp());
    let out = lsp.handle_json(&input);
    let count = i32::try_from(out.len()).expect("message count fits i32");
    set_result(&join_messages(&out));
    count
}

/// Handle one LSP JSON-RPC message against the session's language server.
/// The result buffer receives a JSON array of outgoing messages (response
/// plus server-initiated notifications); returns their count, negative on
/// error. The session outlives `exit` and can be re-initialized.
#[unsafe(no_mangle)]
pub extern "C" fn wasm_lsp_message(handle: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_lsp_message(s, ptr, len)),
        "wasm_lsp_message panicked",
    )
}

// ── Dialect switching ────────────────────────────────────────────────

fn run_set_dialect(session: &mut Session, ptr: u32) -> i32 {
    if ptr == 0 {
        set_result("null dialect pointer");
        return 1;
    }
    session.dialect_template = ptr;
    session.rebuild_dialect();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_dialect(handle: u32, ptr: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_set_dialect(s, ptr)),
        "wasm_set_dialect panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_dialect(handle: u32) {
    with_session(handle, |s| {
        s.dialect_template = 0;
        s.rebuild_dialect();
        0
    });
}

// ── Cflag list ───────────────────────────────────────────────────────

#[derive(Serialize)]
struct CflagListEntry {
    name: &'static str,
    #[serde(rename = "minVersion")]
    min_version: u32,
    category: &'static str,
}

fn cflag_list_json() -> String {
    let entries: Vec<CflagListEntry> = SqliteFlag::all()
        .iter()
        .map(|&flag| CflagListEntry {
            name: flag.name(),
            min_version: flag.min_version().as_int().cast_unsigned(),
            category: flag.categories().first().copied().unwrap_or("parser"),
        })
        .collect();
    serde_json::to_string(&entries).expect("cflag list serialization failed")
}

fn run_get_cflag_list() -> i32 {
    set_result(&cflag_list_json());
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_get_cflag_list() -> i32 {
    catch_unwind(run_get_cflag_list, "wasm_get_cflag_list panicked")
}

// ── Version / cflag overrides ─────────────────────────────────────────
//
// These configure the session's dialect with a target SQLite version or
// compile-time flags. Overrides persist across `wasm_set_dialect` calls on
// the same session and are re-applied when the dialect template changes.

fn run_set_sqlite_version(session: &mut Session, ptr: u32, len: u32) -> i32 {
    let s = try_wasm!(decode_input(ptr, len));
    let version = try_wasm!(SqliteVersion::parse_with_latest(&s));
    session.version = version;
    session.rebuild_dialect();
    0
}

fn run_set_cflag(session: &mut Session, ptr: u32, len: u32) -> i32 {
    let s = try_wasm!(decode_input(ptr, len));
    let flag = try_wasm!(SqliteFlag::from_name(&s).ok_or_else(|| format!("unknown cflag: {s}")));
    session.cflags = std::mem::take(&mut session.cflags).with(flag);
    session.rebuild_dialect();
    0
}

fn run_clear_cflag(session: &mut Session, ptr: u32, len: u32) -> i32 {
    let s = try_wasm!(decode_input(ptr, len));
    let flag = try_wasm!(SqliteFlag::from_name(&s).ok_or_else(|| format!("unknown cflag: {s}")));
    session.cflags = std::mem::take(&mut session.cflags).without(flag);
    session.rebuild_dialect();
    0
}

fn run_clear_all_cflags(session: &mut Session) -> i32 {
    session.cflags = SqliteFlags::default();
    session.rebuild_dialect();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_sqlite_version(handle: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_set_sqlite_version(s, ptr, len)),
        "wasm_set_sqlite_version panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_cflag(handle: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_set_cflag(s, ptr, len)),
        "wasm_set_cflag panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_cflag(handle: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_clear_cflag(s, ptr, len)),
        "wasm_clear_cflag panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_all_cflags(handle: u32) -> i32 {
    catch_unwind(
        || with_session(handle, run_clear_all_cflags),
        "wasm_clear_all_cflags panicked",
    )
}

// ── Embedded SQL WASM exports (experimental) ─────────────────────────
//
// lang encoding: 0 = Python, 1 = TypeScript/JavaScript. The language is an
// explicit parameter on every call — there is no mode state.
// NOTE: Embedded language support is experimental and may change.

use syntaqlite::embedded::{EmbeddedAnalyzer, EmbeddedFragment};

fn embedded_fragments(lang: u32, source: &str) -> Result<Vec<EmbeddedFragment>, String> {
    match lang {
        0 => Ok(syntaqlite::embedded::extract_python(source)),
        1 => Ok(syntaqlite::embedded::extract_typescript(source)),
        _ => Err(format!("unknown host language id: {lang}")),
    }
}

#[derive(Serialize)]
struct WasmHole {
    start: usize,
    end: usize,
}

#[derive(Serialize)]
struct WasmFragment {
    start: usize,
    end: usize,
    sql: String,
    holes: Vec<WasmHole>,
}

fn run_embedded_extract(lang: u32, ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let fragments = try_wasm!(embedded_fragments(lang, &source));
    let count = i32::try_from(fragments.len()).expect("fragment count fits i32");
    let items: Vec<WasmFragment> = fragments
        .iter()
        .map(|f| WasmFragment {
            start: f.sql_range().start.as_usize(),
            end: f.sql_range().end.as_usize(),
            sql: f.sql_text().to_string(),
            holes: f
                .holes()
                .iter()
                .map(|h| WasmHole {
                    start: h.host_range().start.as_usize(),
                    end: h.host_range().end.as_usize(),
                })
                .collect(),
        })
        .collect();
    set_result(&serde_json::to_string(&items).expect("fragment serialization failed"));
    count
}

fn run_embedded_diagnostics(session: &mut Session, lang: u32, ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let fragments = try_wasm!(embedded_fragments(lang, &source));
    let dialect = try_wasm!(session.dialect());
    let diags = EmbeddedAnalyzer::new(dialect).analyze(&fragments);
    let count = i32::try_from(diags.len()).expect("diag count fits i32");
    set_result(&serde_json::to_string(&diags).expect("embedded diagnostic serialization failed"));
    count
}

fn run_embedded_semantic_tokens(session: &mut Session, lang: u32, ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let fragments = try_wasm!(embedded_fragments(lang, &source));
    let dialect = try_wasm!(session.dialect());
    let encoded = EmbeddedAnalyzer::new(dialect).semantic_tokens_encoded(&fragments, &source);
    let token_count = i32::try_from(encoded.len() / 5).expect("token count fits i32");
    set_result_u32s(&encoded);
    token_count
}

/// Extract SQL fragments from `source` for host language `lang`.
/// Dialect-independent, so no session handle is required.
#[unsafe(no_mangle)]
pub extern "C" fn wasm_embedded_extract(lang: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || run_embedded_extract(lang, ptr, len),
        "wasm_embedded_extract panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_embedded_diagnostics(handle: u32, lang: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_embedded_diagnostics(s, lang, ptr, len)),
        "wasm_embedded_diagnostics panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_embedded_semantic_tokens(handle: u32, lang: u32, ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || with_session(handle, |s| run_embedded_semantic_tokens(s, lang, ptr, len)),
        "wasm_embedded_semantic_tokens panicked",
    )
}

fn main() {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cflag_list_returns_valid_json() {
        let json = cflag_list_json();
        let entries: Vec<serde_json::Value> = serde_json::from_str(&json).unwrap();
        assert_eq!(entries.len(), 42, "expected 42 cflag entries");
        assert_eq!(entries[0]["name"], "SQLITE_OMIT_ALTERTABLE");
        assert!(entries[0]["minVersion"].as_u64().is_some());
        assert_eq!(entries[0]["category"], "parser");
    }

    #[test]
    fn cflag_list_all_names_match_from_name() {
        let json = cflag_list_json();
        let entries: Vec<serde_json::Value> = serde_json::from_str(&json).unwrap();
        for entry in &entries {
            let name = entry["name"].as_str().unwrap();
            assert!(
                SqliteFlag::from_name(name).is_some(),
                "cflag list contains unknown flag name: {name}"
            );
        }
    }

    fn result_text() -> String {
        RESULT_BUF.with(|buf| String::from_utf8(buf.borrow().clone()).unwrap())
    }

    #[test]
    fn session_handles_are_distinct_and_freeable() {
        let a = session_new();
        let b = session_new();
        assert_ne!(a, 0);
        assert_ne!(b, 0);
        assert_ne!(a, b);
        session_free(a);
        // Freed handle is rejected; live handle still works.
        assert_eq!(with_session(a, |_| 0), -1);
        assert_eq!(result_text(), "invalid session handle");
        assert_eq!(with_session(b, |_| 0), 0);
        session_free(b);
    }

    #[test]
    fn invalid_handle_is_rejected() {
        assert_eq!(with_session(0, |_| 0), -1);
        assert_eq!(result_text(), "invalid session handle");
    }

    #[test]
    fn join_messages_forms_a_json_array() {
        assert_eq!(join_messages(&[]), "[]");
        let joined = join_messages(&[r#"{"a":1}"#.to_string(), r#"{"b":2}"#.to_string()]);
        let parsed: Vec<serde_json::Value> = serde_json::from_str(&joined).unwrap();
        assert_eq!(parsed.len(), 2);
    }

    #[test]
    fn lsp_message_initialize_roundtrip() {
        // The dialect template pointer path is wasm-only; inject the SQLite
        // dialect directly to exercise the dispatcher plumbing natively.
        let mut session = Session::new();
        session.dialect = Some(syntaqlite::sqlite_dialect().into());
        let out = session.lsp().unwrap().handle_json(
            r#"{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}"#,
        );
        assert_eq!(out.len(), 1);
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert!(resp["result"]["capabilities"].is_object());
    }

    #[test]
    fn session_without_dialect_reports_error() {
        let h = session_new();
        let status = with_session(h, |s| match s.lsp() {
            Ok(_) => 0,
            Err(e) => {
                set_result(&e);
                -1
            }
        });
        assert_eq!(status, -1);
        assert_eq!(
            result_text(),
            "no dialect loaded: call wasm_set_dialect first"
        );
        session_free(h);
    }
}
