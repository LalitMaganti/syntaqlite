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
// Fast path: when `synq_span_needs_resolve(sp)` is false, `offset` and
// `length` are directly usable as byte positions in the source string you
// passed to `syntaqlite_parser_reset`.  Otherwise the span lives inside a
// macro expansion layer and direct access will produce garbage — call
// `syntaqlite_parser_resolve_span` to get the real source text + byte
// range (which points at the entire macro call site for expanded spans),
// or `syntaqlite_parser_expansion_traceback` if you need the full
// outermost → innermost expansion chain.
typedef struct SyntaqliteSourceSpan {
  uint32_t offset;
  uint16_t length;
  uint8_t flags;
  uint8_t _layer_id;  // Internal: 0 = source, >0 = macro expansion layer.
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

// Returns non-zero if this span requires `syntaqlite_parser_resolve_span` to
// access its text and source position.  When zero, `offset`/`length` are
// directly usable as byte positions in the parser's input string.
static inline int synq_span_needs_resolve(SyntaqliteSourceSpan sp) {
  return sp._layer_id != 0;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_TYPES_H
