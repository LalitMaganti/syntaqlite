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
// Offsets describe a *position within a specific buffer*, so they are
// split by the buffer they index into.  All are byte-based:
//   - StmtOffset   — byte offset from the start of the current
//                    statement's source slice (`syntaqlite_parser_text`).
//   - DocOffset    — byte offset from the start of the full bound
//                    source (`syntaqlite_parser_full_text`).
//   - LayerOffset  — byte offset into an expansion layer's internal
//                    buffer (e.g. `SyntaqliteTextSpan`,
//                    `SyntaqliteMacroArgSegment`).  Resolve via the
//                    parser's span accessors; never use directly as a
//                    source offset.
//
// Lengths are unitless byte counts — a length of 5 means "5 bytes",
// regardless of which buffer those bytes come from.  A single typedef
// (`SyntaqliteLength`) flags a value as "byte count" rather than an
// opaque id / flag / ordinal.
//
// Indices and numeric positions:
//   - TokenIdx     — 0-based index into a statement's token stream.
//   - LineNumber / ColumnNumber — 1-based, with `0` meaning "unknown".

typedef uint32_t SyntaqliteStmtOffset;
typedef uint32_t SyntaqliteDocOffset;
typedef uint32_t SyntaqliteLayerOffset;

typedef uint32_t SyntaqliteLength;

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
  SyntaqliteLength length;
  uint32_t flags;
  uint32_t _layer_id;  // Internal: 0 = source, >0 = macro expansion layer.
} SyntaqliteTextSpan;

// ── Span flags ───────────────────────────────────────────────────────────────

// Quote character flags on `SyntaqliteTextSpan.flags`.  At most one is
// set; none means the identifier was unquoted.  Prefer the accessors
// below over reading these bits directly.
//
// The span's offset/length point at the *dequoted* inner text — the
// surrounding quote bytes are not part of the span.  These flags are
// the only way to recover which character bracketed the identifier
// after dequoting.
#define SYNTAQLITE_SPAN_FLAG_QUOTE_DOUBLE ((uint32_t)1u)    // "..."
#define SYNTAQLITE_SPAN_FLAG_QUOTE_BACKTICK ((uint32_t)2u)  // `...`
#define SYNTAQLITE_SPAN_FLAG_QUOTE_BRACKET ((uint32_t)4u)   // [...]

#define SYNTAQLITE_SPAN_QUOTE_MASK                                           \
  (SYNTAQLITE_SPAN_FLAG_QUOTE_DOUBLE | SYNTAQLITE_SPAN_FLAG_QUOTE_BACKTICK | \
   SYNTAQLITE_SPAN_FLAG_QUOTE_BRACKET)

// Was this identifier quoted in source?  Returns nonzero if `sp` came
// from any of `"..."`, `` `...` ``, or `[...]`.
static inline int syntaqlite_span_is_quoted(SyntaqliteTextSpan sp) {
  return (sp.flags & SYNTAQLITE_SPAN_QUOTE_MASK) != 0;
}

// The character that opened this identifier's quotes in source: `"`,
// `` ` ``, or `[`.  Returns 0 if the span was unquoted.  For `[...]`
// only the opener is reported; the closer is always `]`.
static inline char syntaqlite_span_quote_char(SyntaqliteTextSpan sp) {
  if (sp.flags & SYNTAQLITE_SPAN_FLAG_QUOTE_DOUBLE)
    return '"';
  if (sp.flags & SYNTAQLITE_SPAN_FLAG_QUOTE_BACKTICK)
    return '`';
  if (sp.flags & SYNTAQLITE_SPAN_FLAG_QUOTE_BRACKET)
    return '[';
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_TYPES_H
