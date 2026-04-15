// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Macro expansion pipeline and span resolution.
//
// Split out of parser.c so the core parse loop and the macro machinery can
// be read and maintained independently.  All cross-file helpers are
// declared in `csrc/parser_internal.h`.

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
#include "syntaqlite_dialect/extent_hooks.h"

#include "csrc/util.h"
#include "csrc/parser_internal.h"

// Forward declarations — defined later in this file but called from
// expand_and_feed (nested expansion) and synq_parser_expand_and_feed_macro.
static void begin_macro_expansion(SyntaqliteParser* p,
                                  uint32_t call_offset,
                                  uint32_t call_length,
                                  const char* name,
                                  uint32_t name_len);
static void synq_end_macro(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Per-node extent tracking hooks
// ---------------------------------------------------------------------------
//
// Invoked from `yy_shift` / `yy_reduce` via the macros in
// `extent_hooks.h`.  When enabled, two parallel shadow stacks mirror
// Lemon's symbol stack:
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

void synq_extent_on_shift(SynqParseCtx* pCtx,
                          unsigned int major,
                          const SynqParseToken* token) {
  (void)major;
  if (!pCtx->collect_node_extents) {
    return;
  }
  SynqExtentRange r;
  if (token->layer_id == 0) {
    r.root_start = token->offset;
    r.root_end = token->offset + token->n;
  } else {
    // Attribute tokens from inside a macro expansion to the outermost
    // call-site range stashed in `begin_macro_expansion`.
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
}

void synq_extent_on_reduce(SynqParseCtx* pCtx, unsigned int nrhs) {
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
  // empty entries are neutral; any cross-layer or pre-existing sentinel
  // collapses the result to the sentinel (length = 0).
  SynqNodeExpandedExtent exp_merged = {0, 0, 0};
  for (uint32_t i = len - nrhs; i < len; i++) {
    SynqNodeExpandedExtent e = syntaqlite_vec_at(&pCtx->expanded_stack, i);
    if (e.length == 0) {
      continue;
    }
    if (exp_merged.length == 0) {
      exp_merged = e;
      continue;
    }
    if (exp_merged.layer_id != e.layer_id) {
      exp_merged.length = 0;  // cross-layer → sentinel
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
      &p->dialect, p->macro_fallback, z + pos, &ttype);
  if (tlen <= 0 || ttype != SYNTAQLITE_TK_LP)
    return 0;
  pos += (uint32_t)tlen;

  // Check for empty args: macro!()
  ttype = 0;
  tlen = SynqSqliteGetTokenVersionWrapped(&p->dialect, p->macro_fallback,
                                          z + pos, &ttype);
  if (tlen > 0 && ttype == SYNTAQLITE_TK_RP) {
    *out_end_offset = pos + (uint32_t)tlen;
    return 0;
  }

  uint32_t arg_count = 0;
  uint32_t depth = 1;
  uint32_t arg_start = pos;

  while (pos < source_len && depth > 0) {
    ttype = 0;
    tlen = SynqSqliteGetTokenVersionWrapped(&p->dialect, p->macro_fallback,
                                            z + pos, &ttype);
    if (tlen <= 0)
      return 0;

    if (ttype == SYNTAQLITE_TK_LP) {
      depth++;
    } else if (ttype == SYNTAQLITE_TK_RP) {
      depth--;
      if (depth == 0) {
        if (arg_count < max_args) {
          out_args[arg_count].offset = arg_start;
          out_args[arg_count].length = pos - arg_start;
        }
        arg_count++;
        *out_end_offset = pos + (uint32_t)tlen;
        return arg_count;
      }
    } else if (depth == 1 && ttype == SYNTAQLITE_TK_COMMA) {
      if (arg_count < max_args) {
        out_args[arg_count].offset = arg_start;
        out_args[arg_count].length = pos - arg_start;
      }
      arg_count++;
      arg_start = pos + (uint32_t)tlen;
    } else if (ttype == SYNTAQLITE_TK_SEMI) {
      return 0;
    }

    pos += (uint32_t)tlen;
  }

  return 0;  // Unbalanced parens.
}

// ---------------------------------------------------------------------------
// Macro expansion result (called from inside the lookup callback)
// ---------------------------------------------------------------------------

SYNTAQLITE_API void syntaqlite_macro_expansion_set_result(
    SyntaqliteParser* p,
    const char* body,
    uint32_t body_len,
    uint32_t def_line,
    uint32_t def_col) {
  SynqExpansionLayer* lyr = &p->layers.data[p->macro_pending_layer];
  // Free any previous result (e.g. if set_result is called twice).
  if (lyr->expansion_data)
    p->mem.xFree((void*)lyr->expansion_data);
  char* d = p->mem.xMalloc(body_len + 1);
  memcpy(d, body, body_len);
  d[body_len] = '\0';
  lyr->expansion_data = d;
  lyr->expansion_len = body_len;
  lyr->def_line = def_line;
  lyr->def_col = def_col;
}

// Forward declaration — mutual recursion with expand_and_feed.
// (canonical declaration in parser_internal.h)

// Tokenize `buf` and feed each token to Lemon.
// `depth` is the current expansion nesting (for recursion limit).
// Returns: 0 = ok, 1 = statement boundary, -1 = error.
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

  // Temporarily swap ctx.source so Lemon action offset computations are
  // relative to the expansion buffer.
  const char* saved_source = p->ctx.source;
  p->ctx.source = buf;

  const unsigned char* z = (const unsigned char*)buf;
  uint32_t pos = 0;

  while (pos < buf_len) {
    uint32_t ttype = 0;
    int64_t tlen = SynqSqliteGetTokenVersionWrapped(
        &p->dialect, p->macro_fallback, z + pos, &ttype);
    if (tlen <= 0)
      break;

    if (ttype == SYNTAQLITE_TK_SPACE || ttype == SYNTAQLITE_TK_COMMENT) {
      pos += (uint32_t)tlen;
      continue;
    }

    // Check for nested macro call: ID followed by '!'.
    // Skip when inside a macro definition body — body should be verbatim.
    uint32_t next_pos = pos + (uint32_t)tlen;
    if (ttype == SYNTAQLITE_TK_ID && next_pos < buf_len && z[next_pos] == '!' &&
        p->ctx.in_macro_def_body == 0) {
      uint32_t nested_end = 0;
      int erc = synq_parser_expand_and_feed_macro(
          p, buf, buf_len, pos, (uint32_t)tlen, next_pos, depth + 1,
          &nested_end);
      if (erc == 0) {
        pos = nested_end;
        continue;
      }
      // erc == -1: not a macro or error — feed ID normally below.
      if (p->had_error) {
        p->ctx.source = saved_source;
        return -1;
      }
    }

    // Feed token to Lemon.  `pos` is the offset within the expansion
    // layer; `p->ctx.layer_id` was set to the current expansion's index
    // before expand_and_feed was called.
    SynqParseToken minor = {.z = buf + pos,
                            .n = (uint32_t)tlen,
                            .type = ttype,
                            .token_idx = 0xFFFFFFFF,
                            .offset = pos,
                            .layer_id = (uint8_t)p->ctx.layer_id,
                            ._pad = {0, 0, 0}};
    SYNQ_PARSER_FEED(p->dialect.tmpl, p->lemon, (int)ttype, minor);
    p->last_token_type = ttype;

    if (p->ctx.error) {
      p->had_error = 1;
      if (p->error_msg[0] == '\0') {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "syntax error in macro expansion near '%.*s'", (int)tlen,
                 buf + pos);
      }
      p->ctx.error = 0;
    }

    if (p->ctx.stmt_completed) {
      p->ctx.stmt_completed = 0;
      p->ctx.source = saved_source;
      return 1;
    }

    pos += (uint32_t)tlen;
  }

  p->ctx.source = saved_source;
  return 0;
}

// Expand a macro call and feed the expanded tokens into the parser.
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
  if (!p->macro_lookup_fn)
    return -1;

  // Check blue-paint: recursion detection.
  for (uint32_t i = 0; i < p->expansion_depth; i++) {
    if (synq_name_eq_ci(p->expansion_names[i], p->expansion_name_lens[i],
                        buf + id_offset, id_len)) {
      snprintf(p->error_msg, sizeof(p->error_msg),
               "recursive macro expansion: '%.*s'", (int)id_len,
               buf + id_offset);
      p->had_error = 1;
      return -1;
    }
  }

  // Scan args.
  SynqMacroArg args[64];
  uint32_t end_offset = 0;
  uint32_t arg_count = synq_parser_scan_macro_args(p, buf, buf_len, bang_offset,
                                                   args, 64, &end_offset);

  SyntaqliteToken token_args[64];
  uint32_t token_arg_count = arg_count < 64 ? arg_count : 64;
  for (uint32_t i = 0; i < token_arg_count; i++) {
    token_args[i].text = buf + args[i].offset;
    token_args[i].length = args[i].length;
    token_args[i].type = 0;
  }
  // Push the expansion layer *before* the callback so set_result /
  // expand_and_set_result can write directly into it.
  uint32_t call_length = end_offset - id_offset;
  begin_macro_expansion(p, id_offset, call_length,
                        buf + id_offset, id_len);

  uint32_t new_layer_idx = syntaqlite_vec_len(&p->layers) - 1;
  p->macro_pending_layer = new_layer_idx;
  p->macro_expansion_args = token_args;
  p->macro_expansion_arg_count = token_arg_count;

  int rc = p->macro_lookup_fn(p->macro_lookup_user_data, p,
                              buf + id_offset, id_len,
                              token_args, token_arg_count);
  p->macro_expansion_args = NULL;
  p->macro_expansion_arg_count = 0;

  if (rc == -1 || rc == -2) {
    // Callback failed — tear down the layer we pushed.
    synq_end_macro(p);
    // Free any data the callback may have written before failing.
    SynqExpansionLayer* lyr = &p->layers.data[new_layer_idx];
    if (lyr->expansion_data)
      p->mem.xFree((void*)lyr->expansion_data);
    lyr->expansion_data = NULL;
    lyr->expansion_len = 0;
    if (rc == -2)
      p->had_error = 1;
    return -1;
  }

  // The callback wrote expansion_data/len/def_line/def_col onto the layer.
  SynqExpansionLayer* lyr = &p->layers.data[new_layer_idx];
  const char* data = lyr->expansion_data;
  uint32_t data_len = lyr->expansion_len;

  uint32_t saved_layer_id = p->ctx.layer_id;
  p->ctx.layer_id = new_layer_idx;

  // Push blue-paint for recursion detection.
  p->expansion_names[p->expansion_depth] = buf + id_offset;
  p->expansion_name_lens[p->expansion_depth] = id_len;
  p->expansion_depth++;

  // Feed expanded tokens (may trigger nested macro expansions).
  int frc = expand_and_feed(p, data, data_len, depth);

  // Pop blue-paint.
  p->expansion_depth--;
  p->ctx.layer_id = saved_layer_id;
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
  // Stash the outermost macro call-site range so per-node extent
  // tracking can attribute tokens from inside this (or any nested)
  // expansion back to the authored source.
  if (p->ctx.layer_id == 0) {
    p->ctx.macro_root_start = call_offset;
    p->ctx.macro_root_end = call_offset + call_length;
  }

  SynqExpansionLayer layer = {
      .call_offset = call_offset,
      .call_length = call_length,
      .name = name,
      .name_len = name_len,
      .parent_layer_id = (uint8_t)p->ctx.layer_id,
  };
  syntaqlite_vec_push(&p->layers, layer, p->mem);
  p->macro_depth++;
}

static void synq_end_macro(SyntaqliteParser* p) {
  if (p->macro_depth > 0) {
    p->macro_depth--;
    // Restore layer_id to parent. If we're back to depth 0, that's layer 0
    // (source). Otherwise, find the parent from the current layer.
    if (p->macro_depth == 0) {
      p->ctx.layer_id = 0;
    } else {
      // Walk back to find the still-active parent layer.
      uint32_t cur = p->ctx.layer_id;
      if (cur > 0 && cur < syntaqlite_vec_len(&p->layers)) {
        p->ctx.layer_id = p->layers.data[cur].parent_layer_id;
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
      !p->macro_fallback)
    return -1;
  // Don't expand macros while parsing a macro definition body — the body
  // should be captured verbatim, with nested macro calls preserved as text.
  if (p->ctx.in_macro_def_body > 0)
    return -1;

  if (p->macro_lookup_fn) {
    uint32_t end_off = 0;
    int erc = synq_parser_expand_and_feed_macro(
        p, p->source, p->source_len, id_offset, id_len, bang_offset, 1,
        &end_off);
    if (erc == 0) {
      p->offset = end_off;
      return 0;
    }
    // Not found or error — if had_error was set, propagate.
    if (p->had_error)
      return -1;
  }

  // No callback, or macro not found — fall through to TK_ID fallback.
  // (We already checked macro_style/macro_fallback at the top.)

  // Scan balanced parens to find the end of name!(args).
  uint32_t end_offset = 0;
  synq_parser_scan_macro_args(p, p->source, p->source_len, bang_offset, NULL, 0,
                              &end_offset);
  if (end_offset == 0)
    return -1;  // Unbalanced parens — still an error.

  uint32_t call_length = end_offset - id_offset;

  // Record macro region so formatter emits verbatim (no expansion data).
  begin_macro_expansion(p, id_offset, call_length, NULL, 0);
  p->ctx.layer_id = syntaqlite_vec_len(&p->layers) - 1;
  synq_end_macro(p);

  // Feed the whole name!(args) span as a single TK_ID to Lemon.
  int rc =
      synq_parser_record_and_feed(p, SYNTAQLITE_TK_ID, id_offset, call_length);
  p->offset = end_offset;
  return rc;
}

// ---------------------------------------------------------------------------
// Macro straddle diagnostic
// ---------------------------------------------------------------------------

int synq_parser_check_macro_straddle(SyntaqliteParser* p) {
  uint32_t layer_count = syntaqlite_vec_len(&p->layers);
  // Sentinel occupies index 0; real expansion layers start at 1.
  if (layer_count <= 1)
    return 0;
  if (!p->dialect.tmpl->range_meta) {
    snprintf(p->error_msg, sizeof(p->error_msg),
             "internal error: grammar has no range_meta but macros were used");
    p->had_error = 1;
    return -1;
  }

  uint32_t node_count = syntaqlite_vec_len(&p->ctx.ast.offsets);
  const SynqExpansionLayer* layers = p->layers.data;

  for (uint32_t nid = 0; nid < node_count; nid++) {
    const uint8_t* raw = synq_arena_cptr(&p->ctx.ast, nid);
    uint32_t tag;
    memcpy(&tag, raw, sizeof(tag));
    if (tag == 0 || tag >= p->dialect.tmpl->node_count)
      continue;

    const SyntaqliteRangeMetaEntry* entry = &p->dialect.tmpl->range_meta[tag];
    if (entry->fields == NULL || entry->count == 0)
      continue;

    for (uint32_t mi = 1; mi < layer_count; mi++) {
      uint32_t r_start = layers[mi].call_offset;
      uint32_t r_end = r_start + layers[mi].call_length;

      int has_inside = 0;
      int has_outside = 0;

      for (uint8_t fi = 0; fi < entry->count; fi++) {
        if (entry->fields[fi].kind != 1)
          continue;  // Not a TextSpan.
        const SyntaqliteTextSpan* sp =
            (const SyntaqliteTextSpan*)(raw + entry->fields[fi].offset);
        if (sp->length == 0)
          continue;

        uint32_t s_start = sp->offset;
        uint32_t s_end = sp->offset + sp->length;

        if (s_start >= r_start && s_end <= r_end) {
          has_inside = 1;
        } else {
          has_outside = 1;
        }
      }

      if (has_inside && has_outside) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "macro expansion straddles node boundary");
        p->had_error = 1;
        return -1;
      }
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------
// Macro lookup callback API
// ---------------------------------------------------------------------------

SYNTAQLITE_API void syntaqlite_parser_set_macro_lookup(
    SyntaqliteParser* p,
    SyntaqliteMacroLookupFn fn,
    void* user_data) {
  p->macro_lookup_fn = fn;
  p->macro_lookup_user_data = user_data;
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
    uint32_t param_count) {
  const SyntaqliteToken* args = p->macro_expansion_args;
  uint32_t arg_count = p->macro_expansion_arg_count;

  if (param_count > 0 && arg_count != param_count)
    return -1;

  // Reuse the parser's scratch vec — reset count but keep the allocation.
  p->macro_expand_buf.count = 0;

  const unsigned char* z = (const unsigned char*)body;
  uint32_t pos = 0;
  while (pos < body_len) {
    uint32_t ttype = 0;
    int64_t tlen =
        SynqSqliteGetTokenVersionWrapped(&p->dialect, 0, z + pos, &ttype);
    if (tlen <= 0)
      break;

    if (ttype == SYNTAQLITE_TK_VARIABLE && body[pos] == '$' && tlen > 1) {
      const char* pname = body + pos + 1;
      uint32_t pname_len = (uint32_t)tlen - 1;

      int found = -1;
      for (uint32_t pi = 0; pi < param_count; pi++) {
        if (param_name_lens[pi] == pname_len &&
            memcmp(param_names[pi], pname, pname_len) == 0) {
          found = (int)pi;
          break;
        }
      }

      if (found < 0)
        return -1;

      if ((uint32_t)found < arg_count) {
        syntaqlite_vec_push_n(&p->macro_expand_buf,
                              (const uint8_t*)args[found].text,
                              args[found].length, p->mem);
      }
    } else {
      syntaqlite_vec_push_n(&p->macro_expand_buf,
                            (const uint8_t*)(body + pos),
                            (uint32_t)tlen, p->mem);
    }

    pos += (uint32_t)tlen;
  }

  // Steal the scratch vec's buffer directly into the layer (no copy).
  // Null-terminate for safety.
  syntaqlite_vec_push_n(&p->macro_expand_buf, (const uint8_t*)"", 1, p->mem);
  SynqExpansionLayer* lyr = &p->layers.data[p->macro_pending_layer];
  if (lyr->expansion_data)
    p->mem.xFree((void*)lyr->expansion_data);
  lyr->expansion_data = (const char*)p->macro_expand_buf.data;
  lyr->expansion_len = p->macro_expand_buf.count - 1;  // exclude NUL
  lyr->def_line = 0;
  lyr->def_col = 0;
  // Detach buffer from vec so it won't be freed when vec is reused.
  p->macro_expand_buf.data = NULL;
  p->macro_expand_buf.count = 0;
  p->macro_expand_buf.capacity = 0;
  return 0;
}

// ---------------------------------------------------------------------------
// Span accessors: span_text, span_expanded_text
// ---------------------------------------------------------------------------

// Walk the layer chain to resolve (layer, offset, length) to an authored
// byte range in the source.  At each layer, collapse to the layer's call
// site in the parent and continue walking.
//
// Bounded by SYNQ_MAX_MACRO_DEPTH; typical case is one or two iterations.
static void span_walk_to_source(SyntaqliteParser* p,
                                uint8_t layer_id,
                                uint32_t offset,
                                uint32_t length,
                                uint32_t* out_offset,
                                uint32_t* out_length) {
  uint32_t off = offset;
  uint32_t len = length;
  uint32_t layer = layer_id;
  uint32_t layers_count = syntaqlite_vec_len(&p->layers);
  for (uint32_t step = 0; step < SYNQ_MAX_MACRO_DEPTH + 1; step++) {
    if (layer == 0 || layer >= layers_count) {
      break;
    }
    const SynqExpansionLayer* cur = &p->layers.data[layer];

    // Collapse to the call site in the parent layer.
    off = cur->call_offset;
    len = cur->call_length;
    layer = cur->parent_layer_id;
  }
  *out_offset = off;
  *out_length = len;
}

SYNTAQLITE_API const char* syntaqlite_parser_span_expanded_text(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    uint32_t* out_len) {
  if (!span || span->length == 0) {
    *out_len = 0;
    return NULL;
  }
  uint8_t layer = span->_layer_id;
  if (layer >= syntaqlite_vec_len(&p->layers)) {
    *out_len = 0;
    return NULL;
  }
  const SynqExpansionLayer* lyr = &p->layers.data[layer];
  if (!lyr->expansion_data ||
      span->offset + span->length > lyr->expansion_len) {
    *out_len = 0;
    return NULL;
  }
  *out_len = span->length;
  return lyr->expansion_data + span->offset;
}

SYNTAQLITE_API const char* syntaqlite_parser_span_text(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    uint32_t* out_len,
    uint32_t* out_offset) {
  if (out_offset)
    *out_offset = 0;
  if (!span || span->length == 0) {
    *out_len = 0;
    return NULL;
  }
  // Fast path: spans in the source layer are already authored positions.
  if (span->_layer_id == 0) {
    if (span->offset + span->length > p->source_len) {
      *out_len = 0;
      return NULL;
    }
    *out_len = span->length;
    if (out_offset)
      *out_offset = span->offset;
    return p->source + span->offset;
  }
  // Walk the expansion chain to find the authored bytes.
  uint32_t off = 0;
  uint32_t len = 0;
  span_walk_to_source(p, span->_layer_id, span->offset, span->length, &off,
                      &len);
  if (off + len > p->source_len) {
    *out_len = 0;
    return NULL;
  }
  *out_len = len;
  if (out_offset)
    *out_offset = off;
  return p->source + off;
}

// Compute 1-based (line, col) for `offset` within `buf[..buf_len]`.  The
// offset is clamped to `buf_len`.
static void compute_line_col(const char* buf,
                             uint32_t buf_len,
                             uint32_t offset,
                             uint32_t* out_line,
                             uint32_t* out_col) {
  if (offset > buf_len)
    offset = buf_len;
  // Walk newline-to-newline with memchr instead of char-by-char so
  // long single-line SQL doesn't become O(offset) per frame.
  uint32_t line = 1;
  uint32_t last_nl_end = 0;  // byte position just past the most recent '\n'
  uint32_t scanned = 0;
  while (scanned < offset) {
    const char* nl =
        (const char*)memchr(buf + scanned, '\n', (size_t)(offset - scanned));
    if (!nl)
      break;
    line++;
    last_nl_end = (uint32_t)(nl - buf) + 1;
    scanned = last_nl_end;
  }
  *out_line = line;
  *out_col = offset - last_nl_end + 1;
}

SYNTAQLITE_API const SyntaqliteTracebackFrame* syntaqlite_parser_traceback(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* sp,
    uint32_t* out_count) {
  if (out_count)
    *out_count = 0;
  // Clear the scratch buffer from any previous call.  Keeps the
  // allocation so repeat calls reuse the same heap block.
  syntaqlite_vec_clear(&p->traceback_buf);
  if (!sp || sp->length == 0)
    return NULL;

  // Walk the layer chain, emitting one frame per layer into a small
  // on-stack buffer (innermost first).  Then reverse into the parser's
  // owned vec so the caller sees outermost first.
  SyntaqliteTracebackFrame tmp[SYNQ_MAX_MACRO_DEPTH + 2];
  uint32_t count = 0;
  uint32_t off = sp->offset;
  uint32_t len = sp->length;
  uint8_t layer_id = sp->_layer_id;
  uint32_t layers_count = syntaqlite_vec_len(&p->layers);

  for (uint32_t step = 0;
       step < SYNQ_MAX_MACRO_DEPTH + 1 && count < SYNQ_MAX_MACRO_DEPTH + 2;
       step++) {
    if (layer_id >= layers_count)
      break;
    const SynqExpansionLayer* lyr = &p->layers.data[layer_id];

    SyntaqliteTracebackFrame* f = &tmp[count++];
    f->name = lyr->name;
    f->name_len = lyr->name_len;
    f->snippet = lyr->expansion_data;
    f->snippet_len = lyr->expansion_len;
    f->offset_in_snippet = off;
    f->length_in_snippet = len;
    compute_line_col(lyr->expansion_data, lyr->expansion_len, off, &f->line,
                     &f->col);

    if (layer_id == 0) {
      // Root (sentinel) — walk terminates.
      break;
    }
    // Walk up to parent at this layer's call site.
    off = lyr->call_offset;
    len = lyr->call_length;
    layer_id = lyr->parent_layer_id;
  }

  if (count == 0)
    return NULL;

  // Reverse into the parser's owned buffer so frame[0] is outermost.
  syntaqlite_vec_ensure(&p->traceback_buf, count, p->mem);
  for (uint32_t i = 0; i < count; i++) {
    p->traceback_buf.data[i] = tmp[count - 1 - i];
  }
  p->traceback_buf.count = count;
  if (out_count)
    *out_count = count;
  return p->traceback_buf.data;
}
