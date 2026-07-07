// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP protocol server — stdio transport over [`LspDispatcher`].

// `LspServer` is intentionally `pub` so it can be re-exported by `lsp/mod.rs`.
// The `server` submodule is private; items here are only reachable via that re-export.
#![allow(unreachable_pub)]

use std::error::Error;

use lsp_server::Connection;

use crate::dialect::AnyDialect;
use crate::lsp::{LspConfig, LspDispatcher};

/// Stdio LSP server for a syntaqlite dialect.
///
/// Runs a JSON-RPC message loop on stdin/stdout, feeding each message to an
/// [`LspDispatcher`] and forwarding its output to the client. Exits cleanly
/// when the client sends the `exit` notification.
///
/// Use this when you want a turnkey LSP binary that editors can launch as a
/// child process. For other transports (e.g., a web worker or test harness),
/// drive an [`LspDispatcher`] directly; for programmatic analysis without
/// the LSP protocol, use [`LspHost`](crate::lsp::LspHost).
///
/// # Example
///
/// ```no_run
/// use syntaqlite::lsp::LspServer;
///
/// // Blocks on stdin/stdout — typically launched by an editor.
/// LspServer::run(syntaqlite::sqlite_dialect()).expect("LSP server failed");
/// ```
pub struct LspServer;

impl LspServer {
    /// Start the LSP server bound to `dialect` and block until shutdown.
    ///
    /// # Errors
    /// Returns `Err` if the LSP connection fails or an unrecoverable I/O error occurs.
    pub fn run(dialect: impl Into<AnyDialect>) -> Result<(), Box<dyn Error + Sync + Send>> {
        Self::run_with_config(dialect, LspConfig::default())
    }

    /// Start the LSP server with project configuration pre-loaded.
    ///
    /// # Errors
    /// Returns `Err` if the LSP connection fails or an unrecoverable I/O error occurs.
    pub fn run_with_config(
        dialect: impl Into<AnyDialect>,
        config: LspConfig,
    ) -> Result<(), Box<dyn Error + Sync + Send>> {
        let (connection, io_threads) = Connection::stdio();
        let mut dispatcher = LspDispatcher::with_config(dialect, config);

        for msg in &connection.receiver {
            for out in dispatcher.handle(msg) {
                connection.sender.send(out)?;
            }
            if dispatcher.exited() {
                break;
            }
        }

        // Close the sender before joining: the writer thread only exits
        // once every Sender handle is dropped.
        drop(connection);
        io_threads.join()?;
        Ok(())
    }
}
