// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Span resolution and traceback.
//
// Navigates the expansion layer tree to resolve AST span coordinates
// back to authored source positions.  Split out of parser_macros.c so
// the macro expansion pipeline and the layer navigation / public span
// API can evolve independently.

#include <string.h>

#include "syntaqlite/parser.h"

#include "csrc/parser_internal.h"

// ---------------------------------------------------------------------------
// Span accessors: span_text, span_expanded_text
// ---------------------------------------------------------------------------

// Try to drill an (offset, length) span through a layer's arg segments.
// If the span lies fully inside a substituted arg, updates *off, *len,
// *layer to the arg's origin and returns 1.  Otherwise returns 0.
static int try_arg_drill(const SynqExpansionLayer* lyr,
                         uint32_t* off,
                         uint32_t* len,
                         uint32_t* layer) {
  for (uint32_t i = 0; i < lyr->arg_segment_count; i++) {
    const SynqArgSegment* seg = &lyr->arg_segments[i];
    if (*off >= seg->sub_offset &&
        *off + *len <= seg->sub_offset + seg->sub_length) {
      uint32_t delta = *off - seg->sub_offset;
      *off = seg->origin_offset + delta;
      *layer = seg->origin_layer_id;
      return 1;
    }
  }
  return 0;
}

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
  uint32_t layers_count = syntaqlite_vec_len(&p->macro.layers);
  // Each iteration either drills into an arg origin layer or moves up
  // to a parent layer; cap defensively at twice the max depth.
  for (uint32_t step = 0; step < 2 * (SYNQ_MAX_MACRO_DEPTH + 1); step++) {
    if (layer == 0 || layer >= layers_count) {
      break;
    }
    const SynqExpansionLayer* cur = &p->macro.layers.data[layer];

    // Arg-segment drill: if the span lies fully inside a substituted arg,
    // the authored bytes live in the segment's origin layer, not via the
    // layer's call site.
    if (try_arg_drill(cur, &off, &len, &layer))
      continue;

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
  uint32_t layer = span->_layer_id;
  if (layer >= syntaqlite_vec_len(&p->macro.layers)) {
    *out_len = 0;
    return NULL;
  }
  const SynqExpansionLayer* lyr = &p->macro.layers.data[layer];
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

// ---------------------------------------------------------------------------
// Traceback
// ---------------------------------------------------------------------------

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
  syntaqlite_vec_clear(&p->macro.traceback_buf);
  if (!sp || sp->length == 0)
    return NULL;

  // Walk the layer chain, emitting one frame per layer into a small
  // on-stack buffer (innermost first).  Then reverse into the parser's
  // owned vec so the caller sees outermost first.
  SyntaqliteTracebackFrame tmp[SYNQ_MAX_MACRO_DEPTH + 2];
  uint32_t count = 0;
  uint32_t off = sp->offset;
  uint32_t len = sp->length;
  uint32_t layer_id = sp->_layer_id;
  uint32_t layers_count = syntaqlite_vec_len(&p->macro.layers);

  for (uint32_t step = 0; step < 2 * (SYNQ_MAX_MACRO_DEPTH + 1) &&
                          count < SYNQ_MAX_MACRO_DEPTH + 2;
       step++) {
    if (layer_id >= layers_count)
      break;
    const SynqExpansionLayer* lyr = &p->macro.layers.data[layer_id];

    if (layer_id == 0) {
      // Root (sentinel) — emit final frame and terminate.
      SyntaqliteTracebackFrame* f = &tmp[count++];
      f->name = lyr->name;
      f->name_len = lyr->name_len;
      f->snippet = lyr->expansion_data;
      f->snippet_len = lyr->expansion_len;
      f->offset_in_snippet = off;
      f->length_in_snippet = len;
      compute_line_col(lyr->expansion_data, lyr->expansion_len, off, &f->line,
                       &f->col);
      break;
    }

    // Arg-segment drill: if the span lies fully inside a substituted arg,
    // skip this layer's frame and drill to the arg's origin.
    uint32_t tb_layer = layer_id;
    if (try_arg_drill(lyr, &off, &len, &tb_layer)) {
      layer_id = (uint8_t)tb_layer;
      continue;
    }

    // No drill — emit a frame for this expansion layer.
    SyntaqliteTracebackFrame* f = &tmp[count++];
    f->name = lyr->name;
    f->name_len = lyr->name_len;
    f->snippet = lyr->expansion_data;
    f->snippet_len = lyr->expansion_len;
    f->offset_in_snippet = off;
    f->length_in_snippet = len;
    compute_line_col(lyr->expansion_data, lyr->expansion_len, off, &f->line,
                     &f->col);

    // Walk up to parent at this layer's call site.
    off = lyr->call_offset;
    len = lyr->call_length;
    layer_id = lyr->parent_layer_id;
  }

  if (count == 0)
    return NULL;

  // Reverse into the parser's owned buffer so frame[0] is outermost.
  syntaqlite_vec_ensure(&p->macro.traceback_buf, count, p->mem);
  for (uint32_t i = 0; i < count; i++) {
    p->macro.traceback_buf.data[i] = tmp[count - 1 - i];
  }
  p->macro.traceback_buf.count = count;
  if (out_count)
    *out_count = count;
  return p->macro.traceback_buf.data;
}
