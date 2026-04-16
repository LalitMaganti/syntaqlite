// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Private header shared by parser.c, parser_macros.c, and parser_dump.c.

#ifndef SYNTAQLITE_CSRC_PARSER_INTERNAL_H
#define SYNTAQLITE_CSRC_PARSER_INTERNAL_H

#include <stdint.h>

#include "syntaqlite/config.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite/parser.h"
#include "syntaqlite_dialect/ast_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

// ── Tunables ────────────────────────────────────────────────────────────────

#define SYNQ_MAX_MACRO_DEPTH 16

#if defined(__GNUC__) || defined(__clang__)
#define SYNQ_NOINLINE __attribute__((noinline))
#define SYNQ_PRINTF(fmt_idx, va_idx) \
  __attribute__((format(printf, fmt_idx, va_idx)))
#elif defined(_MSC_VER)
#define SYNQ_NOINLINE __declspec(noinline)
#define SYNQ_PRINTF(fmt_idx, va_idx)
#else
#define SYNQ_NOINLINE
#define SYNQ_PRINTF(fmt_idx, va_idx)
#endif

// ── Macro expansion types ────────────────────────────────────────────────

// A comma-separated argument extracted from a macro call site.
typedef struct SynqMacroArg {
  uint32_t offset;  // Byte offset in the source buffer.
  uint32_t length;  // Byte length of the argument text.
} SynqMacroArg;

// Resolved arg segment on an expansion layer.  Records where a substituted
// arg landed in the expansion buffer and where the original text lives in
// the parent layer, enabling span resolution to drill through $param
// substitutions back to the caller's authored arg text.
typedef struct SynqArgSegment {
  uint32_t sub_offset;       // Where the substituted arg starts in expansion.
  uint32_t sub_length;       // Length in expansion buffer.
  uint32_t origin_layer_id;  // Layer that owns the arg text.
  uint32_t origin_offset;    // Arg text offset in origin layer.
  uint32_t origin_length;    // Arg text length in origin layer.
} SynqArgSegment;

// Expansion layer record.  `_layer_id` on AST spans indexes directly into
// the parser's layers vector.  Entry 0 is a sentinel for the original
// source (expansion_data = source pointer, parent_layer_id = 0,
// call_offset/call_length = 0, name all NULL).
// Actual expansions start at index 1.
//
// `expansion_data` is owned (allocated via p->mem, freed in reset_stmt /
// destroy) for layers produced by the lookup callback.  For the sentinel
// (layer 0, expansion_data = source pointer) and the incremental-API
// begin_macro layers (expansion_data = NULL), the pointer is NOT freed.
//
// `name` borrows from the source buffer or a parent expansion buffer and
// is NOT freed by the parser.
typedef struct SynqExpansionLayer {
  uint32_t call_offset;        // Byte offset of macro call in parent layer.
  uint32_t call_length;        // Byte length of entire macro call.
  const char* expansion_data;  // Expanded text (NULL for sentinel/fallback).
  uint32_t expansion_len;      // Length of expanded text.

  // Definition provenance.
  const char* name;   // Macro name (borrowed), or NULL.
  uint32_t name_len;  // Length of name.
  uint32_t def_line;  // Macro definition line (1-based, 0=unknown).
  uint32_t def_col;   // Macro definition column (1-based, 0=unknown).

  // Arg segments: sorted by sub_offset, non-overlapping.  Allocated via
  // p->mem; freed in reset_stmt / destroy.  NULL when no $param subs.
  SynqArgSegment* arg_segments;
  uint32_t arg_segment_count;

  uint32_t parent_layer_id;  // Layer containing the call (0 = source).
} SynqExpansionLayer;

typedef SYNQ_VEC(SynqExpansionLayer) SynqExpansionLayerVec;

// ── Macro expansion state ────────────────────────────────────────────────
//
// Groups callback registration, per-invocation state, scratch buffers,
// and recursion detection.  Accessed almost exclusively from
// parser_macros.c; parser.c only touches init/free/reset.

typedef struct SynqMacroState {
  // Callback registration.
  SyntaqliteMacroLookupFn lookup_fn;
  void* lookup_user_data;

  // Per-invocation state (set before callback, cleared after).
  // `pending_layer` indexes into p->layers for the layer the callback
  // should write into via set_result / expand_and_set_result.
  uint32_t pending_layer;
  const SyntaqliteToken* expansion_args;
  uint32_t expansion_arg_count;

  // Scratch buffers (reused across invocations, freed in destroy).
  SYNQ_VEC(uint8_t) expand_buf;  // Template expansion output.
  SYNQ_VEC(uint8_t) body_buf;    // NUL-terminated body staging.

  // Blue-paint recursion detection: names of macros currently being expanded.
  const char* expansion_names[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_name_lens[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_depth;

  // Nesting depth (0 = not in macro).
  uint32_t depth;
} SynqMacroState;

// ── Parser struct ───────────────────────────────────────────────────────────

struct SyntaqliteParser {
  // ── Core (shared by parser.c, parser_macros.c, parser_dump.c) ──────────
  SyntaqliteMemMethods mem;
  SyntaqliteDialect dialect;
  void* lemon;
  SynqParseCtx ctx;
  const char* source;
  uint32_t source_len;
  uint32_t offset;          // Tokenizer cursor into source.
  uint32_t had_error;       // Sticky error flag for current result.
  char error_msg[256];      // Error message buffer.
  uint32_t macro_fallback;  // 1 = unregistered name!(args) becomes TK_ID.

  // ── Parser-only state (only parser.c) ──────────────────────────────────
  uint32_t last_token_type;  // Last non-whitespace token fed to Lemon.
  uint32_t finished;         // 1 after EOF has been sent to Lemon.
  uint32_t had_comment;      // 1 if any comment token was seen this stmt.
  int32_t last_status;       // Last SYNTAQLITE_PARSE_* status returned.
  uint32_t trace;
  uint32_t collect_tokens;
  uint32_t sealed;
  uint32_t pending_reset;  // 1 after feed_token signals completion;
                           // cleared on next feed_token call.
  SYNQ_VEC(SyntaqliteComment) comments;
  SYNQ_VEC(SyntaqliteParserToken) tokens;

  // Scratch buffer for materializing the expanded text of mixed-layer
  // nodes in `syntaqlite_parser_node_expanded_text`.  Rewritten on every
  // call, invalidated by the next call / `reset_stmt`.
  SYNQ_VEC(uint8_t) node_expanded_buf;

  // ── Layer tree (shared — parser.c + parser_macros.c) ───────────────────
  // Entry 0 is a sentinel representing the original source; actual
  // expansions start at index 1.  `_layer_id` on AST spans indexes
  // directly into this vector.
  SynqExpansionLayerVec layers;

  // Scratch buffer for `syntaqlite_parser_traceback`.  Rewritten on every
  // call; pointers returned from one call are invalidated by the next
  // (and by `reset_stmt`).
  SYNQ_VEC(SyntaqliteTracebackFrame) traceback_buf;

  // ── Macro expansion state (mostly parser_macros.c) ─────────────────────
  SynqMacroState macro;
};

// ── Cross-file helpers ──────────────────────────────────────────────────────

// Set the parser's last_status and return it (used by both parser.c and
// parser_macros.c as a convenient exit helper).
int32_t synq_parser_set_result_status(SyntaqliteParser* p, int32_t rc);

// Feed one token to Lemon. Returns 0/1/-1 (see parser.c for semantics).
int synq_parser_feed_one_token(SyntaqliteParser* p,
                               uint32_t token_type,
                               const char* text,
                               uint32_t len,
                               uint32_t token_idx);

// Record a token into p->tokens (if enabled) and feed it to Lemon.
int synq_parser_record_and_feed(SyntaqliteParser* p,
                                uint32_t cur_type,
                                uint32_t cur_offset,
                                uint32_t cur_len);

// Scan balanced parens for macro args.  Defined in parser_macros.c.
uint32_t synq_parser_scan_macro_args(SyntaqliteParser* p,
                                     const char* source,
                                     uint32_t source_len,
                                     uint32_t bang_offset,
                                     SynqMacroArg* out_args,
                                     uint32_t max_args,
                                     uint32_t* out_end_offset);

// Expand a macro call via the lookup callback, push the expansion layer,
// feed its tokens, and clean up.  Returns 0 on success, -1 if not a
// macro or on error.  Updates *out_end_offset to the position past ')'.
int synq_parser_expand_and_feed_macro(SyntaqliteParser* p,
                                      const char* buf,
                                      uint32_t buf_len,
                                      uint32_t id_offset,
                                      uint32_t id_len,
                                      uint32_t bang_offset,
                                      uint32_t depth,
                                      uint32_t* out_end_offset);

// Try to expand a Rust-style macro call: ID!(args).  Defined in
// parser_macros.c.
int synq_parser_try_macro_call(SyntaqliteParser* p,
                               uint32_t id_offset,
                               uint32_t id_len,
                               uint32_t bang_offset);

// Diagnose macro expansions that straddle AST node boundaries.  Defined in
// parser_macros.c.
int synq_parser_check_macro_straddle(SyntaqliteParser* p);

// ── Macro state lifecycle ───────────────────────────────────────────────────

// Initialize macro state vecs (callback/expansion fields zeroed by memset).
void synq_macro_state_init(SynqMacroState* m);

// Free macro state scratch buffers.
void synq_macro_state_free(SynqMacroState* m, SyntaqliteMemMethods mem);

// ── Layer helpers ───────────────────────────────────────────────────────────

// Free owned expansion data and arg segments on layers 1..N (skip sentinel).
void synq_layers_free_owned(SynqExpansionLayerVec* layers,
                            SyntaqliteMemMethods mem);

// Push the source sentinel at index 0.
void synq_layers_push_sentinel(SynqExpansionLayerVec* layers,
                               const char* source,
                               uint32_t source_len,
                               SyntaqliteMemMethods mem);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_CSRC_PARSER_INTERNAL_H
