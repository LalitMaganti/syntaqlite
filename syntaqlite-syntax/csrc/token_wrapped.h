// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Version-compatibility wrapper for the SQLite tokenizer.
//
// SynqSqliteGetTokenVersionWrapped reclassifies tokens that were
// introduced in newer SQLite versions, so the parser can target
// an older version of the grammar.

#ifndef SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H
#define SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H

#include <stdint.h>

#include "syntaqlite/grammar.h"

int64_t SynqSqliteGetTokenVersionWrapped(const SyntaqliteGrammar* env,
                                         const unsigned char* z,
                                         uint32_t* tokenType);

#endif  // SYNTAQLITE_INTERNAL_TOKEN_WRAPPED_H
