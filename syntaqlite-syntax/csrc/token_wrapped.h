// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Version-compatibility wrapper for the SQLite tokenizer.
//
// SynqSqliteGetTokenVersionWrapped reclassifies tokens that were
// introduced in newer SQLite versions, so the parser can target
// an older version of the grammar.  It also reclassifies '!' to
// TK_BANG when the caller indicates macro calls are possible
// (Rust-style dialects or callers with macro_fallback enabled).

#ifndef SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H
#define SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H

#include <stdint.h>

#include "csrc/dialect_dispatch.h"
#include "csrc/tokens.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite_dialect/dialect_macros.h"

int64_t SynqSqliteGetTokenVersionWrapped(const SyntaqliteDialect* env,
                                         uint32_t macro_fallback,
                                         const unsigned char* z,
                                         uint32_t* tokenType);

// Inline so the parser's per-token loop doesn't pay a call per token.
static inline int64_t synq_get_token_wrapped(const SyntaqliteDialect* g,
                                             uint32_t macro_fallback,
                                             const unsigned char* z,
                                             uint32_t* tokenType) {
  int token_type_int = 0;
  int64_t len = SYNQ_GET_TOKEN(g, z, &token_type_int);
  // Test the token type before the target version: nearly every token fails
  // the type test, so the version is rarely read.
  uint32_t type = (uint32_t)token_type_int;

#ifndef SYNTAQLITE_OMIT_MACROS
  if (type == SYNTAQLITE_TK_ILLEGAL && len == 1 && z[0] == '!' &&
      (g->tmpl->macro_style == SYNQ_MACRO_STYLE_RUST || macro_fallback)) {
    *tokenType = SYNTAQLITE_TK_BANG;
    return 1;
  }
#endif

  if (type == SYNTAQLITE_TK_PTR && SYNQ_VER_LT(g, 3038000)) {
    /* -> and ->> operators added in 3.38.
    ** Return just the '-' as TK_MINUS; next call picks up '>' naturally. */
    *tokenType = SYNTAQLITE_TK_MINUS;
    return 1;
  }

  if (type == SYNTAQLITE_TK_QNUMBER && SYNQ_VER_LT(g, 3046000)) {
    /* Digit separators added in 3.46.
    ** Truncate to the first underscore. */
    int64_t j;
    int saw_dot = 0;
    for (j = 0; j < len; j++) {
      if (z[j] == '_')
        break;
      if (z[j] == '.')
        saw_dot = 1;
    }
    *tokenType = saw_dot ? SYNTAQLITE_TK_FLOAT : SYNTAQLITE_TK_INTEGER;
    return j;
  }

  *tokenType = type;
  return len;
}

#endif  // SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H
