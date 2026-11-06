// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Per-node extent tracking hooks called from the Lemon-generated
// parser via the post-lemon patch step in
// `parser_pipeline::patch_generated_parser_files`.  The macros bridge
// Lemon's `yyParser` (which carries `%extra_context SynqParseCtx*
// pCtx`) to the underlying `synq_extent_on_*` functions.

#ifndef SYNTAQLITE_INTERNAL_EXTENT_HOOKS_H
#define SYNTAQLITE_INTERNAL_EXTENT_HOOKS_H

#include "syntaqlite_dialect/ast_builder.h"  // for SynqParseCtx, SynqParseToken
#include "syntaqlite_dialect/dialect_abi.h"  // for SYNTAQLITE_DIALECT_API

#ifdef __cplusplus
extern "C" {
#endif

// Called from the top of Lemon's `yy_shift` for every terminal shift.
// Pushes a `(root_start, root_end)` range onto the shadow stack when
// per-node extent tracking is enabled.
SYNTAQLITE_DIALECT_API void synq_extent_on_shift(SynqParseCtx* pCtx,
                                                 unsigned int major,
                                                 const SynqParseToken* token);

// Called from the top of Lemon's `yy_reduce` for every rule reduction,
// before the user action switch.  Pops `nrhs` entries from the shadow
// stack and pushes their merged range.
SYNTAQLITE_DIALECT_API void synq_extent_on_reduce(SynqParseCtx* pCtx,
                                                  unsigned int nrhs);

// Called from grammar actions of rules whose parent rule is
// {NEVER-REDUCE}: merges the shadow-stack entry directly below the top
// into the top (both authored and expanded ranges), standing in for the
// parent reduction that will never run.  Follow with
// `synq_extent_record` to re-record the widened extent on the node.
SYNTAQLITE_DIALECT_API void synq_extent_fold_below_into_top(SynqParseCtx* pCtx);

// Applied to Lemon's `yy_shift` by the post-lemon patch step.
#if defined(__GNUC__) || defined(__clang__)
#define SYNQ_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define SYNQ_ALWAYS_INLINE inline
#endif

// Lemon stores `yyRuleInfoNRhs[r]` as the negative of the rule's RHS
// symbol count, so the reduce macro negates it to recover `nrhs`.
//
// With extents and macros both off, the hooks only track `lemon_depth`, so
// do that inline instead of calling into the runtime on every shift and
// reduce.
#define synq_on_shift(yypParser, yyMajor, yyMinor_ptr)                         \
  do {                                                                         \
    SynqParseCtx* synq_ctx_ = (yypParser)->pCtx;                               \
    if (synq_ctx_->collect_node_extents | synq_ctx_->macro_root_layer) {       \
      synq_extent_on_shift(synq_ctx_, (unsigned int)(yyMajor), (yyMinor_ptr)); \
    } else {                                                                   \
      synq_ctx_->lemon_depth++;                                                \
    }                                                                          \
  } while (0)

#define synq_on_reduce(yypParser, yyruleno)                              \
  do {                                                                   \
    SynqParseCtx* synq_ctx_ = (yypParser)->pCtx;                         \
    unsigned int synq_nrhs_ = (unsigned int)(-yyRuleInfoNRhs[yyruleno]); \
    if (synq_ctx_->collect_node_extents | synq_ctx_->macro_root_layer) { \
      synq_extent_on_reduce(synq_ctx_, synq_nrhs_);                      \
    } else {                                                             \
      synq_ctx_->lemon_depth = synq_ctx_->lemon_depth + 1 - synq_nrhs_;  \
    }                                                                    \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_INTERNAL_EXTENT_HOOKS_H
