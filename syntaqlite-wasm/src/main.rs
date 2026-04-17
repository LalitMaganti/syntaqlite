// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.
#![allow(missing_docs)] // ABI exports don't need rustdoc
#![cfg_attr(test, expect(clippy::unwrap_used))]

use std::cell::{Cell, RefCell};
use std::slice;

use serde::Serialize;

use syntaqlite::any::AnyDialect;
use syntaqlite::fmt::KeywordCase;
use syntaqlite::lsp::LspHost;
use syntaqlite::util::{SqliteFlag, SqliteFlags, SqliteVersion};
use syntaqlite::{FormatConfig, Formatter, ValidationConfig};

thread_local! {
    static RESULT_BUF: RefCell<Vec<u8>> = const { RefCell::new(Vec::new()) };
    /// Global LspHost reused across wasm calls. Invalidated when session context changes.
    static LSP_HOST: RefCell<Option<LspHost>> = const { RefCell::new(None) };
    /// Active dialect. Set via `wasm_set_dialect`; `None` until a dialect side module is loaded.
    static DIALECT: RefCell<Option<AnyDialect>> = const { RefCell::new(None) };
    /// Raw pointer to the active `SyntaqliteDialectTemplate`, retained so version/cflag
    /// overrides can rebuild the dialect without reloading the side module.
    static DIALECT_PTR: Cell<u32> = const { Cell::new(0) };
    /// Active embedded language. `None` = raw SQL; `Some(n)` = embedded (0 = Python, 1 = TypeScript).
    static EMBEDDED_LANG: RefCell<Option<u32>> = const { RefCell::new(None) };
    /// SQLite version override applied on top of the loaded dialect.
    static SQLITE_VERSION: RefCell<SqliteVersion> = const { RefCell::new(SqliteVersion::Latest) };
    /// Cflag overrides applied on top of the loaded dialect.
    static SQLITE_CFLAGS: RefCell<SqliteFlags> = RefCell::new(SqliteFlags::default());
}

/// Sentinel passed to [`wasm_set_language_mode`] to select raw SQL mode.
const LANG_SQL_SENTINEL: u32 = u32::MAX;

fn get_embedded_lang() -> Option<u32> {
    EMBEDDED_LANG.with(|cell| *cell.borrow())
}

fn get_dialect() -> Option<AnyDialect> {
    DIALECT.with(|cell| cell.borrow().clone())
}

fn take_or_create_lsp_host() -> Result<LspHost, String> {
    LSP_HOST.with(|cell| cell.borrow_mut().take()).map_or_else(
        || {
            get_dialect()
                .ok_or_else(|| "no dialect loaded: call wasm_set_dialect first".to_string())
                .map(LspHost::with_dialect)
        },
        Ok,
    )
}

fn store_lsp_host(lsp: LspHost) {
    LSP_HOST.with(|cell| *cell.borrow_mut() = Some(lsp));
}

fn invalidate_lsp_host() {
    LSP_HOST.with(|h| h.borrow_mut().take());
}

/// Rebuild the active dialect from the stored template pointer, applying any
/// version/cflag overrides. No-op when no side module is loaded yet.
fn rebuild_dialect_from_ptr() {
    let ptr = DIALECT_PTR.with(Cell::get);
    if ptr == 0 {
        return;
    }
    let version = SQLITE_VERSION.with(|v| *v.borrow());
    let cflags = SQLITE_CFLAGS.with(|c| *c.borrow());
    // SAFETY: ptr was validated in run_set_dialect when it was stored.
    let dialect = unsafe {
        AnyDialect::from_c_dialect_ptr(ptr as *const syntaqlite::any::ffi::CDialectTemplate)
    }
    .with_version(version)
    .with_cflags(cflags);
    DIALECT.with(|cell| *cell.borrow_mut() = Some(dialect));
    invalidate_lsp_host();
}

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

/// Unwraps a `Result`, writing the error to the result buffer and returning `$code` on failure.
macro_rules! try_wasm {
    ($expr:expr) => {
        try_wasm!($expr, 1)
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

fn run_ast_json(ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let dialect = try_wasm!(get_dialect().ok_or("no dialect loaded: call wasm_set_dialect first"));
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
pub extern "C" fn wasm_ast_json(ptr: u32, len: u32) -> i32 {
    catch_unwind(|| run_ast_json(ptr, len), "wasm_ast_json panicked")
}

// ── Formatter ────────────────────────────────────────────────────────

fn run_fmt(
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
    let dialect = try_wasm!(get_dialect().ok_or("no dialect loaded: call wasm_set_dialect first"));
    let mut formatter = Formatter::with_dialect_config(dialect, &config);
    let sql = try_wasm!(formatter.format(&source).map_err(|e| e.to_string()));
    set_result(&sql);
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_fmt(
    ptr: u32,
    len: u32,
    line_width: u32,
    indent_width: u32,
    keyword_case: u32,
    semicolons: u32,
) -> i32 {
    run_fmt(ptr, len, line_width, indent_width, keyword_case, semicolons)
}

// ── Session context ──────────────────────────────────────────────────

fn run_set_session_context(ptr: u32, len: u32) -> i32 {
    let input = try_wasm!(decode_input(ptr, len));
    let mut lsp = try_wasm!(take_or_create_lsp_host());
    try_wasm!(lsp.set_session_context_from_json(&input));
    store_lsp_host(lsp);
    0
}

fn run_set_session_context_ddl(ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len));
    let mut lsp = try_wasm!(take_or_create_lsp_host());
    let result = lsp.set_session_context_from_ddl(&source, None);
    store_lsp_host(lsp);
    match result {
        Ok(()) => 0,
        Err(errors) => {
            set_result(&errors.join("\n"));
            1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_session_context(ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || run_set_session_context(ptr, len),
        "wasm_set_session_context panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_session_context() -> i32 {
    invalidate_lsp_host();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_session_context_ddl(ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || run_set_session_context_ddl(ptr, len),
        "wasm_set_session_context_ddl panicked",
    )
}

// ── Diagnostics / semantic tokens / completions ──────────────────────

const WASM_DOC_URI: &str = "wasm://input";

fn run_diagnostics(ptr: u32, len: u32, version: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len), -1);
    let mut lsp = try_wasm!(take_or_create_lsp_host(), -1);
    lsp.update_document(WASM_DOC_URI, version.cast_signed(), source);
    let all_diags = lsp.all_diagnostics(WASM_DOC_URI, &ValidationConfig::default());
    let total_count = all_diags.len();
    set_result(&serde_json::to_string(&all_diags).expect("diagnostic serialization failed"));
    store_lsp_host(lsp);
    i32::try_from(total_count).expect("diagnostic count fits i32")
}

fn run_semantic_tokens(ptr: u32, len: u32, range_start: u32, range_end: u32, version: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len), -1);
    let mut lsp = try_wasm!(take_or_create_lsp_host(), -1);
    lsp.update_document(WASM_DOC_URI, version.cast_signed(), source);
    let range = if range_start == 0 && range_end == 0xFFFF_FFFF {
        None
    } else {
        Some((range_start as usize, range_end as usize))
    };
    let encoded = lsp.semantic_tokens_encoded(WASM_DOC_URI, range);
    let token_count = i32::try_from(encoded.len() / 5).expect("token count fits i32");
    set_result_u32s(&encoded);
    store_lsp_host(lsp);
    token_count
}

#[derive(Serialize)]
struct CompletionItem {
    label: String,
    kind: &'static str,
}

fn run_completions(ptr: u32, len: u32, offset: u32, version: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len), -1);
    let mut lsp = try_wasm!(take_or_create_lsp_host(), -1);
    lsp.update_document(WASM_DOC_URI, version.cast_signed(), source);
    let entries = lsp.completion_items(WASM_DOC_URI, offset as usize);
    let count = i32::try_from(entries.len()).expect("completion count fits i32");
    let items: Vec<CompletionItem> = entries
        .into_iter()
        .map(|e| CompletionItem {
            label: e.label().to_string(),
            kind: e.kind().as_str(),
        })
        .collect();
    set_result(&serde_json::to_string(&items).expect("completions serialization failed"));
    store_lsp_host(lsp);
    count
}

/// Set the active language mode. Pass `u32::MAX` for raw SQL mode, or a host-language
/// code (0 = Python, 1 = TypeScript) for embedded-SQL mode. After this call,
/// `wasm_diagnostics`, `wasm_semantic_tokens`, and `wasm_extract` dispatch automatically.
///
/// # Experimental
///
/// Embedded language support is experimental and may change in future releases.
#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_language_mode(lang: u32) {
    let embedded = if lang == LANG_SQL_SENTINEL {
        None
    } else {
        Some(lang)
    };
    EMBEDDED_LANG.with(|cell| *cell.borrow_mut() = embedded);
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_diagnostics(ptr: u32, len: u32, version: u32) -> i32 {
    catch_unwind(
        || match get_embedded_lang() {
            Some(lang) => run_embedded_diagnostics(lang, ptr, len),
            None => run_diagnostics(ptr, len, version),
        },
        "wasm_diagnostics panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_semantic_tokens(
    ptr: u32,
    len: u32,
    range_start: u32,
    range_end: u32,
    version: u32,
) -> i32 {
    catch_unwind(
        || match get_embedded_lang() {
            Some(lang) => run_embedded_semantic_tokens(lang, ptr, len),
            None => run_semantic_tokens(ptr, len, range_start, range_end, version),
        },
        "wasm_semantic_tokens panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_completions(ptr: u32, len: u32, offset: u32, version: u32) -> i32 {
    catch_unwind(
        || run_completions(ptr, len, offset, version),
        "wasm_completions panicked",
    )
}

// ── Dialect switching ────────────────────────────────────────────────

fn run_set_dialect(ptr: u32) -> i32 {
    if ptr == 0 {
        set_result("null dialect pointer");
        return 1;
    }
    DIALECT_PTR.with(|c| c.set(ptr));
    rebuild_dialect_from_ptr();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_dialect(ptr: u32) -> i32 {
    catch_unwind(|| run_set_dialect(ptr), "wasm_set_dialect panicked")
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_dialect() {
    DIALECT_PTR.with(|c| c.set(0));
    DIALECT.with(|cell| *cell.borrow_mut() = None);
    invalidate_lsp_host();
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
// These configure the active dialect with a target SQLite version or
// compile-time flags. Stored state is re-applied whenever a dialect is
// loaded (or reloaded) via `wasm_set_dialect`.

fn run_set_sqlite_version(ptr: u32, len: u32) -> i32 {
    let s = try_wasm!(decode_input(ptr, len));
    let version = try_wasm!(SqliteVersion::parse_with_latest(&s));
    SQLITE_VERSION.with(|v| *v.borrow_mut() = version);
    rebuild_dialect_from_ptr();
    0
}

fn run_set_cflag(ptr: u32, len: u32) -> i32 {
    let s = try_wasm!(decode_input(ptr, len));
    let flag = try_wasm!(SqliteFlag::from_name(&s).ok_or_else(|| format!("unknown cflag: {s}")));
    SQLITE_CFLAGS.with(|c| {
        let mut guard = c.borrow_mut();
        *guard = std::mem::take(&mut *guard).with(flag);
    });
    rebuild_dialect_from_ptr();
    0
}

fn run_clear_cflag(ptr: u32, len: u32) -> i32 {
    let s = try_wasm!(decode_input(ptr, len));
    let flag = try_wasm!(SqliteFlag::from_name(&s).ok_or_else(|| format!("unknown cflag: {s}")));
    SQLITE_CFLAGS.with(|c| {
        let mut guard = c.borrow_mut();
        *guard = std::mem::take(&mut *guard).without(flag);
    });
    rebuild_dialect_from_ptr();
    0
}

fn run_clear_all_cflags() -> i32 {
    SQLITE_CFLAGS.with(|c| *c.borrow_mut() = SqliteFlags::default());
    rebuild_dialect_from_ptr();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_sqlite_version(ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || run_set_sqlite_version(ptr, len),
        "wasm_set_sqlite_version panicked",
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_set_cflag(ptr: u32, len: u32) -> i32 {
    catch_unwind(|| run_set_cflag(ptr, len), "wasm_set_cflag panicked")
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_cflag(ptr: u32, len: u32) -> i32 {
    catch_unwind(|| run_clear_cflag(ptr, len), "wasm_clear_cflag panicked")
}

#[unsafe(no_mangle)]
pub extern "C" fn wasm_clear_all_cflags() -> i32 {
    catch_unwind(run_clear_all_cflags, "wasm_clear_all_cflags panicked")
}

// ── Embedded SQL WASM exports (experimental) ─────────────────────────
//
// lang encoding: 0 = Python, 1 = TypeScript/JavaScript
// NOTE: Embedded language support is experimental and may change.

use syntaqlite::embedded::{EmbeddedAnalyzer, EmbeddedFragment};

fn embedded_fragments(lang: u32, source: &str) -> Result<Vec<EmbeddedFragment>, String> {
    match lang {
        0 => Ok(syntaqlite::embedded::extract_python(source)),
        1 => Ok(syntaqlite::embedded::extract_typescript(source)),
        _ => Err(format!("unknown host language id: {lang}")),
    }
}

fn make_embedded_analyzer() -> Result<EmbeddedAnalyzer, String> {
    let dialect = get_dialect().ok_or("no dialect loaded: call wasm_set_dialect first")?;
    Ok(EmbeddedAnalyzer::new(dialect))
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
    let source = try_wasm!(decode_input(ptr, len), -1);
    let fragments = try_wasm!(embedded_fragments(lang, &source), -1);
    let count = i32::try_from(fragments.len()).expect("fragment count fits i32");
    let items: Vec<WasmFragment> = fragments
        .iter()
        .map(|f| WasmFragment {
            start: f.sql_range().start,
            end: f.sql_range().end,
            sql: f.sql_text().to_string(),
            holes: f
                .holes()
                .iter()
                .map(|h| WasmHole {
                    start: h.host_range().start,
                    end: h.host_range().end,
                })
                .collect(),
        })
        .collect();
    set_result(&serde_json::to_string(&items).expect("fragment serialization failed"));
    count
}

fn run_embedded_diagnostics(lang: u32, ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len), -1);
    let fragments = try_wasm!(embedded_fragments(lang, &source), -1);
    let diags = try_wasm!(make_embedded_analyzer(), -1).validate(&fragments);
    let count = i32::try_from(diags.len()).expect("diag count fits i32");
    set_result(&serde_json::to_string(&diags).expect("embedded diagnostic serialization failed"));
    count
}

fn run_embedded_semantic_tokens(lang: u32, ptr: u32, len: u32) -> i32 {
    let source = try_wasm!(decode_input(ptr, len), -1);
    let fragments = try_wasm!(embedded_fragments(lang, &source), -1);
    let encoded =
        try_wasm!(make_embedded_analyzer(), -1).semantic_tokens_encoded(&fragments, &source);
    let token_count = i32::try_from(encoded.len() / 5).expect("token count fits i32");
    set_result_u32s(&encoded);
    token_count
}

/// Extract SQL fragments from the current source using the active language mode.
/// Returns 0 with no result in SQL mode. In embedded mode, dispatches to the
/// appropriate extractor based on the language set by `wasm_set_language_mode`.
#[unsafe(no_mangle)]
pub extern "C" fn wasm_extract(ptr: u32, len: u32) -> i32 {
    catch_unwind(
        || match get_embedded_lang() {
            Some(lang) => run_embedded_extract(lang, ptr, len),
            None => 0,
        },
        "wasm_extract panicked",
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
}
