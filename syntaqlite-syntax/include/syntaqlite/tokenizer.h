// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Streaming tokenizer for SQLite SQL.
//
// The lowest-level entry point — splits SQL text into a flat sequence of
// tokens without any parsing or tree structure. Most users don't need this
// directly; the parser (parser.h) handles tokenization internally.
// Use this when you need raw token access
// (syntax highlighting, custom analysis, etc.).
//
// Lifecycle: create → reset → next (loop) → destroy.
//
// Usage:
//   SyntaqliteTokenizer* tok = syntaqlite_tokenizer_create(NULL);
//   syntaqlite_tokenizer_reset(tok, sql, len);
//   SyntaqliteToken token;
//   while (syntaqlite_tokenizer_next(tok, &token)) {
//     // process token.type, token.text, token.length
//   }
//   syntaqlite_tokenizer_destroy(tok);

#ifndef SYNTAQLITE_TOKENIZER_H
#define SYNTAQLITE_TOKENIZER_H

#include <stdint.h>

#include "syntaqlite/config.h"
#include "syntaqlite/grammar.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

// Opaque tokenizer handle (heap-allocated, reusable across inputs).
typedef struct SyntaqliteTokenizer SyntaqliteTokenizer;

// A single token produced by the tokenizer. The text pointer points into
// the source buffer bound by the last reset() call, so it is only valid
// while that buffer is alive. The text is NOT null-terminated.
typedef struct SyntaqliteToken {
  const char* text;  // Pointer into source text.
  uint32_t length;   // Token length in bytes.
  uint32_t type;     // Token type ordinal.
} SyntaqliteToken;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Allocate a tokenizer bound to a dialect environment. The env is copied —
// the caller's struct does not need to outlive the tokenizer, but the dialect
// pointer inside must remain valid for the tokenizer's lifetime.
// The mem methods are copied — pass NULL for all defaults (malloc/free).
SYNTAQLITE_API SyntaqliteTokenizer* syntaqlite_tokenizer_create_with_grammar(
    const SyntaqliteMemMethods* mem,
    SyntaqliteGrammar env);

// Bind a source buffer and start tokenizing from the beginning. The source
// must remain valid until the next reset() or destroy(). Can be called
// again to tokenize a new input without reallocating.
SYNTAQLITE_API void syntaqlite_tokenizer_reset(SyntaqliteTokenizer* tok,
                                               const char* source,
                                               uint32_t len);

// Advance to the next token. Returns 1 if a token was written to *out,
// 0 at end-of-input. Every token is returned, including whitespace and
// comments.
SYNTAQLITE_API uint32_t syntaqlite_tokenizer_next(SyntaqliteTokenizer* tok,
                                                  SyntaqliteToken* out);

// Free the tokenizer and all its memory. No-op if tok is NULL.
SYNTAQLITE_API void syntaqlite_tokenizer_destroy(SyntaqliteTokenizer* tok);

// ---------------------------------------------------------------------------
// SQLite dialect convenience (opt-out: -DSYNTAQLITE_OMIT_SQLITE_API)
// ---------------------------------------------------------------------------

#ifndef SYNTAQLITE_OMIT_SQLITE_API
// Allocate a tokenizer for the built-in SQLite grammar.
// The mem methods are copied — pass NULL for all defaults (malloc/free).
SYNTAQLITE_API SyntaqliteTokenizer* syntaqlite_tokenizer_create(
    const SyntaqliteMemMethods* mem);

SYNTAQLITE_API SyntaqliteGrammar syntaqlite_sqlite_grammar(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_TOKENIZER_H
