// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Per-node extent tracking hooks called from the Lemon-generated
// parser.
//
// These hooks run on every terminal shift and every rule reduction
// inside `yy_shift` / `yy_reduce`.  They are injected by
// `syntaqlite-buildtools` via the post-lemon patching step in
// `parser_pipeline::patch_generated_parser_files`.
//
// The macros `synq_on_shift` / `synq_on_reduce` are invoked from
// the generated parser where `yypParser` (Lemon's internal parser
// struct) is in scope; they extract `pCtx` (the `%extra_context`
// field) and delegate to the underlying `synq_extent_on_*`
// functions.  Keeping the macros here rather than declaring them
// inline in the parse.c patch keeps the call-site compact and
// centralizes the `yyParser` → `SynqParseCtx*` bridge.
//
// At this point in the stack the underlying functions are
// intentionally no-ops — a follow-up change will add the shadow
// stack, the `collect_node_extents` flag, and the actual extent
// recording logic.  Landing the hook plumbing in its own small
// PR isolates the Lemon-patch mechanics from the extent-tracking
// semantics.

#ifndef SYNTAQLITE_INTERNAL_EXTENT_HOOKS_H
#define SYNTAQLITE_INTERNAL_EXTENT_HOOKS_H

#include "syntaqlite_dialect/ast_builder.h"  // for SynqParseCtx, SynqParseToken

#ifdef __cplusplus
extern "C" {
#endif

// Called from the top of Lemon's `yy_shift` for every terminal shift.
// Will push a `(root_start, root_end)` byte range onto the shadow
// stack when per-node extent tracking is enabled; currently a no-op.
void synq_extent_on_shift(SynqParseCtx* pCtx,
                          unsigned int major,
                          const SynqParseToken* token);

// Called from the top of Lemon's `yy_reduce` for every rule reduction,
// before the user action switch.  Will pop `-yyRuleInfoNRhs[ruleno]`
// entries from the shadow stack, compute the merged range, and push
// the result back so the user action can read its own extent;
// currently a no-op.
void synq_extent_on_reduce(SynqParseCtx* pCtx, unsigned int ruleno);

// Convenience macros invoked from the patched Lemon template.  At the
// call site `yyParser` is Lemon's generated parser struct, carrying
// the `%extra_context` field `pCtx` directly (see `_common.y`:
// `%extra_context {SynqParseCtx* pCtx}`).
#define synq_on_shift(yypParser, yyMajor, yyMinor_ptr)             \
  synq_extent_on_shift((yypParser)->pCtx, (unsigned int)(yyMajor), \
                       (yyMinor_ptr))

#define synq_on_reduce(yypParser, yyruleno) \
  synq_extent_on_reduce((yypParser)->pCtx, (yyruleno))

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_INTERNAL_EXTENT_HOOKS_H
