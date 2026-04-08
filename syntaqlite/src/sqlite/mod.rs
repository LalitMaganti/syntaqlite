// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `SQLite` dialect — semantic handle, function catalog, and formatter statics.

// `cflags` is always available — `SqliteFlag` ordinals are stable and
// grammar-agnostic; callers use them without the full sqlite feature.
pub(crate) mod cflags;

#[cfg(feature = "sqlite")]
pub(crate) mod dialect;
#[cfg(feature = "sqlite")]
pub(crate) mod functions_catalog;
