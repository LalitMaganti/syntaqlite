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
// The `offset` / `length` fields are not directly usable: for spans
// tokenized from a macro expansion they reference an internal expansion
// layer buffer, not the input source.  Always read a span through the
// parser's span accessors:
//
//   - `syntaqlite_parser_span_text(p, &span, &len)` — authored bytes
//     (slice of input source); walks through macro call sites and
//     substituted arg segments.
//   - `syntaqlite_parser_span_expanded_text(p, &span, &len)` — the
//     bytes the tokenizer actually saw, which for macro-expanded spans
//     live in an expansion layer buffer.
//   - `syntaqlite_parser_traceback(p, &span, ...)` — the full outermost
//     → innermost expansion chain for diagnostics, with argument-level
//     drill-through fidelity for spans inside substituted macro args.
typedef struct SyntaqliteTextSpan {
  uint32_t offset;
  uint32_t length;
  uint8_t flags;
  uint8_t _layer_id;  // Internal: 0 = source, >0 = macro expansion layer.
  uint8_t _pad[2];
} SyntaqliteTextSpan;

// ── Span flags ───────────────────────────────────────────────────────────────

// Identifier was quoted in source (`"..."`, `` `...` ``, or `[...]`).
// The span points to the dequoted inner text; the formatter re-wraps in
// `"..."`.
#define SYNTAQLITE_SPAN_FLAG_QUOTED ((uint8_t)1u)

static inline int synq_span_is_quoted(SyntaqliteTextSpan sp) {
  return (sp.flags & SYNTAQLITE_SPAN_FLAG_QUOTED) != 0;
}

static inline SyntaqliteTextSpan synq_span_set_quoted(SyntaqliteTextSpan sp) {
  sp.flags |= SYNTAQLITE_SPAN_FLAG_QUOTED;
  return sp;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_TYPES_H
