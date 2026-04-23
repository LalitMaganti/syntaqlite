// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Parse context and AST builder interface.
// Provides:
//   - SynqParseCtx: parse/AST state threaded via %extra_argument
//   - SynqParseToken: terminal token type (used as %token_type in lemon
//   grammar)
//   - synq_span(): converts SynqParseToken to SyntaqliteTextSpan
//   - AST builder functions: synq_parse_build, synq_parse_list_append, etc.
//   - AST_NODE macro for in-place AST node mutation
//
// Grammar actions receive pCtx via lemon's %extra_argument mechanism.

#ifndef SYNTAQLITE_EXT_AST_BUILDER_H
#define SYNTAQLITE_EXT_AST_BUILDER_H

#include <stdint.h>
#include <string.h>

#include "syntaqlite/dialect.h"
#include "syntaqlite/parser.h"
#include "syntaqlite/types.h"
#include "syntaqlite_dialect/arena.h"
#include "syntaqlite_dialect/vec.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// List descriptor: lightweight metadata for one in-progress list.
// ---------------------------------------------------------------------------

typedef struct SynqListDesc {
  uint32_t node_id;  // reserved arena ID
  uint32_t offset;   // start index into child_buf
  uint32_t tag;
} SynqListDesc;

// Half-open byte range in the authored source, used by per-node extent
// tracking.  Sentinel `(UINT32_MAX, 0)` marks an unrecorded/empty range.
typedef struct SynqExtentRange {
  uint32_t root_start;
  uint32_t root_end;
} SynqExtentRange;

// Inclusive token-index range covered by a node, parallel to
// SynqExtentRange.  Drives O(1) node→token-range lookup in
// `syntaqlite_node_token_range`.  Indices refer to entries in
// `p->tokens`, which with the unified token-stream contains both
// authored-source and macro-expansion tokens.  Sentinel
// `(UINT32_MAX, UINT32_MAX)` marks a node with no recorded tokens.
typedef struct SynqTokenRange {
  uint32_t first_tok;
  uint32_t last_tok;
} SynqTokenRange;

// Layer-local byte range used by per-node *expanded*-text tracking —
// the bytes the tokenizer saw for a node, in whichever layer buffer
// they live.
//
// Sentinel states:
//   {0, 0, 0}                  — epsilon (no tokens); neutral in merges.
//   {_, 0, SYNQ_CROSS_LAYER}   — cross-layer poison; propagates through
//                                parent merges so the fast path falls
//                                through to the slow path.
#define SYNQ_CROSS_LAYER UINT32_MAX
typedef struct SynqNodeExpandedExtent {
  uint32_t offset;
  uint32_t length;
  uint32_t layer_id;
} SynqNodeExpandedExtent;

// Straddle stack entry values (packed into uint32_t):
//   0              = source terminal
//   1..N           = terminal from outermost expansion layer N
//   SYNQ_STRADDLE_NEUTRAL = non-terminal / epsilon (neutral in checks)
#define SYNQ_STRADDLE_NEUTRAL UINT32_MAX

// ---------------------------------------------------------------------------
// Parse context — threaded through grammar actions via %extra_argument
// ---------------------------------------------------------------------------

typedef struct SynqParseCtx {
  // AST storage
  SyntaqliteMemMethods mem;
  SynqArena ast;
  SYNQ_VEC(uint32_t) child_buf;
  SYNQ_VEC(SynqListDesc) list_stack;

  // Parser state
  const char* source;  // Source text base pointer (for offset computation).
  const SyntaqliteDialect* env;   // Dialect env (for cflag checks in actions).
  uint32_t root;                  // Root node ID of the current statement.
  uint32_t stmt_completed;        // Set by grammar actions when ecmd reduces.
  uint32_t pending_explain_mode;  // 1=EXPLAIN, 2=EXPLAIN QUERY PLAN (set by
                                  // explain rule, consumed by cmdx ::= cmd).
  uint32_t error;                 // Set when a syntax error occurs.
  uint32_t error_offset;          // Byte offset of the error token in source.
  uint32_t error_length;          // Byte length of the error token.
  uint32_t saw_subquery;  // Set by grammar actions when a subquery is reduced.
  uint32_t saw_update_delete_limit;  // Set when ORDER BY / LIMIT used on DELETE
                                     // or UPDATE.

  // Token marking — points to the parser's token list (NULL if not collecting).
  // Typed as void* because SYNQ_VEC produces anonymous struct types; the
  // synq_mark_as_id() helper casts it to the right layout.
  void* tokens;

  // Expansion layer index for span construction.
  // 0 = original source, 1+ = index into the layer tree (1-based).
  uint32_t layer_id;

  // Counter for "currently parsing inside a macro definition body".
  // While > 0, the tokenizer skips macro expansion so the body is captured
  // verbatim instead of being recursively expanded.  Set/cleared by
  // grammar actions on entering/leaving the body production.
  uint32_t in_macro_def_body;

  // Byte offset of the token Lemon is currently processing (in
  // root-source coordinates).  Set at the start of
  // `synq_parser_shift_token` *before* `SYNQ_PARSER_FEED` runs, so
  // empty-rule reductions firing inside the feed observe the offset of
  // the token they're about to be shifted alongside.  BEFORE-style
  // markers use this to capture the start position of a non-terminal
  // (whitespace before the first terminal is excluded).  Valid only
  // for tokens shifted from the root source layer.
  uint32_t cur_shift_start;

  // Byte offset just past the end of the most recently shifted terminal
  // (in root-source coordinates).  Updated in
  // `synq_parser_shift_token` *after* `SYNQ_PARSER_FEED` returns, so
  // that empty-rule reductions firing inside the feed see the end of
  // the *previous* shifted terminal, not the current one.  AFTER-style
  // markers use this to capture the end position of a non-terminal.
  // Valid only for tokens shifted from the root source layer.
  uint32_t last_shifted_end;

  // Per-node extent tracking.  Opt-in via `collect_node_extents`.
  // `extent_stack` / `node_extents` track the merged *authored*
  // source range (in root coordinates) — used by
  // `syntaqlite_parser_node_text`.  `expanded_stack` /
  // `node_expanded_extents` track the merged *expanded* range in the
  // tokens' own layer — used by
  // `syntaqlite_parser_node_expanded_text`; mixed-layer merges
  // collapse to a sentinel.  `macro_root_*` caches the outermost
  // currently-active macro call site in root coordinates so tokens
  // shifted inside expansions can be attributed back to the authored
  // source.
  SYNQ_VEC(SynqExtentRange) extent_stack;
  SYNQ_VEC(SynqExtentRange) node_extents;
  SYNQ_VEC(SynqNodeExpandedExtent) expanded_stack;
  SYNQ_VEC(SynqNodeExpandedExtent) node_expanded_extents;
  // Token-index shadow stack, parallel to extent_stack.  Pushed on
  // shift with the shifted token's `token_idx` (UINT32_MAX when
  // collect_tokens is off); merged on reduce with min/max semantics;
  // committed at the top per node in `synq_extent_record`.
  SYNQ_VEC(SynqTokenRange) token_range_stack;
  SYNQ_VEC(SynqTokenRange) node_token_ranges;
  uint32_t collect_node_extents;
  uint32_t macro_root_start;
  uint32_t macro_root_end;
  uint32_t macro_root_layer;    // Outermost expansion layer idx (set on entry).
  uint32_t has_macro_straddle;  // Sticky flag set during reduce.
  uint32_t lemon_depth;  // Lemon stack depth (always tracked, for lazy init).
  SYNQ_VEC(uint32_t) straddle_stack;  // Lazily initialized on first macro use.
} SynqParseCtx;

// Common header for all list nodes in the arena.
typedef struct SynqListHeader {
  uint32_t tag;
  uint32_t count;
} SynqListHeader;

// ---------------------------------------------------------------------------
// AST node access macro (for in-place mutation in grammar actions)
// ---------------------------------------------------------------------------

// Cast the arena pointer for a node ID to a void pointer.
// Dialect code should further cast to the dialect-specific node union.
#define AST_NODE(arena_ptr, id) ((void*)synq_arena_ptr((arena_ptr), (id)))

// Type-safe arena access — casts through void* to suppress -Wcast-align.
#define AST_NODE_AS(type, arena_ptr, id) \
  ((type*)((void*)synq_arena_ptr((arena_ptr), (id))))

// ---------------------------------------------------------------------------
// AST builder functions
// ---------------------------------------------------------------------------

// Flush the topmost list from the stack into the arena.
static inline void synq_parse_list_flush_top(SynqParseCtx* ctx) {
  SynqListDesc* desc = &syntaqlite_vec_at(
      &ctx->list_stack, syntaqlite_vec_len(&ctx->list_stack) - 1);
  uint32_t count = syntaqlite_vec_len(&ctx->child_buf) - desc->offset;
  uint32_t children_size = count * (uint32_t)sizeof(uint32_t);

  SynqListHeader hdr = {.tag = desc->tag, .count = count};
  synq_arena_commit(&ctx->ast, desc->node_id, &hdr, (uint32_t)sizeof(hdr),
                    ctx->mem);
  synq_arena_append(&ctx->ast,
                    &syntaqlite_vec_at(&ctx->child_buf, desc->offset),
                    children_size, ctx->mem);

  syntaqlite_vec_truncate(&ctx->child_buf, desc->offset);
  (void)syntaqlite_vec_pop(&ctx->list_stack);
}

static inline void synq_parse_ctx_init(SynqParseCtx* ctx,
                                       SyntaqliteMemMethods mem) {
  ctx->mem = mem;
  synq_arena_init(&ctx->ast);
  syntaqlite_vec_init(&ctx->child_buf);
  syntaqlite_vec_init(&ctx->list_stack);
  syntaqlite_vec_init(&ctx->extent_stack);
  syntaqlite_vec_init(&ctx->node_extents);
  syntaqlite_vec_init(&ctx->expanded_stack);
  syntaqlite_vec_init(&ctx->node_expanded_extents);
  syntaqlite_vec_init(&ctx->token_range_stack);
  syntaqlite_vec_init(&ctx->node_token_ranges);
  ctx->collect_node_extents = 0;
  ctx->macro_root_start = 0;
  ctx->macro_root_end = 0;
  ctx->macro_root_layer = 0;
  ctx->has_macro_straddle = 0;
  ctx->lemon_depth = 0;
  syntaqlite_vec_init(&ctx->straddle_stack);
}

static inline void synq_parse_ctx_free(SynqParseCtx* ctx) {
  syntaqlite_vec_free(&ctx->child_buf, ctx->mem);
  syntaqlite_vec_free(&ctx->list_stack, ctx->mem);
  syntaqlite_vec_free(&ctx->extent_stack, ctx->mem);
  syntaqlite_vec_free(&ctx->node_extents, ctx->mem);
  syntaqlite_vec_free(&ctx->expanded_stack, ctx->mem);
  syntaqlite_vec_free(&ctx->node_expanded_extents, ctx->mem);
  syntaqlite_vec_free(&ctx->token_range_stack, ctx->mem);
  syntaqlite_vec_free(&ctx->node_token_ranges, ctx->mem);
  syntaqlite_vec_free(&ctx->straddle_stack, ctx->mem);
  synq_arena_free(&ctx->ast, ctx->mem);
}

// Reset to empty state, keeping allocated memory for reuse.
static inline void synq_parse_ctx_clear(SynqParseCtx* ctx) {
  syntaqlite_vec_clear(&ctx->child_buf);
  syntaqlite_vec_clear(&ctx->list_stack);
  syntaqlite_vec_clear(&ctx->extent_stack);
  syntaqlite_vec_clear(&ctx->node_extents);
  syntaqlite_vec_clear(&ctx->expanded_stack);
  syntaqlite_vec_clear(&ctx->node_expanded_extents);
  syntaqlite_vec_clear(&ctx->token_range_stack);
  syntaqlite_vec_clear(&ctx->node_token_ranges);
  synq_arena_clear(&ctx->ast);
  ctx->macro_root_start = 0;
  ctx->macro_root_end = 0;
  ctx->macro_root_layer = 0;
  ctx->has_macro_straddle = 0;
  ctx->lemon_depth = 0;
  syntaqlite_vec_clear(&ctx->straddle_stack);
}

// Record the current shadow-stack tops (authored + expanded) as the
// extents for `node_id`.  Called right after a node is allocated, so
// the tops are the merged ranges for the rule currently being
// reduced.  List node ids are re-recorded on each append, so the
// final stored value is the full list's extent.  Node ids are
// monotonically allocated, so `node_id` is either equal to
// `node_extents.count` (new) or less (existing).
static inline void synq_extent_record(SynqParseCtx* ctx, uint32_t node_id) {
  if (!ctx->collect_node_extents) {
    return;
  }
  uint32_t stack_len = syntaqlite_vec_len(&ctx->extent_stack);
  if (stack_len == 0) {
    return;
  }
  SynqExtentRange top = syntaqlite_vec_at(&ctx->extent_stack, stack_len - 1);
  SynqNodeExpandedExtent exp_top =
      syntaqlite_vec_at(&ctx->expanded_stack, stack_len - 1);
  SynqTokenRange tr_top =
      syntaqlite_vec_at(&ctx->token_range_stack, stack_len - 1);
  if (node_id < ctx->node_extents.count) {
    syntaqlite_vec_at(&ctx->node_extents, node_id) = top;
    syntaqlite_vec_at(&ctx->node_expanded_extents, node_id) = exp_top;
    syntaqlite_vec_at(&ctx->node_token_ranges, node_id) = tr_top;
  } else {
    syntaqlite_vec_push(&ctx->node_extents, top, ctx->mem);
    syntaqlite_vec_push(&ctx->node_expanded_extents, exp_top, ctx->mem);
    syntaqlite_vec_push(&ctx->node_token_ranges, tr_top, ctx->mem);
  }
}

// Re-record the current shadow-stack extent for an existing node ID.
// Use this in multi-RHS grammar rules that pass through a child node ID
// without allocating a new node (e.g. `A = B;`).  Without this call,
// node_extents[child] retains the child's original (narrower) range
// instead of the merged range of the enclosing rule.
static inline uint32_t synq_pass(SynqParseCtx* ctx, uint32_t child) {
  synq_extent_record(ctx, child);
  return child;
}

// Generic node builder: copy node data into the arena.
static inline uint32_t synq_parse_build(SynqParseCtx* ctx,
                                        const void* node_data,
                                        uint32_t node_size) {
  uint32_t node_id =
      synq_arena_alloc(&ctx->ast, node_data, node_size, ctx->mem);
  synq_extent_record(ctx, node_id);
  return node_id;
}

static inline uint32_t synq_parse_list_append(SynqParseCtx* ctx,
                                              uint32_t tag,
                                              uint32_t list_id,
                                              uint32_t child) {
  if (list_id == SYNTAQLITE_NULL_NODE) {
    SynqListDesc desc;
    desc.node_id = synq_arena_reserve_id(&ctx->ast, ctx->mem);
    desc.offset = syntaqlite_vec_len(&ctx->child_buf);
    desc.tag = tag;
    syntaqlite_vec_push(&ctx->list_stack, desc, ctx->mem);
    syntaqlite_vec_push(&ctx->child_buf, child, ctx->mem);
    synq_extent_record(ctx, desc.node_id);
    return desc.node_id;
  }

  // Auto-flush completed inner lists above the target.
  while (syntaqlite_vec_at(&ctx->list_stack,
                           syntaqlite_vec_len(&ctx->list_stack) - 1)
             .node_id != list_id) {
    synq_parse_list_flush_top(ctx);
  }
  syntaqlite_vec_push(&ctx->child_buf, child, ctx->mem);
  synq_extent_record(ctx, list_id);
  return list_id;
}

// Like list_append, but inserts the child at the front of the list.
// Used for right-recursive grammar rules where the innermost (last in source)
// clause reduces first, so each outer clause must prepend to maintain source
// order.
static inline uint32_t synq_parse_list_prepend(SynqParseCtx* ctx,
                                               uint32_t tag,
                                               uint32_t list_id,
                                               uint32_t child) {
  if (list_id == SYNTAQLITE_NULL_NODE) {
    return synq_parse_list_append(ctx, tag, list_id, child);
  }

  // Auto-flush completed inner lists above the target.
  while (syntaqlite_vec_at(&ctx->list_stack,
                           syntaqlite_vec_len(&ctx->list_stack) - 1)
             .node_id != list_id) {
    synq_parse_list_flush_top(ctx);
  }

  // Find the list descriptor to get its start offset.
  SynqListDesc* desc = &syntaqlite_vec_at(
      &ctx->list_stack, syntaqlite_vec_len(&ctx->list_stack) - 1);
  uint32_t insert_at = desc->offset;
  uint32_t len = syntaqlite_vec_len(&ctx->child_buf);

  // Make room: push a dummy, shift elements right, insert at front.
  syntaqlite_vec_push(&ctx->child_buf, child, ctx->mem);
  for (uint32_t i = len; i > insert_at; --i) {
    syntaqlite_vec_at(&ctx->child_buf, i) =
        syntaqlite_vec_at(&ctx->child_buf, i - 1);
  }
  syntaqlite_vec_at(&ctx->child_buf, insert_at) = child;
  synq_extent_record(ctx, list_id);
  return list_id;
}

static inline void synq_parse_list_flush(SynqParseCtx* ctx) {
  while (syntaqlite_vec_len(&ctx->list_stack) > 0) {
    synq_parse_list_flush_top(ctx);
  }
}

// ---------------------------------------------------------------------------
// Token → span conversion
// ---------------------------------------------------------------------------

static inline SyntaqliteTextSpan synq_span(SynqParseCtx* ctx,
                                           SynqParseToken tok) {
  (void)ctx;
  if (tok.z == NULL)
    return (SyntaqliteTextSpan){0};
  return (SyntaqliteTextSpan){
      .offset = tok.offset,
      .length = tok.n,
      .flags = 0,
      ._layer_id = tok.layer_id,
  };
}

// Like synq_span() but strips surrounding quote characters from quoted
// identifiers, matching SQLite's tokenExpr() dequoting behavior.
// Handles "...", `...`, and [...] forms.  For unquoted tokens, equivalent
// to synq_span().  Records which quote character bracketed the identifier
// in the span flags so consumers can recover the original quote style.
static inline SyntaqliteTextSpan synq_span_dequote(SynqParseCtx* ctx,
                                                   SynqParseToken tok) {
  (void)ctx;
  if (tok.z == NULL)
    return (SyntaqliteTextSpan){0};
  if (tok.n >= 2) {
    char open = tok.z[0];
    char close = tok.z[tok.n - 1];
    if ((open == '"' && close == '"') || (open == '`' && close == '`') ||
        (open == '[' && close == ']')) {
      uint32_t kind_flag = (open == '"')   ? SYNTAQLITE_SPAN_FLAG_QUOTE_DOUBLE
                           : (open == '`') ? SYNTAQLITE_SPAN_FLAG_QUOTE_BACKTICK
                                           : SYNTAQLITE_SPAN_FLAG_QUOTE_BRACKET;
      SyntaqliteTextSpan sp = {
          .offset = tok.offset + 1,
          .length = tok.n - 2,
          .flags = kind_flag,
          ._layer_id = tok.layer_id,
      };
      return sp;
    }
  }
  return (SyntaqliteTextSpan){
      .offset = tok.offset,
      .length = tok.n,
      ._layer_id = tok.layer_id,
  };
}

#define SYNQ_NO_SPAN ((SyntaqliteTextSpan){0})

// Mark a token as "used as identifier" (fallback from keyword).
// O(1) — uses the token_idx stored in SynqParseToken at collection time.
static inline void synq_mark_as_id(SynqParseCtx* ctx, SynqParseToken tok) {
  if (!ctx->tokens || tok.token_idx == 0xFFFFFFFF)
    return;
  // ctx->tokens is a void* pointing to SYNQ_VEC(SyntaqliteParserToken).
  // The vec layout is: { SyntaqliteParserToken* data; uint32_t count; uint32_t
  // capacity; }
  typedef struct {
    SyntaqliteParserToken* data;
    uint32_t count;
    uint32_t capacity;
  } TokenVec;
  TokenVec* tv = (TokenVec*)ctx->tokens;
  tv->data[tok.token_idx].flags |= SYNQ_TOKEN_FLAG_AS_ID;
}

// Mark a token as "used as function name" in a function-call expression.
// O(1) — uses the token_idx stored in SynqParseToken at collection time.
static inline void synq_mark_as_function(SynqParseCtx* ctx,
                                         SynqParseToken tok) {
  if (!ctx->tokens || tok.token_idx == 0xFFFFFFFF)
    return;
  // ctx->tokens is a void* pointing to SYNQ_VEC(SyntaqliteParserToken).
  // The vec layout is: { SyntaqliteParserToken* data; uint32_t count; uint32_t
  // capacity; }
  typedef struct {
    SyntaqliteParserToken* data;
    uint32_t count;
    uint32_t capacity;
  } TokenVec;
  TokenVec* tv = (TokenVec*)ctx->tokens;
  tv->data[tok.token_idx].flags |= SYNQ_TOKEN_FLAG_AS_FUNCTION;
}

// Mark a token as "used as type name" in type contexts.
// O(1) — uses the token_idx stored in SynqParseToken at collection time.
static inline void synq_mark_as_type(SynqParseCtx* ctx, SynqParseToken tok) {
  if (!ctx->tokens || tok.token_idx == 0xFFFFFFFF)
    return;
  // ctx->tokens is a void* pointing to SYNQ_VEC(SyntaqliteParserToken).
  // The vec layout is: { SyntaqliteParserToken* data; uint32_t count; uint32_t
  // capacity; }
  typedef struct {
    SyntaqliteParserToken* data;
    uint32_t count;
    uint32_t capacity;
  } TokenVec;
  TokenVec* tv = (TokenVec*)ctx->tokens;
  tv->data[tok.token_idx].flags |= SYNQ_TOKEN_FLAG_AS_TYPE;
}

// Range field metadata types (SyntaqliteFieldRangeMeta,
// SyntaqliteRangeMetaEntry) are defined in syntaqlite/dialect.h.

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_EXT_AST_BUILDER_H
