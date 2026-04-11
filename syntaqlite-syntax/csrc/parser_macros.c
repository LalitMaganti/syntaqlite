// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Macro registry, expansion pipeline, and span resolution.
//
// Split out of parser.c so the core parse loop and the macro machinery can
// be read and maintained independently.  All cross-file helpers are
// declared in `csrc/parser_internal.h`.

#include <stdio.h>
#include <string.h>

#include "csrc/dialect_dispatch.h"
#include "csrc/hashmap.h"
#include "csrc/token_wrapped.h"
#include "csrc/tokens.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite/parser.h"
#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/dialect_types.h"

#include "csrc/parser_internal.h"

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
// Template expansion ($param substitution)
// ---------------------------------------------------------------------------

// Expand a template macro body by substituting $param references.
// Uses the tokenizer to identify TK_VARIABLE tokens rather than
// hand-rolling identifier scanning.
//
// Allocates `*out_buf` via p->mem; caller owns the result.  Also allocates
// `*out_segments` (one per $param substitution) via p->mem; caller owns it
// and must free it (typically by moving it onto the pushed expansion
// layer).  `origin_layer_id` identifies the layer that owns `arg_source`.
//
// Returns 0 on success, -1 on error (unknown $param).
static int expand_template(SyntaqliteParser* p,
                           const SynqMacroEntry* entry,
                           const SynqMacroArg* args,
                           uint32_t arg_count,
                           const char* arg_source,
                           uint32_t origin_layer_id,
                           char** out_buf,
                           uint32_t* out_len,
                           SynqArgSegment** out_segments,
                           uint32_t* out_segment_count) {
  // Pre-size: body length + some slack for arg text.
  uint32_t cap = entry->body_len + 64;
  char* buf = p->mem.xMalloc(cap);
  uint32_t len = 0;
  const char* body = entry->body;
  uint32_t blen = entry->body_len;
  const unsigned char* z = (const unsigned char*)body;

  // Segment list (grows as substitutions happen; NULL until first push).
  SynqArgSegment* segments = NULL;
  uint32_t seg_count = 0;
  uint32_t seg_cap = 0;

  uint32_t pos = 0;
  while (pos < blen) {
    uint32_t ttype = 0;
    int64_t tlen = SynqSqliteGetTokenVersionWrapped(
        &p->dialect, p->macro_fallback, z + pos, &ttype);
    if (tlen <= 0)
      break;

    if (ttype == SYNTAQLITE_TK_VARIABLE && body[pos] == '$' && tlen > 1) {
      // $param — look up the name after '$'.
      const char* pname = body + pos + 1;
      uint32_t pname_len = (uint32_t)tlen - 1;

      int found = -1;
      for (uint32_t pi = 0; pi < entry->param_count; pi++) {
        if (entry->param_name_lens[pi] == pname_len &&
            memcmp(entry->param_names[pi], pname, pname_len) == 0) {
          found = (int)pi;
          break;
        }
      }

      if (found < 0) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "unknown macro parameter '$%.*s'", (int)pname_len, pname);
        p->mem.xFree(buf);
        if (segments)
          p->mem.xFree(segments);
        return -1;
      }

      // Substitute the arg text.
      if ((uint32_t)found < arg_count) {
        uint32_t alen = args[found].length;
        while (len + alen > cap) {
          cap *= 2;
          buf = p->mem.xRealloc(buf, cap);
        }
        uint32_t sub_offset = len;
        memcpy(buf + len, arg_source + args[found].offset, alen);
        len += alen;

        // Record the substitution.  Empty args produce no segment — they
        // have nothing for a traceback to point at.
        if (alen > 0) {
          if (seg_count == seg_cap) {
            seg_cap = seg_cap == 0 ? 4 : seg_cap * 2;
            segments = segments
                           ? p->mem.xRealloc(segments,
                                             seg_cap * sizeof(SynqArgSegment))
                           : p->mem.xMalloc(seg_cap * sizeof(SynqArgSegment));
          }
          segments[seg_count++] = (SynqArgSegment){
              .sub_offset = sub_offset,
              .sub_length = alen,
              .origin_layer_id = origin_layer_id,
              .origin_offset = args[found].offset,
              .origin_length = alen,
          };
        }
      }
      // else: arg not provided — substitute empty string (no segment).
    } else {
      // Copy token verbatim.
      while (len + (uint32_t)tlen > cap) {
        cap *= 2;
        buf = p->mem.xRealloc(buf, cap);
      }
      memcpy(buf + len, body + pos, (uint32_t)tlen);
      len += (uint32_t)tlen;
    }

    pos += (uint32_t)tlen;
  }

  // Null-terminate so the tokenizer has a sentinel when scanning ahead.
  while (len + 1 > cap) {
    cap *= 2;
    buf = p->mem.xRealloc(buf, cap);
  }
  buf[len] = '\0';

  *out_buf = buf;
  *out_len = len;
  *out_segments = segments;
  *out_segment_count = seg_count;
  return 0;
}

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
      SynqMacroExpansion nested = {0};
      int erc = synq_parser_expand_macro(p, buf, buf_len, pos, (uint32_t)tlen,
                                         next_pos, &nested);
      if (erc == 0) {
        // The nested call site is `name!(args)` within the parent buffer.
        // call_offset is `pos` (position of the name token), call_length is
        // the entire `name!(args)` span.
        uint32_t nested_call_length = nested.end_offset - pos;
        int frc = synq_parser_feed_macro_expansion(p, pos, nested_call_length,
                                                   &nested, depth + 1);
        if (frc < 0) {
          p->ctx.source = saved_source;
          return -1;
        }
        pos = nested.end_offset;
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

// Pure macro expansion: look up the macro, scan args, expand the template.
// Fills `out` with the expanded text and metadata.  The caller owns
// out->data (allocated via p->mem) until it is transferred to
// feed_macro_expansion().
// Returns 0 on success, -1 if not a registered macro or on error.
int synq_parser_expand_macro(SyntaqliteParser* p,
                             const char* buf,
                             uint32_t buf_len,
                             uint32_t id_offset,
                             uint32_t id_len,
                             uint32_t bang_offset,
                             SynqMacroExpansion* out) {
  SynqMacroEntry* entry;
  SYNQ_MAP_FIND(p->macro_table, p->macro_table_size, buf + id_offset, id_len,
                entry);
  if (!entry)
    return -1;

  // Check blue-paint: recursion detection.
  for (uint32_t i = 0; i < p->expansion_depth; i++) {
    if (synq_name_eq_ci(p->expansion_names[i], p->expansion_name_lens[i],
                        entry->name, entry->name_len)) {
      snprintf(p->error_msg, sizeof(p->error_msg),
               "recursive macro expansion: '%.*s'", (int)entry->name_len,
               entry->name);
      p->had_error = 1;
      return -1;
    }
  }

  // Parse args.
  SynqMacroArg args[64];
  uint32_t end_offset = 0;
  uint32_t arg_count = synq_parser_scan_macro_args(p, buf, buf_len, bang_offset,
                                                   args, 64, &end_offset);

  // Check arg count.
  if (entry->param_count > 0 && arg_count != entry->param_count) {
    snprintf(p->error_msg, sizeof(p->error_msg),
             "macro '%.*s' expects %u args, got %u", (int)entry->name_len,
             entry->name, entry->param_count, arg_count);
    p->had_error = 1;
    return -1;
  }

  // Expand template.  The origin layer for arg segments is the current
  // layer being parsed — at top-level this is 0 (source), at nested
  // expansion it's the enclosing macro's layer.
  char* expanded = NULL;
  uint32_t expanded_len = 0;
  SynqArgSegment* segments = NULL;
  uint32_t segment_count = 0;
  if (expand_template(p, entry, args, arg_count, buf, p->ctx.layer_id,
                      &expanded, &expanded_len, &segments,
                      &segment_count) < 0) {
    p->had_error = 1;
    return -1;
  }

  out->entry = entry;
  out->data = expanded;
  out->data_len = expanded_len;
  out->end_offset = end_offset;
  out->arg_segments = segments;
  out->arg_segment_count = segment_count;
  return 0;
}

// ---------------------------------------------------------------------------
// Macro region tracking (internal helper + public begin/end)
// ---------------------------------------------------------------------------

// Internal: push a new expansion layer with optional expansion data and
// optional registry-entry provenance.  `entry` may be NULL for layers
// created via the incremental begin/end_macro API (no definition metadata).
static void begin_macro_expansion(SyntaqliteParser* p,
                                  uint32_t call_offset,
                                  uint32_t call_length,
                                  const char* expansion_data,
                                  uint32_t expansion_len,
                                  const SynqMacroEntry* entry) {
  SynqExpansionLayer layer = {
      .call_offset = call_offset,
      .call_length = call_length,
      .expansion_data = expansion_data,
      .expansion_len = expansion_len,
      .template_body = entry ? entry->body : NULL,
      .template_body_len = entry ? entry->body_len : 0,
      .name = entry ? entry->name : NULL,
      .name_len = entry ? entry->name_len : 0,
      .def_line = entry ? entry->def_line : 0,
      .def_col = entry ? entry->def_col : 0,
      .arg_segments = NULL,
      .arg_segment_count = 0,
      .parent_layer_id = (uint8_t)p->ctx.layer_id,
  };
  syntaqlite_vec_push(&p->layers, layer, p->mem);
  p->macro_depth++;
}

SYNTAQLITE_API void syntaqlite_parser_begin_macro(SyntaqliteParser* p,
                                                  uint32_t call_offset,
                                                  uint32_t call_length) {
  begin_macro_expansion(p, call_offset, call_length, NULL, 0, NULL);
  // Set layer_id so spans created while this macro is active reference it.
  p->ctx.layer_id = syntaqlite_vec_len(&p->layers) - 1;
}

SYNTAQLITE_API void syntaqlite_parser_end_macro(SyntaqliteParser* p) {
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

// Register an expansion, feed its tokens, and clean up.
//
// call_offset/call_length locate the macro call in the original source
// (0/0 for nested expansions that have no source-level position).
// Takes ownership of exp->data via the pushed expansion layer.
// begin_macro and end_macro are called symmetrically within this function.
// Returns 0 on success, -1 on error.
int synq_parser_feed_macro_expansion(SyntaqliteParser* p,
                                     uint32_t call_offset,
                                     uint32_t call_length,
                                     SynqMacroExpansion* exp,
                                     uint32_t depth) {
  // Push the new expansion layer (owns the expansion buffer).
  begin_macro_expansion(p, call_offset, call_length, exp->data, exp->data_len,
                        exp->entry);

  // Transfer ownership of the arg segments onto the layer we just pushed.
  // expand_template allocated them via p->mem; reset_stmt/destroy frees
  // them along with the layer.
  uint32_t new_layer_idx = syntaqlite_vec_len(&p->layers) - 1;
  p->layers.data[new_layer_idx].arg_segments = exp->arg_segments;
  p->layers.data[new_layer_idx].arg_segment_count = exp->arg_segment_count;
  exp->arg_segments = NULL;
  exp->arg_segment_count = 0;

  // Set layer_id so spans created during expansion reference this entry.
  uint32_t saved_layer_id = p->ctx.layer_id;
  p->ctx.layer_id = new_layer_idx;

  // Push blue-paint for recursion detection.
  p->expansion_names[p->expansion_depth] = exp->entry->name;
  p->expansion_name_lens[p->expansion_depth] = exp->entry->name_len;
  p->expansion_depth++;

  // Feed expanded tokens (may trigger nested macro expansions).
  int rc = expand_and_feed(p, exp->data, exp->data_len, depth);

  // Pop blue-paint.
  p->expansion_depth--;

  // Restore layer_id.
  p->ctx.layer_id = saved_layer_id;

  syntaqlite_parser_end_macro(p);

  return rc < 0 ? -1 : 0;
}

// ---------------------------------------------------------------------------
// Top-level macro dispatch during parsing
// ---------------------------------------------------------------------------

// Try to expand a Rust-style macro call: ID!(args).
// Requires macro_style == RUST and a matching registry entry (or fallback
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

  // Look up macro in registry.
  SynqMacroEntry* entry = NULL;
  if (p->macro_table_size > 0) {
    SYNQ_MAP_FIND(p->macro_table, p->macro_table_size, p->source + id_offset,
                  id_len, entry);
  }

  if (entry) {
    // Registered macro — expand template, then feed.
    SynqMacroExpansion exp = {0};
    if (synq_parser_expand_macro(p, p->source, p->source_len, id_offset, id_len,
                                 bang_offset, &exp) < 0)
      return -1;
    uint32_t call_length = exp.end_offset - id_offset;
    if (synq_parser_feed_macro_expansion(p, id_offset, call_length, &exp, 1) <
        0)
      return -1;
    p->offset = exp.end_offset;
    return 0;
  }

  // Unregistered macro — fallback to TK_ID.  Always allowed when the
  // grammar declares RUST-style macros; otherwise only when macro_fallback
  // is explicitly set (e.g. embedded-SQL hole placeholders).
  if (p->dialect.tmpl->macro_style != SYNQ_MACRO_STYLE_RUST &&
      !p->macro_fallback)
    return -1;

  // Scan balanced parens to find the end of name!(args).
  uint32_t end_offset = 0;
  synq_parser_scan_macro_args(p, p->source, p->source_len, bang_offset, NULL, 0,
                              &end_offset);
  if (end_offset == 0)
    return -1;  // Unbalanced parens — still an error.

  uint32_t call_length = end_offset - id_offset;

  // Record macro region so formatter emits verbatim (no expansion data).
  syntaqlite_parser_begin_macro(p, id_offset, call_length);
  syntaqlite_parser_end_macro(p);

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
    const uint8_t* raw = (const uint8_t*)synq_arena_ptr(&p->ctx.ast, nid);
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
          continue;  // Not a SourceSpan.
        const SyntaqliteSourceSpan* sp =
            (const SyntaqliteSourceSpan*)(raw + entry->fields[fi].offset);
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
// Macro registry CRUD
// ---------------------------------------------------------------------------

// Helper: duplicate a string via the parser's allocator.
static char* synq_strdup(SyntaqliteMemMethods mem,
                         const char* s,
                         uint32_t len) {
  char* d = mem.xMalloc(len + 1);
  memcpy(d, s, len);
  d[len] = '\0';
  return d;
}

// Free a single macro entry's owned strings.
void synq_parser_free_macro_entry(SyntaqliteParser* p, SynqMacroEntry* e) {
  p->mem.xFree(e->name);
  p->mem.xFree(e->body);
  if (e->param_names) {
    for (uint32_t i = 0; i < e->param_count; i++)
      p->mem.xFree(e->param_names[i]);
    p->mem.xFree(e->param_names);
    p->mem.xFree(e->param_name_lens);
  }
  e->name = NULL;
  e->body = NULL;
  e->param_names = NULL;
  e->param_name_lens = NULL;
  e->state = SYNQ_MAP_EMPTY;
}

SYNTAQLITE_API int syntaqlite_parser_register_macro(
    SyntaqliteParser* p,
    const char* name,
    uint32_t name_len,
    const char* const* param_names,
    uint32_t param_count,
    const char* body,
    uint32_t body_len,
    uint32_t def_line,
    uint32_t def_col) {
  SynqMacroEntry* slot;
  SYNQ_MAP_INSERT(p->macro_table, p->macro_table_size, p->macro_table_count,
                  name, name_len, p->mem, SYNQ_MACRO_TABLE_INITIAL_SIZE, slot);

  // If the slot already has a live entry, free old data first.
  if (slot->name) {
    synq_parser_free_macro_entry(p, slot);
    slot->state = SYNQ_MAP_LIVE;
  }

  slot->name = synq_strdup(p->mem, name, name_len);
  slot->name_len = name_len;
  slot->body = synq_strdup(p->mem, body, body_len);
  slot->body_len = body_len;
  slot->param_count = param_count;
  slot->def_line = def_line;
  slot->def_col = def_col;

  if (param_count > 0) {
    slot->param_names = p->mem.xMalloc(param_count * sizeof(char*));
    slot->param_name_lens = p->mem.xMalloc(param_count * sizeof(uint32_t));
    for (uint32_t i = 0; i < param_count; i++) {
      uint32_t plen = (uint32_t)strlen(param_names[i]);
      slot->param_names[i] = synq_strdup(p->mem, param_names[i], plen);
      slot->param_name_lens[i] = plen;
    }
  } else {
    slot->param_names = NULL;
    slot->param_name_lens = NULL;
  }

  return 0;
}

SYNTAQLITE_API int syntaqlite_parser_deregister_macro(SyntaqliteParser* p,
                                                      const char* name,
                                                      uint32_t name_len) {
  SynqMacroEntry* entry;
  SYNQ_MAP_FIND(p->macro_table, p->macro_table_size, name, name_len, entry);
  if (!entry)
    return -1;
  synq_parser_free_macro_entry(p, entry);
  entry->state = SYNQ_MAP_TOMBSTONE;
  p->macro_table_count--;
  return 0;
}

// ---------------------------------------------------------------------------
// Span accessors: span_text, span_expanded_text, span_text_range
// ---------------------------------------------------------------------------

// Walk the layer chain to resolve (layer, offset, length) to an authored
// byte range in the source.  At each layer, first check whether the span
// falls inside one of the layer's arg segments — if so, drill through to
// the arg's origin layer at the translated offset.  Otherwise collapse to
// the layer's call site in the parent and continue walking.
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
  // Bound the walk.  Each iteration either drills into an arg origin
  // layer or moves up to a parent layer; cap defensively at twice the
  // maximum expansion depth since a drill + collapse can both happen.
  for (uint32_t step = 0; step < 2 * (SYNQ_MAX_MACRO_DEPTH + 1); step++) {
    if (layer == 0 || layer >= layers_count) {
      break;
    }
    const SynqExpansionLayer* cur = &p->layers.data[layer];

    // Arg-segment drill: if the span lies fully inside a substituted arg
    // of THIS layer, the authored bytes live in the segment's origin
    // layer, not via the layer's call site.
    int drilled = 0;
    for (uint32_t i = 0; i < cur->arg_segment_count; i++) {
      const SynqArgSegment* seg = &cur->arg_segments[i];
      if (off >= seg->sub_offset &&
          off + len <= seg->sub_offset + seg->sub_length) {
        uint32_t delta = off - seg->sub_offset;
        off = seg->origin_offset + delta;
        // origin_length is always >= len at this point (the substituted
        // text is copied verbatim), so len is unchanged.
        layer = seg->origin_layer_id;
        drilled = 1;
        break;
      }
    }
    if (drilled)
      continue;

    // Otherwise, collapse to the call site in the parent layer.
    off = cur->call_offset;
    len = cur->call_length;
    layer = cur->parent_layer_id;
  }
  *out_offset = off;
  *out_length = len;
}

SYNTAQLITE_API const char* syntaqlite_parser_span_expanded_text(
    SyntaqliteParser* p,
    const SyntaqliteSourceSpan* span,
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
    const SyntaqliteSourceSpan* span,
    uint32_t* out_len) {
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
  return p->source + off;
}

SYNTAQLITE_API SyntaqliteTextRange
syntaqlite_parser_span_text_range(SyntaqliteParser* p,
                                  const SyntaqliteSourceSpan* span) {
  SyntaqliteTextRange r = {0, 0};
  if (!span || span->length == 0) {
    return r;
  }
  if (span->_layer_id == 0) {
    if (span->offset + span->length > p->source_len)
      return r;
    r.start = span->offset;
    r.end = span->offset + span->length;
    return r;
  }
  uint32_t off = 0;
  uint32_t len = 0;
  span_walk_to_source(p, span->_layer_id, span->offset, span->length, &off,
                      &len);
  if (off + len > p->source_len)
    return r;
  r.start = off;
  r.end = off + len;
  return r;
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
  uint32_t line = 1;
  uint32_t col = 1;
  for (uint32_t i = 0; i < offset; i++) {
    if (buf[i] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }
  *out_line = line;
  *out_col = col;
}

SYNTAQLITE_API const SyntaqliteTracebackFrame* syntaqlite_parser_traceback(
    SyntaqliteParser* p,
    const SyntaqliteSourceSpan* sp,
    uint32_t* out_count) {
  if (out_count)
    *out_count = 0;
  // Clear the scratch buffer from any previous call.  Keeps the
  // allocation so repeat calls reuse the same heap block.
  syntaqlite_vec_clear(&p->traceback_buf);
  if (!sp || sp->length == 0)
    return NULL;

  // Walk the layer chain, emitting one frame per layer into a small
  // on-stack buffer (innermost first).  When the current position lies
  // inside a substituted arg segment, drill through to the arg's
  // origin layer and retry — no frame is emitted for the layer we
  // drilled past, because the span's "real" location at that level is
  // the arg-origin text, not the substitution site.  Then reverse
  // into the parser's owned vec so the caller sees outermost first.
  SyntaqliteTracebackFrame tmp[SYNQ_MAX_MACRO_DEPTH + 2];
  uint32_t count = 0;
  uint32_t off = sp->offset;
  uint32_t len = sp->length;
  uint8_t layer_id = sp->_layer_id;
  uint32_t layers_count = syntaqlite_vec_len(&p->layers);

  // Bound the walk: each iteration either drills or walks up one parent,
  // so at most 2 * (depth + 1) iterations.
  for (uint32_t step = 0; step < 2 * (SYNQ_MAX_MACRO_DEPTH + 1) &&
                          count < SYNQ_MAX_MACRO_DEPTH + 2;
       step++) {
    if (layer_id >= layers_count)
      break;
    const SynqExpansionLayer* lyr = &p->layers.data[layer_id];

    // Arg-segment drill: skip emitting a frame for this layer.
    int drilled = 0;
    for (uint32_t i = 0; i < lyr->arg_segment_count; i++) {
      const SynqArgSegment* seg = &lyr->arg_segments[i];
      if (off >= seg->sub_offset && off < seg->sub_offset + seg->sub_length) {
        off = seg->origin_offset + (off - seg->sub_offset);
        layer_id = seg->origin_layer_id;
        drilled = 1;
        break;
      }
    }
    if (drilled)
      continue;

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
    // Walk up to parent at this layer's call site.  The call site spans
    // the whole `name!(...)` call.
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
