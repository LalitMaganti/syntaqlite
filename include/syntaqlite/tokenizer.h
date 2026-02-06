// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Streaming tokenizer for SQLite SQL.
//
// Usage:
//   SyntaqliteTokenizer *tok = syntaqlite_tokenizer_create();
//   syntaqlite_tokenizer_reset(tok, sql, len);
//   SyntaqliteToken token;
//   while (syntaqlite_tokenizer_next(tok, &token)) {
//     // process token
//   }
//   syntaqlite_tokenizer_destroy(tok);

#ifndef SYNTAQLITE_TOKENIZER_H
#define SYNTAQLITE_TOKENIZER_H

#include <stdint.h>

// Token type constants (TK_SELECT, TK_SEMI, etc.)
// When SYNTAQLITE_CUSTOM_TOKENS is defined, the generated header
// automatically includes the dialect-specific tokens instead.
#include "syntaqlite/sqlite_tokens_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

// Token returned by the tokenizer.
typedef struct SyntaqliteToken {
    const char *text;   // Pointer into source text (not null-terminated)
    uint32_t length;    // Token length in bytes
    uint16_t type;      // TK_* token type
} SyntaqliteToken;

// Opaque tokenizer handle.
typedef struct SyntaqliteTokenizer SyntaqliteTokenizer;

// Create a tokenizer. Call reset() to bind a source buffer before use.
SyntaqliteTokenizer *syntaqlite_tokenizer_create(void);

// Bind (or rebind) a source buffer and reset the cursor to the beginning.
// The source must remain valid until the next reset or destroy.
void syntaqlite_tokenizer_reset(SyntaqliteTokenizer *tok, const char *source,
                                uint32_t len);

// Get the next token. Returns 1 if a token was produced, 0 at EOF.
// All tokens are returned including TK_SPACE and TK_COMMENT.
int syntaqlite_tokenizer_next(SyntaqliteTokenizer *tok, SyntaqliteToken *out);

// Destroy the tokenizer and free resources.
void syntaqlite_tokenizer_destroy(SyntaqliteTokenizer *tok);

#ifdef __cplusplus
}
#endif

#endif // SYNTAQLITE_TOKENIZER_H
