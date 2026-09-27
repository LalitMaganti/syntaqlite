// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Node expansion lets the grammar mark a node for the host to replace with its
// own text. The host expands the marked nodes once their statement is parsed,
// so it sees the whole statement, and each replacement is recorded as a
// rewrite next to macro calls, so it works everywhere they do.

#include <stdio.h>
#include <string.h>

#include "csrc/parser_internal.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite/parser.h"
#include "syntaqlite_dialect/ast_builder.h"

#ifndef SYNTAQLITE_OMIT_MACROS

SYNTAQLITE_API int32_t
syntaqlite_parser_set_node_expander(SyntaqliteParser* p,
                                    SyntaqliteNodeExpandFn fn,
                                    void* user_data) {
  p->node_expansion.expander = fn;
  p->node_expansion.user_data = user_data;
  return SYNTAQLITE_OK;
}

SYNTAQLITE_API void syntaqlite_node_expansion_set_result(SyntaqliteParser* p,
                                                         const char* text,
                                                         SyntaqliteLength len) {
  SynqNodeExpansionState* s = &p->node_expansion;
  if (s->result)
    p->mem.xFree(s->result);
  s->result = p->mem.xMalloc(len + 1);
  memcpy(s->result, text, len);
  s->result[len] = '\0';
  s->result_len = len;
}

// Expands the nodes the statement marked. Returns 0 if one fails, with the
// parser's error set.
static int synq_expand_marked_nodes(SyntaqliteParser* p) {
  SynqNodeExpansionState* s = &p->node_expansion;
  if (!s->expander)
    return 1;
  // Marked in the order they were parsed, so a node inside another is
  // expanded first.
  for (uint32_t i = 0; i < syntaqlite_vec_len(&p->ctx.marked_nodes); i++) {
    SynqMarkedNode node = syntaqlite_vec_at(&p->ctx.marked_nodes, i);
    uint32_t home = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    int ok = synq_node_site(p, node.node_id, &home, &start, &end) &&
             s->expander(s->user_data, p, node.node_id) ==
                 SYNTAQLITE_NODE_EXPAND_OK &&
             s->result;
    if (!ok) {
      if (s->result)
        p->mem.xFree(s->result);
      s->result = NULL;
      if (p->error_msg[0] == '\0') {
        snprintf(p->error_msg, sizeof(p->error_msg), "expanding %.*s failed",
                 (int)node.name_len, node.name);
      }
      return 0;
    }

    // Recorded as a layer, like a macro call's, but never fed back into the
    // parser.
    uint32_t body_offset = 0;
    uint32_t body_length = 0;
    synq_body_call_range(&p->rewrites.layers.data[home], start, end - start,
                         &body_offset, &body_length);
    SynqExpansionLayer layer = {
        .expansion_data = s->result,
        .expansion_len = s->result_len,
        .call_offset = start,
        .call_length = end - start,
        .name = node.name,
        .name_len = node.name_len,
        .body_call_offset = body_offset,
        .body_call_length = body_length,
        .parent_layer_id = home,
        .kind = SYNTAQLITE_REWRITE_NODE_EXPANSION,
    };
    syntaqlite_vec_push(&p->rewrites.layers, layer, p->mem);
    s->result = NULL;
  }
  return 1;
}

int synq_parser_expand_nodes(SyntaqliteParser* p) {
  int ok = synq_expand_marked_nodes(p);
  // Each marked node is expanded at most once, whether or not all were.
  syntaqlite_vec_clear(&p->ctx.marked_nodes);
  return ok;
}

#endif  // !SYNTAQLITE_OMIT_MACROS
