// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Runtime ↔ dialect ABI contract.
//
// Dialects can be linked into the runtime statically (amalgamation builds,
// cargo-built native binaries) or loaded as separately-compiled side modules
// (emscripten MAIN_MODULE=2 / SIDE_MODULE=1, future `dlopen` support). In the
// latter case, neither half of the link sees the other's symbols at compile
// time, and dead-code elimination will drop any symbol that isn't explicitly
// marked as part of the cross-module surface.
//
// Two kinds of calls cross this boundary:
//
//   dialect → runtime
//     The generated grammar action code and Lemon-emitted parser call back
//     into runtime-owned helpers during reduce/shift. Everything exported
//     here lives in `syntaqlite-syntax/csrc/parser_extents.c` or similar
//     runtime-side C files. Current members:
//       - synq_extent_on_shift   (see syntaqlite_dialect/extent_hooks.h)
//       - synq_extent_on_reduce  (see syntaqlite_dialect/extent_hooks.h)
//
//   runtime → dialect
//     The runtime drives each dialect through a `SyntaqliteDialectTemplate`
//     whose function pointers reference symbols emitted by the dialect's
//     generated parser/tokenizer. Current members (per dialect, Pascal-cased):
//       - Synq<Dialect>ParseAlloc / ParseInit / ParseFinalize / ParseFree
//       - Synq<Dialect>Parse
//       - Synq<Dialect>ParseTrace
//       - Synq<Dialect>ParseExpectedTokens
//       - Synq<Dialect>ParseCompletionContext
//       - Synq<Dialect>ParseFallback
//       - Synq<Dialect>GetToken
//
// Every declaration in that set must be tagged with `SYNTAQLITE_DIALECT_API`
// so it survives dead-code elimination and is visible across module
// boundaries. Renaming or changing the signature of any symbol above is an
// ABI break: update both sides in lockstep.

#ifndef SYNTAQLITE_INTERNAL_DIALECT_ABI_H
#define SYNTAQLITE_INTERNAL_DIALECT_ABI_H

// `used` keeps wasm-ld / LTO from discarding the symbol when the main module
// has no direct reference to it (the referencing side module is linked
// separately). `visibility("default")` ensures it ends up in the module's
// export table rather than being internal.
#if defined(__GNUC__) || defined(__clang__)
#define SYNTAQLITE_DIALECT_API __attribute__((used, visibility("default")))
#else
#define SYNTAQLITE_DIALECT_API
#endif

#endif  // SYNTAQLITE_INTERNAL_DIALECT_ABI_H
