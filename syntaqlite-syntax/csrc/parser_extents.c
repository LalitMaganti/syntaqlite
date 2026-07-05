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
//   * `extent_stack` carries both the merged *authored* byte range in
//     root-source coordinates (used by `syntaqlite_parser_node_text`)
//     and the inclusive token-index range into `p->tokens` (used by
//     `syntaqlite_node_token_range`).  Macro tokens push the outermost
//     call-site byte range stashed in `begin_macro_expansion` with
//     their real `token_idx` (since the token-stream unification,
//     every shifted terminal has an index regardless of layer).
//     Epsilon pushes a sentinel that is neutral under min/max merging
//     for both ranges.
//
//   * `expanded_stack` tracks the merged *expanded* range in the
//     tokens' own layer — used by
//     `syntaqlite_parser_node_expanded_text`.  Same-layer merges keep
//     the layer; mixed-layer merges collapse to the sentinel
//     `(length=0)`, since no contiguous expansion slice can represent
//     a node whose tokens cross layers.
//
// Independently, `straddle_stack` (a lightweight uint32_t vec) tracks
// macro_root per Lemon stack symbol for O(1) straddle detection.  It
// is lazily initialized on first macro use via `lemon_depth`; without
// macros the only cost is one integer increment/decrement per
// shift/reduce.

// Merge `e` into `acc`: min/max over the authored byte range, min/max
// over the token-index range with UINT32_MAX as the "no tokens"
// sentinel on either side.
static void synq_extent_merge(SynqExtentRange* acc, SynqExtentRange e) {
  if (e.root_start < acc->root_start) {
    acc->root_start = e.root_start;
  }
  if (e.root_end > acc->root_end) {
    acc->root_end = e.root_end;
  }
  if (e.first_tok != UINT32_MAX) {
    if (acc->first_tok == UINT32_MAX || e.first_tok < acc->first_tok) {
      acc->first_tok = e.first_tok;
    }
    if (acc->last_tok == UINT32_MAX || e.last_tok > acc->last_tok) {
      acc->last_tok = e.last_tok;
    }
  }
}

// Merge `e` into `acc` in expanded-layer coordinates: same-layer merges
// union the ranges, epsilon ({0,0,0}) is neutral, and cross-layer
// combinations poison `acc` (SYNQ_CROSS_LAYER), which then absorbs all
// further merges.
static void synq_expanded_merge(SynqNodeExpandedExtent* acc,
                                SynqNodeExpandedExtent e) {
  if (acc->layer_id == SYNQ_CROSS_LAYER) {
    return;  // already poisoned
  }
  if (e.layer_id == SYNQ_CROSS_LAYER) {
    *acc = e;  // propagate poison
    return;
  }
  if (e.length == 0) {
    return;  // epsilon
  }
  if (acc->length == 0) {
    *acc = e;
    return;
  }
  if (acc->layer_id != e.layer_id) {
    *acc = (SynqNodeExpandedExtent){0, 0, SYNQ_CROSS_LAYER};
    return;
  }
  uint32_t start = acc->offset < e.offset ? acc->offset : e.offset;
  uint32_t end_a = acc->offset + acc->length;
  uint32_t end_b = e.offset + e.length;
  uint32_t end = end_a > end_b ? end_a : end_b;
  acc->offset = start;
  acc->length = end - start;
}

void synq_extent_on_shift(SynqParseCtx* pCtx,
                          unsigned int major,
                          const SynqParseToken* token) {
  (void)major;

  pCtx->lemon_depth++;

  // Straddle stack: only active after first macro (macro_root_layer > 0).
  if (pCtx->macro_root_layer) {
    uint32_t mr = (token->layer_id != 0) ? pCtx->macro_root_layer : 0;
    syntaqlite_vec_push(&pCtx->straddle_stack, mr, pCtx->mem);
  }

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
  // Token-index range: valid when the shifted token has a real index
  // in `p->tokens` (all shifted terminals since the token-stream
  // unification, regardless of layer).  UINT32_MAX means "no token
  // recorded" (collect_tokens off, or layer-N shift with no index).
  if (token->token_idx == 0xFFFFFFFFu) {
    r.first_tok = UINT32_MAX;
    r.last_tok = UINT32_MAX;
  } else {
    r.first_tok = token->token_idx;
    r.last_tok = token->token_idx;
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
  // Reduce pops nrhs symbols and pushes 1: net change = 1 - nrhs.
  pCtx->lemon_depth = pCtx->lemon_depth + 1 - nrhs;

  // Straddle detection on the lightweight stack.
  if (pCtx->macro_root_layer) {
    uint32_t slen = syntaqlite_vec_len(&pCtx->straddle_stack);
    if (!pCtx->has_macro_straddle) {
      uint32_t first = SYNQ_STRADDLE_NEUTRAL;
      for (uint32_t i = slen - nrhs; i < slen; i++) {
        uint32_t v = syntaqlite_vec_at(&pCtx->straddle_stack, i);
        if (v == SYNQ_STRADDLE_NEUTRAL)
          continue;
        if (first == SYNQ_STRADDLE_NEUTRAL) {
          first = v;
        } else if (v != first) {
          pCtx->has_macro_straddle = 1;
          break;
        }
      }
    }
    syntaqlite_vec_truncate(&pCtx->straddle_stack, slen - nrhs);
    syntaqlite_vec_push(&pCtx->straddle_stack, SYNQ_STRADDLE_NEUTRAL,
                        pCtx->mem);
  }

  if (!pCtx->collect_node_extents) {
    return;
  }
  uint32_t len = syntaqlite_vec_len(&pCtx->extent_stack);

  // Merge both the authored byte range and the token-index range in a
  // single pass.  Each is tracked with its own sentinel
  // (byte: root_start==UINT32_MAX && root_end==0; token: first_tok==UINT32_MAX)
  // so a node that reduced over macro-expansion-only tokens keeps its
  // byte range (from the call site) even when token indices are absent,
  // and vice versa.
  SynqExtentRange merged = {UINT32_MAX, 0, UINT32_MAX, UINT32_MAX};
  for (uint32_t i = len - nrhs; i < len; i++) {
    synq_extent_merge(&merged, syntaqlite_vec_at(&pCtx->extent_stack, i));
  }
  syntaqlite_vec_truncate(&pCtx->extent_stack, len - nrhs);
  syntaqlite_vec_push(&pCtx->extent_stack, merged, pCtx->mem);

  SynqNodeExpandedExtent exp_merged = {0, 0, 0};
  for (uint32_t i = len - nrhs; i < len; i++) {
    synq_expanded_merge(&exp_merged,
                        syntaqlite_vec_at(&pCtx->expanded_stack, i));
  }
  syntaqlite_vec_truncate(&pCtx->expanded_stack, len - nrhs);
  syntaqlite_vec_push(&pCtx->expanded_stack, exp_merged, pCtx->mem);
}

void synq_extent_fold_below_into_top(SynqParseCtx* pCtx) {
  if (!pCtx->collect_node_extents) {
    return;
  }
  uint32_t len = syntaqlite_vec_len(&pCtx->extent_stack);
  if (len < 2) {
    return;
  }
  synq_extent_merge(&syntaqlite_vec_at(&pCtx->extent_stack, len - 1),
                    syntaqlite_vec_at(&pCtx->extent_stack, len - 2));
  synq_expanded_merge(&syntaqlite_vec_at(&pCtx->expanded_stack, len - 1),
                      syntaqlite_vec_at(&pCtx->expanded_stack, len - 2));
}
