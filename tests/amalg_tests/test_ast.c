// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Minimal AST dumper for amalgamation integration tests.
//
// Compiled against a generated syntaqlite_<dialect>.{h,c} amalgamation.
// Reads SQL from stdin, parses each statement, and dumps the AST.
// The GRAMMAR_HEADER and GRAMMAR_FN macros are set at compile time to
// select the grammar header and grammar accessor function.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include GRAMMAR_HEADER

int main(void) {
  static char buf[256 * 1024];
  size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
  buf[n] = '\0';

  SyntaqliteGrammar env = GRAMMAR_FN();
  SyntaqliteParser* p = syntaqlite_parser_create_with_grammar(NULL, env);
  syntaqlite_parser_reset(p, buf, (uint32_t)n);

  int32_t rc;
  int count = 0;

  while ((rc = syntaqlite_parser_next(p)) != SYNTAQLITE_PARSE_DONE) {
    if (rc == SYNTAQLITE_PARSE_ERROR) {
      const char* msg = syntaqlite_result_error_msg(p);
      fprintf(stderr, "parse error: %s\n", msg ? msg : "unknown");
      syntaqlite_parser_destroy(p);
      return 1;
    }
    if (count > 0)
      printf("----\n");
    char* dump = syntaqlite_dump_node(p, syntaqlite_result_root(p), 0);
    if (dump) {
      fputs(dump, stdout);
      free(dump);
    }
    count++;
  }

  syntaqlite_parser_destroy(p);
  return 0;
}
