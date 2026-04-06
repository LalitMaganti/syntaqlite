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

typedef struct SyntaqliteSourceSpan {
  uint32_t offset;
  uint16_t length;
  uint16_t flags;
} SyntaqliteSourceSpan;

// ── Span flags ───────────────────────────────────────────────────────────────

// Identifier was quoted in source (`"..."`, `` `...` ``, or `[...]`).
// The span points to the dequoted inner text; the formatter re-wraps in
// `"..."`.
#define SYNTAQLITE_SPAN_FLAG_QUOTED ((uint16_t)1u)

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
