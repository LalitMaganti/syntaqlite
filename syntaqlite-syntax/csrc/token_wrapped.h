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

#include "syntaqlite/dialect.h"

int64_t SynqSqliteGetTokenVersionWrapped(const SyntaqliteDialect* env,
                                         uint32_t macro_fallback,
                                         const unsigned char* z,
                                         uint32_t* tokenType);

#endif  // SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H
