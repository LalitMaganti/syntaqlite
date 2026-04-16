// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Per-node extent tracking hooks, invoked from Lemon's yy_shift / yy_reduce
// via the macros in extent_hooks.h.  Operates on SynqParseCtx* — conceptually
// part of the AST builder, split into its own file to keep ast_builder.h
// declaration-only.

#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/extent_hooks.h"

// ---------------------------------------------------------------------------
// Per-node extent tracking hooks
// ---------------------------------------------------------------------------
//
// When enabled, two parallel shadow stacks mirror Lemon's symbol stack:
//
//   * `extent_stack` tracks the merged *authored* range in root-source
//     coordinates — used by `syntaqlite_parser_node_text`.  Macro
//     tokens push the outermost call-site range stashed in
//     `begin_macro_expansion`.  Epsilon pushes the sentinel
//     `(UINT32_MAX, 0)`, which is neutral under min/max merging.
//
//   * `expanded_stack` tracks the merged *expanded* range in the
//     tokens' own layer — used by
//     `syntaqlite_parser_node_expanded_text`.  Same-layer merges keep
//     the layer; mixed-layer merges collapse to the sentinel
//     `(length=0)`, since no contiguous expansion slice can represent
//     a node whose tokens cross layers.

void synq_extent_on_shift(SynqParseCtx* pCtx,
                          unsigned int major,
                          const SynqParseToken* token) {
  (void)major;
  if (!pCtx->collect_node_extents) {
    return;
  }
  SynqExtentRange r;
  if (token->layer_id == 0) {
    r.root_start = token->offset;
    r.root_end = token->offset + token->n;
  } else {
    // Attribute tokens from inside a macro expansion to the outermost
    // call-site range stashed in `begin_macro_expansion`.
    r.root_start = pCtx->macro_root_start;
    r.root_end = pCtx->macro_root_end;
  }
  syntaqlite_vec_push(&pCtx->extent_stack, r, pCtx->mem);

  SynqNodeExpandedExtent e = {
      .offset = token->offset,
      .length = token->n,
      .layer_id = token->layer_id,
  };
  syntaqlite_vec_push(&pCtx->expanded_stack, e, pCtx->mem);
}

void synq_extent_on_reduce(SynqParseCtx* pCtx, unsigned int nrhs) {
  if (!pCtx->collect_node_extents) {
    return;
  }
  uint32_t len = syntaqlite_vec_len(&pCtx->extent_stack);

  SynqExtentRange merged = {UINT32_MAX, 0};
  for (uint32_t i = len - nrhs; i < len; i++) {
    SynqExtentRange e = syntaqlite_vec_at(&pCtx->extent_stack, i);
    if (e.root_start < merged.root_start) {
      merged.root_start = e.root_start;
    }
    if (e.root_end > merged.root_end) {
      merged.root_end = e.root_end;
    }
  }
  syntaqlite_vec_truncate(&pCtx->extent_stack, len - nrhs);
  syntaqlite_vec_push(&pCtx->extent_stack, merged, pCtx->mem);

  // Merge expanded-layer spans: all same layer → merge in that layer;
  // epsilon entries ({0,0,0}) are neutral; cross-layer poison
  // (layer_id == SYNQ_CROSS_LAYER) propagates upward unconditionally.
  SynqNodeExpandedExtent exp_merged = {0, 0, 0};
  for (uint32_t i = len - nrhs; i < len; i++) {
    SynqNodeExpandedExtent e = syntaqlite_vec_at(&pCtx->expanded_stack, i);
    if (e.layer_id == SYNQ_CROSS_LAYER) {
      exp_merged = e;  // propagate poison
      break;
    }
    if (e.length == 0) {
      continue;  // skip epsilon
    }
    if (exp_merged.length == 0) {
      exp_merged = e;
      continue;
    }
    if (exp_merged.layer_id != e.layer_id) {
      exp_merged = (SynqNodeExpandedExtent){0, 0, SYNQ_CROSS_LAYER};
      break;
    }
    uint32_t start =
        exp_merged.offset < e.offset ? exp_merged.offset : e.offset;
    uint32_t end_a = exp_merged.offset + exp_merged.length;
    uint32_t end_b = e.offset + e.length;
    uint32_t end = end_a > end_b ? end_a : end_b;
    exp_merged.offset = start;
    exp_merged.length = end - start;
  }
  syntaqlite_vec_truncate(&pCtx->expanded_stack, len - nrhs);
  syntaqlite_vec_push(&pCtx->expanded_stack, exp_merged, pCtx->mem);
}
