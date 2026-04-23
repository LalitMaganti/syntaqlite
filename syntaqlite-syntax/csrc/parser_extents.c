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
//
// Independently, `straddle_stack` (a lightweight uint32_t vec) tracks
// macro_root per Lemon stack symbol for O(1) straddle detection.  It
// is lazily initialized on first macro use via `lemon_depth`; without
// macros the only cost is one integer increment/decrement per
// shift/reduce.

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
  syntaqlite_vec_push(&pCtx->extent_stack, r, pCtx->mem);

  SynqNodeExpandedExtent e = {
      .offset = token->offset,
      .length = token->n,
      .layer_id = token->layer_id,
  };
  syntaqlite_vec_push(&pCtx->expanded_stack, e, pCtx->mem);

  // Token-index shadow: push the shifted token's index (or epsilon
  // sentinel when collect_tokens is off and token_idx is UINT32_MAX).
  // With the unified token-stream, this indexes into `p->tokens` for
  // every shifted token regardless of layer, so node_token_range
  // reports ranges that span macro expansions.
  SynqTokenRange tr;
  if (token->token_idx == 0xFFFFFFFFu) {
    tr.first_tok = UINT32_MAX;
    tr.last_tok = UINT32_MAX;
  } else {
    tr.first_tok = token->token_idx;
    tr.last_tok = token->token_idx;
  }
  syntaqlite_vec_push(&pCtx->token_range_stack, tr, pCtx->mem);
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

  // Merge token-index ranges: min/max over non-sentinel entries.
  SynqTokenRange tr_merged = {UINT32_MAX, UINT32_MAX};
  for (uint32_t i = len - nrhs; i < len; i++) {
    SynqTokenRange t = syntaqlite_vec_at(&pCtx->token_range_stack, i);
    if (t.first_tok == UINT32_MAX) {
      continue;  // epsilon
    }
    if (tr_merged.first_tok == UINT32_MAX ||
        t.first_tok < tr_merged.first_tok) {
      tr_merged.first_tok = t.first_tok;
    }
    if (tr_merged.last_tok == UINT32_MAX || t.last_tok > tr_merged.last_tok) {
      tr_merged.last_tok = t.last_tok;
    }
  }
  syntaqlite_vec_truncate(&pCtx->token_range_stack, len - nrhs);
  syntaqlite_vec_push(&pCtx->token_range_stack, tr_merged, pCtx->mem);
}
