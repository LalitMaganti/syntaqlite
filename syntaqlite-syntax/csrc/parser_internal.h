// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Private header shared by parser.c, parser_macros.c, and parser_dump.c.

#ifndef SYNTAQLITE_CSRC_PARSER_INTERNAL_H
#define SYNTAQLITE_CSRC_PARSER_INTERNAL_H

#include <stdint.h>
#include <stdio.h>

#include "csrc/tokens.h"
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
#define SYNQ_MAX_MACRO_ARGS 64

// Per-token comment index entry.  Each entry records where the owning
// token's leading / trailing comments live in `p->comments`.  Used by
// `syntaqlite_token_{leading,trailing}_comments` for O(1) lookup.
typedef struct SynqTokenComments {
  uint32_t leading_first;  // Index into p->comments; UINT32_MAX if count==0.
  uint32_t leading_count;
  uint32_t trailing_first;  // Index into p->comments; UINT32_MAX if count==0.
  uint32_t trailing_count;
} SynqTokenComments;

#define SYNQ_TOKEN_COMMENTS_EMPTY \
  ((SynqTokenComments){UINT32_MAX, 0, UINT32_MAX, 0})

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

// ── Macro expansion (compiled out with -DSYNTAQLITE_OMIT_MACROS) ─────────
//
// All macro-related types, struct fields, and helper declarations live
// inside this guard.  When SYNTAQLITE_OMIT_MACROS is defined, the parser
// struct shrinks and parser_macros.c / parser_spans.c compile to empty
// translation units.  Public APIs that reference macro state (span_text,
// traceback, expanded_text, etc.) are stubbed in parser.c.

#ifndef SYNTAQLITE_OMIT_MACROS

// A comma-separated argument extracted from a macro call site.
typedef struct SynqMacroArg {
  uint32_t offset;  // Byte offset in the source buffer.
  uint32_t length;  // Byte length of the argument text.
} SynqMacroArg;

// Resolved arg segment on an expansion layer.  Records where a substituted
// arg landed in the expansion buffer and where the original text lives in
// the parent layer, enabling span resolution to drill through $param
// substitutions back to the caller's authored arg text.
//
// `body_offset` / `body_length` record the `$param` token position in the
// macro's authored body (pre-substitution).  Populated by the
// template-expansion path; zero for the set_result_with_arg_map API where
// the caller did not supply authored-body positions.
typedef struct SynqArgSegment {
  uint32_t body_offset;      // Position of $param token in authored body.
  uint32_t body_length;      // Length of $param token in authored body.
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

  // Top-level call-site argument spans, populated on every layer
  // that went through `synq_parser_scan_macro_args` (both registered
  // and fallback paths).  Offsets are in the same coordinate system
  // as `call_offset`: statement-relative for top-level layers,
  // otherwise relative to the parent layer's buffer.  Allocated via
  // p->mem and freed in reset_stmt / destroy.  NULL when `name!()`
  // has zero args or the scan overflowed the stack buffer.
  SynqMacroArg* args;
  uint32_t arg_count;

  // 1 if this layer is a fallback layer (unregistered `name!(args)`
  // kept verbatim as a TK_ID — no expansion buffer, no $param
  // substitutions).  0 for registered macros that expanded into
  // `expansion_data`.
  uint32_t is_fallback;

  // Position of this nested call in the *parent's authored body*,
  // computed by inverting the length shifts from the parent's $param
  // substitutions.  Both fields equal SYNTAQLITE_MACRO_BODY_CALL_ARG_INTERNAL
  // (UINT32_MAX) when the call was tokenized from a substituted arg's
  // text (no clean body position) and consumers should descend through
  // the matching arg segment instead.  Zero for top-level layers
  // (parent_layer_id == 0).
  uint32_t body_call_offset;
  uint32_t body_call_length;

  uint32_t parent_layer_id;  // Layer containing the call (0 = source).
} SynqExpansionLayer;

typedef SYNQ_VEC(SynqExpansionLayer) SynqExpansionLayerVec;
typedef SYNQ_VEC(uint8_t) SynqByteVec;

// All macro-related parser state, including layer tree and scratch buffers.
// Factored into a single sub-struct so the parser struct has one guarded
// field: `SynqMacroState macro;`.
typedef struct SynqMacroState {
  // ── Configuration ──────────────────────────────────────────────────────
  uint32_t macro_fallback;  // 1 = unregistered name!(args) becomes TK_ID.

  // ── Callback registration ──────────────────────────────────────────────
  SyntaqliteMacroLookupFn lookup_fn;
  void* lookup_user_data;

  // ── Per-invocation state (set before callback, cleared after) ──────────
  // `pending_layer` indexes into layers for the layer the callback
  // should write into via set_result / expand_and_set_result.
  uint32_t pending_layer;
  const SyntaqliteToken* expansion_args;
  uint32_t expansion_arg_count;

  // ── Scratch buffers (reused across invocations, freed in destroy) ──────
  SYNQ_VEC(uint8_t) expand_buf;  // Template expansion output.
  SYNQ_VEC(uint8_t) body_buf;    // NUL-terminated body staging.
  // Flat scratch arena for eager arg pre-expansion.  Usage is
  // stack-disciplined: each `synq_parser_expand_and_feed_macro` records
  // `count` on entry and truncates back to that mark before feeding its
  // own expansion, so an inner call's pre-expansion temporaries don't
  // fragment the outer caller's accumulating bytes (when the outer is
  // itself in pre-expansion).  Pointers into this vec are taken only
  // after `pre_expand_args` is done pushing, since pushes can realloc.
  SynqByteVec arg_scratch;

  // 1 while expand_and_feed is running in scratch (pre-expansion)
  // mode.  When set, helper tokenizers skip side effects that would
  // store mis-attributed metadata (comments) — the bytes are preserved
  // verbatim into arg_scratch and get re-tokenized at the correct
  // layer when the substituted body runs through Lemon-mode feed.
  int in_pre_expand;

  // ── Blue-paint recursion detection ─────────────────────────────────────
  const char* expansion_names[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_name_lens[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_depth;

  // ── Nesting depth (0 = not in macro) ───────────────────────────────────
  uint32_t depth;

  // ── Layer tree ─────────────────────────────────────────────────────────
  // Entry 0 is a sentinel representing the original source; actual
  // expansions start at index 1.  `_layer_id` on AST spans indexes
  // directly into this vector.
  SynqExpansionLayerVec layers;

  // ── Scratch buffers for span/text APIs ─────────────────────────────────
  // Scratch for `syntaqlite_parser_traceback`.
  SYNQ_VEC(SyntaqliteTracebackFrame) traceback_buf;
  // Scratch for `syntaqlite_parser_node_expanded_text`.
  SYNQ_VEC(uint8_t) node_expanded_buf;
} SynqMacroState;

#endif  // !SYNTAQLITE_OMIT_MACROS

// ── Parser struct ───────────────────────────────────────────────────────────

struct SyntaqliteParser {
  // ── Core ───────────────────────────────────────────────────────────────
  SyntaqliteMemMethods mem;
  SyntaqliteDialect dialect;
  void* lemon;
  SynqParseCtx ctx;
  const char* source;
  uint32_t source_len;
  uint32_t offset;      // Tokenizer cursor into source.
  uint32_t had_error;   // Sticky error flag for current result.
  char error_msg[256];  // Error message buffer.

  // Current statement's byte range.  Set by parser_next / feed_token on
  // the first byte consumed; stmt_end_offset is finalized at statement
  // completion.  `stmt_start_offset == UINT32_MAX` means no statement
  // has been produced yet; `stmt_source` then equals `p->source`.
  //
  // Every layer-0 offset the parser emits (tokens, comments, node
  // extents, arena TextSpan.offset, macro rewrite call_offset with
  // source parent, error_offset) is measured from `stmt_source`.
  uint32_t stmt_start_offset;
  uint32_t stmt_end_offset;
  const char* stmt_source;

  // ── Parser-only state (only parser.c) ──────────────────────────────────
  uint32_t last_token_type;  // Last non-whitespace token fed to Lemon.
  uint32_t finished;         // 1 after EOF has been sent to Lemon.
  uint32_t had_comment;      // 1 if any comment token was seen this stmt.
  // End offset (in `p->ctx.source` coordinates) of the most recent
  // token recorded into `p->tokens` this statement, or `UINT32_MAX`
  // if none.  Used by `synq_parser_record_comment` to classify a
  // comment as TRAILING (same line as the preceding token, in the
  // same layer buffer) vs LEADING (its own line, before any token, or
  // in a different layer from the last push).
  uint32_t last_pushed_token_ctx_end;
  uint32_t last_pushed_token_layer;  // Layer of that token; UINT32_MAX if none.
  int32_t last_status;               // Last SYNTAQLITE_PARSE_* status returned.
  uint32_t trace;
  uint32_t collect_tokens;
  uint32_t sealed;
  uint32_t pending_reset;  // 1 after feed_token signals completion;
                           // cleared on next feed_token call.
  SYNQ_VEC(SyntaqliteComment) comments;
  SYNQ_VEC(SyntaqliteParserToken) tokens;

  // Per-token comment index, parallel to `tokens` (one entry per
  // shifted terminal).  Populated incrementally by
  // `synq_parser_record_comment` and seeded at token-push in
  // `synq_parser_shift_token` from `pending_orphan_leading`.  Lets
  // `syntaqlite_token_{leading,trailing}_comments` answer in O(1)
  // without a separate build step.  `leading_first` / `trailing_first`
  // are `UINT32_MAX` when the corresponding count is zero.
  SYNQ_VEC(SynqTokenComments) token_comments;

  // Orphan bucket for leading comments whose predicted owner token
  // has not been pushed yet.  On the next token push this gets copied
  // into the new `token_comments` entry and reset to empty.  If the
  // statement ends with this still populated, it's the "statement-
  // trailing with no owner" case and is surfaced via
  // `token_leading_comments(ntokens)`.  Only `leading_*` is ever
  // non-empty: trailing comments always have a previous token.
  SynqTokenComments pending_orphan_leading;

  // ── Macro expansion state (compiled out with SYNTAQLITE_OMIT_MACROS) ───
#ifndef SYNTAQLITE_OMIT_MACROS
  SynqMacroState macro;
#endif
};

// ── Cross-file helpers ──────────────────────────────────────────────────────

// Whitespace-or-comment classifier. Centralizes the skip predicate used by
// every site that needs to skip over insignificant tokens (the high-level
// feed_token path, scan_macro_args, and the macro-expansion loop and its
// ID-BANG lookahead).
static inline int synq_token_is_skip(uint32_t type) {
  return type == SYNTAQLITE_TK_SPACE || type == SYNTAQLITE_TK_COMMENT;
}

// Record a comment span into p->comments. `offset` is the byte offset into
// p->source. The owning token_idx and side are computed from the parser's
// current state (last_layer0_token_end + p->tokens length): TRAILING when
// the previous layer-0 token ends on the same source line as this comment,
// LEADING otherwise (in which case the predicted owner is the next token
// to be pushed). Caller must check p->collect_tokens before calling.
void synq_parser_record_comment(SyntaqliteParser* p,
                                uint32_t offset,
                                uint32_t len);

// Set the parser's last_status and return it (used by both parser.c and
// parser_macros.c as a convenient exit helper).
int32_t synq_parser_set_result_status(SyntaqliteParser* p, int32_t rc);

// Unified token shift — the sole path that terminals take into Lemon.
// Pushes to p->tokens (if collect_tokens is on and text is non-null),
// builds the SynqParseToken with a real token_idx, feeds Lemon, and
// updates per-layer parser bookkeeping.
//
// `layer_offset` is interpreted layer-locally:
//   - layer 0: statement-relative
//   - layer N: buffer-local (offset into the expansion layer)
// `p->ctx.layer_id` must already reflect the token's layer.
//
// Returns 1 if Lemon flagged `stmt_completed`, 0 otherwise.  Layer-N
// callers (macro expansion) handle their own error messages and clear
// `p->ctx.error` themselves — this function sets `p->had_error` but
// leaves `p->ctx.error` intact when layer_id != 0.
int synq_parser_shift_token(SyntaqliteParser* p,
                            uint32_t token_type,
                            const char* text,
                            uint32_t len,
                            uint32_t layer_offset);

#ifndef SYNTAQLITE_OMIT_MACROS

// Scan balanced parens for macro args.  Defined in parser_macros.c.
uint32_t synq_parser_scan_macro_args(SyntaqliteParser* p,
                                     const char* source,
                                     uint32_t source_len,
                                     uint32_t bang_offset,
                                     SynqMacroArg* out_args,
                                     uint32_t max_args,
                                     uint32_t* out_end_offset);

// Expand a macro call via the lookup callback, push the expansion
// layer, feed its tokens, and clean up.  The sink (Lemon vs.
// arg_scratch) is determined by `p->macro.in_pre_expand` — set by
// `pre_expand_args` when an outer call is pre-expanding its own
// args, cleared otherwise.  Returns 0 on success, -1 if not a macro
// or on error.  Updates *out_end_offset to the position past ')'.
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

static inline int synq_parser_check_macro_straddle(SyntaqliteParser* p) {
  if (!p->ctx.has_macro_straddle)
    return 0;
  snprintf(p->error_msg, sizeof(p->error_msg),
           "macro expansion straddles node boundary");
  p->had_error = 1;
  return -1;
}

// Initialize macro state vecs (callback/expansion fields zeroed by memset).
void synq_macro_state_init(SynqMacroState* m);

// Free all macro state buffers.
void synq_macro_state_free(SynqMacroState* m, SyntaqliteMemMethods mem);

// Free owned expansion data and arg segments on layers 1..N (skip sentinel).
void synq_layers_free_owned(SynqExpansionLayerVec* layers,
                            SyntaqliteMemMethods mem);

// Push the source sentinel at index 0.
void synq_layers_push_sentinel(SynqExpansionLayerVec* layers,
                               const char* source,
                               uint32_t source_len,
                               SyntaqliteMemMethods mem);

#endif  // !SYNTAQLITE_OMIT_MACROS

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_CSRC_PARSER_INTERNAL_H
