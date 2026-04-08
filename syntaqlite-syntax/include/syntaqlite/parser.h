// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Streaming parser for SQL — the main entry point for AST access.
//
// Produces a typed AST from SQL text. Each call to syntaqlite_parser_next()
// parses one statement and returns a SYNTAQLITE_PARSE_* status code. Result
// details are accessed via the syntaqlite_result_*() accessors, which are
// valid until the next syntaqlite_parser_next(), reset(), or destroy() call.
// The arena is reset between statements, so only O(statement) memory is used.
//
// Lifecycle: create → [configure] → reset → next (loop) → read nodes → destroy.
// A single parser can be reused across inputs by calling reset() again.
//
// Usage:
//   SyntaqliteParser* p = syntaqlite_parser_create(NULL);
//   syntaqlite_parser_reset(p, sql, len);
//   for (;;) {
//     int32_t rc = syntaqlite_parser_next(p);
//     switch (rc) {
//       case SYNTAQLITE_PARSE_DONE:
//         goto done;
//       case SYNTAQLITE_PARSE_OK: {
//         uint32_t root = syntaqlite_result_root(p);
//         const void* node = syntaqlite_parser_node(p, root);
//         // cast to dialect-specific node type and switch on tag ...
//         break;
//       }
//       case SYNTAQLITE_PARSE_ERROR: {
//         fprintf(stderr, "%s\n", syntaqlite_result_error_msg(p));
//         // syntaqlite_result_recovery_root(p) may return a partial AST,
//         // or SYNTAQLITE_NULL_NODE if no recovery was possible. Either way,
//         // keep looping — the parser will continue to the next statement.
//         break;
//       }
//     }
//   }
//   syntaqlite_parser_destroy(p);
//
// Token/comment capture is OFF by default. If you need
// syntaqlite_result_tokens() / syntaqlite_result_comments() (for formatting,
// diagnostics, etc.), call syntaqlite_parser_set_collect_tokens(p, 1) before
// the first reset().
// For custom dialects, see the "Advanced" section below.
// For macro-aware or incremental token feeding, see incremental.h.

#ifndef SYNTAQLITE_PARSER_H
#define SYNTAQLITE_PARSER_H

#include <stdint.h>
#include <stdio.h>

#include "syntaqlite/config.h"
#include "syntaqlite/grammar.h"
#include "syntaqlite/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

// Opaque parser handle (heap-allocated, reusable across inputs).
typedef struct SyntaqliteParser SyntaqliteParser;

// Return codes from syntaqlite_parser_next() and syntaqlite_parser_finish().
//
//   DONE  = no statement (all input consumed, or only bare semicolons)
//   OK    = statement parsed cleanly; nodes are valid
//   ERROR = statement has syntax/runtime error
//           - use syntaqlite_result_recovery_root() to check whether a
//             partial recovery tree is available
//           - syntaqlite_result_root() is always SYNTAQLITE_NULL_NODE on ERROR
//
// The integer values are stable ABI (DONE=0, OK=1, ERROR=-1).
#define SYNTAQLITE_PARSE_DONE 0
#define SYNTAQLITE_PARSE_OK 1
#define SYNTAQLITE_PARSE_ERROR (-1)

// A comment captured during parsing.
typedef struct SyntaqliteComment {
  uint32_t offset;  // Byte offset in source.
  uint32_t length;  // Byte length.
  uint8_t kind;     // 0 = line comment (--), 1 = block comment (/* */).
} SyntaqliteComment;

// Token-usage flags: set by the parser during disambiguation to record how
// each token was consumed.  Use SYNQ_TOKEN_FLAG_* as bitmasks on the flags
// field of SyntaqliteParserToken.
typedef uint32_t SyntaqliteParserTokenFlags;
#define SYNQ_TOKEN_FLAG_AS_ID \
  ((SyntaqliteParserTokenFlags)1)  // Consumed as identifier (keyword fallback).
#define SYNQ_TOKEN_FLAG_AS_FUNCTION \
  ((SyntaqliteParserTokenFlags)2)  // Consumed as function name.
#define SYNQ_TOKEN_FLAG_AS_TYPE \
  ((SyntaqliteParserTokenFlags)4)  // Consumed as type name.

// A non-whitespace, non-comment token position captured during parsing.
typedef struct SyntaqliteParserToken {
  uint32_t offset;  // Byte offset in source.
  uint32_t length;  // Byte length.
  uint32_t type;    // Original token type from tokenizer (pre-fallback).
  SyntaqliteParserTokenFlags flags;  // Bitmask of SYNQ_TOKEN_FLAG_* values.
} SyntaqliteParserToken;

// A recorded macro invocation region.
// For the input-side begin/end API see incremental.h.
typedef struct SyntaqliteMacroRegion {
  uint32_t call_offset;  // Byte offset of macro call in original source.
  uint32_t call_length;  // Byte length of entire macro call.
} SyntaqliteMacroRegion;

// ---------------------------------------------------------------------------
// Core API
// ---------------------------------------------------------------------------

// Allocate a parser bound to a specific grammar environment.
SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create_with_grammar(
    const SyntaqliteMemMethods* mem,
    SyntaqliteGrammar env);

// Bind a source buffer and reset all internal state. The source must remain
// valid until the next reset() or destroy(). Can be called again to parse a
// new input without reallocating — all previous nodes are invalidated.
SYNTAQLITE_API void syntaqlite_parser_reset(SyntaqliteParser* p,
                                            const char* source,
                                            uint32_t len);

// Parse the next SQL statement. Call in a loop until SYNTAQLITE_PARSE_DONE.
// Bare semicolons between statements are skipped automatically.
// The arena is reset at the start of each call — pointers from the previous
// call become invalid.
//
// Returns one of the SYNTAQLITE_PARSE_* codes.
SYNTAQLITE_API int32_t syntaqlite_parser_next(SyntaqliteParser* p);

// Free the parser, its arena, and all its nodes. No-op if p is NULL.
SYNTAQLITE_API void syntaqlite_parser_destroy(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Result accessors
// Valid until the next syntaqlite_parser_next(), reset(), or destroy() call.
// ---------------------------------------------------------------------------

// Statement root node ID for SYNTAQLITE_PARSE_OK results.
// Returns SYNTAQLITE_NULL_NODE for DONE/ERROR.
SYNTAQLITE_API uint32_t syntaqlite_result_root(SyntaqliteParser* p);

// Partial recovery root for SYNTAQLITE_PARSE_ERROR results.
// Returns SYNTAQLITE_NULL_NODE when no recovery tree is available.
// Recovery trees may include grammar-level error nodes where parsing resumed.
SYNTAQLITE_API uint32_t syntaqlite_result_recovery_root(SyntaqliteParser* p);

// Human-readable error message, or NULL.
SYNTAQLITE_API const char* syntaqlite_result_error_msg(SyntaqliteParser* p);

// Byte offset of error token (0xFFFFFFFF = unknown).
SYNTAQLITE_API uint32_t syntaqlite_result_error_offset(SyntaqliteParser* p);

// Byte length of error token (0 = unknown).
SYNTAQLITE_API uint32_t syntaqlite_result_error_length(SyntaqliteParser* p);

// Per-statement token/comment/macro arrays.
// Token/comment arrays are empty unless collect_tokens is enabled via
// syntaqlite_parser_set_collect_tokens(p, 1) before first reset().
SYNTAQLITE_API const SyntaqliteComment* syntaqlite_result_comments(
    SyntaqliteParser* p,
    uint32_t* count);
SYNTAQLITE_API const SyntaqliteParserToken* syntaqlite_result_tokens(
    SyntaqliteParser* p,
    uint32_t* count);
SYNTAQLITE_API const SyntaqliteMacroRegion* syntaqlite_result_macros(
    SyntaqliteParser* p,
    uint32_t* count);

// ---------------------------------------------------------------------------
// Arena accessors
// ---------------------------------------------------------------------------

// Look up a node by its arena ID. The returned pointer is valid until the
// next syntaqlite_parser_next(), reset(), or destroy(). Cast to the
// dialect-specific node union type and use the tag field to determine which
// member to read.
SYNTAQLITE_API const void* syntaqlite_parser_node(SyntaqliteParser* p,
                                                  uint32_t node_id);

// Return a pointer to the source text bound by the last reset() call.
SYNTAQLITE_API const char* syntaqlite_parser_source(SyntaqliteParser* p);

// Return the byte length of the source text bound by the last reset() call.
SYNTAQLITE_API uint32_t syntaqlite_parser_source_length(SyntaqliteParser* p);

// Return the number of nodes currently in the arena.
SYNTAQLITE_API uint32_t syntaqlite_parser_node_count(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Source span helpers
// ---------------------------------------------------------------------------

static inline const char* syntaqlite_span_text(SyntaqliteParser* p,
                                               SyntaqliteSourceSpan span,
                                               uint32_t* out_len) {
  if (span.length == 0) {
    *out_len = 0;
    return NULL;
  }
  *out_len = span.length;
  return syntaqlite_parser_source(p) + span.offset;
}

static inline uint32_t syntaqlite_span_is_present(SyntaqliteSourceSpan span) {
  return span.length != 0;
}

// ---------------------------------------------------------------------------
// Node and list helpers
// ---------------------------------------------------------------------------

static inline uint32_t syntaqlite_node_is_present(uint32_t node_id) {
  return node_id != SYNTAQLITE_NULL_NODE;
}

static inline uint32_t syntaqlite_list_count(const void* list_node) {
  const uint32_t* raw = (const uint32_t*)list_node;
  return raw[1];
}

static inline uint32_t syntaqlite_list_child_id(const void* list_node,
                                                uint32_t index) {
  const uint32_t* raw = (const uint32_t*)list_node;
  return raw[2 + index];
}

static inline const void* syntaqlite_list_child(SyntaqliteParser* p,
                                                const void* list_node,
                                                uint32_t index) {
  uint32_t child_id = syntaqlite_list_child_id(list_node, index);
  if (child_id == SYNTAQLITE_NULL_NODE)
    return NULL;
  return syntaqlite_parser_node(p, child_id);
}

// ---------------------------------------------------------------------------
// Typed access macros
// ---------------------------------------------------------------------------

#define SYNTAQLITE_NODE(p, Type, id) \
  ((id) == SYNTAQLITE_NULL_NODE      \
       ? (const Type*)0              \
       : (const Type*)syntaqlite_parser_node((p), (id)))

#define SYNTAQLITE_LIST_ITEM(p, Type, list, i) \
  ((const Type*)syntaqlite_list_child((p), (list), (i)))

#define SYNTAQLITE_LIST_FOREACH(p, Type, var, list_id)                    \
  for (const void *                                                       \
           _sqlist_##var = syntaqlite_node_is_present(list_id)            \
                               ? syntaqlite_parser_node((p), (list_id))   \
                               : 0,                                       \
          *_sqonce_##var = 0;                                             \
       !_sqonce_##var; _sqonce_##var = (const void*)1)                    \
    for (uint32_t _sqi_##var = 0,                                         \
                  _sqn_##var = _sqlist_##var                              \
                                   ? syntaqlite_list_count(_sqlist_##var) \
                                   : 0;                                   \
         _sqi_##var < _sqn_##var; _sqi_##var++)                           \
      for (const Type* var =                                              \
               SYNTAQLITE_LIST_ITEM(p, Type, _sqlist_##var, _sqi_##var);  \
           var; var = 0)

// ============================================================================
// Configuration — call after create(), before first reset()
// ============================================================================

// Enable token/comment collection for result_tokens/result_comments.
// Default: off (0), in which case those arrays are empty.
// Returns 0 on success, -1 if the parser has already been used.
SYNTAQLITE_API int32_t syntaqlite_parser_set_collect_tokens(SyntaqliteParser* p,
                                                            uint32_t enable);

// Enable parser trace output (debug builds only). Default: off (0).
// Returns 0 on success, -1 if the parser has already been used.
SYNTAQLITE_API int32_t syntaqlite_parser_set_trace(SyntaqliteParser* p,
                                                   uint32_t enable);

// Enable macro fallback: when the dialect uses SYNQ_MACRO_STYLE_RUST and a
// name!(args) call is encountered but the name is NOT in the macro registry,
// consume the entire name!(args) as a single TK_ID token instead of raising
// a parse error. A MacroRegion is recorded so the formatter can emit the
// call verbatim. Default: off (0).
// Returns 0 on success, -1 if the parser has already been used.
SYNTAQLITE_API int32_t syntaqlite_parser_set_macro_fallback(SyntaqliteParser* p,
                                                            uint32_t enable);

// ============================================================================
// Debugging
// ============================================================================

// Dump an AST node tree as indented text. Returns a malloc'd NUL-terminated
// string. The caller must free() the result. Returns NULL on allocation
// failure.
SYNTAQLITE_API char* syntaqlite_dump_node(SyntaqliteParser* p,
                                          uint32_t node_id,
                                          uint32_t indent);

// ============================================================================
// Advanced: custom dialects
// ============================================================================

#ifndef SYNTAQLITE_OMIT_SQLITE_API
// Allocate a parser for the built-in SQLite grammar. The parser is inert
// until reset() is called. Pass NULL for mem to use malloc/free.
SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create(
    const SyntaqliteMemMethods* mem);

SYNTAQLITE_API SyntaqliteGrammar syntaqlite_sqlite_grammar(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_PARSER_H
