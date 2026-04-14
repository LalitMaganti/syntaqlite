// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Compile-only driver that verifies the amalgamation builds cleanly
// under strict C++ warnings: -Wall -Wextra -Werror.
//
// This file is NOT executed — it only needs to compile successfully.
// If the amalgamation introduces any warnings, this test will fail at
// build time, catching regressions before they reach downstream consumers.

#include GRAMMAR_HEADER

int main() {
  SyntaqliteDialect env = GRAMMAR_FN();
  SyntaqliteParser* p = syntaqlite_parser_create_with_dialect(nullptr, env);
  const char* sql = "SELECT 1";
  syntaqlite_parser_reset(p, sql, 8);
  syntaqlite_parser_next(p);
  syntaqlite_parser_destroy(p);
  return 0;
}
