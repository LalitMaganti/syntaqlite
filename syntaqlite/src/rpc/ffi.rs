// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! JSON-RPC C API (`include/syntaqlite/rpc.h`).
//!
//! Wires the parent [`crate::rpc`] dispatch onto a C boundary: one UTF-8 JSON
//! request in, one UTF-8 JSON envelope out, plus an opaque session handle. The
//! same protocol as the CLI's `serve json`, for in-process embedding (the
//! Python bindings load the cdylib via ctypes; any C/C++ host works too).
//!
//! Only the opaque [`SyntaqliteRpc`] handle and raw byte pointers cross the
//! boundary, so there are no `#[repr(C)]` layout assertions. Sessions are
//! single-threaded. No panic ever unwinds across the edge.

#[cfg(feature = "dynload")]
use std::os::raw::c_char;
use std::panic::{self, AssertUnwindSafe};

use crate::rpc::{self, RpcSession};

/// Opaque handle backing a `Box<RpcSession>`. Zero-variant so Rust can't
/// construct it; accessed only via pointer casts, like `SyntaqliteAnalyzer`.
pub(crate) enum SyntaqliteRpc {}

/// Error envelope for the panic path — built without serde so it can't itself
/// fail. Callers always get valid JSON, never null/crash.
const PANIC_ENVELOPE: &[u8] = br#"{"ok":false,"error":"internal panic during rpc call"}"#;

/// Session bound to the built-in `SQLite` dialect. Null on panic.
#[cfg(feature = "sqlite")]
#[unsafe(no_mangle)]
pub(crate) extern "C" fn syntaqlite_rpc_create_sqlite() -> *mut SyntaqliteRpc {
    panic::catch_unwind(|| {
        let dialect = crate::sqlite_dialect().erase();
        Box::into_raw(Box::new(RpcSession::new(&dialect))).cast::<SyntaqliteRpc>()
    })
    .unwrap_or(std::ptr::null_mut())
}

/// Session for a dynamically-loaded dialect (CLI `--dialect`/`--dialect-name`).
/// Null on load error or panic.
///
/// # Safety
///
/// `path` must be valid for `path_len` bytes; `name`, if non-null, for
/// `name_len` bytes.
#[cfg(feature = "dynload")]
#[unsafe(no_mangle)]
pub(crate) unsafe extern "C" fn syntaqlite_rpc_create_dialect(
    path: *const c_char,
    path_len: u64,
    name: *const c_char,
    name_len: u64,
) -> *mut SyntaqliteRpc {
    panic::catch_unwind(AssertUnwindSafe(|| {
        if path.is_null() {
            return std::ptr::null_mut();
        }
        // SAFETY: caller guarantees `path` is valid for `path_len` bytes.
        let path_bytes = unsafe {
            std::slice::from_raw_parts(path.cast::<u8>(), usize::try_from(path_len).unwrap_or(0))
        };
        let Ok(path_str) = std::str::from_utf8(path_bytes) else {
            return std::ptr::null_mut();
        };
        let name_str = if name.is_null() {
            None
        } else {
            // SAFETY: caller guarantees `name` is valid for `name_len` bytes.
            let name_bytes = unsafe {
                std::slice::from_raw_parts(
                    name.cast::<u8>(),
                    usize::try_from(name_len).unwrap_or(0),
                )
            };
            match std::str::from_utf8(name_bytes) {
                Ok(s) => Some(s),
                Err(_) => return std::ptr::null_mut(),
            }
        };
        match crate::any::AnyDialect::load(path_str, name_str) {
            Ok(dialect) => {
                Box::into_raw(Box::new(RpcSession::new(&dialect))).cast::<SyntaqliteRpc>()
            }
            Err(_) => std::ptr::null_mut(),
        }
    }))
    .unwrap_or(std::ptr::null_mut())
}

/// Run one JSON request. Writes the response length to `*out_len` and returns
/// a fresh, NUL-terminated UTF-8 envelope (`*out_len` excludes the NUL); free
/// it with [`syntaqlite_rpc_free`]. On panic still returns an error envelope;
/// returns null (and `*out_len = 0`) only when `handle` is null.
///
/// # Safety
///
/// `handle` must be a live, not-yet-destroyed handle; `request` valid for
/// `request_len` bytes; `out_len` valid for one write.
#[unsafe(no_mangle)]
pub(crate) unsafe extern "C" fn syntaqlite_rpc_call(
    handle: *mut SyntaqliteRpc,
    request: *const u8,
    request_len: u64,
    out_len: *mut u64,
) -> *mut u8 {
    if handle.is_null() {
        if !out_len.is_null() {
            // SAFETY: caller guarantees `out_len` is valid for one write.
            unsafe { *out_len = 0 };
        }
        return std::ptr::null_mut();
    }

    // AssertUnwindSafe is sound: on panic the caller discards this response and
    // the owner destroys the session; `call_json` builds a fresh response each
    // call, so no partially-mutated state is observed across the boundary.
    let result = panic::catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: `handle` is a live `Box<RpcSession>`; sole reference for this call.
        let session = unsafe { &mut *handle.cast::<RpcSession>() };
        let req = if request.is_null() {
            ""
        } else {
            // SAFETY: caller guarantees `request` is valid for `request_len` bytes.
            let bytes = unsafe {
                std::slice::from_raw_parts(request, usize::try_from(request_len).unwrap_or(0))
            };
            std::str::from_utf8(bytes).unwrap_or("")
        };
        rpc::call_json(session, req)
    }));

    let payload: &[u8] = match &result {
        Ok(s) => s.as_bytes(),
        Err(_) => PANIC_ENVELOPE,
    };
    if !out_len.is_null() {
        // SAFETY: caller guarantees `out_len` is valid for one write.
        unsafe { *out_len = payload.len() as u64 };
    }
    into_raw_buf(payload)
}

/// Leak `payload` + a trailing NUL into an exact-capacity allocation
/// (`capacity == len == payload.len() + 1`), matching [`syntaqlite_rpc_free`].
fn into_raw_buf(payload: &[u8]) -> *mut u8 {
    let mut v = Vec::with_capacity(payload.len() + 1);
    v.extend_from_slice(payload);
    v.push(0); // courtesy NUL; `*out_len` excludes it.
    let mut boxed = v.into_boxed_slice();
    let ptr = boxed.as_mut_ptr();
    std::mem::forget(boxed);
    ptr
}

/// Free a [`syntaqlite_rpc_call`] buffer. No-op if null.
///
/// # Safety
///
/// `(ptr, len)` must be exactly what a prior call returned, freed once.
#[unsafe(no_mangle)]
pub(crate) unsafe extern "C" fn syntaqlite_rpc_free(ptr: *mut u8, len: u64) {
    if ptr.is_null() {
        return;
    }
    let _ = panic::catch_unwind(AssertUnwindSafe(|| {
        let total = usize::try_from(len).unwrap_or(0) + 1;
        // SAFETY: the buffer was leaked from a `Box<[u8]>` of `len + 1` bytes
        // (payload plus courtesy NUL); reconstruct it identically and drop.
        let boxed: Box<[u8]> =
            unsafe { Box::from_raw(std::ptr::slice_from_raw_parts_mut(ptr, total)) };
        drop(boxed);
    }));
}

/// Destroy a session. No-op if null; the handle must not be used afterward.
///
/// # Safety
///
/// `handle` must come from a create fn and be destroyed at most once.
#[unsafe(no_mangle)]
pub(crate) unsafe extern "C" fn syntaqlite_rpc_destroy(handle: *mut SyntaqliteRpc) {
    if handle.is_null() {
        return;
    }
    let _ = panic::catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: caller guarantees `handle` came from a create fn, destroyed once.
        drop(unsafe { Box::from_raw(handle.cast::<RpcSession>()) });
    }));
}
