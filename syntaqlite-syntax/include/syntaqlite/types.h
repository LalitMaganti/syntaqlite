// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Core types shared between the engine and dialect layers.

#ifndef SYNTAQLITE_TYPES_H
#define SYNTAQLITE_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYNTAQLITE_NULL_NODE 0xFFFFFFFFu

typedef uint32_t SyntaqliteCompletionContext;
#define SYNTAQLITE_COMPLETION_CONTEXT_UNKNOWN ((SyntaqliteCompletionContext)0)
#define SYNTAQLITE_COMPLETION_CONTEXT_EXPRESSION \
  ((SyntaqliteCompletionContext)1)
#define SYNTAQLITE_COMPLETION_CONTEXT_TABLE_REF ((SyntaqliteCompletionContext)2)

// A span field embedded in an AST node.
//
// `offset` and `length` are directly usable as byte positions in the source
// string you passed to `syntaqlite_parser_reset` — BUT ONLY if your input
// contains no macro expansions.  If you use macros, `offset`/`length` may
// reference an internal expansion buffer (not your source) and will produce
// garbage if used directly.  In that case you must call
// `syntaqlite_parser_resolve_span_for_node` to get the real source position
// (which points at the entire macro call site for expanded spans).
//
// `_buf_idx` is an internal implementation detail and should not be read by
// consumers.  Treat it as opaque padding.
typedef struct SyntaqliteSourceSpan {
  uint32_t offset;
  uint16_t length;
  uint8_t flags;
  uint8_t _buf_idx;  // Internal: 0 = source, >0 = macro expansion buffer.
} SyntaqliteSourceSpan;

// ── Span flags ───────────────────────────────────────────────────────────────

// Identifier was quoted in source (`"..."`, `` `...` ``, or `[...]`).
// The span points to the dequoted inner text; the formatter re-wraps in
// `"..."`.
#define SYNTAQLITE_SPAN_FLAG_QUOTED ((uint8_t)1u)

static inline int synq_span_is_quoted(SyntaqliteSourceSpan sp) {
  return (sp.flags & SYNTAQLITE_SPAN_FLAG_QUOTED) != 0;
}

static inline SyntaqliteSourceSpan synq_span_set_quoted(
    SyntaqliteSourceSpan sp) {
  sp.flags |= SYNTAQLITE_SPAN_FLAG_QUOTED;
  return sp;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_TYPES_H
