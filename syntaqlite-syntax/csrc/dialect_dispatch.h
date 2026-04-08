// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Dispatch macros for parser/tokenizer grammar functions.
//
// In amalgamation builds all C code compiles as one unit, so we can call
// grammar functions directly instead of going through function pointers.
// Define SYNTAQLITE_INLINE_DIALECT_DISPATCH to a header path that provides
// the SYNQ_PARSER_ALLOC, etc. macros for your grammar.

#ifndef SYNTAQLITE_INTERNAL_DIALECT_DISPATCH_H
#define SYNTAQLITE_INTERNAL_DIALECT_DISPATCH_H

#if defined(SYNTAQLITE_INLINE_DIALECT_DISPATCH)
#include SYNTAQLITE_INLINE_DIALECT_DISPATCH
#elif !defined(SYNQ_PARSER_ALLOC)
// Default: function pointer dispatch through the grammar struct.
#define SYNQ_PARSER_ALLOC(g, m, c) (g)->parser_alloc(m, c)
#define SYNQ_PARSER_INIT(g, p, c) (g)->parser_init(p, c)
#define SYNQ_PARSER_FINALIZE(g, p) (g)->parser_finalize(p)
#define SYNQ_PARSER_FREE(g, p, f) (g)->parser_free(p, f)
#define SYNQ_PARSER_FEED(g, p, t, m) (g)->parser_feed(p, t, m)
#define SYNQ_PARSER_TRACE(g, f, s) \
  do {                             \
    if ((g)->parser_trace)         \
      (g)->parser_trace(f, s);     \
  } while (0)
#define SYNQ_GET_TOKEN(g, z, t) (g)->tmpl->get_token(g, z, t)
#endif

#endif  // SYNTAQLITE_INTERNAL_DIALECT_DISPATCH_H
