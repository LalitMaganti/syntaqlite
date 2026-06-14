// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `serve json` — long-lived JSON RPC server over stdio.
//!
//! The Python bindings spawn `syntaqlite serve json` once and multiplex all
//! parse/format/tokenize/validate calls over a single subprocess, amortizing
//! process startup and dialect initialization across a session.
//!
//! ## Protocol
//!
//! On startup, the server writes `READY\n` to stdout, then reads requests in
//! a loop until the client sends `{"op":"quit"}` or closes stdin.
//!
//! **Request:** one JSON object per line (newline-terminated, no embedded newlines).
//! **Response:** one JSON object per line — `{"ok":true,"result":<value>}` on
//! success, `{"ok":false,"error":<string>}` on failure.
//!
//! All per-op dispatch lives in [`syntaqlite::rpc`]; this module owns only
//! the stdio framing, the `READY` handshake, and the `quit` early-return so
//! the same dispatch can be reused by the in-process C API.

use std::io::{self, BufRead, BufReader, Write};

use serde_json::Value;
use syntaqlite::any::AnyDialect;
use syntaqlite::rpc::{self, RpcSession};

pub(crate) fn run(dialect: &AnyDialect) -> Result<(), String> {
    let stdin = io::stdin();
    let stdout = io::stdout();
    let mut reader = BufReader::new(stdin.lock());
    let mut writer = io::BufWriter::new(stdout.lock());

    writeln!(writer, "READY").map_err(|e| format!("write READY: {e}"))?;
    writer.flush().map_err(|e| format!("flush READY: {e}"))?;

    let mut session = RpcSession::new(dialect);

    let mut line = String::new();
    loop {
        line.clear();
        let n = reader
            .read_line(&mut line)
            .map_err(|e| format!("read: {e}"))?;
        if n == 0 {
            return Ok(()); // EOF
        }
        let trimmed = line.trim_end_matches(['\n', '\r']);
        if trimmed.is_empty() {
            continue;
        }

        // `quit` is a transport concern handled here, not in the shared
        // dispatch: peek at the op without committing to full deserialization.
        if let Ok(v) = serde_json::from_str::<Value>(trimmed)
            && v.get("op").and_then(Value::as_str) == Some("quit")
        {
            return Ok(());
        }

        let response = rpc::call_json(&mut session, trimmed);
        writer
            .write_all(response.as_bytes())
            .map_err(|e| format!("write: {e}"))?;
        writer.write_all(b"\n").map_err(|e| format!("write: {e}"))?;
        writer.flush().map_err(|e| format!("flush: {e}"))?;
    }
}
