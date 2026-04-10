// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Core parser: lifecycle, main parse loop, result accessors, incremental
// token-feeding API, configuration, arena accessors.
//
// Macro registry, expansion, and span resolution live in parser_macros.c.
// AST dump lives in parser_dump.c.  Cross-file helpers are declared in
// csrc/parser_internal.h.

#include <stdio.h>
#include <string.h>

#include "csrc/dialect_dispatch.h"
#include "csrc/hashmap.h"
#include "csrc/token_wrapped.h"
#include "csrc/tokens.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite/parser.h"
#include "syntaqlite_dialect/ast_builder.h"

#include "csrc/parser_internal.h"

// ---------------------------------------------------------------------------
// Forward declarations of file-local helpers
// ---------------------------------------------------------------------------

static void reset_stmt(SyntaqliteParser* p);
static int32_t stmt_boundary(SyntaqliteParser* p);
static int finish_input(SyntaqliteParser* p);

int32_t synq_parser_set_result_status(SyntaqliteParser* p, int32_t rc) {
  p->last_status = rc;
  return rc;
}

// Local short-hand.
#define set_result_status synq_parser_set_result_status

// ---------------------------------------------------------------------------
// Internal: reusable state-reset helpers
// ---------------------------------------------------------------------------

// Reinitialize the Lemon parser automaton to its initial state.
// Called after real-statement completion (cmdx ::= cmd . reduces with SEMI
// as the LALR(1) lookahead, leaving SEMI shifted but ecmd ::= cmdx SEMI .
// pending).  Reinitializing discards that half-reduced state.
// NOT called for bare semicolons or error-recovery completions — those
// reduce via ecmd ::= SEMI . or ecmd ::= error SEMI . using the *next*
// token as the lookahead, so that token is already consumed by Lemon.
static void lemon_reinit(SyntaqliteParser* p) {
  SYNQ_PARSER_FINALIZE(p->dialect.tmpl, p->lemon);
  SYNQ_PARSER_INIT(p->dialect.tmpl, p->lemon, &p->ctx);
  p->last_token_type = 0;
}

// Reset all per-statement output state: arena, token/comment/macro vectors,
// context flags, and error state.  Called at the *start* of the next
// statement (not at completion) so that callers can read the previous
// statement's results between calls.
static void reset_stmt(SyntaqliteParser* p) {
  synq_parse_ctx_clear(&p->ctx);
  syntaqlite_vec_clear(&p->comments);
  syntaqlite_vec_clear(&p->tokens);
  // Free owned expansion buffers from previous statement.
  // Skip index 0 (sentinel) — its expansion_data points to source, not
  // malloc'd.
  for (uint32_t i = 1; i < syntaqlite_vec_len(&p->macro_regions); i++) {
    if (p->macro_regions.data[i].expansion_data)
      p->mem.xFree((void*)p->macro_regions.data[i].expansion_data);
  }
  syntaqlite_vec_clear(&p->macro_expansions);
  syntaqlite_vec_clear(&p->macro_regions);
  // Push sentinel at index 0 (source buffer).  p->source may be NULL on
  // the very first reset_stmt call (before syntaqlite_parser_reset sets it),
  // which is fine — syntaqlite_parser_reset re-clears and re-pushes.
  if (p->source) {
    SyntaqliteMacroRegion pub_sentinel = {0, 0};
    syntaqlite_vec_push(&p->macro_expansions, pub_sentinel, p->mem);
    SynqMacroRegion sentinel = {p->source, p->source_len, 0};
    syntaqlite_vec_push(&p->macro_regions, sentinel, p->mem);
  }
  p->expansion_depth = 0;
  p->ctx.buf_idx = 0;
  p->ctx.root = SYNTAQLITE_NULL_NODE;
  p->ctx.stmt_completed = 0;
  p->ctx.pending_explain_mode = 0;
  p->ctx.error = 0;
  p->ctx.saw_subquery = 0;
  p->ctx.saw_update_delete_limit = 0;
  p->had_comment = 0;
  p->had_error = 0;
  p->error_msg[0] = '\0';
  p->ctx.error_offset = 0xFFFFFFFF;
  p->ctx.error_length = 0;
  p->ctx.tokens = p->collect_tokens ? &p->tokens : NULL;
}

// Handle a statement boundary after feed_one_token returns 1.
// Reinitializes Lemon and classifies the completed statement:
//   SYNTAQLITE_PARSE_OK    — successful statement (root is set)
//   SYNTAQLITE_PARSE_ERROR — statement with syntax error(s)
//   SYNTAQLITE_PARSE_DONE  — bare semicolon (no statement produced)
static int32_t stmt_boundary(SyntaqliteParser* p) {
  lemon_reinit(p);

  // Bare semicolon — no statement produced.
  if (p->ctx.root == SYNTAQLITE_NULL_NODE && !p->had_error)
    return SYNTAQLITE_PARSE_DONE;

  if (synq_parser_check_macro_straddle(p) < 0)
    return SYNTAQLITE_PARSE_ERROR;

  if (p->had_error) {
    p->had_error = 0;  // consumed for this result
    return SYNTAQLITE_PARSE_ERROR;
  }
  return SYNTAQLITE_PARSE_OK;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create_with_dialect(
    const SyntaqliteMemMethods* mem,
    const SyntaqliteDialect dialect) {
  SyntaqliteMemMethods m = mem ? *mem : SYNTAQLITE_MEM_METHODS_DEFAULT;
  SyntaqliteParser* p = m.xMalloc(sizeof(SyntaqliteParser));
  memset(p, 0, sizeof(*p));
  p->mem = m;
  p->dialect = dialect;
  p->lemon = SYNQ_PARSER_ALLOC(dialect.tmpl, m.xMalloc, &p->ctx);
  synq_parse_ctx_init(&p->ctx, m);
  syntaqlite_vec_init(&p->comments);
  syntaqlite_vec_init(&p->tokens);
  syntaqlite_vec_init(&p->macro_expansions);
  syntaqlite_vec_init(&p->macro_regions);
  // macro_table, expansion state already zeroed by memset
  return p;
}

#ifndef SYNTAQLITE_OMIT_SQLITE_API
SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create(
    const SyntaqliteMemMethods* mem) {
  SyntaqliteDialect dialect = syntaqlite_sqlite_dialect();
  return syntaqlite_parser_create_with_dialect(mem, dialect);
}
#endif

SYNTAQLITE_API void syntaqlite_parser_reset(SyntaqliteParser* p,
                                            const char* source,
                                            uint32_t len) {
  // Seal the parser on first use — configuration is frozen after this.
  p->sealed = 1;

  lemon_reinit(p);
  reset_stmt(p);

  p->source = source;
  p->source_len = len;
  p->offset = 0;
  p->finished = 0;
  p->pending_reset = 0;
  p->last_status = SYNTAQLITE_PARSE_DONE;
  p->macro_depth = 0;

  // Re-push sentinels with the correct source pointer (reset_stmt may have
  // pushed ones with the old source, or none if source was NULL).
  syntaqlite_vec_clear(&p->macro_expansions);
  syntaqlite_vec_clear(&p->macro_regions);
  {
    SyntaqliteMacroRegion pub_sentinel = {0, 0};
    syntaqlite_vec_push(&p->macro_expansions, pub_sentinel, p->mem);
    SynqMacroRegion sentinel = {source, len, 0};
    syntaqlite_vec_push(&p->macro_regions, sentinel, p->mem);
  }

  p->ctx.source = source;
  p->ctx.env = &p->dialect;
}

SYNTAQLITE_API void syntaqlite_parser_destroy(SyntaqliteParser* p) {
  if (p) {
    SYNQ_PARSER_FREE(p->dialect.tmpl, p->lemon, p->mem.xFree);
    synq_parse_ctx_free(&p->ctx);
    syntaqlite_vec_free(&p->comments, p->mem);
    syntaqlite_vec_free(&p->tokens, p->mem);
    // Free owned expansion buffers and macro vectors.
    // Skip index 0 (sentinel) — its expansion_data points to source, not
    // malloc'd.
    for (uint32_t i = 1; i < syntaqlite_vec_len(&p->macro_regions); i++) {
      if (p->macro_regions.data[i].expansion_data)
        p->mem.xFree((void*)p->macro_regions.data[i].expansion_data);
    }
    syntaqlite_vec_free(&p->macro_expansions, p->mem);
    syntaqlite_vec_free(&p->macro_regions, p->mem);
    // Free macro registry.
    if (p->macro_table) {
      for (uint32_t i = 0; i < p->macro_table_size; i++) {
        if (p->macro_table[i].state == SYNQ_MAP_LIVE)
          synq_parser_free_macro_entry(p, &p->macro_table[i]);
      }
      p->mem.xFree(p->macro_table);
    }
    p->mem.xFree(p);
  }
}

// ---------------------------------------------------------------------------
// Internal: feed one real token to Lemon.
// Returns: 0 = keep going, 1 = statement completed, -1 = unrecoverable error.
// ---------------------------------------------------------------------------

int synq_parser_feed_one_token(SyntaqliteParser* p,
                               uint32_t token_type,
                               const char* text,
                               uint32_t len,
                               uint32_t token_idx) {
  uint32_t tok_offset = 0;
  if (text != NULL) {
    // Compute offset relative to the current buffer (which may be the
    // original source or a macro expansion buffer during expand_and_feed).
    tok_offset = (uint32_t)(text - p->ctx.source);
  }
  SynqParseToken minor = {
      .z = text,
      .n = len,
      .type = token_type,
      .token_idx = token_idx,
      .offset = tok_offset,
      .buf_idx = (uint8_t)p->ctx.buf_idx,
      ._pad = {0, 0, 0},
  };
  SYNQ_PARSER_FEED(p->dialect.tmpl, p->lemon, (int)token_type, minor);
  p->last_token_type = token_type;

  if (p->ctx.error) {
    p->had_error = 1;
    if (p->error_msg[0] == '\0') {
      if (text) {
        p->ctx.error_offset = (uint32_t)(text - p->source);
        p->ctx.error_length = (uint32_t)len;
        snprintf(p->error_msg, sizeof(p->error_msg), "syntax error near '%.*s'",
                 len, text);
      } else {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "incomplete SQL statement");
      }
    }
    p->ctx.error = 0;  // Lemon is now driving recovery.
    return 0;
  }

  if (p->ctx.stmt_completed) {
    p->ctx.stmt_completed = 0;
    return 1;
  }

  return 0;
}

// Local shorthand for the cross-file helper.
#define feed_one_token synq_parser_feed_one_token

// ---------------------------------------------------------------------------
// Internal: synthesize SEMI + EOF to finish parsing.
// Returns a SYNTAQLITE_PARSE_* code.
// ---------------------------------------------------------------------------

static int finish_input(SyntaqliteParser* p) {
  // No real tokens were fed (only whitespace/comments).
  if (p->last_token_type == 0) {
    p->finished = 1;
    // If comments were seen, return PARSE_OK (root will be NULL_NODE).
    // This matches SQLite's sqlite3_prepare_v2 which returns SQLITE_OK
    // for comment-only input.
    if (p->had_comment) {
      return set_result_status(p, SYNTAQLITE_PARSE_OK);
    }
    return set_result_status(p, SYNTAQLITE_PARSE_DONE);
  }

  // Synthesize SEMI if the last token wasn't one.
  if (p->last_token_type != SYNTAQLITE_TK_SEMI) {
    int rc = feed_one_token(p, SYNTAQLITE_TK_SEMI, NULL, 0, 0xFFFFFFFF);
    if (rc == 1) {
      int32_t status = stmt_boundary(p);
      if (status != SYNTAQLITE_PARSE_DONE) {
        p->finished = 1;
        return set_result_status(p, status);
      }
      // bare semicolon — fall through to EOF
    }
  }

  // Send end-of-input (EOF) to flush the final reduction.
  SynqParseToken eof = {.z = NULL,
                        .n = 0,
                        .type = 0,
                        .token_idx = 0xFFFFFFFF,
                        .offset = 0,
                        .buf_idx = 0,
                        ._pad = {0, 0, 0}};
  SYNQ_PARSER_FEED(p->dialect.tmpl, p->lemon, 0, eof);
  p->finished = 1;

  if (p->ctx.error) {
    p->had_error = 1;
    if (p->ctx.error_offset == 0xFFFFFFFF) {
      p->ctx.error_offset = p->offset;
    }
    if (p->error_msg[0] == '\0') {
      snprintf(p->error_msg, sizeof(p->error_msg), "incomplete SQL statement");
    }
    return set_result_status(p, SYNTAQLITE_PARSE_ERROR);
  }

  if (p->ctx.root != SYNTAQLITE_NULL_NODE) {
    if (synq_parser_check_macro_straddle(p) < 0)
      return set_result_status(p, SYNTAQLITE_PARSE_ERROR);
    return set_result_status(
        p, p->had_error ? SYNTAQLITE_PARSE_ERROR : SYNTAQLITE_PARSE_OK);
  }

  if (p->had_error)
    return set_result_status(p, SYNTAQLITE_PARSE_ERROR);

  return set_result_status(p, SYNTAQLITE_PARSE_DONE);
}

// ---------------------------------------------------------------------------
// Internal: token recording and feeding
// ---------------------------------------------------------------------------

// Record a token and feed it to Lemon.  Returns 1 if a real statement
// boundary was reached (caller should return stmt_boundary()), 0 otherwise.
int synq_parser_record_and_feed(SyntaqliteParser* p,
                                uint32_t cur_type,
                                uint32_t cur_offset,
                                uint32_t cur_len) {
  uint32_t tidx = 0xFFFFFFFF;
  if (p->collect_tokens) {
    SyntaqliteParserToken tp = {cur_offset, cur_len, cur_type, 0};
    syntaqlite_vec_push(&p->tokens, tp, p->mem);
    tidx = syntaqlite_vec_len(&p->tokens) - 1;
  }
  int rc = feed_one_token(p, cur_type, p->source + cur_offset, cur_len, tidx);
  // After parse_failure, Lemon stops reducing — force a boundary on SEMI
  // so errors don't bleed into subsequent statements.
  if (p->had_error && rc == 0 && cur_type == SYNTAQLITE_TK_SEMI)
    rc = 1;
  if (rc == 1 && (p->ctx.root != SYNTAQLITE_NULL_NODE || p->had_error))
    return 1;
  return 0;
}

// Record a comment token (outlined from the hot loop).
SYNQ_NOINLINE
static void record_comment(SyntaqliteParser* p, uint32_t offset, uint32_t len) {
  const unsigned char* z = (const unsigned char*)p->source;
  SyntaqliteComment t = {offset, len,
                         z[offset] == '-' ? (uint8_t)0 : (uint8_t)1};
  syntaqlite_vec_push(&p->comments, t, p->mem);
}

// ---------------------------------------------------------------------------
// High-level API
// ---------------------------------------------------------------------------

// Tokenize the next non-whitespace token, recording any comments along the
// way.  Returns the token length (0 at end-of-input).  `*out_offset` and
// `*out_type` are set to the position and type of the returned token.
static int64_t next_token(SyntaqliteParser* p,
                          const unsigned char* z,
                          uint32_t pos,
                          uint32_t* out_offset,
                          uint32_t* out_type) {
  while (pos < p->source_len && z[pos] != '\0') {
    uint32_t type = 0;
    int64_t len = SynqSqliteGetTokenVersionWrapped(
        &p->dialect, p->macro_fallback, z + pos, &type);
    if (len <= 0)
      return 0;
    if (type == SYNTAQLITE_TK_SPACE) {
      pos += (uint32_t)len;
      continue;
    }
    *out_offset = pos;
    *out_type = type;
    return len;
  }
  *out_offset = pos;
  *out_type = 0;
  return 0;
}

SYNTAQLITE_API int32_t syntaqlite_parser_next(SyntaqliteParser* p) {
  reset_stmt(p);

  if (p->finished)
    return set_result_status(p, SYNTAQLITE_PARSE_DONE);

  const unsigned char* z = (const unsigned char*)p->source;

  // 1-token lookahead: tokenize the first token before entering the loop.
  uint32_t cur_type = 0;
  uint32_t cur_offset = 0;
  int64_t cur_len = next_token(p, z, p->offset, &cur_offset, &cur_type);

  while (cur_len > 0) {
    // Handle comments: record and advance without feeding to Lemon.
    // This keeps comment recording in the main loop so that lookahead
    // never eagerly consumes comments belonging to the next statement.
    if (cur_type == SYNTAQLITE_TK_COMMENT) {
      p->had_comment = 1;
      if (p->collect_tokens)
        record_comment(p, cur_offset, (uint32_t)cur_len);
      cur_len = next_token(p, z, cur_offset + (uint32_t)cur_len, &cur_offset,
                           &cur_type);
      continue;
    }

    p->offset = cur_offset + (uint32_t)cur_len;

    // Tokenize the lookahead — always one token ahead.
    uint32_t la_offset = 0;
    uint32_t la_type = 0;
    int64_t la_len = next_token(p, z, p->offset, &la_offset, &la_type);

    // Macro detection: ID followed by TK_BANG ('!').  The token wrapper
    // produces TK_BANG for any dialect that may have macro calls (Rust-style
    // dialects or any dialect with macro_fallback enabled).
    if (cur_type == SYNTAQLITE_TK_ID && la_type == SYNTAQLITE_TK_BANG) {
      int mrc = synq_parser_try_macro_call(p, cur_offset, (uint32_t)cur_len,
                                           la_offset);
      if (mrc == 1)
        return set_result_status(p, stmt_boundary(p));
      if (mrc == 0) {
        // Macro consumed tokens past the lookahead — re-tokenize.
        cur_len = next_token(p, z, p->offset, &cur_offset, &cur_type);
        continue;
      }
    }

    // Normal token (or macro fallthrough): record + feed to Lemon.
    if (synq_parser_record_and_feed(p, cur_type, cur_offset, (uint32_t)cur_len))
      return set_result_status(p, stmt_boundary(p));

    // Shift: lookahead becomes current.
    cur_type = la_type;
    cur_offset = la_offset;
    cur_len = la_len;
  }

  // End of input.
  return finish_input(p);
}

// ---------------------------------------------------------------------------
// Result accessors
// ---------------------------------------------------------------------------

SYNTAQLITE_API uint32_t syntaqlite_result_root(SyntaqliteParser* p) {
  if (p->last_status != SYNTAQLITE_PARSE_OK) {
    return SYNTAQLITE_NULL_NODE;
  }
  return p->ctx.root;
}

SYNTAQLITE_API uint32_t syntaqlite_result_recovery_root(SyntaqliteParser* p) {
  if (p->last_status != SYNTAQLITE_PARSE_ERROR) {
    return SYNTAQLITE_NULL_NODE;
  }
  return p->ctx.root;
}

SYNTAQLITE_API const char* syntaqlite_result_error_msg(SyntaqliteParser* p) {
  return p->error_msg[0] ? p->error_msg : NULL;
}

SYNTAQLITE_API uint32_t syntaqlite_result_error_offset(SyntaqliteParser* p) {
  return p->ctx.error_offset;
}

SYNTAQLITE_API uint32_t syntaqlite_result_error_length(SyntaqliteParser* p) {
  return p->ctx.error_length;
}

SYNTAQLITE_API const SyntaqliteComment* syntaqlite_result_comments(
    SyntaqliteParser* p,
    uint32_t* count) {
  *count = syntaqlite_vec_len(&p->comments);
  return p->comments.data;
}

SYNTAQLITE_API const SyntaqliteParserToken* syntaqlite_result_tokens(
    SyntaqliteParser* p,
    uint32_t* count) {
  *count = syntaqlite_vec_len(&p->tokens);
  return p->tokens.data;
}

SYNTAQLITE_API const SyntaqliteMacroRegion* syntaqlite_result_macros(
    SyntaqliteParser* p,
    uint32_t* count) {
  uint32_t total = syntaqlite_vec_len(&p->macro_expansions);
  // Skip sentinel at index 0.
  if (total <= 1) {
    *count = 0;
    return NULL;
  }
  *count = total - 1;
  return p->macro_expansions.data + 1;
}

// ---------------------------------------------------------------------------
// Arena accessors
// ---------------------------------------------------------------------------

SYNTAQLITE_API const void* syntaqlite_parser_node(SyntaqliteParser* p,
                                                  uint32_t node_id) {
  return (const void*)synq_arena_ptr(&p->ctx.ast, node_id);
}

SYNTAQLITE_API uint32_t syntaqlite_parser_node_count(SyntaqliteParser* p) {
  return syntaqlite_vec_len(&p->ctx.ast.offsets);
}

SYNTAQLITE_API const char* syntaqlite_parser_source(SyntaqliteParser* p) {
  return p->source;
}

SYNTAQLITE_API uint32_t syntaqlite_parser_source_length(SyntaqliteParser* p) {
  return p->source_len;
}

// ---------------------------------------------------------------------------
// Low-level token-feeding API
// ---------------------------------------------------------------------------

SYNTAQLITE_API int32_t syntaqlite_parser_feed_token(SyntaqliteParser* p,
                                                    uint32_t token_type,
                                                    const char* text,
                                                    uint32_t len) {
  // Deferred reset: clear previous statement's data before processing the
  // first token of the next one.  Lemon was already reinitialized eagerly
  // by stmt_boundary() when the previous statement completed.
  if (p->pending_reset) {
    reset_stmt(p);
    p->pending_reset = 0;
  }

  // Skip whitespace silently.
  if (token_type == SYNTAQLITE_TK_SPACE) {
    return set_result_status(p, SYNTAQLITE_PARSE_DONE);
  }

  // Record comments but don't feed to Lemon.
  if (token_type == SYNTAQLITE_TK_COMMENT) {
    if (p->collect_tokens && text) {
      uint32_t tok_offset = (uint32_t)(text - p->source);
      SyntaqliteComment t = {tok_offset, len,
                             (uint8_t)(text[0] == '-' ? 0 : 1)};
      syntaqlite_vec_push(&p->comments, t, p->mem);
    }
    return set_result_status(p, SYNTAQLITE_PARSE_DONE);
  }

  // Capture non-whitespace, non-comment token positions.
  uint32_t tidx = 0xFFFFFFFF;
  if (p->collect_tokens && text) {
    uint32_t tok_offset = (uint32_t)(text - p->source);
    SyntaqliteParserToken tp = {tok_offset, len, token_type, 0};
    syntaqlite_vec_push(&p->tokens, tp, p->mem);
    tidx = syntaqlite_vec_len(&p->tokens) - 1;
  }

  int rc = feed_one_token(p, token_type, text, len, tidx);
  if (rc < 0)
    return set_result_status(p, SYNTAQLITE_PARSE_ERROR);

  if (rc == 1) {
    // Bare semicolons (ecmd ::= SEMI.) and error-recovery completions
    // (ecmd ::= error SEMI.) have root == NULL_NODE and may have consumed
    // the next token as an LALR(1) lookahead.  Do NOT reinitialize Lemon —
    // the consumed token is already in Lemon's state and will be processed
    // normally on the next feed_token call.
    if (p->ctx.root == SYNTAQLITE_NULL_NODE) {
      if (p->had_error) {
        p->had_error = 0;
        p->pending_reset = 1;
        return set_result_status(p, SYNTAQLITE_PARSE_ERROR);
      }
      return set_result_status(p, SYNTAQLITE_PARSE_DONE);
    }

    // Real statement — cmdx ::= cmd. fired with SEMI as the lookahead,
    // leaving ecmd ::= cmdx SEMI. pending.  Reinitialize Lemon.
    int32_t status = stmt_boundary(p);
    p->pending_reset = 1;
    return set_result_status(p, status);
  }

  return set_result_status(p, SYNTAQLITE_PARSE_DONE);
}

SYNTAQLITE_API uint32_t syntaqlite_parser_expected_tokens(SyntaqliteParser* p,
                                                          uint32_t* out_tokens,
                                                          uint32_t out_cap) {
  if (p == NULL || p->dialect.tmpl == NULL ||
      p->dialect.tmpl->parser_expected_tokens == NULL) {
    return 0;
  }
  return p->dialect.tmpl->parser_expected_tokens(p->lemon, out_tokens, out_cap);
}

SYNTAQLITE_API SyntaqliteCompletionContext
syntaqlite_parser_completion_context(SyntaqliteParser* p) {
  if (p == NULL || p->dialect.tmpl == NULL ||
      p->dialect.tmpl->parser_completion_context == NULL) {
    return SYNTAQLITE_COMPLETION_CONTEXT_UNKNOWN;
  }
  return (SyntaqliteCompletionContext)
      p->dialect.tmpl->parser_completion_context(p->lemon);
}

SYNTAQLITE_API int32_t syntaqlite_parser_finish(SyntaqliteParser* p) {
  if (p->pending_reset) {
    // Nothing pending after a completed statement — done.
    p->pending_reset = 0;
    return set_result_status(p, SYNTAQLITE_PARSE_DONE);
  }
  return finish_input(p);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

SYNTAQLITE_API int32_t syntaqlite_parser_set_trace(SyntaqliteParser* p,
                                                   uint32_t enable) {
  if (p->sealed)
    return -1;
  p->trace = enable;
  if (enable) {
    SYNQ_PARSER_TRACE(p->dialect.tmpl, stderr, "parser> ");
  } else {
    SYNQ_PARSER_TRACE(p->dialect.tmpl, NULL, NULL);
  }
  return 0;
}

SYNTAQLITE_API int32_t syntaqlite_parser_set_collect_tokens(SyntaqliteParser* p,
                                                            uint32_t enable) {
  if (p->sealed)
    return -1;
  p->collect_tokens = enable;
  return 0;
}

SYNTAQLITE_API int32_t syntaqlite_parser_set_macro_fallback(SyntaqliteParser* p,
                                                            uint32_t enable) {
  if (p->sealed)
    return -1;
  p->macro_fallback = enable;
  return 0;
}
