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
// Two parallel shadow stacks mirror Lemon's symbol stack:
//
//   * `extent_stack` tracks the merged *authored* range in root-source
//     coordinates — used by `syntaqlite_parser_node_text`.  Macro
//     tokens push the outermost call-site range stashed in
//     `begin_macro_expansion`.  Epsilon pushes the sentinel
//     `(UINT32_MAX, 0)`, which is neutral under min/max merging.
//     Only maintained when `collect_node_extents` is enabled.
//
//   * `expanded_stack` tracks the merged *expanded* range in the
//     tokens' own layer — used by
//     `syntaqlite_parser_node_expanded_text`.  Same-layer merges keep
//     the layer; mixed-layer merges collapse to the sentinel
//     `(length=0)`, since no contiguous expansion slice can represent
//     a node whose tokens cross layers.
//     Always maintained — also carries `macro_root` and `from_shift`
//     for O(1) macro straddle detection.

void synq_extent_on_shift(SynqParseCtx* pCtx,
                          unsigned int major,
                          const SynqParseToken* token) {
  (void)major;

  uint32_t macro_root =
      (token->layer_id != 0) ? pCtx->macro_root_layer : 0;
  SynqNodeExpandedExtent e = {
      .offset = token->offset,
      .length = token->n,
      .layer_id = token->layer_id,
      .macro_root = macro_root,
      .from_shift = 1,
  };
  syntaqlite_vec_push(&pCtx->expanded_stack, e, pCtx->mem);

  if (!pCtx->collect_node_extents) {
    return;
  }
  SynqExtentRange r;
  if (token->layer_id == 0) {
    r.root_start = token->offset;
    r.root_end = token->offset + token->n;
  } else {
    r.root_start = pCtx->macro_root_start;
    r.root_end = pCtx->macro_root_end;
  }
  syntaqlite_vec_push(&pCtx->extent_stack, r, pCtx->mem);
}

void synq_extent_on_reduce(SynqParseCtx* pCtx, unsigned int nrhs) {
  uint32_t exp_len = syntaqlite_vec_len(&pCtx->expanded_stack);

  // Straddle detection: if any two terminal (from_shift=1) entries
  // have different macro_root values, the expansion boundary cuts
  // through this grammar production.
  if (!pCtx->has_macro_straddle) {
    uint32_t first_term_root = SYNQ_MACRO_ROOT_NEUTRAL;
    for (uint32_t i = exp_len - nrhs; i < exp_len; i++) {
      SynqNodeExpandedExtent e =
          syntaqlite_vec_at(&pCtx->expanded_stack, i);
      if (!e.from_shift)
        continue;
      if (first_term_root == SYNQ_MACRO_ROOT_NEUTRAL) {
        first_term_root = e.macro_root;
      } else if (e.macro_root != first_term_root) {
        pCtx->has_macro_straddle = 1;
        break;
      }
    }
  }

  // Merge expanded-layer spans.
  SynqNodeExpandedExtent exp_merged = {
      0, 0, 0, SYNQ_MACRO_ROOT_NEUTRAL, 0};
  for (uint32_t i = exp_len - nrhs; i < exp_len; i++) {
    SynqNodeExpandedExtent e = syntaqlite_vec_at(&pCtx->expanded_stack, i);

    if (exp_merged.macro_root == SYNQ_MACRO_ROOT_NEUTRAL &&
        e.macro_root != SYNQ_MACRO_ROOT_NEUTRAL) {
      exp_merged.macro_root = e.macro_root;
    }

    if (e.layer_id == SYNQ_CROSS_LAYER) {
      uint32_t saved_mr = exp_merged.macro_root;
      exp_merged = e;
      exp_merged.macro_root = saved_mr;
      exp_merged.from_shift = 0;
      break;
    }
    if (e.length == 0) {
      continue;
    }
    if (exp_merged.length == 0) {
      exp_merged.layer_id = e.layer_id;
      exp_merged.offset = e.offset;
      exp_merged.length = e.length;
      continue;
    }
    if (exp_merged.layer_id != e.layer_id) {
      uint32_t saved_mr = exp_merged.macro_root;
      exp_merged =
          (SynqNodeExpandedExtent){0, 0, SYNQ_CROSS_LAYER, 0, 0};
      exp_merged.macro_root = saved_mr;
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
  exp_merged.from_shift = 0;
  syntaqlite_vec_truncate(&pCtx->expanded_stack, exp_len - nrhs);
  syntaqlite_vec_push(&pCtx->expanded_stack, exp_merged, pCtx->mem);

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
}
