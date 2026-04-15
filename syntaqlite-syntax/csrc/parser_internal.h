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

  uint8_t parent_layer_id;  // Layer containing the call (0 = source).
} SynqExpansionLayer;

// ── Parser struct ───────────────────────────────────────────────────────────

struct SyntaqliteParser {
  SyntaqliteMemMethods mem;
  SyntaqliteDialect dialect;
  void* lemon;
  SynqParseCtx ctx;
  const char* source;
  uint32_t source_len;
  uint32_t offset;           // Tokenizer cursor into source.
  uint32_t last_token_type;  // Last non-whitespace token fed to Lemon.
  uint32_t finished;         // 1 after EOF has been sent to Lemon.
  uint32_t had_comment;      // 1 if any comment token was seen this stmt.
  uint32_t had_error;        // Sticky error flag for current result.
  int32_t last_status;       // Last SYNTAQLITE_PARSE_* status returned.
  char error_msg[256];       // Error message buffer.
  uint32_t trace;
  uint32_t collect_tokens;
  uint32_t macro_fallback;  // 1 = unregistered name!(args) becomes TK_ID.
  uint32_t sealed;
  uint32_t pending_reset;  // 1 after feed_token signals completion; cleared on
                           // the next feed_token call (arena reset deferred).
  SYNQ_VEC(SyntaqliteComment) comments;
  SYNQ_VEC(SyntaqliteParserToken) tokens;
  uint32_t macro_depth;  // Nesting depth (0 = not in macro).

  // Unified layer tree.  Entry 0 is a sentinel representing the original
  // source; actual expansions start at index 1.  `_layer_id` on AST spans
  // indexes directly into this vector.
  SYNQ_VEC(SynqExpansionLayer) layers;

  // Scratch buffer owned by the parser for `syntaqlite_parser_traceback`.
  // Rewritten on every call; pointers returned from one call are
  // invalidated by the next (and by `reset_stmt`).
  SYNQ_VEC(SyntaqliteTracebackFrame) traceback_buf;

  // Scratch buffer owned by the parser for materializing the expanded
  // text of mixed-layer nodes in `syntaqlite_parser_node_expanded_text`.
  // Same lifetime semantics as `traceback_buf` — rewritten on every
  // call, invalidated by the next call / `reset_stmt`.
  SYNQ_VEC(uint8_t) node_expanded_buf;

  // ── Macro lookup callback ──────────────────────────────────────────────
  SyntaqliteMacroLookupFn macro_lookup_fn;
  void* macro_lookup_user_data;
  // Index into p->layers of the layer the callback should write into.
  // Set by expand_and_feed_macro before invoking the callback;
  // set_result / expand_and_set_result write expansion_data / def_line /
  // def_col directly onto layers.data[macro_pending_layer].
  uint32_t macro_pending_layer;
  const SyntaqliteToken* macro_expansion_args;
  uint32_t macro_expansion_arg_count;

  // Scratch buffer for template expansion in expand_and_set_result.
  // Reused across invocations to avoid repeated allocation.
  SYNQ_VEC(uint8_t) macro_expand_buf;

  // ── Expansion state ───────────────────────────────────────────────────
  // Blue-paint recursion detection: names of macros currently being expanded.
  const char* expansion_names[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_name_lens[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_depth;
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

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_CSRC_PARSER_INTERNAL_H
