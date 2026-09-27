// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Node expansion lets the grammar hand a freshly parsed node to the host, which
// replaces it with its own text. The replacement is recorded as a rewrite next
// to macro calls, so it works everywhere they do.

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
  SynqExpansionLayer* lyr =
      &p->rewrites.layers.data[p->node_expansion.pending_layer];
  if (lyr->expansion_data)
    p->mem.xFree((void*)lyr->expansion_data);
  char* d = p->mem.xMalloc(len + 1);
  memcpy(d, text, len);
  d[len] = '\0';
  lyr->expansion_data = d;
  lyr->expansion_len = len;
}

void synq_parser_expand_node(SynqParseCtx* ctx,
                             uint32_t node_id,
                             const char* name,
                             uint32_t name_len) {
  SyntaqliteParser* p = (SyntaqliteParser*)ctx->parser;
  if (!p || !p->node_expansion.expander)
    return;
  uint32_t home = 0;
  uint32_t start = 0;
  uint32_t end = 0;
  if (!synq_node_site(p, node_id, &home, &start, &end))
    return;

  // Flush the lists inside this node so the host sees all of it. Lists
  // belonging to enclosing nodes are further down the stack and are left alone.
  uint32_t first_node =
      syntaqlite_vec_at(&ctx->node_bounds, node_id).first_node;
  while (syntaqlite_vec_len(&ctx->list_stack) > 0 &&
         syntaqlite_vec_at(&ctx->list_stack,
                           syntaqlite_vec_len(&ctx->list_stack) - 1)
                 .node_id >= first_node) {
    synq_parse_list_flush_top(ctx);
  }

  // Record the node as a layer, like a macro call. Unlike a macro, the
  // replacement is never fed back into the parser.
  uint32_t body_offset = 0;
  uint32_t body_length = 0;
  synq_body_call_range(&p->rewrites.layers.data[home], start, end - start,
                       &body_offset, &body_length);
  SynqExpansionLayer layer = {
      .call_offset = start,
      .call_length = end - start,
      .name = name,
      .name_len = name_len,
      .body_call_offset = body_offset,
      .body_call_length = body_length,
      .parent_layer_id = home,
      .kind = SYNTAQLITE_REWRITE_NODE_EXPANSION,
  };
  syntaqlite_vec_push(&p->rewrites.layers, layer, p->mem);
  uint32_t idx = syntaqlite_vec_len(&p->rewrites.layers) - 1;

  p->node_expansion.pending_layer = idx;
  int rc = p->node_expansion.expander(p->node_expansion.user_data, p, node_id);
  p->node_expansion.pending_layer = 0;

  if (rc != SYNTAQLITE_NODE_EXPAND_OK) {
    SynqExpansionLayer* lyr = &p->rewrites.layers.data[idx];
    if (lyr->expansion_data)
      p->mem.xFree((void*)lyr->expansion_data);
    p->rewrites.layers.count--;
    ctx->error = 1;
    if (p->error_msg[0] == '\0') {
      snprintf(p->error_msg, sizeof(p->error_msg), "expanding %.*s failed",
               (int)name_len, name);
    }
  }
}

#endif  // !SYNTAQLITE_OMIT_MACROS
