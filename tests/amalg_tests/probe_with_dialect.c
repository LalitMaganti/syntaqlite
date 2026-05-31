// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// API surface probe: generic *_with_dialect entry points.
// The public header hides these in Full mode unless the user opts out
// (-DSYNTAQLITE_NO_INLINE_DIALECT_DISPATCH) or moves the runtime to a
// separate TU (-DSYNTAQLITE_OMIT_RUNTIME). Compile-fails the cases
// where calling them with a non-matching dialect would mis-parse.

#include GRAMMAR_HEADER

int main(void) {
  SyntaqliteDialect env = GRAMMAR_FN();
  SyntaqliteParser* p = syntaqlite_parser_create_with_dialect(NULL, env);
  SyntaqliteTokenizer* t = syntaqlite_tokenizer_create_with_dialect(NULL, env);
  syntaqlite_parser_destroy(p);
  syntaqlite_tokenizer_destroy(t);
  return 0;
}
