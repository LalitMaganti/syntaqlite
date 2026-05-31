// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Dispatch macros for parser/tokenizer dialect functions.
//
// In amalgamation builds all C code compiles as one unit, so we can call
// dialect functions directly instead of going through function pointers.
//
// Resolution order:
//   1. The Full amalgamation pre-defines SYNQ_PARSER_ALLOC, etc. inline at
//      the top of the .c so this header's branches are skipped entirely.
//      Define SYNTAQLITE_NO_INLINE_DIALECT_DISPATCH to disable that and
//      fall through to the function-pointer fallback below.
//   2. Otherwise, define SYNTAQLITE_INLINE_DIALECT_DISPATCH to a header
//      path that provides the SYNQ_PARSER_ALLOC, etc. macros for your
//      dialect (used by hand-built consumers without the Full amalgamation).
//   3. Otherwise, fall back to function-pointer dispatch through the
//      dialect template struct.

#ifndef SYNTAQLITE_INTERNAL_DIALECT_DISPATCH_H
#define SYNTAQLITE_INTERNAL_DIALECT_DISPATCH_H

#if defined(SYNTAQLITE_INLINE_DIALECT_DISPATCH)
#include SYNTAQLITE_INLINE_DIALECT_DISPATCH
#elif !defined(SYNQ_PARSER_ALLOC)
// Default: function pointer dispatch through the dialect template struct.
#define SYNQ_PARSER_ALLOC(d, m, c) (d)->parser_alloc(m, c)
#define SYNQ_PARSER_INIT(d, p, c) (d)->parser_init(p, c)
#define SYNQ_PARSER_FINALIZE(d, p) (d)->parser_finalize(p)
#define SYNQ_PARSER_FREE(d, p, f) (d)->parser_free(p, f)
#define SYNQ_PARSER_FEED(d, p, t, m) (d)->parser_feed(p, t, m)
#define SYNQ_PARSER_TRACE(d, f, s) \
  do {                             \
    if ((d)->parser_trace)         \
      (d)->parser_trace(f, s);     \
  } while (0)
#define SYNQ_GET_TOKEN(env, z, t) (env)->tmpl->get_token(env, z, t)
#endif

#endif  // SYNTAQLITE_INTERNAL_DIALECT_DISPATCH_H
