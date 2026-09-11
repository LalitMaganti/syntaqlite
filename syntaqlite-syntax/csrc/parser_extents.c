// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Per-node extent tracking hooks, invoked from Lemon's yy_shift / yy_reduce
// via the macros in extent_hooks.h.  Operates on SynqParseCtx* — conceptually
// part of the AST builder, split into its own file to keep ast_builder.h
// declaration-only.

#include <assert.h>

#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/extent_hooks.h"

// Token ranges on the live Lemon stack and a snapshot of the current RHS.
// AST-associated bindings are mutable current state, never immutable overlays.
typedef struct SynqSourceRange {
  uint32_t first, end;
} SynqSourceRange;
typedef struct SynqSourceBinding {
  uint32_t role, next;
  SynqSourceRange range;
} SynqSourceBinding;
// Live neighbours define a partition of the original token stream. A live
// token owns [itself, next); retiring it joins that interval to its
// predecessor. Original comment metadata remains valid, including comments
// recorded later.
typedef struct SynqSourceAnchor {
  uint32_t prev, next;
} SynqSourceAnchor;
struct SynqSourceBindings {
  SYNQ_VEC(SynqSourceAnchor) anchors;
  uint32_t first_anchor, last_anchor;
  SYNQ_VEC(SynqSourceRange) stack;
  SYNQ_VEC(SynqSourceRange) rhs;
  SYNQ_VEC(uint32_t) heads;
  SYNQ_VEC(SynqSourceBinding) bindings;
};

void synq_source_enable(SynqParseCtx* ctx, uint32_t enable) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  if (enable) {
    if (s)
      return;
    s = ctx->mem.xMalloc(sizeof(*s));
    syntaqlite_vec_init(&s->anchors);
    s->first_anchor = s->last_anchor = UINT32_MAX;
    syntaqlite_vec_init(&s->stack);
    syntaqlite_vec_init(&s->rhs);
    syntaqlite_vec_init(&s->heads);
    syntaqlite_vec_init(&s->bindings);
    ctx->source_bindings = s;
  } else if (s) {
    syntaqlite_vec_free(&s->anchors, ctx->mem);
    syntaqlite_vec_free(&s->stack, ctx->mem);
    syntaqlite_vec_free(&s->rhs, ctx->mem);
    syntaqlite_vec_free(&s->heads, ctx->mem);
    syntaqlite_vec_free(&s->bindings, ctx->mem);
    ctx->mem.xFree(s);
    ctx->source_bindings = NULL;
  }
}

void synq_source_clear(SynqParseCtx* ctx) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  syntaqlite_vec_clear(&s->anchors);
  s->first_anchor = s->last_anchor = UINT32_MAX;
  syntaqlite_vec_clear(&s->stack);
  syntaqlite_vec_clear(&s->rhs);
  syntaqlite_vec_clear(&s->heads);
  syntaqlite_vec_clear(&s->bindings);
}

// Called when a token is recorded, before feeding Lemon. A lookahead token
// can already exist when a reduction retires an earlier token.
void synq_source_record_token(SynqParseCtx* ctx) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  uint32_t index = syntaqlite_vec_len(&s->anchors);
  SynqSourceAnchor anchor = {s->last_anchor, UINT32_MAX};
  if (s->last_anchor != UINT32_MAX)
    syntaqlite_vec_at(&s->anchors, s->last_anchor).next = index;
  else
    s->first_anchor = index;
  syntaqlite_vec_push(&s->anchors, anchor, ctx->mem);
  s->last_anchor = index;
}

uint32_t synq_source_anchor_end(const SynqParseCtx* ctx, uint32_t token) {
  const struct SynqSourceBindings* s = ctx->source_bindings;
  if (!s)
    return UINT32_MAX;
  uint32_t count = syntaqlite_vec_len(&s->anchors);
  uint32_t next;
  if (token == UINT32_MAX) {
    next = s->first_anchor;
  } else {
    if (token >= count)
      return UINT32_MAX;
    next = syntaqlite_vec_at(&s->anchors, token).next;
    if (next == token)
      return UINT32_MAX;  // retired
  }
  return next == UINT32_MAX ? count : next;
}

void synq_source_retire_rhs_impl(SynqParseCtx* ctx,
                                 uint32_t first,
                                 uint32_t end) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  if (!s)
    return;
  assert(first <= end && end <= syntaqlite_vec_len(&s->rhs));
  for (uint32_t i = first; i < end; ++i) {
    SynqSourceRange range = syntaqlite_vec_at(&s->rhs, i);
    for (uint32_t token = range.first; token < range.end; ++token) {
      SynqSourceAnchor* a = &syntaqlite_vec_at(&s->anchors, token);
      // Grammar ownership declarations must be disjoint. Each authored token
      // is retired once, keeping the total work linear in the token count.
      assert(a->next != token);
      if (a->prev != UINT32_MAX)
        syntaqlite_vec_at(&s->anchors, a->prev).next = a->next;
      else
        s->first_anchor = a->next;
      if (a->next != UINT32_MAX)
        syntaqlite_vec_at(&s->anchors, a->next).prev = a->prev;
      else
        s->last_anchor = a->prev;
      a->next = token;
    }
  }
}

static void synq_source_merge(SynqSourceRange* into, SynqSourceRange from) {
  if (from.first == UINT32_MAX)
    return;
  if (into->first == UINT32_MAX || from.first < into->first)
    into->first = from.first;
  if (into->end == UINT32_MAX || from.end > into->end)
    into->end = from.end;
}

static void synq_source_reduce(SynqParseCtx* ctx, uint32_t nrhs) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  uint32_t n = syntaqlite_vec_len(&s->stack);
  assert(n >= nrhs);
  syntaqlite_vec_clear(&s->rhs);
  SynqSourceRange merged = {UINT32_MAX, UINT32_MAX};
  for (uint32_t i = n - nrhs; i < n; ++i) {
    SynqSourceRange r = syntaqlite_vec_at(&s->stack, i);
    syntaqlite_vec_push(&s->rhs, r, ctx->mem);
    synq_source_merge(&merged, r);
  }
  syntaqlite_vec_truncate(&s->stack, n - nrhs);
  syntaqlite_vec_push(&s->stack, merged, ctx->mem);
}

void synq_source_bind_rhs_impl(SynqParseCtx* ctx,
                               uint32_t node,
                               uint32_t role,
                               uint32_t first,
                               uint32_t end) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  if (!s || node == SYNTAQLITE_NULL_NODE)
    return;
  assert(first <= end && end <= syntaqlite_vec_len(&s->rhs));
  SynqSourceRange range = {UINT32_MAX, UINT32_MAX};
  for (uint32_t i = first; i < end; ++i)
    synq_source_merge(&range, syntaqlite_vec_at(&s->rhs, i));
  while (syntaqlite_vec_len(&s->heads) <= node)
    syntaqlite_vec_push(&s->heads, UINT32_MAX, ctx->mem);
  // Each node has a bounded set of named grammar roles. Rebinding a role
  // replaces its current range instead of allocating historical occurrences.
  uint32_t head = syntaqlite_vec_at(&s->heads, node);
  for (uint32_t i = head; i != UINT32_MAX;
       i = syntaqlite_vec_at(&s->bindings, i).next) {
    SynqSourceBinding* b = &syntaqlite_vec_at(&s->bindings, i);
    if (b->role == role) {
      b->range = range;
      return;
    }
  }
  uint32_t id = syntaqlite_vec_len(&s->bindings);
  SynqSourceBinding binding = {role, head, range};
  syntaqlite_vec_push(&s->bindings, binding, ctx->mem);
  syntaqlite_vec_at(&s->heads, node) = id;
}

int32_t synq_source_range(const SynqParseCtx* ctx,
                          uint32_t node,
                          uint32_t role,
                          uint32_t* first,
                          uint32_t* end) {
  struct SynqSourceBindings* s = ctx->source_bindings;
  if (!s || node >= syntaqlite_vec_len(&s->heads))
    return 0;
  for (uint32_t i = syntaqlite_vec_at(&s->heads, node); i != UINT32_MAX;
       i = syntaqlite_vec_at(&s->bindings, i).next) {
    const SynqSourceBinding* b = &syntaqlite_vec_at(&s->bindings, i);
    if (b->role == role) {
      *first = b->range.first;
      *end = b->range.end;
      return 1;
    }
  }
  return 0;
}

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

void synq_extent_record_list_append(SynqParseCtx* ctx,
                                    uint32_t list_id,
                                    uint32_t child) {
  if (!ctx->collect_node_extents)
    return;
  SynqExtentRange range = syntaqlite_vec_at(&ctx->node_extents, child);
  SynqNodeExpandedExtent expanded =
      syntaqlite_vec_at(&ctx->node_expanded_extents, child);
  if (list_id < syntaqlite_vec_len(&ctx->node_extents)) {
    synq_extent_merge(&syntaqlite_vec_at(&ctx->node_extents, list_id), range);
    synq_expanded_merge(
        &syntaqlite_vec_at(&ctx->node_expanded_extents, list_id), expanded);
  } else {
    syntaqlite_vec_push(&ctx->node_extents, range, ctx->mem);
    syntaqlite_vec_push(&ctx->node_expanded_extents, expanded, ctx->mem);
  }
}

void synq_extent_on_shift(SynqParseCtx* pCtx,
                          unsigned int major,
                          const SynqParseToken* token) {
  (void)major;
  if (pCtx->source_bindings) {
    SynqSourceRange range = {token->token_idx, token->token_idx == UINT32_MAX
                                                   ? UINT32_MAX
                                                   : token->token_idx + 1};
    syntaqlite_vec_push(&pCtx->source_bindings->stack, range, pCtx->mem);
  }

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
  if (pCtx->source_bindings)
    synq_source_reduce(pCtx, nrhs);
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
  if (pCtx->source_bindings) {
    struct SynqSourceBindings* s = pCtx->source_bindings;
    uint32_t n = syntaqlite_vec_len(&s->stack);
    if (n >= 2)
      synq_source_merge(&syntaqlite_vec_at(&s->stack, n - 1),
                        syntaqlite_vec_at(&s->stack, n - 2));
  }
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
