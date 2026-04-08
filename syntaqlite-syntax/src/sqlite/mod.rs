// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// `tokens` and `cflags` require the `sqlite-minimal` feature. The `sqlite`
// feature implies `sqlite-minimal`, so these are available in both cases.
#[cfg(feature = "sqlite-minimal")]
pub(crate) mod cflags;
#[cfg(feature = "sqlite-minimal")]
pub(crate) mod tokens;

#[cfg(feature = "sqlite")]
pub(crate) mod ast;
#[cfg(feature = "sqlite")]
pub(crate) mod ffi;
#[cfg(feature = "sqlite")]
pub(crate) mod grammar;
