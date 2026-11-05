// Copyright 2026 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Links against a prebuilt libsyntaqlite-parser and parses a statement, to
// check the library exports and works. See tools/build-parser-library.

#include "libsyntaqlite_parser.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  const char* sql = "SELECT id FROM users; SELECT 1";
  SyntaqliteParser* p = syntaqlite_parser_create(NULL);
  syntaqlite_parser_reset(p, sql, (uint32_t)strlen(sql));

  int statements = 0;
  for (;;) {
    int32_t rc = syntaqlite_parser_next(p);
    if (rc == SYNTAQLITE_PARSE_DONE) {
      break;
    }
    if (rc == SYNTAQLITE_PARSE_ERROR) {
      fprintf(stderr, "parse error: %s\n", syntaqlite_result_error_msg(p));
      syntaqlite_parser_destroy(p);
      return 1;
    }
    statements++;
  }
  syntaqlite_parser_destroy(p);

  if (statements != 2) {
    fprintf(stderr, "expected 2 statements, got %d\n", statements);
    return 1;
  }
  printf("libsyntaqlite-parser smoke test: OK\n");
  return 0;
}
