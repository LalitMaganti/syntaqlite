// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Incremental (token-feeding) parser API.
//
// An alternative to syntaqlite_parser_next() for embedders that perform their
// own tokenization — for example, to support macro expansion before parsing.
// Feed tokens one at a time after calling syntaqlite_parser_reset(); the
// parser signals statement boundaries as tokens arrive.
//
// When feed_token or finish returns SYNTAQLITE_PARSE_OK, read the successful
// tree via syntaqlite_result_root(). When the status is
// SYNTAQLITE_PARSE_ERROR, read diagnostics via syntaqlite_result_error_*()
// and inspect syntaqlite_result_recovery_root() for an optional partial tree
// (which may include grammar-level error nodes).
// The result is valid until the next feed_token, finish, reset, or destroy.
//
// Usage:
//   SyntaqliteParser* p = syntaqlite_parser_create(NULL);
//   // Optional: enable if you need result_tokens/result_comments.
//   syntaqlite_parser_set_collect_tokens(p, 1);
//   syntaqlite_parser_reset(p, source, len);
//   while (has_more_tokens) {
//     int32_t rc = syntaqlite_parser_feed_token(p, type, text, tlen);
//     switch (rc) {
//       case SYNTAQLITE_PARSE_DONE:
//         break;
//       case SYNTAQLITE_PARSE_OK: {
//         uint32_t root = syntaqlite_result_root(p);
//         // read nodes ...
//         break;
//       }
//       case SYNTAQLITE_PARSE_ERROR:
//         if (syntaqlite_result_recovery_root(p) == SYNTAQLITE_NULL_NODE)
//           goto done;
//         break;
//     }
//   }
//   int32_t rc = syntaqlite_parser_finish(p);
//   if (rc == SYNTAQLITE_PARSE_OK) { /* final statement complete */ }
// done:
//   syntaqlite_parser_destroy(p);
//
// Read accumulated rewrites via syntaqlite_result_rewrite_count() /
// syntaqlite_result_rewrite_at() after parsing.

#ifndef SYNTAQLITE_INCREMENTAL_PARSER_H
#define SYNTAQLITE_INCREMENTAL_PARSER_H

#include "syntaqlite/parser.h"
#include "syntaqlite/tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Token feeding
// ---------------------------------------------------------------------------

// Feed a single token. TK_SPACE is silently skipped. TK_COMMENT is recorded
// as a comment only when collect_tokens is enabled, and is not fed to parser.
//
// Returns a SYNTAQLITE_PARSE_* code:
//   DONE      = keep going (statement not yet complete)
//   OK        = statement completed cleanly
//   ERROR     = statement has parse/runtime error (may still have recovery
//   root)
SYNTAQLITE_API int32_t syntaqlite_parser_feed_token(SyntaqliteParser* p,
                                                    uint32_t token_type,
                                                    const char* text,
                                                    SyntaqliteLength len);

// Signal end-of-input. Synthesizes a SEMI if needed and sends EOF to the
// parser. Returns a SYNTAQLITE_PARSE_* code.
SYNTAQLITE_API int32_t syntaqlite_parser_finish(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Completion / lookahead
// ---------------------------------------------------------------------------

// Enumerate terminal tokens that are valid next lookaheads at the parser's
// current state. Returns the total number of expected tokens.
// If out_tokens is non-NULL, up to out_cap token IDs are written.
SYNTAQLITE_API uint32_t syntaqlite_parser_expected_tokens(SyntaqliteParser* p,
                                                          uint32_t* out_tokens,
                                                          uint32_t out_cap);

// Return the semantic completion context at the parser's current state.
// One of SYNTAQLITE_COMPLETION_CONTEXT_*.
SYNTAQLITE_API SyntaqliteCompletionContext
syntaqlite_parser_completion_context(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Macro lookup callback
// ---------------------------------------------------------------------------

// Return codes for SyntaqliteMacroLookupFn callbacks. Stable ABI.
//   OK        = macro found and expansion set via set_result*
//   NOT_FOUND = no macro with that name is registered
//   ERROR     = macro was found but expansion failed
#define SYNTAQLITE_MACRO_LOOKUP_OK 0
#define SYNTAQLITE_MACRO_LOOKUP_NOT_FOUND (-1)
#define SYNTAQLITE_MACRO_LOOKUP_ERROR (-2)

// Returns SYNTAQLITE_MACRO_LOOKUP_OK on success (the callback must call
// set_result first), SYNTAQLITE_MACRO_LOOKUP_NOT_FOUND if the macro does
// not exist, SYNTAQLITE_MACRO_LOOKUP_ERROR on expansion error.
typedef int (*SyntaqliteMacroLookupFn)(void* user_data,
                                       SyntaqliteParser* parser,
                                       const char* name,
                                       SyntaqliteLength name_len,
                                       const SyntaqliteToken* args,
                                       uint32_t arg_count);

// Returns SYNTAQLITE_OK on success, SYNTAQLITE_ERR_OMITTED if macros are
// compiled out (SYNTAQLITE_OMIT_MACROS).
SYNTAQLITE_API int32_t
syntaqlite_parser_set_macro_lookup(SyntaqliteParser* p,
                                   SyntaqliteMacroLookupFn fn,
                                   void* user_data);

// ---------------------------------------------------------------------------
// Node expansion
// ---------------------------------------------------------------------------
//
// Macros replace text before it's parsed. Node expansion works the other way
// around: some dialect syntax is parsed as normal, then the host compiles it
// into SQL. The grammar marks which nodes this applies to and, once their
// statement is parsed, the host's expander replaces each of them. The host
// sees the whole statement while it does, such as the CTEs a node can read.
//
// The replacement is recorded as a rewrite (see
// `syntaqlite_result_rewrite_at`), just like a macro call, so it also works
// inside macro bodies and arguments. The replacement is never parsed: the AST
// keeps the original node.

// Return codes for SyntaqliteNodeExpandFn callbacks.
#define SYNTAQLITE_NODE_EXPAND_OK 0
#define SYNTAQLITE_NODE_EXPAND_ERROR (-1)

// Called for each node the grammar marked, once the statement is parsed, so
// the whole statement can be read; a node inside another is expanded first.
// It should call `syntaqlite_node_expansion_set_result` and return
// SYNTAQLITE_NODE_EXPAND_OK, or return SYNTAQLITE_NODE_EXPAND_ERROR to fail the
// statement.
typedef int (*SyntaqliteNodeExpandFn)(void* user_data,
                                      SyntaqliteParser* parser,
                                      uint32_t node_id);

// Sets the callback which expands nodes the grammar marks. Needs node extents
// (`syntaqlite_parser_set_collect_node_extents`), which place each node:
// without them every expansion fails its statement. Returns SYNTAQLITE_OK on
// success, or SYNTAQLITE_ERR_OMITTED if macros are compiled out, since node
// expansion needs the same rewrite machinery.
SYNTAQLITE_API int32_t
syntaqlite_parser_set_node_expander(SyntaqliteParser* p,
                                    SyntaqliteNodeExpandFn fn,
                                    void* user_data);

// Sets the text to replace the current node with. Only call this from inside
// the expander callback.
SYNTAQLITE_API void syntaqlite_node_expansion_set_result(SyntaqliteParser* p,
                                                         const char* text,
                                                         SyntaqliteLength len);

// ---------------------------------------------------------------------------
// Macro expansion result (called from inside the lookup callback)
// ---------------------------------------------------------------------------

// Set the expanded body for the current macro invocation.
// `def_line` / `def_col` are 1-based definition position for tracebacks
// (pass 0/0 if unknown).
SYNTAQLITE_API void syntaqlite_macro_expansion_set_result(
    SyntaqliteParser* p,
    const char* body,
    SyntaqliteLength body_len,
    SyntaqliteLineNumber def_line,
    SyntaqliteColumnNumber def_col);

// Describes where one macro argument was pasted into the expansion body.
// Used by set_result_with_arg_map to enable span drilling through
// $param substitutions.
typedef struct SyntaqliteArgMapping {
  SyntaqliteLayerOffset body_offset;  // Byte offset in `body` where arg
                                      // text starts.
  uint32_t arg_index;  // Index into the args array passed to the callback.
} SyntaqliteArgMapping;

// Like set_result, but with arg-mapping metadata for span drilling.
// Each mapping says "at body_offset, I pasted arg[arg_index]".
// Enables span_text to drill through substitutions to the caller's
// authored arg text instead of collapsing to the whole call site.
SYNTAQLITE_API void syntaqlite_macro_expansion_set_result_with_arg_map(
    SyntaqliteParser* p,
    const char* body,
    SyntaqliteLength body_len,
    SyntaqliteLineNumber def_line,
    SyntaqliteColumnNumber def_col,
    const SyntaqliteArgMapping* mappings,
    uint32_t mapping_count);

// Template expansion helper: substitute `$param` placeholders with the
// current invocation's args and call set_result.  Arg segments are built
// automatically.  Returns SYNTAQLITE_OK on success.
//
// `flags` is a bitwise OR of SYNTAQLITE_EXPAND_* constants (0 for defaults).
//
// By default, encountering a `$param` that does not match any entry in
// `param_names` is an error (returns -1).  Pass
// SYNTAQLITE_EXPAND_PASSTHROUGH_UNKNOWN to copy unknown `$param` tokens
// verbatim into the expansion buffer instead.
#define SYNTAQLITE_EXPAND_PASSTHROUGH_UNKNOWN 0x1u

SYNTAQLITE_API int syntaqlite_macro_expansion_expand_and_set_result(
    SyntaqliteParser* p,
    const char* body,
    SyntaqliteLength body_len,
    const char* const* param_names,
    const SyntaqliteLength* param_name_lens,
    uint32_t param_count,
    uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_INCREMENTAL_PARSER_H
