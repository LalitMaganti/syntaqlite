// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `serve` subcommand — long-lived RPC server over stdio.
//!
//! Each protocol lives in its own submodule. The CLI selects one via
//! `syntaqlite serve <protocol>` (e.g. `syntaqlite serve json`). Adding a
//! new wire format is a new module plus a new `ServeProtocol` variant in
//! `cli.rs`; existing protocols stay untouched.

pub(crate) mod json;

pub(crate) use json::run as run_json;
