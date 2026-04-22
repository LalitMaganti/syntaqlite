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
#include "syntaqlite/dialect.h"
#include "syntaqlite/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Parser handle and return codes
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

// Generic success/error codes for setters and other small APIs that
// return 0/-1 status.  Stable ABI.
//
//   OK               = operation succeeded
//   ERR_ALREADY_USED = configuration call made after the parser was sealed
//                      (i.e. after the first reset()/next()/feed_token())
//   ERR_OMITTED      = feature compiled out of this build (e.g. macros
//                      disabled via SYNTAQLITE_OMIT_MACROS)
#define SYNTAQLITE_OK 0
#define SYNTAQLITE_ERR_ALREADY_USED (-1)
#define SYNTAQLITE_ERR_OMITTED (-1)

// ---------------------------------------------------------------------------
// Core API — create, reset, parse, destroy
// ---------------------------------------------------------------------------

#ifndef SYNTAQLITE_OMIT_SQLITE_API
// Allocate a parser for the built-in SQLite dialect.  The parser is
// inert until `reset()` binds a source buffer.  Pass NULL for `mem` to
// use malloc/free.
SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create(
    const SyntaqliteMemMethods* mem);
#endif

// Bind a source buffer and reset all internal state. The source must remain
// valid until the next reset() or destroy(). Can be called again to parse a
// new input without reallocating — all previous nodes are invalidated.
SYNTAQLITE_API void syntaqlite_parser_reset(SyntaqliteParser* p,
                                            const char* source,
                                            SyntaqliteLength len);

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
// Configuration — call after create(), before the first reset()
// ---------------------------------------------------------------------------

// Enable token/comment collection for result_tokens/result_comments.
// Default: off (0), in which case those arrays are empty.
// Returns SYNTAQLITE_OK on success, SYNTAQLITE_ERR_ALREADY_USED if the
// parser has already been used.
SYNTAQLITE_API int32_t syntaqlite_parser_set_collect_tokens(SyntaqliteParser* p,
                                                            uint32_t enable);

// Enable parser trace output (debug builds only). Default: off (0).
// Returns SYNTAQLITE_OK on success, SYNTAQLITE_ERR_ALREADY_USED if the
// parser has already been used.
SYNTAQLITE_API int32_t syntaqlite_parser_set_trace(SyntaqliteParser* p,
                                                   uint32_t enable);

// Enable macro fallback: when the dialect uses SYNQ_MACRO_STYLE_RUST and a
// name!(args) call is encountered but the name is NOT in the macro registry,
// consume the entire name!(args) as a single TK_ID token instead of raising
// a parse error. A MacroRewrite is recorded so the formatter can emit the
// call verbatim. Default: off (0).
// Returns SYNTAQLITE_OK on success, SYNTAQLITE_ERR_ALREADY_USED if the
// parser has already been used, SYNTAQLITE_ERR_OMITTED if macros are
// compiled out (SYNTAQLITE_OMIT_MACROS).
SYNTAQLITE_API int32_t syntaqlite_parser_set_macro_fallback(SyntaqliteParser* p,
                                                            uint32_t enable);

// Enable per-node extent tracking.  When enabled, the parser records
// the source byte range of every AST node it commits to the arena,
// accessible via `syntaqlite_parser_node_text`.  Default: off (0).
// Returns SYNTAQLITE_OK on success, SYNTAQLITE_ERR_ALREADY_USED if the
// parser has already been used.
SYNTAQLITE_API int32_t
syntaqlite_parser_set_collect_node_extents(SyntaqliteParser* p,
                                           uint32_t enable);

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

// Statement-relative byte offset of error token (0xFFFFFFFF = unknown).
SYNTAQLITE_API SyntaqliteStmtOffset
syntaqlite_result_error_offset(SyntaqliteParser* p);

// Byte length of error token (0 = unknown).
SYNTAQLITE_API SyntaqliteLength
syntaqlite_result_error_length(SyntaqliteParser* p);

// A comment captured during parsing.
//
// Each comment is bound at parse time to one of the statement's tokens:
//   - `side == SYNQ_COMMENT_LEADING` and `token_idx == N` means the comment
//     appears immediately before token N (no significant token between them
//     in the source) and either token N is the first token of the statement
//     or the comment is on its own source line.
//   - `side == SYNQ_COMMENT_TRAILING` and `token_idx == N` means the comment
//     appears on the same source line as token N's end, with no significant
//     token between them.
// `token_idx` indexes into the same per-statement token array returned by
// `syntaqlite_result_tokens`.  When the comment trails the last token of a
// statement and no following statement exists, `token_idx` may equal
// `count` (one past the last token) — consumers should treat that as
// "statement-trailing with no owner."
typedef uint8_t SyntaqliteCommentSide;
#define SYNQ_COMMENT_LEADING ((SyntaqliteCommentSide)0)
#define SYNQ_COMMENT_TRAILING ((SyntaqliteCommentSide)1)

typedef struct SyntaqliteComment {
  SyntaqliteStmtOffset offset;
  SyntaqliteLength length;
  SyntaqliteTokenIdx token_idx;  // Index of the owning token in p->tokens.
  uint8_t kind;  // 0 = line comment (--), 1 = block comment (/* */).
  uint8_t side;  // SYNQ_COMMENT_LEADING or SYNQ_COMMENT_TRAILING.
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
//
// For tokens produced by macro expansion, `offset` is a byte position in
// the expansion layer's internal buffer (identified by `_layer_id`), not a
// position in the input source.  Consumers that need source-level
// positions should resolve the token's span via the parser's span
// accessors rather than using `offset` directly.
typedef struct SyntaqliteParserToken {
  SyntaqliteLayerOffset offset;
  SyntaqliteLength length;
  uint32_t type;  // Original token type from tokenizer (pre-fallback).
  SyntaqliteParserTokenFlags flags;  // Bitmask of SYNQ_TOKEN_FLAG_* values.
  uint32_t _layer_id;  // Internal: 0 = original source, >0 = expansion layer.
} SyntaqliteParserToken;

// Per-statement token/comment arrays.
// Empty unless collect_tokens is enabled via
// syntaqlite_parser_set_collect_tokens(p, 1) before first reset().
SYNTAQLITE_API const SyntaqliteComment* syntaqlite_result_comments(
    SyntaqliteParser* p,
    uint32_t* count);
SYNTAQLITE_API const SyntaqliteParserToken* syntaqlite_result_tokens(
    SyntaqliteParser* p,
    uint32_t* count);

// Get the comments attached to a specific token.
//
// Returns a pointer to the first comment in `p->comments` whose
// `token_idx == token_idx` and `side == side`, and writes the count of
// such comments to `*count`.  Comments attached to the same token are
// contiguous in `p->comments` and recorded in source order.
//
// Returns NULL with `*count == 0` when there are no matching comments.
SYNTAQLITE_API const SyntaqliteComment* syntaqlite_token_leading_comments(
    SyntaqliteParser* p,
    SyntaqliteTokenIdx token_idx,
    uint32_t* count);

SYNTAQLITE_API const SyntaqliteComment* syntaqlite_token_trailing_comments(
    SyntaqliteParser* p,
    SyntaqliteTokenIdx token_idx,
    uint32_t* count);

// Sentinel value for `SyntaqliteMacroRewrite::parent_idx` meaning "this
// rewrite applies directly to the authored source" (i.e. the rewrite is
// not nested inside another macro's expansion).
#define SYNTAQLITE_MACRO_PARENT_SOURCE UINT32_MAX

// Sentinel value for `SyntaqliteMacroRewrite::body_call_offset` and
// `body_call_length` meaning "this call was tokenized from a $param
// substitution — it has no position in the parent's authored body;
// consumers should descend through the matching arg segment instead."
#define SYNTAQLITE_MACRO_BODY_CALL_ARG_INTERNAL UINT32_MAX

// A recorded macro invocation — enough information to reconstruct a
// source-to-expanded rewrite tree (e.g. to drive Perfetto's
// SqlSource::Rewriter or an equivalent).
//
// Entries are reported in insertion order: outer macros appear before the
// nested macros they contain, and macros at the same nesting level appear
// in source order.
//
// `parent_idx` is either SYNTAQLITE_MACRO_PARENT_SOURCE (the rewrite
// replaces a range in the authored source) or the index of another entry
// in this same flat list (the rewrite replaces a range in that entry's
// `expansion` buffer).
//
// `call_offset` / `call_length` describe the byte range of the macro
// call inside the parent's text: statement-relative when `parent_idx`
// is the source sentinel, otherwise relative to the parent entry's
// `expansion` buffer.
//
// `expansion` is the replacement text for that range.  It is NOT
// NUL-terminated; use `expansion_len`.  Nested macro calls appearing
// inside `expansion` are reported as separate entries that reference this
// entry via their `parent_idx`.
//
// `name` is the macro name as it appears at the call site (NOT
// NUL-terminated; use `name_len`).
//
// `def_line` / `def_col` record the 1-based line/column of the macro
// definition (0 if unknown), for traceback purposes.
//
// Pointers (`expansion`, `name`) are owned by the parser and remain valid
// until the next `syntaqlite_parser_next`, `syntaqlite_parser_reset`, or
// `syntaqlite_parser_destroy` call.
typedef struct SyntaqliteMacroRewrite {
  uint32_t parent_idx;
  // Statement-relative when parent_idx == SYNTAQLITE_MACRO_PARENT_SOURCE,
  // otherwise relative to the parent entry's `expansion` buffer.
  SyntaqliteLayerOffset call_offset;
  SyntaqliteLength call_length;
  const char* expansion;
  SyntaqliteLength expansion_len;
  const char* name;
  SyntaqliteLength name_len;
  SyntaqliteLineNumber def_line;
  SyntaqliteColumnNumber def_col;
  // Position of this call in the *parent's authored body*, computed by
  // inverting the length shifts the parent's $param substitutions
  // introduced.  Both fields equal SYNTAQLITE_MACRO_BODY_CALL_ARG_INTERNAL
  // (UINT32_MAX) when the call was tokenized from a substituted arg's
  // text (no meaningful body position) and consumers should descend
  // through the matching arg segment instead.
  //
  // For top-level rewrites (parent_idx == SYNTAQLITE_MACRO_PARENT_SOURCE)
  // the parent is the authored source, so these equal call_offset /
  // call_length.
  SyntaqliteLayerOffset body_call_offset;
  SyntaqliteLength body_call_length;
  // The buffer the `call_offset` — and every arg offset returned by
  // syntaqlite_macro_rewrite_arg_at — indexes into.  For top-level
  // rewrites (parent_idx == SYNTAQLITE_MACRO_PARENT_SOURCE) this is
  // the current statement source slice; for nested rewrites it is
  // the parent entry's `expansion` buffer.  Consumers can slice the
  // call text as `parent_buffer + call_offset` and the arg texts
  // likewise, without resolving parent_idx themselves.
  //
  // Not NUL-terminated; use `parent_buffer_len`.  Owned by the
  // parser and valid until the next reset / next / destroy call.
  const char* parent_buffer;
  SyntaqliteLength parent_buffer_len;
  // 1 if this call went down the fallback path (unregistered name!
  // kept verbatim as a TK_ID, no expansion, no $param substitutions);
  // 0 if it was expanded by a registered macro.  `expansion_len` is
  // also a useful tell (0 for fallback), but this flag is the
  // authoritative signal.
  uint32_t is_fallback;
} SyntaqliteMacroRewrite;

// Number of macro rewrites recorded for the current statement.
SYNTAQLITE_API uint32_t syntaqlite_result_macro_count(SyntaqliteParser* p);

// Returns the rewrite at `idx` (0-based).  Returns a zero-initialized
// struct if `idx >= syntaqlite_result_macro_count(p)`.
SYNTAQLITE_API SyntaqliteMacroRewrite
syntaqlite_result_macro_rewrite_at(SyntaqliteParser* p, uint32_t idx);

// One $param substitution within a macro expansion.
//
// `body_offset` / `body_length` locate the `$param` token in the macro's
// authored body.  Populated by the template-expansion path; zero for
// macros registered via the raw set_result_with_arg_map API.
//
// `expansion_offset` / `expansion_length` locate the substituted arg
// text in the rewrite's `expansion` buffer.
//
// `origin_parent_idx` + `origin_offset` + `origin_length` locate the
// arg text where it was authored — either in the original source
// (`origin_parent_idx == SYNTAQLITE_MACRO_PARENT_SOURCE`) or in another
// rewrite's `expansion` buffer (rewrite index).  Consumers walk the
// chain of $param substitutions by recursing into the origin rewrite's
// arg segments.
typedef struct SyntaqliteMacroArgSegment {
  SyntaqliteLayerOffset body_offset;
  SyntaqliteLength body_length;
  SyntaqliteLayerOffset expansion_offset;
  SyntaqliteLength expansion_length;
  uint32_t origin_parent_idx;
  SyntaqliteLayerOffset origin_offset;
  SyntaqliteLength origin_length;
} SyntaqliteMacroArgSegment;

// Number of arg segments recorded on the rewrite at `rewrite_idx`.
// Returns 0 if `rewrite_idx` is out of range.
SYNTAQLITE_API uint32_t
syntaqlite_macro_rewrite_arg_segment_count(SyntaqliteParser* p,
                                           uint32_t rewrite_idx);

// Returns the arg segment at `segment_idx` on the rewrite at
// `rewrite_idx`.  Returns a zero-initialized struct if either index is
// out of range.
SYNTAQLITE_API SyntaqliteMacroArgSegment
syntaqlite_macro_rewrite_arg_segment_at(SyntaqliteParser* p,
                                        uint32_t rewrite_idx,
                                        uint32_t segment_idx);

// One top-level argument of a macro call, as written at the call
// site.  Populated for both registered (expanded) and fallback calls:
// the parser scans `name!(a, b, c)` the same way regardless of
// whether `name` resolved to a registered macro.
//
// `offset` is in the same coordinate system as the enclosing
// rewrite's `call_offset`, and indexes into the enclosing rewrite's
// `parent_buffer`.  Slice the arg text as
// `rewrite.parent_buffer + offset` for `length` bytes.  Leading and
// trailing whitespace / comments are trimmed from the range.
typedef struct SyntaqliteMacroCallArg {
  SyntaqliteLayerOffset offset;
  SyntaqliteLength length;
} SyntaqliteMacroCallArg;

// Number of top-level call-site arg spans recorded on the rewrite at
// `rewrite_idx`.  Returns 0 for `name!()` with no args, calls whose
// arity exceeded the scan buffer (rare; >64 args), or out-of-range
// indices.
SYNTAQLITE_API uint32_t
syntaqlite_macro_rewrite_arg_count(SyntaqliteParser* p, uint32_t rewrite_idx);

// Returns the call-site arg at `arg_idx` on the rewrite at
// `rewrite_idx`.  Returns a zero-initialized struct if either index
// is out of range.
SYNTAQLITE_API SyntaqliteMacroCallArg
syntaqlite_macro_rewrite_arg_at(SyntaqliteParser* p,
                                uint32_t rewrite_idx,
                                uint32_t arg_idx);

// ---------------------------------------------------------------------------
// Arena accessors
// ---------------------------------------------------------------------------

// Look up a node by its arena ID. The returned pointer is valid until the
// next syntaqlite_parser_next(), reset(), or destroy(). Cast to the
// dialect-specific node union type and use the tag field to determine which
// member to read.
SYNTAQLITE_API const void* syntaqlite_parser_node(SyntaqliteParser* p,
                                                  uint32_t node_id);

// Source slice for the last-completed statement.  Writes the
// statement's document-absolute byte offset within the bound source to
// `*out_offset` (optional) and its length to `*out_len` (optional).
// Every offset the parser emits for this statement — tokens, comments,
// node extents, spans, error offset, macro rewrite call offsets — is
// measured from the returned pointer.  Returns NULL / 0 / 0 when no
// statement has been produced yet.
SYNTAQLITE_API const char* syntaqlite_parser_text(
    SyntaqliteParser* p,
    SyntaqliteDocOffset* out_offset,
    SyntaqliteLength* out_len);

// Full SQL source bound by the last reset() call.  For multi-statement
// input, this is the whole input.
SYNTAQLITE_API const char* syntaqlite_parser_full_text(
    SyntaqliteParser* p,
    SyntaqliteLength* out_len);

// Post-expansion text for the current statement — materializes the
// statement's source with every currently-active macro call replaced
// by its expansion into a parser-owned scratch buffer.  The returned
// pointer is valid until the next `*_expanded_text` call on the same
// parser or until the parser advances to the next statement.
//
// `*out_len` receives the byte length of the materialized buffer,
// which may differ from the authored statement length.
SYNTAQLITE_API const char* syntaqlite_parser_expanded_text(
    SyntaqliteParser* p,
    SyntaqliteLength* out_len);

// Return the number of nodes currently in the arena.
SYNTAQLITE_API uint32_t syntaqlite_parser_node_count(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Source text accessors — spans, nodes, and tracebacks
// ---------------------------------------------------------------------------

// One frame in a span traceback, produced by
// `syntaqlite_parser_traceback`.  Each frame is self-contained: it
// carries a snippet buffer to render the caret against, the offset
// within that buffer, and an optional macro name for non-root frames.
//
// `name` is `NULL` (and `name_len == 0`) for the root source frame, and
// borrows from the macro registry entry (valid for the parse's lifetime)
// for macro expansion frames.  `snippet` borrows either the user's input
// source (for the root frame) or the corresponding expansion layer's
// buffer (for macro frames).
//
// `line` and `col` are 1-based and computed from `offset_in_snippet`
// within `snippet`.
typedef struct SyntaqliteTracebackFrame {
  const char* name;
  SyntaqliteLength name_len;
  SyntaqliteLineNumber line;
  SyntaqliteColumnNumber col;
  const char* snippet;
  SyntaqliteLength snippet_len;
  SyntaqliteLayerOffset offset_in_snippet;
  SyntaqliteLength length_in_snippet;
} SyntaqliteTracebackFrame;

// Build a traceback for a span and return a pointer to a parser-owned
// frame array.  Frames are ordered from outermost (the root source
// frame) to innermost (the position inside the deepest macro
// expansion layer).
//
// When the span was tokenized inside a substituted macro argument, the
// walk drills through the substitution to the argument's origin layer
// — the innermost frame points at the user's authored arg text rather
// than at the macro call site.
//
// Writes the number of frames into `*out_count`.  For a span not
// inside any macro expansion, returns a single-frame slice with
// `name == NULL`.  Returns NULL (and writes 0 to `*out_count`) for
// empty or invalid spans.
//
// The returned pointer is valid until the next call to
// `syntaqlite_parser_traceback` on the same parser or until the next
// `syntaqlite_parser_next` resets the current statement — callers
// that need to retain frames across such calls must copy them out.
SYNTAQLITE_API const SyntaqliteTracebackFrame* syntaqlite_parser_traceback(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    uint32_t* out_count);

// Post-expansion text for `span` — the bytes the tokenizer actually saw.
// For macro-free spans, a slice of the input source.  For spans inside a
// macro expansion, a slice of the expansion layer's buffer.  Writes the
// slice's byte length to `*out_len`.  Always a direct slice — no
// allocation.
//
// Returns NULL (and writes 0 to `*out_len`) for empty or invalid spans.
SYNTAQLITE_API const char* syntaqlite_parser_span_expanded_text(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    SyntaqliteLength* out_len);

// Authored text for `span` — always a slice of the current statement's
// source (the same buffer `syntaqlite_parser_text` returns).
//
// For macro-free spans, identical to `span_expanded_text`.  For spans
// inside a macro expansion, walks the expansion-layer chain:
//   - If the span falls inside a substituted `$param` arg segment, drills
//     to the arg's origin text in the caller's layer (recursively).
//   - Otherwise, collapses to the outermost `name!(...)` call site.
//
// Always a direct slice of the statement source — no allocation.
// Returns NULL (and writes 0 to `*out_len`) for empty or invalid spans.
//
// `out_offset` is optional; when non-NULL it receives the
// statement-relative byte offset where the returned slice begins.
// To convert to a document-absolute offset, add the offset
// `syntaqlite_parser_text` writes to its own `out_offset`.
// Written 0 for empty or invalid spans.
SYNTAQLITE_API const char* syntaqlite_parser_span_text(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    SyntaqliteLength* out_len,
    SyntaqliteStmtOffset* out_offset);

// Authored source text for AST node `node_id` — the analogue of
// `syntaqlite_parser_span_text` for whole nodes rather than spans.
// Offsets are statement-relative, same as `syntaqlite_parser_span_text`.
//
// Requires per-node extent tracking to be enabled via
// `syntaqlite_parser_set_collect_node_extents` before the first
// `reset()`.  Returns a direct slice of the current statement's
// source — no allocation.
//
// Returns NULL (and writes 0 to `*out_len` / `*out_offset`) when
// extent tracking is disabled, the node id is unknown, or no extent
// was recorded for it.
SYNTAQLITE_API const char* syntaqlite_parser_node_text(
    SyntaqliteParser* p,
    uint32_t node_id,
    SyntaqliteLength* out_len,
    SyntaqliteStmtOffset* out_offset);

// Post-expansion text for AST node `node_id` — the analogue of
// `syntaqlite_parser_span_expanded_text` for whole nodes.
//
// For nodes whose tokens all live in a single layer (the common
// case), the return value is a direct slice of that layer's buffer —
// the input source for root-layer nodes, or a macro expansion buffer
// for nodes built entirely from one expansion.
//
// For nodes whose tokens cross layers (e.g. `SELECT id!(42)` where
// `SELECT` lives in the root source and `42` lives in `id`'s
// expansion), the result is materialized into a parser-owned scratch
// buffer by walking the node's root range and inlining each enclosed
// macro call's expansion.  The returned pointer is valid until the
// next call to `syntaqlite_parser_node_expanded_text` on the same
// parser or until the parser advances to the next statement — copy
// the bytes out if you need them to outlive that.
//
// Returns NULL (and writes 0 to `*out_len`) when extent tracking is
// disabled, the node id is unknown, or no extent was recorded for it.
SYNTAQLITE_API const char* syntaqlite_parser_node_expanded_text(
    SyntaqliteParser* p,
    uint32_t node_id,
    SyntaqliteLength* out_len);

// ---------------------------------------------------------------------------
// Macro-expansion queries
// ---------------------------------------------------------------------------

// Returns 1 if `span` was tokenized from the original source (layer 0),
// 0 if it was produced by a macro expansion.  Returns 0 for empty spans.
static inline int syntaqlite_span_is_macro_free(
    const SyntaqliteTextSpan* span) {
  return span->length > 0 && span->_layer_id == 0;
}

// Returns 1 if all tokens of AST node `node_id` live in layer 0 (original
// source), 0 otherwise.  Returns 0 when extent tracking is disabled, the
// node id is unknown, or no extent was recorded.
//
// Requires `syntaqlite_parser_set_collect_node_extents(p, 1)` before the
// first `reset()`.
SYNTAQLITE_API int syntaqlite_node_is_macro_free(SyntaqliteParser* p,
                                                 uint32_t node_id);

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

// ---------------------------------------------------------------------------
// Debugging
// ---------------------------------------------------------------------------

// Dump an AST node tree as indented text. Returns a malloc'd NUL-terminated
// string. The caller must free() the result. Returns NULL on allocation
// failure.
SYNTAQLITE_API char* syntaqlite_dump_node(SyntaqliteParser* p,
                                          uint32_t node_id,
                                          uint32_t indent);

// ---------------------------------------------------------------------------
// Advanced: custom dialects
// ---------------------------------------------------------------------------

// Allocate a parser bound to a specific dialect environment.  Use this
// for custom dialects; for the built-in SQLite dialect prefer
// `syntaqlite_parser_create`.
SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create_with_dialect(
    const SyntaqliteMemMethods* mem,
    SyntaqliteDialect env);

#ifndef SYNTAQLITE_OMIT_SQLITE_API
// Return the built-in SQLite dialect handle.
SYNTAQLITE_API SyntaqliteDialect syntaqlite_sqlite_dialect(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_PARSER_H
