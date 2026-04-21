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

// ── Position / length / index typedefs ───────────────────────────────────────
//
// These typedefs are documentation aliases over `uint32_t` — they describe
// what *kind* of value a field or parameter holds (which is essential when
// reading the API at a glance) without changing the binary layout or
// imposing any compile-time enforcement.  Mixing them at the call site is
// not a type error in C.
//
// The kinds (all byte-based; UTF-16 positions are LSP-only and live on the
// Rust side):
//   - Stmt*       — byte offset/length measured from the start of the
//                   current statement's source slice
//                   (`syntaqlite_parser_text`).
//   - Doc*        — byte offset/length measured from the start of the full
//                   bound source (`syntaqlite_parser_full_text`).
//   - Layer*      — byte offset/length measured from the start of an
//                   expansion layer's internal buffer (e.g.
//                   `SyntaqliteTextSpan`, `SyntaqliteMacroArgSegment`).
//                   Resolve via the parser's span accessors; never use
//                   directly as a source offset.
//   - TokenIdx    — 0-based index into a statement's token stream.
//   - LineNumber / ColumnNumber — 1-based, with `0` meaning "unknown".

typedef uint32_t SyntaqliteStmtOffset;
typedef uint32_t SyntaqliteStmtLen;

typedef uint32_t SyntaqliteDocOffset;
typedef uint32_t SyntaqliteDocLen;

typedef uint32_t SyntaqliteLayerOffset;
typedef uint32_t SyntaqliteLayerLen;

typedef uint32_t SyntaqliteTokenIdx;

typedef uint32_t SyntaqliteLineNumber;
typedef uint32_t SyntaqliteColumnNumber;

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
  SyntaqliteLayerOffset offset;
  SyntaqliteLayerLen length;
  uint32_t flags;
  uint32_t _layer_id;  // Internal: 0 = source, >0 = macro expansion layer.
} SyntaqliteTextSpan;

// ── Span flags ───────────────────────────────────────────────────────────────

// Identifier was quoted in source (`"..."`, `` `...` ``, or `[...]`).
// The span points to the dequoted inner text; the formatter re-wraps in
// `"..."`.
#define SYNTAQLITE_SPAN_FLAG_QUOTED ((uint32_t)1u)

// Which quote character bracketed the identifier in source.  Set in
// addition to SYNTAQLITE_SPAN_FLAG_QUOTED.  Only one of these is set at a
// time.  Consumers (e.g. SQLite's double-quoted-string bug-compat in the
// analyzer) need to distinguish `"..."` from `` `...` `` and `[...]`,
// which the analyzer can't recover from the dequoted span alone.
#define SYNTAQLITE_SPAN_FLAG_QUOTE_DOUBLE ((uint32_t)2u)
#define SYNTAQLITE_SPAN_FLAG_QUOTE_BACKTICK ((uint32_t)4u)
#define SYNTAQLITE_SPAN_FLAG_QUOTE_BRACKET ((uint32_t)8u)

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
