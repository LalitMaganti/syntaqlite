// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Version-compatibility wrapper for the SQLite tokenizer.
//
// Out-of-line entry point for synq_get_token_wrapped in token_wrapped.h,
// where the version gates live.

#include "csrc/token_wrapped.h"

int64_t SynqSqliteGetTokenVersionWrapped(const SyntaqliteDialect* g,
                                         uint32_t macro_fallback,
                                         const unsigned char* z,
                                         uint32_t* tokenType) {
  return synq_get_token_wrapped(g, macro_fallback, z, tokenType);
}
