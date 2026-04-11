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
// For macro region tracking, bracket expanded tokens with
// syntaqlite_parser_begin_macro() / syntaqlite_parser_end_macro(). Read
// accumulated regions via syntaqlite_result_macro_count() /
// syntaqlite_result_macro_at() after parsing.

#ifndef SYNTAQLITE_INCREMENTAL_PARSER_H
#define SYNTAQLITE_INCREMENTAL_PARSER_H

#include "syntaqlite/parser.h"

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
                                                    uint32_t len);

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
// Macro region tracking
// ---------------------------------------------------------------------------

// Mark subsequent fed tokens as being inside a macro expansion.
// call_offset/call_length describe the macro call's byte range in the
// original source. Calls may nest (for nested macro expansions).
SYNTAQLITE_API void syntaqlite_parser_begin_macro(SyntaqliteParser* p,
                                                  uint32_t call_offset,
                                                  uint32_t call_length);

// End the innermost macro expansion region.
SYNTAQLITE_API void syntaqlite_parser_end_macro(SyntaqliteParser* p);

// ---------------------------------------------------------------------------
// Macro registration
// ---------------------------------------------------------------------------

// Register a template macro.  Copies all strings.
// The macro body uses $param placeholders (e.g. "$x + 1").
//
// `def_line` / `def_col` are the 1-based line/column of the defining
// statement (e.g. `CREATE PERFETTO MACRO foo`).  They are stored on the
// registry entry and surfaced by expansion tracebacks so error frames for
// macro bodies can reference the authoring position.  Pass 0/0 if unknown.
//
// Returns 0 on success.
SYNTAQLITE_API int syntaqlite_parser_register_macro(
    SyntaqliteParser* p,
    const char* name,
    uint32_t name_len,
    const char* const* param_names,
    uint32_t param_count,
    const char* body,
    uint32_t body_len,
    uint32_t def_line,
    uint32_t def_col);

// Deregister a macro by name.  Returns 0 on success, -1 if not found.
SYNTAQLITE_API int syntaqlite_parser_deregister_macro(SyntaqliteParser* p,
                                                      const char* name,
                                                      uint32_t name_len);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_INCREMENTAL_PARSER_H
