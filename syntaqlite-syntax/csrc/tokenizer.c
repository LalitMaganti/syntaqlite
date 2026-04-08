// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/tokenizer.h"

#include <string.h>

#include "csrc/token_wrapped.h"
#include "syntaqlite/grammar.h"

struct SyntaqliteTokenizer {
  SyntaqliteMemMethods mem;
  SyntaqliteGrammar env;
  const char* source;
  uint32_t len;
  uint32_t offset;
};

SYNTAQLITE_API SyntaqliteTokenizer* syntaqlite_tokenizer_create_with_grammar(
    const SyntaqliteMemMethods* mem,
    const SyntaqliteGrammar env) {
  SyntaqliteMemMethods m = mem ? *mem : SYNTAQLITE_MEM_METHODS_DEFAULT;
  SyntaqliteTokenizer* tok = m.xMalloc(sizeof(SyntaqliteTokenizer));
  memset(tok, 0, sizeof(*tok));
  tok->mem = m;
  tok->env = env;
  return tok;
}

#ifndef SYNTAQLITE_OMIT_SQLITE_API
SYNTAQLITE_API SyntaqliteTokenizer* syntaqlite_tokenizer_create(
    const SyntaqliteMemMethods* mem) {
  SyntaqliteGrammar env = syntaqlite_sqlite_grammar();
  return syntaqlite_tokenizer_create_with_grammar(mem, env);
}
#endif

SYNTAQLITE_API void syntaqlite_tokenizer_reset(SyntaqliteTokenizer* tok,
                                               const char* source,
                                               uint32_t len) {
  tok->source = source;
  tok->len = len;
  tok->offset = 0;
}

SYNTAQLITE_API uint32_t syntaqlite_tokenizer_next(SyntaqliteTokenizer* tok,
                                                  SyntaqliteToken* out) {
  if (tok->offset >= tok->len) {
    return 0;
  }

  uint32_t token_type = 0;
  int64_t token_len = SynqSqliteGetTokenVersionWrapped(
      &tok->env, (const unsigned char*)tok->source + tok->offset, &token_type);

  out->text = tok->source + tok->offset;
  out->length = (uint32_t)token_len;
  out->type = token_type;

  tok->offset += (uint32_t)token_len;
  return 1;
}

SYNTAQLITE_API void syntaqlite_tokenizer_destroy(SyntaqliteTokenizer* tok) {
  if (tok) {
    tok->mem.xFree(tok);
  }
}
