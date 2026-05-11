// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Macro expansion pipeline: arg scanning, lookup callback dispatch,
// template expansion, layer lifecycle, and straddle diagnostics.
//
// Compiled out entirely when SYNTAQLITE_OMIT_MACROS is defined.
//
// Span resolution and traceback live in parser_spans.c.
// Per-node extent tracking hooks live in parser_extents.c.
// Cross-file helpers are declared in csrc/parser_internal.h.

#ifndef SYNTAQLITE_OMIT_MACROS

#include <stdio.h>
#include <string.h>

#include "csrc/dialect_dispatch.h"
#include "csrc/token_wrapped.h"
#include "csrc/tokens.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite/parser.h"
#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/dialect_types.h"

#include "csrc/parser_internal.h"
#include "csrc/util.h"

// Forward declarations — defined later in this file but called from
// expand_and_feed (nested expansion) and synq_parser_expand_and_feed_macro.
static void begin_macro_expansion(SyntaqliteParser* p,
                                  uint32_t call_offset,
                                  uint32_t call_length,
                                  const char* name,
                                  uint32_t name_len);
static void synq_end_macro(SyntaqliteParser* p);

// Forward-scan an expansion buffer past whitespace and comments and return
// the next significant token. `*out_pos` is updated to that token's offset;
// `*out_type` and the return value give its type and length. Returns 0
// (with *out_type == 0) at end-of-buffer or tokenizer failure.
//
// Comments inside expansion buffers are recorded to `p->comments` with
// `layer_id = p->ctx.layer_id` (> 0) so consumers that want every
// comment anywhere in the tree — e.g. authored-source + expansion-body
// — can find them via `syntaqlite_token_leading_comments`.  Callers
// filtering to user-authored comments check `Comment.layer_id == 0`.
static int64_t synq_macro_skip(SyntaqliteParser* p,
                               const unsigned char* z,
                               uint32_t buf_len,
                               uint32_t* out_pos,
                               uint32_t* out_type) {
  uint32_t pos = *out_pos;
  while (pos < buf_len) {
    uint32_t ttype = 0;
    int64_t tlen = SynqSqliteGetTokenVersionWrapped(
        &p->dialect, p->macro.macro_fallback, z + pos, &ttype);
    if (tlen <= 0)
      break;
    if (!synq_token_is_skip(ttype)) {
      *out_pos = pos;
      *out_type = ttype;
      return tlen;
    }
    // In pre-expansion (scratch) mode, ctx.layer_id and `pos` don't
    // describe a buffer record_comment can correctly attribute to: the
    // macro layer hasn't been pushed yet and `pos` is local to the
    // arg-text being scanned, not to ctx.source.  Skip recording here
    // — the comment bytes are preserved into arg_scratch and will be
    // re-tokenized at the right layer when the substituted body is
    // fed through Lemon mode.
    if (ttype == SYNTAQLITE_TK_COMMENT && p->collect_tokens &&
        !p->macro.in_pre_expand) {
      synq_parser_record_comment(p, pos, (uint32_t)tlen);
    }
    pos += (uint32_t)tlen;
  }
  *out_pos = pos;
  *out_type = 0;
  return 0;
}

// ---------------------------------------------------------------------------
// Macro argument scanning
// ---------------------------------------------------------------------------

// Scan balanced parens after '!' and split into comma-separated args.
// Returns arg count on success, 0 if not a valid macro call.
// `source`/`source_len` is the buffer being scanned (may be original source
// or an expansion buffer for nested macros).
uint32_t synq_parser_scan_macro_args(SyntaqliteParser* p,
                                     const char* source,
                                     uint32_t source_len,
                                     uint32_t bang_offset,
                                     SynqMacroArg* out_args,
                                     uint32_t max_args,
                                     uint32_t* out_end_offset) {
  const unsigned char* z = (const unsigned char*)source;
  uint32_t pos = bang_offset + 1;  // skip '!'

  // Expect LP.
  uint32_t ttype = 0;
  int64_t tlen = SynqSqliteGetTokenVersionWrapped(
      &p->dialect, p->macro.macro_fallback, z + pos, &ttype);
  if (tlen <= 0 || ttype != SYNTAQLITE_TK_LP)
    return 0;
  pos += (uint32_t)tlen;

  // Check for empty args: macro!()
  ttype = 0;
  tlen = SynqSqliteGetTokenVersionWrapped(&p->dialect, p->macro.macro_fallback,
                                          z + pos, &ttype);
  if (tlen > 0 && ttype == SYNTAQLITE_TK_RP) {
    *out_end_offset = pos + (uint32_t)tlen;
    return 0;
  }

  uint32_t arg_count = 0;
  uint32_t depth = 1;
  uint32_t arg_start = pos;
  uint32_t arg_end = pos;  // end of last significant token in current arg

  while (pos < source_len && depth > 0) {
    ttype = 0;
    tlen = SynqSqliteGetTokenVersionWrapped(
        &p->dialect, p->macro.macro_fallback, z + pos, &ttype);
    if (tlen <= 0)
      return 0;

    int is_skip = synq_token_is_skip(ttype);

    // The fallback-macro path stores the whole call as a single
    // TK_ID, so the main token loop in parser.c never sees tokens
    // inside the call and never calls `synq_parser_record_comment`
    // for them. Record them here instead so consumers that ask
    // "are there any comments in byte range [call_off, call_end)?"
    // (notably the formatter's structured-args bail) see the truth.
    // Same suppression as synq_macro_skip in pre-expansion mode.
    if (ttype == SYNTAQLITE_TK_COMMENT && p->collect_tokens &&
        !p->macro.in_pre_expand) {
      synq_parser_record_comment(p, pos, (uint32_t)tlen);
    }

    if (ttype == SYNTAQLITE_TK_LP) {
      depth++;
    } else if (ttype == SYNTAQLITE_TK_RP) {
      depth--;
      if (depth == 0) {
        if (arg_count < max_args) {
          out_args[arg_count].offset = arg_start;
          out_args[arg_count].length = arg_end - arg_start;
        }
        arg_count++;
        *out_end_offset = pos + (uint32_t)tlen;
        return arg_count;
      }
    } else if (depth == 1 && ttype == SYNTAQLITE_TK_COMMA) {
      if (arg_count < max_args) {
        out_args[arg_count].offset = arg_start;
        out_args[arg_count].length = arg_end - arg_start;
      }
      arg_count++;
      arg_start = pos + (uint32_t)tlen;
      arg_end = arg_start;
    } else if (ttype == SYNTAQLITE_TK_SEMI) {
      return 0;
    }

    // Trim leading whitespace/comments by advancing arg_start.
    // Trim trailing implicitly: arg_end only advances past significant tokens.
    if (depth >= 1 && is_skip && pos == arg_start) {
      arg_start = pos + (uint32_t)tlen;
      arg_end = arg_start;
    } else if (!is_skip) {
      arg_end = pos + (uint32_t)tlen;
    }

    pos += (uint32_t)tlen;
  }

  return 0;  // Unbalanced parens.
}

// ---------------------------------------------------------------------------
// Macro state + layer lifecycle helpers
// ---------------------------------------------------------------------------

void synq_macro_state_init(SynqMacroState* m) {
  syntaqlite_vec_init(&m->expand_buf);
  syntaqlite_vec_init(&m->body_buf);
  syntaqlite_vec_init(&m->arg_scratch);
  syntaqlite_vec_init(&m->layers);
  syntaqlite_vec_init(&m->traceback_buf);
  syntaqlite_vec_init(&m->node_expanded_buf);
}

void synq_macro_state_free(SynqMacroState* m, SyntaqliteMemMethods mem) {
  syntaqlite_vec_free(&m->expand_buf, mem);
  syntaqlite_vec_free(&m->body_buf, mem);
  syntaqlite_vec_free(&m->arg_scratch, mem);
  synq_layers_free_owned(&m->layers, mem);
  syntaqlite_vec_free(&m->layers, mem);
  syntaqlite_vec_free(&m->traceback_buf, mem);
  syntaqlite_vec_free(&m->node_expanded_buf, mem);
}

void synq_layers_free_owned(SynqExpansionLayerVec* layers,
                            SyntaqliteMemMethods mem) {
  for (uint32_t i = 1; i < syntaqlite_vec_len(layers); i++) {
    SynqExpansionLayer* lyr = &layers->data[i];
    if (lyr->expansion_data)
      mem.xFree((void*)lyr->expansion_data);
    if (lyr->arg_segments)
      mem.xFree(lyr->arg_segments);
    if (lyr->args)
      mem.xFree(lyr->args);
  }
}

void synq_layers_push_sentinel(SynqExpansionLayerVec* layers,
                               const char* source,
                               uint32_t source_len,
                               SyntaqliteMemMethods mem) {
  SynqExpansionLayer sentinel = {
      .call_offset = 0,
      .call_length = 0,
      .expansion_data = source,
      .expansion_len = source_len,
      .parent_layer_id = 0,
  };
  syntaqlite_vec_push(layers, sentinel, mem);
}

// ---------------------------------------------------------------------------
// Macro expansion result (called from inside the lookup callback)
// ---------------------------------------------------------------------------

// Internal: free previous layer data and arg segments.
static void layer_free_data(SyntaqliteParser* p, SynqExpansionLayer* lyr) {
  if (lyr->expansion_data)
    p->mem.xFree((void*)lyr->expansion_data);
  if (lyr->arg_segments)
    p->mem.xFree(lyr->arg_segments);
  if (lyr->args)
    p->mem.xFree(lyr->args);
  lyr->expansion_data = NULL;
  lyr->expansion_len = 0;
  lyr->arg_segments = NULL;
  lyr->arg_segment_count = 0;
  lyr->args = NULL;
  lyr->arg_count = 0;
  lyr->is_fallback = 0;
}

SYNTAQLITE_API void syntaqlite_macro_expansion_set_result(SyntaqliteParser* p,
                                                          const char* body,
                                                          uint32_t body_len,
                                                          uint32_t def_line,
                                                          uint32_t def_col) {
  SynqExpansionLayer* lyr = &p->macro.layers.data[p->macro.pending_layer];
  layer_free_data(p, lyr);
  char* d = p->mem.xMalloc(body_len + 1);
  memcpy(d, body, body_len);
  d[body_len] = '\0';
  lyr->expansion_data = d;
  lyr->expansion_len = body_len;
  lyr->def_line = def_line;
  lyr->def_col = def_col;
}

SYNTAQLITE_API void syntaqlite_macro_expansion_set_result_with_arg_map(
    SyntaqliteParser* p,
    const char* body,
    uint32_t body_len,
    uint32_t def_line,
    uint32_t def_col,
    const SyntaqliteArgMapping* mappings,
    uint32_t mapping_count) {
  syntaqlite_macro_expansion_set_result(p, body, body_len, def_line, def_col);

  if (mapping_count == 0)
    return;

  // Build resolved SynqArgSegment array from the caller's mappings.
  SynqExpansionLayer* lyr = &p->macro.layers.data[p->macro.pending_layer];
  const SyntaqliteToken* args = p->macro.expansion_args;
  uint32_t arg_count = p->macro.expansion_arg_count;
  uint32_t origin_layer_id = lyr->parent_layer_id;
  // Origin-layer base: stmt_source for layer 0 (so origin_offset is
  // statement-relative), expansion buffer otherwise.
  const char* origin_base =
      origin_layer_id == 0
          ? p->stmt_source
          : p->macro.layers.data[origin_layer_id].expansion_data;

  SynqArgSegment* segs = p->mem.xMalloc(mapping_count * sizeof(SynqArgSegment));
  uint32_t seg_count = 0;

  for (uint32_t i = 0; i < mapping_count; i++) {
    uint32_t ai = mappings[i].arg_index;
    if (ai >= arg_count)
      continue;
    uint32_t alen = args[ai].length;
    if (alen == 0)
      continue;
    segs[seg_count++] = (SynqArgSegment){
        .sub_offset = mappings[i].body_offset,
        .sub_length = alen,
        .origin_layer_id = origin_layer_id,
        .origin_offset = (uint32_t)(args[ai].text - origin_base),
        .origin_length = alen,
    };
  }

  lyr->arg_segments = segs;
  lyr->arg_segment_count = seg_count;
}

// Forward declaration — mutual recursion with expand_and_feed.
// (canonical declaration in parser_internal.h)

// Tokenize `buf` and dispatch each token according to
// `p->macro.in_pre_expand`:
//   * 0 → feed the token to Lemon (normal expansion).
//   * 1 → append `buf` bytes — including inter-token whitespace and
//     comments, with nested calls' expansion content spliced in — to
//     `p->macro.arg_scratch` (eager arg pre-expansion sink).
//
// `depth` is the current expansion nesting (for recursion limit).
// Returns: 0 = ok, 1 = statement boundary (Lemon mode only), -1 = error.
//
// Trailing whitespace / comments after the last significant token in
// `buf` are NOT appended in scratch mode — only bytes up to the end of
// each token are flushed.  That's fine for arg pre-expansion: trailing
// whitespace doesn't affect re-tokenization.
static int expand_and_feed(SyntaqliteParser* p,
                           const char* buf,
                           uint32_t buf_len,
                           uint32_t depth) {
  if (depth >= SYNQ_MAX_MACRO_DEPTH) {
    snprintf(p->error_msg, sizeof(p->error_msg),
             "macro expansion depth limit exceeded (%d)", SYNQ_MAX_MACRO_DEPTH);
    p->had_error = 1;
    return -1;
  }

  // Swap ctx.source so any read against `p->ctx.source` during
  // tokenization (Lemon action offsets, fallback paths) lines up with
  // the buf we're processing.
  const char* saved_source = p->ctx.source;
  p->ctx.source = buf;

  const int to_scratch = p->macro.in_pre_expand;
  const unsigned char* z = (const unsigned char*)buf;
  uint32_t pos = 0;
  // Scratch-mode cursor: bytes in [last_emit_end, current emission
  // start) are inter-token whitespace/comments we still owe the sink.
  uint32_t last_emit_end = 0;

  while (pos < buf_len) {
    uint32_t ttype = 0;
    int64_t tlen = synq_macro_skip(p, z, buf_len, &pos, &ttype);
    if (tlen <= 0)
      break;

    // Check for nested macro call: ID followed by TK_BANG, mirroring the
    // main parse loop's next_token() behaviour and skipping any whitespace
    // or comments between the two (issue #130). Suppressed inside macro
    // definition bodies — those should be tokenized verbatim.
    uint32_t bang_pos = pos + (uint32_t)tlen;
    uint32_t la_type = 0;
    if (ttype == SYNTAQLITE_TK_ID && p->ctx.in_macro_def_body == 0)
      synq_macro_skip(p, z, buf_len, &bang_pos, &la_type);
    if (ttype == SYNTAQLITE_TK_ID && la_type == SYNTAQLITE_TK_BANG &&
        p->ctx.in_macro_def_body == 0) {
      uint32_t nested_end = 0;
      // In scratch mode, flush pending inter-token bytes before
      // recursing — the recursive call writes inner's expansion
      // *content* into the scratch, not the inner's call source.
      if (to_scratch && pos > last_emit_end) {
        syntaqlite_vec_push_n(&p->macro.arg_scratch,
                              (const uint8_t*)(buf + last_emit_end),
                              pos - last_emit_end, p->mem);
      }
      int erc = synq_parser_expand_and_feed_macro(p, buf, buf_len, pos,
                                                  (uint32_t)tlen, bang_pos,
                                                  depth + 1, &nested_end);
      if (erc == 0) {
        pos = nested_end;
        last_emit_end = nested_end;
        continue;
      }
      // erc == -1: not a macro or error — feed ID normally below.
      if (p->had_error) {
        p->ctx.source = saved_source;
        return -1;
      }
    }

    if (to_scratch) {
      uint32_t token_end = pos + (uint32_t)tlen;
      syntaqlite_vec_push_n(&p->macro.arg_scratch,
                            (const uint8_t*)(buf + last_emit_end),
                            token_end - last_emit_end, p->mem);
      last_emit_end = token_end;
      pos = token_end;
      continue;
    }

    // Feed the token through the unified shift path: it pushes to
    // `p->tokens` with a real `token_idx` (tagged with the current
    // expansion layer) and then feeds Lemon.  `pos` is the token's
    // offset within the expansion buffer; `p->ctx.layer_id` was set
    // to the current expansion's index before expand_and_feed was
    // called.  For layer-N the shift function leaves `p->ctx.error`
    // intact so we can attach a macro-specific error message here.
    int frc = synq_parser_shift_token(p, ttype, buf + pos, (uint32_t)tlen,
                                      (uint32_t)pos);

    if (p->ctx.error) {
      p->had_error = 1;
      if (p->error_msg[0] == '\0') {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "syntax error in macro expansion near '%.*s'", (int)tlen,
                 buf + pos);
      }
      p->ctx.error = 0;
    }

    if (frc == 1 || p->ctx.stmt_completed) {
      p->ctx.stmt_completed = 0;
      p->ctx.source = saved_source;
      return 1;
    }

    pos += (uint32_t)tlen;
  }

  p->ctx.source = saved_source;
  return 0;
}

// Pre-expand each arg that contains a nested macro call into the
// shared `arg_scratch` arena; args without '!' are passed through.
// Runs before the callee's blue-paint frame is pushed so nested calls
// in args are checked against the *caller's* stack (diamond pattern).
//
// Pointers into `arg_scratch.data` are deferred until all pushes are
// done because intermediate pushes can realloc and invalidate any
// pointer captured mid-loop.  The arena is truncated back to its
// caller's mark in `synq_parser_expand_and_feed_macro` before the
// caller feeds its own expansion.
// Inner worker: assumes `p->macro.in_pre_expand` is already 1.  Kept
// separate from `pre_expand_args` so that wrapper owns the flag's
// save/restore around a single call site.
static int pre_expand_args_inner(SyntaqliteParser* p,
                                 const char* buf,
                                 uint32_t arg_count,
                                 const SynqMacroArg* args,
                                 SyntaqliteToken* token_args,
                                 uint32_t depth) {
  uint32_t arg_starts[SYNQ_MAX_MACRO_ARGS];
  uint32_t arg_lens[SYNQ_MAX_MACRO_ARGS];
  uint8_t expanded[SYNQ_MAX_MACRO_ARGS] = {0};

  for (uint32_t i = 0; i < arg_count; i++) {
    const char* arg_text = buf + args[i].offset;
    uint32_t arg_len = args[i].length;
    if (!memchr(arg_text, '!', arg_len))
      continue;

    arg_starts[i] = p->macro.arg_scratch.count;
    if (expand_and_feed(p, arg_text, arg_len, depth) < 0)
      return -1;
    arg_lens[i] = p->macro.arg_scratch.count - arg_starts[i];
    expanded[i] = 1;
  }

  // Resolve pointers now that all pushes (including any from recursive
  // calls) are done; arg_scratch.data is stable from this point until
  // the caller's truncate/feed.
  for (uint32_t i = 0; i < arg_count; i++) {
    if (!expanded[i])
      continue;
    token_args[i].text = (const char*)p->macro.arg_scratch.data + arg_starts[i];
    token_args[i].length = arg_lens[i];
  }
  return 0;
}

static int pre_expand_args(SyntaqliteParser* p,
                           const char* buf,
                           uint32_t arg_count,
                           const SynqMacroArg* args,
                           SyntaqliteToken* token_args,
                           uint32_t depth) {
  // `in_pre_expand` is the dynamic-scope flag that tells everyone
  // downstream — expand_and_feed (which writes to scratch instead of
  // Lemon), synq_macro_skip / scan_macro_args (which suppress comment
  // recording) — that we're inside arg pre-expansion.  Save and
  // restore so nesting through recursive macro calls is well-behaved:
  // an inner pre_expand_args inherits the 1 and restores to 1.
  int saved = p->macro.in_pre_expand;
  p->macro.in_pre_expand = 1;
  int rc = pre_expand_args_inner(p, buf, arg_count, args, token_args, depth);
  p->macro.in_pre_expand = saved;
  return rc;
}

// Expand a macro call and feed the expanded tokens into the parser.
// The sink (Lemon vs. arg_scratch) is read from
// `p->macro.in_pre_expand`, which is owned by `pre_expand_args` and
// set whenever we're inside an outer call's arg pre-expansion.  No
// per-call propagation needed.
//
// Combines lookup-callback invocation, layer creation, and token feeding
// into a single operation.  The layer is pushed *before* the callback so
// that set_result / expand_and_set_result can write directly into it.
//
// Returns 0 on success, -1 if not a registered macro or on error.
// On success, *out_end_offset is set to the byte past the closing paren.
int synq_parser_expand_and_feed_macro(SyntaqliteParser* p,
                                      const char* buf,
                                      uint32_t buf_len,
                                      uint32_t id_offset,
                                      uint32_t id_len,
                                      uint32_t bang_offset,
                                      uint32_t depth,
                                      uint32_t* out_end_offset) {
  if (!p->macro.lookup_fn)
    return -1;

  // Check blue-paint: recursion detection.
  for (uint32_t i = 0; i < p->macro.expansion_depth; i++) {
    if (synq_name_eq_ci(p->macro.expansion_names[i],
                        p->macro.expansion_name_lens[i], buf + id_offset,
                        id_len)) {
      snprintf(p->error_msg, sizeof(p->error_msg),
               "recursive macro expansion: '%.*s'", (int)id_len,
               buf + id_offset);
      p->had_error = 1;
      return -1;
    }
  }

  // Scan args.
  SynqMacroArg args[SYNQ_MAX_MACRO_ARGS];
  uint32_t end_offset = 0;
  uint32_t arg_count = synq_parser_scan_macro_args(
      p, buf, buf_len, bang_offset, args, SYNQ_MAX_MACRO_ARGS, &end_offset);

  SyntaqliteToken token_args[SYNQ_MAX_MACRO_ARGS];
  uint32_t token_arg_count =
      arg_count < SYNQ_MAX_MACRO_ARGS ? arg_count : SYNQ_MAX_MACRO_ARGS;
  for (uint32_t i = 0; i < token_arg_count; i++) {
    token_args[i].text = buf + args[i].offset;
    token_args[i].length = args[i].length;
    token_args[i].type = 0;
  }

  // Capture arg_scratch high-water mark.  pre_expand_args pushes
  // temporaries above it; we restore to this mark before feeding our
  // own expansion so that (a) when our caller is itself in scratch
  // mode, our expansion bytes append at the caller's position keeping
  // its slot contiguous, and (b) we don't leak temporaries on errors.
  uint32_t scratch_mark = p->macro.arg_scratch.count;

  if (pre_expand_args(p, buf, token_arg_count, args, token_args, depth) < 0) {
    p->macro.arg_scratch.count = scratch_mark;
    return -1;
  }

  // Push the expansion layer *before* the callback so set_result /
  // expand_and_set_result can write directly into it.
  uint32_t call_length = end_offset - id_offset;
  begin_macro_expansion(p, id_offset, call_length, buf + id_offset, id_len);

  uint32_t new_layer_idx = syntaqlite_vec_len(&p->macro.layers) - 1;

  p->macro.pending_layer = new_layer_idx;
  p->macro.expansion_args = token_args;
  p->macro.expansion_arg_count = token_arg_count;

  int rc = p->macro.lookup_fn(p->macro.lookup_user_data, p, buf + id_offset,
                              id_len, token_args, token_arg_count);
  p->macro.expansion_args = NULL;
  p->macro.expansion_arg_count = 0;

  if (rc == -1 || rc == -2) {
    SynqExpansionLayer* lyr = &p->macro.layers.data[new_layer_idx];
    if (lyr->expansion_data)
      p->mem.xFree((void*)lyr->expansion_data);
    if (lyr->arg_segments)
      p->mem.xFree(lyr->arg_segments);
    if (lyr->args)
      p->mem.xFree(lyr->args);
    p->macro.layers.count--;
    p->macro.depth--;
    if (rc == -2)
      p->had_error = 1;
    p->macro.arg_scratch.count = scratch_mark;
    return -1;
  }

  // The callback wrote expansion_data/len/def_line/def_col onto the
  // layer.  Persist the call-site arg spans now (after the callback,
  // since `set_result` calls `layer_free_data` which would wipe them
  // otherwise) so downstream consumers — formatter, spans API,
  // traceback — can read them without re-running scan_macro_args.
  // Offsets in `args[]` are buf-relative; rebase top-level layers to
  // statement-relative so they match how `begin_macro_expansion`
  // stored `call_offset`.
  SynqExpansionLayer* lyr = &p->macro.layers.data[new_layer_idx];
  if (token_arg_count > 0) {
    SynqMacroArg* heap = p->mem.xMalloc(token_arg_count * sizeof(SynqMacroArg));
    uint32_t shift = lyr->parent_layer_id == 0 ? p->stmt_start_offset : 0;
    for (uint32_t i = 0; i < token_arg_count; i++) {
      heap[i].offset = args[i].offset - shift;
      heap[i].length = args[i].length;
    }
    lyr->args = heap;
    lyr->arg_count = token_arg_count;
  }
  const char* data = lyr->expansion_data;
  uint32_t data_len = lyr->expansion_len;

  p->ctx.layer_id = new_layer_idx;

  // Push blue-paint for recursion detection.
  p->macro.expansion_names[p->macro.expansion_depth] = buf + id_offset;
  p->macro.expansion_name_lens[p->macro.expansion_depth] = id_len;
  p->macro.expansion_depth++;

  // Truncate scratch back to the caller's mark before feeding our
  // expansion.  In scratch mode, this exposes the caller's
  // accumulating position so our expansion bytes append contiguously
  // onto it.  In Lemon mode, it just frees our pre-expansion temps.
  p->macro.arg_scratch.count = scratch_mark;

  // Feed expanded tokens (may trigger nested macro expansions).
  int frc = expand_and_feed(p, data, data_len, depth);

  p->macro.expansion_depth--;
  synq_end_macro(p);

  if (frc < 0)
    return -1;

  *out_end_offset = end_offset;
  return 0;
}

// ---------------------------------------------------------------------------
// Macro region tracking (internal helper + public begin/end)
// ---------------------------------------------------------------------------

// Internal: push a new expansion layer.
// expansion_data, def_line, def_col are left zeroed — the callback fills
// them via set_result / expand_and_set_result.
static void begin_macro_expansion(SyntaqliteParser* p,
                                  uint32_t call_offset,
                                  uint32_t call_length,
                                  const char* name,
                                  uint32_t name_len) {
  // Top-level call_offset is absolute; rebase to statement-relative
  // so layer call_offset / macro_root_start / body_call_offset match
  // every other per-statement offset.
  if (p->ctx.layer_id == 0) {
    call_offset -= p->stmt_start_offset;
  }

  // Stash the outermost macro call-site range so per-node extent
  // tracking can attribute tokens from inside this (or any nested)
  // expansion back to the authored source.
  if (p->ctx.layer_id == 0) {
    p->ctx.macro_root_start = call_offset;
    p->ctx.macro_root_end = call_offset + call_length;
    if (p->ctx.macro_root_layer == 0) {
      for (uint32_t i = 0; i < p->ctx.lemon_depth; i++)
        syntaqlite_vec_push(&p->ctx.straddle_stack, SYNQ_STRADDLE_NEUTRAL,
                            p->mem);
    }
    p->ctx.macro_root_layer = syntaqlite_vec_len(&p->macro.layers);
  }

  // Compute position of this call in the parent's *authored* body by
  // inverting the length shifts introduced by the parent's $param
  // substitutions.  For top-level calls the parent is the source layer
  // (no arg segments), so the shifts stay zero and body_call_offset /
  // body_call_length equal call_offset / call_length.
  //
  // A segment's relationship to the call range determines its effect:
  //   * fully before call → its length delta shifts body_call_offset
  //   * fully after call  → no effect
  //   * seg contains call (incl. equal bounds) → arg-internal: the call
  //     was tokenized from this arg's substituted text
  //   * seg strictly inside call → its length delta shrinks body_call_length
  //   * partial overlap → arg-internal
  //
  // `body_shift` is signed: positive when the substitution is longer
  // than the `$param` token (body grows), negative when shorter (body
  // shrinks).  Either way, `body_coord = sub_coord - accumulated_shift`.
  //
  // The "contains" check must run before "strictly inside" so the
  // equal-bounds case (common for `m!(arg)` where arg is itself a
  // macro call) is classified as arg-internal rather than inside.
  const SynqExpansionLayer* parent = &p->macro.layers.data[p->ctx.layer_id];
  uint32_t call_end = call_offset + call_length;
  int64_t prefix_shift = 0;
  int64_t inner_shift = 0;
  int arg_internal = 0;
  for (uint32_t i = 0; i < parent->arg_segment_count; i++) {
    const SynqArgSegment* seg = &parent->arg_segments[i];
    uint32_t seg_end = seg->sub_offset + seg->sub_length;
    int64_t body_shift = (int64_t)seg->sub_length - (int64_t)seg->body_length;
    if (seg_end <= call_offset) {
      prefix_shift += body_shift;
    } else if (seg->sub_offset >= call_end) {
      // Fully after the call — no effect.
    } else if (seg->sub_offset <= call_offset && seg_end >= call_end) {
      arg_internal = 1;
      break;
    } else if (seg->sub_offset >= call_offset && seg_end <= call_end) {
      inner_shift += body_shift;
    } else {
      arg_internal = 1;
      break;
    }
  }
  uint32_t body_call_offset =
      arg_internal ? SYNTAQLITE_MACRO_BODY_CALL_ARG_INTERNAL
                   : (uint32_t)((int64_t)call_offset - prefix_shift);
  uint32_t body_call_length =
      arg_internal ? SYNTAQLITE_MACRO_BODY_CALL_ARG_INTERNAL
                   : (uint32_t)((int64_t)call_length - inner_shift);

  SynqExpansionLayer layer = {
      .call_offset = call_offset,
      .call_length = call_length,
      .name = name,
      .name_len = name_len,
      .body_call_offset = body_call_offset,
      .body_call_length = body_call_length,
      .parent_layer_id = p->ctx.layer_id,
  };
  syntaqlite_vec_push(&p->macro.layers, layer, p->mem);
  p->macro.depth++;
}

static void synq_end_macro(SyntaqliteParser* p) {
  if (p->macro.depth > 0) {
    p->macro.depth--;
    // Restore layer_id to parent. If we're back to depth 0, that's layer 0
    // (source). Otherwise, find the parent from the current layer.
    if (p->macro.depth == 0) {
      p->ctx.layer_id = 0;
    } else {
      // Walk back to find the still-active parent layer.
      uint32_t cur = p->ctx.layer_id;
      if (cur > 0 && cur < syntaqlite_vec_len(&p->macro.layers)) {
        p->ctx.layer_id = p->macro.layers.data[cur].parent_layer_id;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Top-level macro dispatch during parsing
// ---------------------------------------------------------------------------

// Try to expand a Rust-style macro call: ID!(args).
// Requires macro_style == RUST and a matching lookup callback (or fallback
// mode). Returns 0 if consumed, -1 if not a macro call, 1 if statement
// boundary.
SYNQ_NOINLINE
int synq_parser_try_macro_call(SyntaqliteParser* p,
                               uint32_t id_offset,
                               uint32_t id_len,
                               uint32_t bang_offset) {
  const unsigned char* z = (const unsigned char*)p->source;
  if (z[bang_offset] != '!')
    return -1;
  if (p->dialect.tmpl->macro_style != SYNQ_MACRO_STYLE_RUST &&
      !p->macro.macro_fallback)
    return -1;
  // Don't expand macros while parsing a macro definition body — the body
  // should be captured verbatim, with nested macro calls preserved as text.
  if (p->ctx.in_macro_def_body > 0)
    return -1;

  if (p->macro.lookup_fn) {
    uint32_t end_off = 0;
    int erc = synq_parser_expand_and_feed_macro(p, p->source, p->source_len,
                                                id_offset, id_len, bang_offset,
                                                1, &end_off);
    if (erc == 0) {
      p->offset = end_off;
      return 0;
    }
    // Not found or error — if had_error was set, propagate.
    if (p->had_error)
      return -1;

    // Lookup callback is registered but the macro was not found.
    // This is a hard error — the user likely misspelled the macro name
    // or forgot to define it.
    snprintf(p->error_msg, sizeof(p->error_msg), "unknown macro '%.*s'",
             (int)id_len, (const char*)z + id_offset);
    p->had_error = 1;
    return -1;
  }

  // No callback — fall through to TK_ID fallback.
  // (We already checked macro_style/macro_fallback at the top.)

  // Scan balanced parens to find the end of name!(args) and capture
  // the top-level arg spans so downstream consumers (notably the
  // formatter's structured-arg pass) don't need to retokenize the
  // call body.  64 is well above any realistic macro arity; calls
  // exceeding that are handled correctly for `end_offset` but have
  // their arg spans dropped (still scanned, just not recorded).
  enum { SYNQ_FALLBACK_ARG_STACK_CAP = 64 };
  uint32_t end_offset = 0;
  SynqMacroArg args_stack[SYNQ_FALLBACK_ARG_STACK_CAP];
  uint32_t arg_count = synq_parser_scan_macro_args(
      p, p->source, p->source_len, bang_offset, args_stack,
      SYNQ_FALLBACK_ARG_STACK_CAP, &end_offset);
  if (end_offset == 0)
    return -1;  // Unbalanced parens — still an error.

  uint32_t call_length = end_offset - id_offset;

  // Record macro region so formatter emits verbatim (no expansion data).
  // Pass the macro name (a source slice) so downstream consumers can
  // read it from the rewrite directly without reparsing the call text.
  begin_macro_expansion(p, id_offset, call_length, (const char*)z + id_offset,
                        id_len);
  p->ctx.layer_id = syntaqlite_vec_len(&p->macro.layers) - 1;

  // Attach captured arg spans to the fresh layer and flag it as a
  // fallback.  scan_macro_args returns source-absolute offsets;
  // begin_macro_expansion rebases top-level call_offset to
  // statement-relative, so apply the same shift to the arg spans.
  SynqExpansionLayer* lyr = &p->macro.layers.data[p->ctx.layer_id];
  lyr->is_fallback = 1;
  if (arg_count > 0 && arg_count <= SYNQ_FALLBACK_ARG_STACK_CAP) {
    SynqMacroArg* heap = p->mem.xMalloc(arg_count * sizeof(SynqMacroArg));
    uint32_t shift = lyr->parent_layer_id == 0 ? p->stmt_start_offset : 0;
    for (uint32_t i = 0; i < arg_count; i++) {
      heap[i].offset = args_stack[i].offset - shift;
      heap[i].length = args_stack[i].length;
    }
    lyr->args = heap;
    lyr->arg_count = arg_count;
  }

  synq_end_macro(p);

  // Feed the whole name!(args) span as a single TK_ID to Lemon.  A
  // TK_ID shift mid-statement cannot complete a statement, so the
  // shift's return value is always 0 and we don't need the main-loop's
  // boundary filter here.
  uint32_t layer_offset = id_offset - p->stmt_start_offset;
  synq_parser_shift_token(p, SYNTAQLITE_TK_ID, p->source + id_offset,
                          call_length, layer_offset);
  p->offset = end_offset;
  return 0;
}

// ---------------------------------------------------------------------------
// Macro lookup callback API
// ---------------------------------------------------------------------------

SYNTAQLITE_API int32_t
syntaqlite_parser_set_macro_lookup(SyntaqliteParser* p,
                                   SyntaqliteMacroLookupFn fn,
                                   void* user_data) {
  p->macro.lookup_fn = fn;
  p->macro.lookup_user_data = user_data;
  return SYNTAQLITE_OK;
}

// ---------------------------------------------------------------------------
// Template expansion helper
// ---------------------------------------------------------------------------

SYNTAQLITE_API int syntaqlite_macro_expansion_expand_and_set_result(
    SyntaqliteParser* p,
    const char* body,
    uint32_t body_len,
    const char* const* param_names,
    const uint32_t* param_name_lens,
    uint32_t param_count,
    uint32_t flags) {
  const SyntaqliteToken* args = p->macro.expansion_args;
  uint32_t arg_count = p->macro.expansion_arg_count;

  if (param_count > 0 && arg_count != param_count)
    return -1;

  // Reuse the parser's scratch vec — reset count but keep the allocation.
  p->macro.expand_buf.count = 0;

  // Stage the body into a NUL-terminated buffer before tokenizing.
  // The tokenizer reads until NUL, but callers may pass non-NUL-terminated
  // buffers (e.g. Rust &str), so we must ensure the NUL sentinel exists.
  p->macro.body_buf.count = 0;
  syntaqlite_vec_push_n(&p->macro.body_buf, (const uint8_t*)body, body_len,
                        p->mem);
  syntaqlite_vec_push(&p->macro.body_buf, 0, p->mem);

  // Collect arg mappings on the stack (max 64 params).  We extend the
  // public SyntaqliteArgMapping with the authored-body position of the
  // $param token (pre-substitution) so downstream tracebacks can anchor
  // substitutions back to the macro definition.
  struct Mapping {
    uint32_t sub_offset;      // Offset in expansion buffer.
    uint32_t arg_index;       // Index into callback args.
    uint32_t body_token_off;  // Offset of $param token in authored body.
    uint32_t body_token_len;  // Length of $param token in authored body.
  } mappings[64];
  uint32_t mapping_count = 0;

  const unsigned char* z = (const unsigned char*)p->macro.body_buf.data;
  const char* zbody = (const char*)p->macro.body_buf.data;
  uint32_t pos = 0;
  while (pos < body_len) {
    uint32_t ttype = 0;
    int64_t tlen =
        SynqSqliteGetTokenVersionWrapped(&p->dialect, 0, z + pos, &ttype);
    if (tlen <= 0)
      break;

    if (ttype == SYNTAQLITE_TK_VARIABLE && zbody[pos] == '$' && tlen > 1) {
      const char* pname = zbody + pos + 1;
      uint32_t pname_len = (uint32_t)tlen - 1;

      int found = -1;
      for (uint32_t pi = 0; pi < param_count; pi++) {
        if (param_name_lens[pi] == pname_len &&
            memcmp(param_names[pi], pname, pname_len) == 0) {
          found = (int)pi;
          break;
        }
      }

      if (found < 0) {
        if (flags & SYNTAQLITE_EXPAND_PASSTHROUGH_UNKNOWN) {
          // Copy the unknown $param verbatim into the expansion buffer.
          syntaqlite_vec_push_n(&p->macro.expand_buf,
                                (const uint8_t*)(zbody + pos), (uint32_t)tlen,
                                p->mem);
          pos += (uint32_t)tlen;
          continue;
        }
        return -1;
      }

      if ((uint32_t)found < arg_count && args[found].length > 0) {
        if (mapping_count < 64) {
          mappings[mapping_count].sub_offset = p->macro.expand_buf.count;
          mappings[mapping_count].arg_index = (uint32_t)found;
          mappings[mapping_count].body_token_off = pos;
          mappings[mapping_count].body_token_len = (uint32_t)tlen;
          mapping_count++;
        }
        syntaqlite_vec_push_n(&p->macro.expand_buf,
                              (const uint8_t*)args[found].text,
                              args[found].length, p->mem);
      }
    } else {
      syntaqlite_vec_push_n(&p->macro.expand_buf, (const uint8_t*)(zbody + pos),
                            (uint32_t)tlen, p->mem);
    }

    pos += (uint32_t)tlen;
  }

  // Steal the scratch vec's buffer directly into the layer (no copy).
  // Null-terminate for safety.
  syntaqlite_vec_push(&p->macro.expand_buf, 0, p->mem);
  SynqExpansionLayer* lyr = &p->macro.layers.data[p->macro.pending_layer];
  layer_free_data(p, lyr);
  lyr->expansion_data = (const char*)p->macro.expand_buf.data;
  lyr->expansion_len = p->macro.expand_buf.count - 1;  // exclude NUL
  lyr->def_line = 0;
  lyr->def_col = 0;
  // Detach buffer from vec so it won't be freed when vec is reused.
  p->macro.expand_buf.data = NULL;
  p->macro.expand_buf.count = 0;
  p->macro.expand_buf.capacity = 0;

  // Build arg segments from the mappings we collected.
  if (mapping_count > 0) {
    uint32_t origin_layer_id = lyr->parent_layer_id;
    const char* origin_base =
        origin_layer_id == 0
            ? p->stmt_source
            : p->macro.layers.data[origin_layer_id].expansion_data;

    SynqArgSegment* segs =
        p->mem.xMalloc(mapping_count * sizeof(SynqArgSegment));
    for (uint32_t i = 0; i < mapping_count; i++) {
      uint32_t ai = mappings[i].arg_index;
      segs[i] = (SynqArgSegment){
          .body_offset = mappings[i].body_token_off,
          .body_length = mappings[i].body_token_len,
          .sub_offset = mappings[i].sub_offset,
          .sub_length = args[ai].length,
          .origin_layer_id = origin_layer_id,
          .origin_offset = (uint32_t)(args[ai].text - origin_base),
          .origin_length = args[ai].length,
      };
    }
    lyr->arg_segments = segs;
    lyr->arg_segment_count = mapping_count;
  }

  return SYNTAQLITE_OK;
}

#endif  // !SYNTAQLITE_OMIT_MACROS
