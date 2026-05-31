// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Compile-only driver that verifies the amalgamation builds cleanly
// under strict C++ warnings: -Wall -Wextra -Werror.
//
// This file is NOT executed — it only needs to compile successfully.
// If the amalgamation introduces any warnings, this test will fail at
// build time, catching regressions before they reach downstream consumers.

#include GRAMMAR_HEADER

#define SYNQ_PASTE_(a, b) a##b
#define SYNQ_PASTE(a, b) SYNQ_PASTE_(a, b)
#define SYNQ_PARSER_CREATE SYNQ_PASTE(syntaqlite_parser_create_, DIALECT_NAME)

int main() {
  // Use the dialect-named create wrapper, which the amalgamation emits
  // unconditionally for any named dialect (the generic *_with_dialect entry
  // point is hidden in Full mode now that dispatch is inlined by default).
  SyntaqliteParser* p = SYNQ_PARSER_CREATE(nullptr);
  const char* sql = "SELECT 1";
  syntaqlite_parser_reset(p, sql, 8);
  syntaqlite_parser_next(p);
  syntaqlite_parser_destroy(p);
  return 0;
}
