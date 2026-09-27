// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Minimal AST dumper for amalgamation integration tests.
//
// Compiled against a generated syntaqlite_<dialect>.{h,c} amalgamation.
// Reads SQL from stdin, parses each statement, and dumps the AST, followed
// by any nodes the grammar handed to the node expander.
// The GRAMMAR_HEADER and GRAMMAR_FN macros are set at compile time to
// select the dialect header and dialect accessor function.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include GRAMMAR_HEADER

/* DIALECT_NAME is the bare token (e.g. `sqlite`, `perfetto`) so we can paste
   it to form the dialect-named create wrappers emitted by the amalgamation. */
#define SYNQ_PASTE_(a, b) a##b
#define SYNQ_PASTE(a, b) SYNQ_PASTE_(a, b)
#define SYNQ_PARSER_CREATE SYNQ_PASTE(syntaqlite_parser_create_, DIALECT_NAME)

#ifndef SYNTAQLITE_OMIT_MACROS
// Replaces each node with a query reading `expanded_<n>`, numbering nodes in
// the order they finish parsing, and says what it can see of the statement.
// Fails on a node whose text starts with "FROM unexpandable", to exercise
// errors.
static int expand_node(void* user_data, SyntaqliteParser* p, uint32_t node_id) {
  int* count = (int*)user_data;
  SyntaqliteLength len = 0;
  SyntaqliteStmtOffset offset = 0;
  const char* text = syntaqlite_parser_node_text(p, node_id, &len, &offset);
  // The whole statement is parsed by the time its nodes are expanded.
  uint32_t stmt_len = 0;
  const char* stmt = syntaqlite_parser_text(p, NULL, &stmt_len);
  printf("expanding \"%.*s\" in \"%.*s\"\n", (int)len, text, (int)stmt_len,
         stmt);
  static const char kFail[] = "FROM unexpandable";
  if (text && len >= sizeof(kFail) - 1 &&
      memcmp(text, kFail, sizeof(kFail) - 1) == 0) {
    return SYNTAQLITE_NODE_EXPAND_ERROR;
  }
  char out[64];
  int n = snprintf(out, sizeof(out), "SELECT * FROM expanded_%d", ++*count);
  syntaqlite_node_expansion_set_result(p, out, (SyntaqliteLength)n);
  return SYNTAQLITE_NODE_EXPAND_OK;
}

static void dump_node_expansions(SyntaqliteParser* p) {
  uint32_t count = syntaqlite_result_rewrite_count(p);
  for (uint32_t i = 0; i < count; i++) {
    SyntaqliteRewrite r = syntaqlite_result_rewrite_at(p, i);
    if (r.kind != SYNTAQLITE_REWRITE_NODE_EXPANSION) {
      continue;
    }
    printf("expanded %.*s", (int)r.name_len, r.name);
    if (r.parent_idx == SYNTAQLITE_REWRITE_PARENT_SOURCE) {
      printf(" in source");
    } else {
      printf(" in rewrite %u", r.parent_idx);
    }
    printf(": \"%.*s\" -> \"%.*s\"\n", (int)r.call_length,
           r.parent_buffer + r.call_offset, (int)r.expansion_len, r.expansion);
  }
}
#endif

int main(void) {
  static char buf[256 * 1024];
  size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
  buf[n] = '\0';

  SyntaqliteParser* p = SYNQ_PARSER_CREATE(NULL);
#ifndef SYNTAQLITE_OMIT_MACROS
  int expansions = 0;
  syntaqlite_parser_set_collect_node_extents(p, 1);
  syntaqlite_parser_set_node_expander(p, expand_node, &expansions);
#endif
  syntaqlite_parser_reset(p, buf, (uint32_t)n);

  int32_t rc;
  int count = 0;

  while ((rc = syntaqlite_parser_next(p)) != SYNTAQLITE_PARSE_DONE) {
    if (rc == SYNTAQLITE_PARSE_ERROR) {
      const char* msg = syntaqlite_result_error_msg(p);
      printf("parse error: %s\n", msg ? msg : "unknown");
      break;
    }
    if (count > 0)
      printf("----\n");
    char* dump = syntaqlite_dump_node(p, syntaqlite_result_root(p), 0);
    if (dump) {
      fputs(dump, stdout);
      free(dump);
    }
#ifndef SYNTAQLITE_OMIT_MACROS
    dump_node_expansions(p);
#endif
    count++;
  }

  syntaqlite_parser_destroy(p);
  return 0;
}
