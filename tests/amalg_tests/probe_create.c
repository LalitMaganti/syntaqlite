// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// API surface probe: dialect-named create wrappers
// (syntaqlite_parser_create_<dialect>, syntaqlite_tokenizer_create_<dialect>).
// The amalgamation emits these unconditionally for any named dialect, so this
// probe is expected to compile in every (dialect, mode, extra_cflags) variant.

#include GRAMMAR_HEADER

#define SYNQ_PASTE_(a, b) a##b
#define SYNQ_PASTE(a, b) SYNQ_PASTE_(a, b)
#define SYNQ_PARSER_CREATE SYNQ_PASTE(syntaqlite_parser_create_, DIALECT_NAME)
#define SYNQ_TOKENIZER_CREATE \
  SYNQ_PASTE(syntaqlite_tokenizer_create_, DIALECT_NAME)

int main(void) {
  SyntaqliteParser* p = SYNQ_PARSER_CREATE(NULL);
  SyntaqliteTokenizer* t = SYNQ_TOKENIZER_CREATE(NULL);
  syntaqlite_parser_destroy(p);
  syntaqlite_tokenizer_destroy(t);
  return 0;
}
