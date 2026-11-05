// Copyright 2026 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Links against the static libsyntaqlite and calls into both its C parser
// and its Rust formatter. See tools/check-static-library.

#include "syntaqlite/formatter.h"
#include "syntaqlite/parser.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  const char* sql = "select 1";

  SyntaqliteParser* p = syntaqlite_parser_create(NULL);
  syntaqlite_parser_reset(p, sql, (uint32_t)strlen(sql));
  int32_t rc = syntaqlite_parser_next(p);
  syntaqlite_parser_destroy(p);
  if (rc == SYNTAQLITE_PARSE_ERROR || rc == SYNTAQLITE_PARSE_DONE) {
    fprintf(stderr, "failed to parse %s\n", sql);
    return 1;
  }

  SyntaqliteFormatter* f = syntaqlite_formatter_create_sqlite();
  int ok = syntaqlite_formatter_format(f, sql, (uint32_t)strlen(sql)) == 0 &&
           strncmp(syntaqlite_formatter_output(f), "SELECT 1;",
                   strlen("SELECT 1;")) == 0;
  syntaqlite_formatter_destroy(f);
  if (!ok) {
    fprintf(stderr, "failed to format %s\n", sql);
    return 1;
  }

  printf("libsyntaqlite static smoke test: OK\n");
  return 0;
}
