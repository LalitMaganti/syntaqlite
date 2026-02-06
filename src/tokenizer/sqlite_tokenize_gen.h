// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Declarations for the extracted SQLite tokenizer (sqlite_tokenize_gen.c).
// DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py

#ifndef SYNQ_SRC_TOKENIZER_SQLITE_TOKENIZE_GEN_H
#define SYNQ_SRC_TOKENIZER_SQLITE_TOKENIZE_GEN_H

#include "src/common/synq_sqlite_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tokenize the next token starting at z.
// Writes the token type to *tokenType and returns the token length.
i64 synq_sqlite3GetToken(const unsigned char *z, int *tokenType);

#ifdef __cplusplus
}
#endif

#endif // SYNQ_SRC_TOKENIZER_SQLITE_TOKENIZE_GEN_H
