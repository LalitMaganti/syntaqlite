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
//   - `syntaqlite_parser_span_text_range(p, &span)` — byte range of
//     `span_text` in the input source.
//   - `syntaqlite_parser_expansion_traceback(p, &span, ...)` — the full
//     outermost → innermost expansion chain for diagnostics.
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

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_TYPES_H
