// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Version-compatibility wrapper for the SQLite tokenizer.
//
// Wraps SynqSqliteGetToken and reclassifies tokens that were introduced
// in newer SQLite versions.  When SQLite adds new token types in future
// versions, add a new version gate here following the existing pattern.

#include "csrc/token_wrapped.h"
#include "csrc/dialect_dispatch.h"
#include "csrc/tokens.h"
#include "syntaqlite/grammar.h"
#include "syntaqlite_dialect/dialect_macros.h"

int64_t SynqSqliteGetTokenVersionWrapped(const SyntaqliteGrammar* g,
                                         const unsigned char* z,
                                         uint32_t* tokenType) {
  int token_type_int = 0;
  int64_t len = SYNQ_GET_TOKEN(g, z, &token_type_int);
  *tokenType = (uint32_t)token_type_int;

  if (SYNQ_VER_LT(g, 3038000) && *tokenType == SYNTAQLITE_TK_PTR) {
    /* -> and ->> operators added in 3.38.
    ** Return just the '-' as TK_MINUS; next call picks up '>' naturally. */
    *tokenType = SYNTAQLITE_TK_MINUS;
    return 1;
  }

  if (SYNQ_VER_LT(g, 3046000) && *tokenType == SYNTAQLITE_TK_QNUMBER) {
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

  return len;
}
