// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Core parser: lifecycle, main parse loop, result accessors, incremental
// token-feeding API, configuration, arena accessors.
//
// Macro expansion lives in parser_macros.c.  Span resolution and
// traceback live in parser_spans.c.  Per-node extent tracking hooks
// live in parser_extents.c.  AST dump lives in parser_dump.c.
// Cross-file helpers are declared in csrc/parser_internal.h.

#include <stdio.h>
#include <string.h>

#include "csrc/dialect_dispatch.h"
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

// Record the first byte consumed for the current statement and sync
// the layer-0 macro sentinel so span walkers resolve layer-0 spans
// against the statement slice.
static void synq_open_statement(SyntaqliteParser* p, uint32_t offset) {
  p->stmt_start_offset = offset;
  p->stmt_source = p->source + offset;
#ifndef SYNTAQLITE_OMIT_MACROS
  if (syntaqlite_vec_len(&p->macro.layers) > 0) {
    SynqExpansionLayer* root = &p->macro.layers.data[0];
    root->expansion_data = p->stmt_source;
    root->expansion_len = p->source_len - offset;
  }
#endif
}

int32_t synq_parser_set_result_status(SyntaqliteParser* p, int32_t rc) {
  p->last_status = rc;
  if (p->stmt_start_offset != UINT32_MAX) {
    p->stmt_end_offset =
        p->offset > p->stmt_start_offset ? p->offset : p->stmt_start_offset;
  }
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
  syntaqlite_vec_clear(&p->comment_idx_leading_start);
  syntaqlite_vec_clear(&p->comment_idx_leading_count);
  syntaqlite_vec_clear(&p->comment_idx_trailing_start);
  syntaqlite_vec_clear(&p->comment_idx_trailing_count);
  p->comment_index_built = 0;
#ifndef SYNTAQLITE_OMIT_MACROS
  syntaqlite_vec_clear(&p->macro.traceback_buf);
  syntaqlite_vec_clear(&p->macro.node_expanded_buf);
  synq_layers_free_owned(&p->macro.layers, p->mem);
  syntaqlite_vec_clear(&p->macro.layers);
  if (p->source)
    synq_layers_push_sentinel(&p->macro.layers, p->source, p->source_len,
                              p->mem);
  p->macro.expansion_depth = 0;
#endif
  p->ctx.layer_id = 0;
  p->ctx.cur_shift_start = 0;
  p->ctx.last_shifted_end = 0;
  p->ctx.root = SYNTAQLITE_NULL_NODE;
  p->ctx.stmt_completed = 0;
  p->ctx.pending_explain_mode = 0;
  p->ctx.error = 0;
  p->ctx.saw_subquery = 0;
  p->ctx.saw_update_delete_limit = 0;
  p->had_comment = 0;
  p->last_layer0_token_end = UINT32_MAX;
  p->had_error = 0;
  p->error_msg[0] = '\0';
  p->ctx.error_offset = 0xFFFFFFFF;
  p->ctx.error_length = 0;
  p->ctx.tokens = p->collect_tokens ? &p->tokens : NULL;
  p->stmt_start_offset = UINT32_MAX;
  p->stmt_end_offset = 0;
  p->stmt_source = p->source;
}

// Handle a statement boundary after shift_token returns 1.
// Reinitializes Lemon and classifies the completed statement:
//   SYNTAQLITE_PARSE_OK    — successful statement (root is set)
//   SYNTAQLITE_PARSE_ERROR — statement with syntax error(s)
//   SYNTAQLITE_PARSE_DONE  — bare semicolon (no statement produced)
static int32_t stmt_boundary(SyntaqliteParser* p) {
  lemon_reinit(p);

  // Bare semicolon — no statement produced.
  if (p->ctx.root == SYNTAQLITE_NULL_NODE && !p->had_error)
    return SYNTAQLITE_PARSE_DONE;

#ifndef SYNTAQLITE_OMIT_MACROS
  if (synq_parser_check_macro_straddle(p) < 0)
    return SYNTAQLITE_PARSE_ERROR;
#endif

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
  syntaqlite_vec_init(&p->comment_idx_leading_start);
  syntaqlite_vec_init(&p->comment_idx_leading_count);
  syntaqlite_vec_init(&p->comment_idx_trailing_start);
  syntaqlite_vec_init(&p->comment_idx_trailing_count);
  p->comment_index_built = 0;
#ifndef SYNTAQLITE_OMIT_MACROS
  synq_macro_state_init(&p->macro);
#endif
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
  p->stmt_start_offset = UINT32_MAX;
  p->stmt_end_offset = 0;
  p->stmt_source = source;
#ifndef SYNTAQLITE_OMIT_MACROS
  p->macro.depth = 0;
  syntaqlite_vec_clear(&p->macro.layers);
  synq_layers_push_sentinel(&p->macro.layers, source, len, p->mem);
#endif

  p->ctx.source = source;
  p->ctx.env = &p->dialect;
}

SYNTAQLITE_API void syntaqlite_parser_destroy(SyntaqliteParser* p) {
  if (p) {
    SYNQ_PARSER_FREE(p->dialect.tmpl, p->lemon, p->mem.xFree);
    synq_parse_ctx_free(&p->ctx);
    syntaqlite_vec_free(&p->comments, p->mem);
    syntaqlite_vec_free(&p->comment_idx_leading_start, p->mem);
    syntaqlite_vec_free(&p->comment_idx_leading_count, p->mem);
    syntaqlite_vec_free(&p->comment_idx_trailing_start, p->mem);
    syntaqlite_vec_free(&p->comment_idx_trailing_count, p->mem);
    syntaqlite_vec_free(&p->tokens, p->mem);
#ifndef SYNTAQLITE_OMIT_MACROS
    synq_macro_state_free(&p->macro, p->mem);
#endif
    p->mem.xFree(p);
  }
}

// ---------------------------------------------------------------------------
// Internal: feed one real token to Lemon.
// Returns: 0 = keep going, 1 = statement completed, -1 = unrecoverable error.
// ---------------------------------------------------------------------------

// Unified token shift: push the token to `p->tokens` (when
// `collect_tokens` is on and `text` is non-null), build the
// `SynqParseToken` with a real `token_idx`, feed Lemon, and maintain
// per-layer parser bookkeeping.  The sole path that terminals take
// into Lemon — called by the main tokenizer loop, by the macro
// expansion loop, by the fallback-macro consolidator, and by the
// low-level incremental feed API.
//
// `text` points to the token bytes in their owning buffer (the source
// for layer 0, an expansion layer's buffer for layer N).  May be NULL
// for synthesized tokens (SEMI/EOF at end-of-input); in that case no
// vec entry is pushed and the shift markers do not advance.
//
// `layer_offset` is the token's offset within its owning buffer:
//   - for layer 0: statement-relative (identical to `text - stmt_source`)
//   - for layer N: position within the expansion buffer
// `p->ctx.layer_id` must already reflect the token's layer.
//
// Returns 1 if Lemon flagged the statement as completed, 0 otherwise.
// Layer-N callers (macro expansion) handle their own error messages
// and clear `p->ctx.error` themselves; for them, this function sets
// `p->had_error` but leaves the error flag intact.
int synq_parser_shift_token(SyntaqliteParser* p,
                            uint32_t token_type,
                            const char* text,
                            uint32_t len,
                            uint32_t layer_offset) {
  uint32_t layer_id = p->ctx.layer_id;
  uint32_t tidx = 0xFFFFFFFF;

  if (p->collect_tokens && text) {
    SyntaqliteParserToken tp = {layer_offset, len, token_type, 0, layer_id};
    syntaqlite_vec_push(&p->tokens, tp, p->mem);
    tidx = syntaqlite_vec_len(&p->tokens) - 1;
    // Drives same-line trailing-comment attachment.  Only advanced for
    // layer-0 tokens so comments emitted from the main tokenizer loop
    // attach relative to the last user-authored token, not relative to
    // some layer-N expansion token.
    if (layer_id == 0) {
      p->last_layer0_token_end = p->stmt_start_offset + layer_offset + len;
    }
  }

  // BEFORE-style empty-rule markers (ast_builder.h:117) are documented
  // as valid for layer-0 shifts only.  Publish cur_shift_start before
  // SYNQ_PARSER_FEED so reductions firing inside the feed observe it.
  if (layer_id == 0) {
    p->ctx.cur_shift_start = layer_offset;
  }

  SynqParseToken minor = {
      .z = text,
      .n = len,
      .type = token_type,
      .token_idx = tidx,
      .offset = layer_offset,
      .layer_id = layer_id,
  };
  SYNQ_PARSER_FEED(p->dialect.tmpl, p->lemon, (int)token_type, minor);
  p->last_token_type = token_type;

  // AFTER-style markers: also layer-0.  Advance after feed so reductions
  // inside feed saw the previous terminal's end.
  if (layer_id == 0) {
    p->ctx.last_shifted_end = layer_offset + len;
  }

  if (p->ctx.error) {
    p->had_error = 1;
    if (layer_id == 0) {
      if (p->error_msg[0] == '\0') {
        if (text) {
          p->ctx.error_offset = layer_offset;
          p->ctx.error_length = len;
          snprintf(p->error_msg, sizeof(p->error_msg),
                   "syntax error near '%.*s'", len, text);
        } else {
          snprintf(p->error_msg, sizeof(p->error_msg),
                   "incomplete SQL statement");
        }
      }
      p->ctx.error = 0;  // Lemon is now driving recovery.
    }
    // Layer-N: leave p->ctx.error set; expand_and_feed builds a
    // macro-specific message and clears the flag.
    return 0;
  }

  if (p->ctx.stmt_completed) {
    p->ctx.stmt_completed = 0;
    return 1;
  }

  return 0;
}

// Local shorthand for the cross-file helper.
#define shift_token synq_parser_shift_token

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
    int rc = shift_token(p, SYNTAQLITE_TK_SEMI, NULL, 0, 0);
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
                        .layer_id = 0};
  SYNQ_PARSER_FEED(p->dialect.tmpl, p->lemon, 0, eof);
  p->finished = 1;

  if (p->ctx.error) {
    p->had_error = 1;
    if (p->ctx.error_offset == 0xFFFFFFFF) {
      p->ctx.error_offset = p->offset - p->stmt_start_offset;
    }
    if (p->error_msg[0] == '\0') {
      snprintf(p->error_msg, sizeof(p->error_msg), "incomplete SQL statement");
    }
    return set_result_status(p, SYNTAQLITE_PARSE_ERROR);
  }

  if (p->ctx.root != SYNTAQLITE_NULL_NODE) {
#ifndef SYNTAQLITE_OMIT_MACROS
    if (synq_parser_check_macro_straddle(p) < 0)
      return set_result_status(p, SYNTAQLITE_PARSE_ERROR);
#endif
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

// Record a comment token (outlined from the hot loop).
//
// Computes attachment from the parser's current state: a comment is
// TRAILING the previous layer-0 token when there is no '\n' in the source
// gap between that token's end and the comment's start; otherwise LEADING
// the next token to be pushed. The predicted owner index for LEADING
// comments equals `vec_len(p->tokens)` — the index the next push will
// land on.
SYNQ_NOINLINE
void synq_parser_record_comment(SyntaqliteParser* p,
                                uint32_t offset,
                                uint32_t len) {
  const unsigned char* z = (const unsigned char*)p->source;

  uint8_t side = SYNQ_COMMENT_LEADING;
  uint32_t owner_idx = syntaqlite_vec_len(&p->tokens);
  uint32_t prev_end = p->last_layer0_token_end;
  if (prev_end != UINT32_MAX && prev_end <= offset &&
      memchr(z + prev_end, '\n', offset - prev_end) == NULL) {
    side = SYNQ_COMMENT_TRAILING;
    owner_idx = syntaqlite_vec_len(&p->tokens) - 1;
  }

  uint32_t layer = p->ctx.layer_id;
  SyntaqliteComment t = {
      .offset = offset - p->stmt_start_offset,
      .length = len,
      .token_idx = owner_idx,
      .kind = z[offset] == '-' ? (uint8_t)0 : (uint8_t)1,
      .side = side,
      .layer_id = (uint8_t)(layer > 0xFF ? 0xFF : layer),
      ._pad = 0,
  };
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
#ifdef SYNTAQLITE_OMIT_MACROS
    int64_t len =
        SynqSqliteGetTokenVersionWrapped(&p->dialect, 0, z + pos, &type);
#else
    int64_t len = SynqSqliteGetTokenVersionWrapped(
        &p->dialect, p->macro.macro_fallback, z + pos, &type);
#endif
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

// ---------------------------------------------------------------------------
// Context-sensitive keyword analysis (mirrors SQLite's analyze*Keyword).
//
// WINDOW, OVER, and FILTER are context-sensitive keywords in SQLite: they act
// as keywords only in specific syntactic positions and are valid identifiers
// everywhere else.  The Lemon grammar cannot handle this via %fallback because
// that would create ambiguity (e.g. `SELECT sum(x) OVER ...` — OVER could be
// a keyword or an alias).
//
// The logic here mirrors the vendored upstream functions in
// sqlite-vendored/sources/fragments/window_keyword_analysis.c.
// ---------------------------------------------------------------------------

// Peek the next non-whitespace, non-comment token via the dialect tokenizer.
// Normalizes identifier-like tokens to TK_ID (mirrors SQLite's getToken
// helper from sqlite-vendored/sources/fragments/window_keyword_analysis.c).
static int synq_peek_token(SyntaqliteParser* p, const unsigned char** pz) {
  const unsigned char* z = *pz;
  int t;
  do {
    int raw = 0;
    z += SYNQ_GET_TOKEN(&p->dialect, z, &raw);
    t = raw;
  } while (t == SYNTAQLITE_TK_SPACE || t == SYNTAQLITE_TK_COMMENT);
  if (t == SYNTAQLITE_TK_ID || t == SYNTAQLITE_TK_STRING ||
      t == SYNTAQLITE_TK_JOIN_KW || t == SYNTAQLITE_TK_WINDOW ||
      t == SYNTAQLITE_TK_OVER ||
      p->dialect.tmpl->parser_fallback(t) == SYNTAQLITE_TK_ID) {
    t = SYNTAQLITE_TK_ID;
  }
  *pz = z;
  return t;
}

// WINDOW → keyword only when followed by <id> AS (a named window def).
static uint32_t synq_analyze_window(SyntaqliteParser* p,
                                    const unsigned char* z) {
  int t = synq_peek_token(p, &z);
  if (t != SYNTAQLITE_TK_ID)
    return SYNTAQLITE_TK_ID;
  t = synq_peek_token(p, &z);
  if (t != SYNTAQLITE_TK_AS)
    return SYNTAQLITE_TK_ID;
  return SYNTAQLITE_TK_WINDOW;
}

// OVER → keyword only when prev was ')' and next is '(' or <id>.
static uint32_t synq_analyze_over(SyntaqliteParser* p,
                                  const unsigned char* z,
                                  uint32_t last_token_type) {
  if (last_token_type == SYNTAQLITE_TK_RP) {
    int t = synq_peek_token(p, &z);
    if (t == SYNTAQLITE_TK_LP || t == SYNTAQLITE_TK_ID)
      return SYNTAQLITE_TK_OVER;
  }
  return SYNTAQLITE_TK_ID;
}

// FILTER → keyword only when prev was ')' and next is '('.
static uint32_t synq_analyze_filter(SyntaqliteParser* p,
                                    const unsigned char* z,
                                    uint32_t last_token_type) {
  if (last_token_type == SYNTAQLITE_TK_RP) {
    int t = synq_peek_token(p, &z);
    if (t == SYNTAQLITE_TK_LP)
      return SYNTAQLITE_TK_FILTER;
  }
  return SYNTAQLITE_TK_ID;
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

  if (cur_len > 0 && p->stmt_start_offset == UINT32_MAX) {
    synq_open_statement(p, cur_offset);
  }

  while (cur_len > 0) {
    // Handle comments: record and advance without feeding to Lemon.
    // This keeps comment recording in the main loop so that lookahead
    // never eagerly consumes comments belonging to the next statement.
    if (cur_type == SYNTAQLITE_TK_COMMENT) {
      p->had_comment = 1;
      if (p->collect_tokens)
        synq_parser_record_comment(p, cur_offset, (uint32_t)cur_len);
      // Advance p->offset so this comment is covered by stmt_end_offset
      // (otherwise text() would not span trailing/comment-only comments).
      p->offset = cur_offset + (uint32_t)cur_len;
      cur_len = next_token(p, z, p->offset, &cur_offset, &cur_type);
      continue;
    }

    p->offset = cur_offset + (uint32_t)cur_len;

    // Context-sensitive keyword reclassification: WINDOW/OVER/FILTER may
    // need to be demoted to TK_ID depending on surrounding tokens.
    if (cur_type == SYNTAQLITE_TK_WINDOW) {
      cur_type = synq_analyze_window(p, z + p->offset);
    } else if (cur_type == SYNTAQLITE_TK_OVER) {
      cur_type = synq_analyze_over(p, z + p->offset, p->last_token_type);
    } else if (cur_type == SYNTAQLITE_TK_FILTER) {
      cur_type = synq_analyze_filter(p, z + p->offset, p->last_token_type);
    }

    // Tokenize the lookahead — always one token ahead.
    uint32_t la_offset = 0;
    uint32_t la_type = 0;
    int64_t la_len = next_token(p, z, p->offset, &la_offset, &la_type);

#ifndef SYNTAQLITE_OMIT_MACROS
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
#endif

    // Normal token (or macro fallthrough): shift into Lemon.
    // After parse_failure Lemon stops reducing — force a boundary on
    // SEMI so errors don't bleed into the next statement.  And filter
    // bare-semicolon rc==1 (no root, no error) so the main loop keeps
    // tokenizing instead of closing an empty statement.
    uint32_t main_layer_offset = cur_offset - p->stmt_start_offset;
    int main_rc = shift_token(p, cur_type, p->source + cur_offset,
                              (uint32_t)cur_len, main_layer_offset);
    if (p->had_error && main_rc == 0 && cur_type == SYNTAQLITE_TK_SEMI)
      main_rc = 1;
    if (main_rc == 1 && (p->ctx.root != SYNTAQLITE_NULL_NODE || p->had_error)) {
      // Eagerly consume same-line trailing comments after the statement
      // terminator so they attach to this statement's last token instead
      // of the next statement's first.  Stop at the first newline or
      // non-skip token; own-line comments belong to the next statement.
      uint32_t scan = p->offset;
      while (scan < p->source_len && z[scan] != '\0') {
        uint32_t tt = 0;
        int64_t tl =
            SynqSqliteGetTokenVersionWrapped(&p->dialect, 0, z + scan, &tt);
        if (tl <= 0)
          break;
        if (tt == SYNTAQLITE_TK_SPACE) {
          if (memchr(z + scan, '\n', (size_t)tl) != NULL)
            break;
          scan += (uint32_t)tl;
          continue;
        }
        if (tt == SYNTAQLITE_TK_COMMENT) {
          if (p->collect_tokens)
            synq_parser_record_comment(p, scan, (uint32_t)tl);
          scan += (uint32_t)tl;
          p->offset = scan;
          continue;
        }
        break;
      }
      return set_result_status(p, stmt_boundary(p));
    }

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

// Build the per-token comment index on first access.  One O(n_comments)
// pass over `p->comments` fills parallel per-token vectors; every
// subsequent `syntaqlite_token_*_comments` / `syntaqlite_node_*_*`
// call is then O(1).  Invalidated by `reset_stmt` between statements.
//
// Slot count is `ntokens + 1` so the orphan "statement-trailing with
// no owner" bucket (`token_idx == ntokens`, `side == LEADING`) is
// indexable alongside regular tokens.
static void build_comment_index(SyntaqliteParser* p) {
  if (p->comment_index_built) {
    return;
  }
  uint32_t ntokens = syntaqlite_vec_len(&p->tokens);
  uint32_t slots = ntokens + 1;
  syntaqlite_vec_clear(&p->comment_idx_leading_start);
  syntaqlite_vec_clear(&p->comment_idx_leading_count);
  syntaqlite_vec_clear(&p->comment_idx_trailing_start);
  syntaqlite_vec_clear(&p->comment_idx_trailing_count);
  for (uint32_t i = 0; i < slots; i++) {
    syntaqlite_vec_push(&p->comment_idx_leading_start, UINT32_MAX, p->mem);
    syntaqlite_vec_push(&p->comment_idx_leading_count, 0, p->mem);
    syntaqlite_vec_push(&p->comment_idx_trailing_start, UINT32_MAX, p->mem);
    syntaqlite_vec_push(&p->comment_idx_trailing_count, 0, p->mem);
  }
  uint32_t total = syntaqlite_vec_len(&p->comments);
  for (uint32_t i = 0; i < total; i++) {
    const SyntaqliteComment* c = &p->comments.data[i];
    if (c->token_idx >= slots) {
      continue;  // defensive: malformed token_idx
    }
    if (c->side == SYNQ_COMMENT_LEADING) {
      uint32_t* start =
          &syntaqlite_vec_at(&p->comment_idx_leading_start, c->token_idx);
      if (*start == UINT32_MAX) {
        *start = i;
      }
      syntaqlite_vec_at(&p->comment_idx_leading_count, c->token_idx)++;
    } else {
      uint32_t* start =
          &syntaqlite_vec_at(&p->comment_idx_trailing_start, c->token_idx);
      if (*start == UINT32_MAX) {
        *start = i;
      }
      syntaqlite_vec_at(&p->comment_idx_trailing_count, c->token_idx)++;
    }
  }
  p->comment_index_built = 1;
}

// O(1) lookup after the first call of the statement (which builds the
// index in one linear pass).
static const SyntaqliteComment* token_side_comments(SyntaqliteParser* p,
                                                    uint32_t token_idx,
                                                    uint8_t side,
                                                    uint32_t* count) {
  build_comment_index(p);
  uint32_t slots = syntaqlite_vec_len(&p->comment_idx_leading_start);
  if (token_idx >= slots) {
    *count = 0;
    return NULL;
  }
  uint32_t start;
  uint32_t cnt;
  if (side == SYNQ_COMMENT_LEADING) {
    start = syntaqlite_vec_at(&p->comment_idx_leading_start, token_idx);
    cnt = syntaqlite_vec_at(&p->comment_idx_leading_count, token_idx);
  } else {
    start = syntaqlite_vec_at(&p->comment_idx_trailing_start, token_idx);
    cnt = syntaqlite_vec_at(&p->comment_idx_trailing_count, token_idx);
  }
  if (start == UINT32_MAX || cnt == 0) {
    *count = 0;
    return NULL;
  }
  *count = cnt;
  return &p->comments.data[start];
}

SYNTAQLITE_API const SyntaqliteComment* syntaqlite_token_leading_comments(
    SyntaqliteParser* p,
    uint32_t token_idx,
    uint32_t* count) {
  return token_side_comments(p, token_idx, SYNQ_COMMENT_LEADING, count);
}

SYNTAQLITE_API const SyntaqliteComment* syntaqlite_token_trailing_comments(
    SyntaqliteParser* p,
    uint32_t token_idx,
    uint32_t* count) {
  return token_side_comments(p, token_idx, SYNQ_COMMENT_TRAILING, count);
}

// Resolve `node_id` to the inclusive token range `[*first_tok, *last_tok]`
// of entries in `p->tokens` that the parser fed to Lemon while reducing
// this node.  O(1), via the side table populated by the extent hooks.
//
// Works across macro boundaries: with the unified token stream, every
// shifted token (including macro-expansion tokens) is in `p->tokens`,
// and the extent hooks push its index here regardless of layer.  The
// returned range therefore covers the full set of tokens that produced
// the node.
SYNTAQLITE_API int32_t
syntaqlite_node_token_range(SyntaqliteParser* p,
                            uint32_t node_id,
                            SyntaqliteTokenIdx* first_tok,
                            SyntaqliteTokenIdx* last_tok) {
  if (!p->ctx.collect_node_extents) {
    return 0;
  }
  if (node_id == SYNTAQLITE_NULL_NODE) {
    return 0;
  }
  if (node_id >= syntaqlite_vec_len(&p->ctx.node_token_ranges)) {
    return 0;
  }
  SynqTokenRange r = syntaqlite_vec_at(&p->ctx.node_token_ranges, node_id);
  if (r.first_tok == UINT32_MAX) {
    return 0;
  }
  *first_tok = r.first_tok;
  *last_tok = r.last_tok;
  return 1;
}

#ifdef SYNTAQLITE_OMIT_MACROS
SYNTAQLITE_API uint32_t syntaqlite_result_macro_count(SyntaqliteParser* p) {
  (void)p;
  return 0;
}
SYNTAQLITE_API SyntaqliteMacroRewrite
syntaqlite_result_macro_rewrite_at(SyntaqliteParser* p, uint32_t idx) {
  (void)p;
  (void)idx;
  return (SyntaqliteMacroRewrite){
      .parent_idx = SYNTAQLITE_MACRO_PARENT_SOURCE,
  };
}
SYNTAQLITE_API uint32_t
syntaqlite_macro_rewrite_arg_segment_count(SyntaqliteParser* p,
                                           uint32_t rewrite_idx) {
  (void)p;
  (void)rewrite_idx;
  return 0;
}
SYNTAQLITE_API SyntaqliteMacroArgSegment
syntaqlite_macro_rewrite_arg_segment_at(SyntaqliteParser* p,
                                        uint32_t rewrite_idx,
                                        uint32_t segment_idx) {
  (void)p;
  (void)rewrite_idx;
  (void)segment_idx;
  return (SyntaqliteMacroArgSegment){0};
}
SYNTAQLITE_API uint32_t
syntaqlite_macro_rewrite_arg_count(SyntaqliteParser* p, uint32_t rewrite_idx) {
  (void)p;
  (void)rewrite_idx;
  return 0;
}
SYNTAQLITE_API SyntaqliteMacroCallArg
syntaqlite_macro_rewrite_arg_at(SyntaqliteParser* p,
                                uint32_t rewrite_idx,
                                uint32_t arg_idx) {
  (void)p;
  (void)rewrite_idx;
  (void)arg_idx;
  return (SyntaqliteMacroCallArg){0};
}
#else
SYNTAQLITE_API uint32_t syntaqlite_result_macro_count(SyntaqliteParser* p) {
  uint32_t total = syntaqlite_vec_len(&p->macro.layers);
  // Entry 0 is the source sentinel; real expansion layers start at 1.
  return total <= 1 ? 0 : total - 1;
}
SYNTAQLITE_API SyntaqliteMacroRewrite
syntaqlite_result_macro_rewrite_at(SyntaqliteParser* p, uint32_t idx) {
  // +1 to skip the source sentinel at index 0.
  uint32_t layer_idx = idx + 1;
  if (layer_idx >= syntaqlite_vec_len(&p->macro.layers)) {
    return (SyntaqliteMacroRewrite){
        .parent_idx = SYNTAQLITE_MACRO_PARENT_SOURCE,
    };
  }
  const SynqExpansionLayer* lyr = &p->macro.layers.data[layer_idx];
  // Internal parent_layer_id 0 = authored source sentinel.  Map it to the
  // public sentinel value; otherwise subtract 1 to account for the skipped
  // source entry.
  uint32_t parent_idx = lyr->parent_layer_id == 0
                            ? SYNTAQLITE_MACRO_PARENT_SOURCE
                            : lyr->parent_layer_id - 1;
  // Resolve the buffer that `call_offset` and every arg offset
  // measure into, so consumers can slice directly without walking
  // the parent chain.  Top-level layers measure into the current
  // statement slice; nested layers measure into the parent layer's
  // expansion buffer.
  const char* parent_buffer;
  uint32_t parent_buffer_len;
  if (lyr->parent_layer_id == 0) {
    parent_buffer = p->stmt_source;
    parent_buffer_len = p->stmt_end_offset - p->stmt_start_offset;
  } else {
    const SynqExpansionLayer* parent =
        &p->macro.layers.data[lyr->parent_layer_id];
    parent_buffer = parent->expansion_data;
    parent_buffer_len = parent->expansion_len;
  }
  return (SyntaqliteMacroRewrite){
      .parent_idx = parent_idx,
      .call_offset = lyr->call_offset,
      .call_length = lyr->call_length,
      .expansion = lyr->expansion_data,
      .expansion_len = lyr->expansion_len,
      .name = lyr->name,
      .name_len = lyr->name_len,
      .def_line = lyr->def_line,
      .def_col = lyr->def_col,
      .body_call_offset = lyr->body_call_offset,
      .body_call_length = lyr->body_call_length,
      .parent_buffer = parent_buffer,
      .parent_buffer_len = parent_buffer_len,
      .is_fallback = lyr->is_fallback,
  };
}

SYNTAQLITE_API uint32_t
syntaqlite_macro_rewrite_arg_segment_count(SyntaqliteParser* p,
                                           uint32_t rewrite_idx) {
  uint32_t layer_idx = rewrite_idx + 1;
  if (layer_idx >= syntaqlite_vec_len(&p->macro.layers))
    return 0;
  return p->macro.layers.data[layer_idx].arg_segment_count;
}

SYNTAQLITE_API SyntaqliteMacroArgSegment
syntaqlite_macro_rewrite_arg_segment_at(SyntaqliteParser* p,
                                        uint32_t rewrite_idx,
                                        uint32_t segment_idx) {
  uint32_t layer_idx = rewrite_idx + 1;
  if (layer_idx >= syntaqlite_vec_len(&p->macro.layers))
    return (SyntaqliteMacroArgSegment){0};
  const SynqExpansionLayer* lyr = &p->macro.layers.data[layer_idx];
  if (segment_idx >= lyr->arg_segment_count)
    return (SyntaqliteMacroArgSegment){0};
  const SynqArgSegment* seg = &lyr->arg_segments[segment_idx];
  // Map internal origin_layer_id (0 = source sentinel) to the public
  // sentinel / rewrite-index scheme used by parent_idx.
  uint32_t origin_parent_idx = seg->origin_layer_id == 0
                                   ? SYNTAQLITE_MACRO_PARENT_SOURCE
                                   : seg->origin_layer_id - 1;
  return (SyntaqliteMacroArgSegment){
      .body_offset = seg->body_offset,
      .body_length = seg->body_length,
      .expansion_offset = seg->sub_offset,
      .expansion_length = seg->sub_length,
      .origin_parent_idx = origin_parent_idx,
      .origin_offset = seg->origin_offset,
      .origin_length = seg->origin_length,
  };
}

SYNTAQLITE_API uint32_t
syntaqlite_macro_rewrite_arg_count(SyntaqliteParser* p, uint32_t rewrite_idx) {
  uint32_t layer_idx = rewrite_idx + 1;
  if (layer_idx >= syntaqlite_vec_len(&p->macro.layers))
    return 0;
  return p->macro.layers.data[layer_idx].arg_count;
}

SYNTAQLITE_API SyntaqliteMacroCallArg
syntaqlite_macro_rewrite_arg_at(SyntaqliteParser* p,
                                uint32_t rewrite_idx,
                                uint32_t arg_idx) {
  uint32_t layer_idx = rewrite_idx + 1;
  if (layer_idx >= syntaqlite_vec_len(&p->macro.layers))
    return (SyntaqliteMacroCallArg){0};
  const SynqExpansionLayer* lyr = &p->macro.layers.data[layer_idx];
  if (arg_idx >= lyr->arg_count)
    return (SyntaqliteMacroCallArg){0};
  const SynqMacroArg* arg = &lyr->args[arg_idx];
  return (SyntaqliteMacroCallArg){
      .offset = arg->offset,
      .length = arg->length,
  };
}
#endif

// ---------------------------------------------------------------------------
// Arena accessors
// ---------------------------------------------------------------------------

SYNTAQLITE_API const void* syntaqlite_parser_node(SyntaqliteParser* p,
                                                  uint32_t node_id) {
  return synq_arena_cptr(&p->ctx.ast, node_id);
}

SYNTAQLITE_API uint32_t syntaqlite_parser_node_count(SyntaqliteParser* p) {
  return syntaqlite_vec_len(&p->ctx.ast.offsets);
}

#ifndef SYNTAQLITE_OMIT_MACROS
static void append_expanded_range(SyntaqliteParser* p,
                                  uint32_t layer_id,
                                  const char* buf,
                                  uint32_t buf_len,
                                  uint32_t start,
                                  uint32_t end);
#endif

SYNTAQLITE_API const char* syntaqlite_parser_full_text(SyntaqliteParser* p,
                                                       uint32_t* out_len) {
  if (out_len) {
    *out_len = p->source_len;
  }
  return p->source;
}

SYNTAQLITE_API const char* syntaqlite_parser_text(SyntaqliteParser* p,
                                                  uint32_t* out_offset,
                                                  uint32_t* out_len) {
  if (p->stmt_start_offset == UINT32_MAX ||
      p->stmt_end_offset <= p->stmt_start_offset ||
      p->stmt_end_offset > p->source_len) {
    if (out_offset)
      *out_offset = 0;
    if (out_len)
      *out_len = 0;
    return NULL;
  }
  if (out_offset)
    *out_offset = p->stmt_start_offset;
  if (out_len)
    *out_len = p->stmt_end_offset - p->stmt_start_offset;
  return p->stmt_source;
}

SYNTAQLITE_API const char* syntaqlite_parser_layer_text(
    SyntaqliteParser* p,
    uint32_t layer_id,
    SyntaqliteLength* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (layer_id == 0) {
    // Authored source for the current statement.  Identical to
    // `syntaqlite_parser_text` but typed for the layer-indexed API.
    if (p->stmt_start_offset == UINT32_MAX ||
        p->stmt_end_offset <= p->stmt_start_offset ||
        p->stmt_end_offset > p->source_len) {
      return NULL;
    }
    if (out_len) {
      *out_len = p->stmt_end_offset - p->stmt_start_offset;
    }
    return p->stmt_source;
  }
#ifdef SYNTAQLITE_OMIT_MACROS
  (void)layer_id;
  return NULL;
#else
  if (layer_id >= syntaqlite_vec_len(&p->macro.layers)) {
    return NULL;
  }
  const SynqExpansionLayer* lyr = &p->macro.layers.data[layer_id];
  if (out_len) {
    *out_len = lyr->expansion_len;
  }
  return lyr->expansion_data;
#endif
}

SYNTAQLITE_API const char* syntaqlite_parser_expanded_text(SyntaqliteParser* p,
                                                           uint32_t* out_len) {
#ifdef SYNTAQLITE_OMIT_MACROS
  uint32_t ignored_offset = 0;
  const char* s = syntaqlite_parser_text(p, &ignored_offset, out_len);
  return s ? s : "";
#else
  if (out_len) {
    *out_len = 0;
  }
  if (p->stmt_start_offset == UINT32_MAX ||
      p->stmt_end_offset <= p->stmt_start_offset ||
      p->stmt_end_offset > p->source_len) {
    return "";
  }
  uint32_t stmt_len = p->stmt_end_offset - p->stmt_start_offset;
  syntaqlite_vec_clear(&p->macro.node_expanded_buf);
  append_expanded_range(p, 0, p->stmt_source, stmt_len, 0, stmt_len);
  if (out_len) {
    *out_len = syntaqlite_vec_len(&p->macro.node_expanded_buf);
  }
  return (const char*)p->macro.node_expanded_buf.data;
#endif
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

  if (text) {
    // Open the statement at the first non-whitespace byte, matching
    // parser_next (which never sees TK_SPACE because the tokenizer
    // skips it).  Leading TK_COMMENT still opens a statement.
    if (p->stmt_start_offset == UINT32_MAX &&
        token_type != SYNTAQLITE_TK_SPACE) {
      synq_open_statement(p, (uint32_t)(text - p->source));
    }
    // Advance p->offset so set_result_status can finalize stmt_end_offset;
    // parser_next updates it during tokenization but feed_token never
    // touches it otherwise.
    uint32_t tok_end = (uint32_t)(text - p->source) + len;
    if (tok_end > p->offset)
      p->offset = tok_end;
  }

  // Skip whitespace and comments without feeding to Lemon.
  if (synq_token_is_skip(token_type)) {
    if (token_type == SYNTAQLITE_TK_COMMENT && p->collect_tokens && text) {
      synq_parser_record_comment(p, (uint32_t)(text - p->source), len);
    }
    return set_result_status(p, SYNTAQLITE_PARSE_DONE);
  }

  uint32_t layer_offset = text ? (uint32_t)(text - p->stmt_source) : 0;
  int rc = shift_token(p, token_type, text, len, layer_offset);
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
    return SYNTAQLITE_ERR_ALREADY_USED;
  p->trace = enable;
  if (enable) {
    SYNQ_PARSER_TRACE(p->dialect.tmpl, stderr, "parser> ");
  } else {
    SYNQ_PARSER_TRACE(p->dialect.tmpl, NULL, NULL);
  }
  return SYNTAQLITE_OK;
}

SYNTAQLITE_API int32_t syntaqlite_parser_set_collect_tokens(SyntaqliteParser* p,
                                                            uint32_t enable) {
  if (p->sealed)
    return SYNTAQLITE_ERR_ALREADY_USED;
  p->collect_tokens = enable;
  return SYNTAQLITE_OK;
}

SYNTAQLITE_API int32_t syntaqlite_parser_set_macro_fallback(SyntaqliteParser* p,
                                                            uint32_t enable) {
  if (p->sealed)
    return SYNTAQLITE_ERR_ALREADY_USED;
#ifdef SYNTAQLITE_OMIT_MACROS
  (void)enable;
  return SYNTAQLITE_ERR_OMITTED;
#else
  p->macro.macro_fallback = enable;
  return SYNTAQLITE_OK;
#endif
}

SYNTAQLITE_API int32_t
syntaqlite_parser_set_collect_node_extents(SyntaqliteParser* p,
                                           uint32_t enable) {
  if (p->sealed)
    return SYNTAQLITE_ERR_ALREADY_USED;
  p->ctx.collect_node_extents = enable;
  return SYNTAQLITE_OK;
}

SYNTAQLITE_API const char* syntaqlite_parser_node_text(SyntaqliteParser* p,
                                                       uint32_t node_id,
                                                       uint32_t* out_len,
                                                       uint32_t* out_offset) {
  if (out_len) {
    *out_len = 0;
  }
  if (out_offset) {
    *out_offset = 0;
  }
  if (!p->ctx.collect_node_extents) {
    return NULL;
  }
  if (node_id >= syntaqlite_vec_len(&p->ctx.node_extents)) {
    return NULL;
  }
  SynqExtentRange r = syntaqlite_vec_at(&p->ctx.node_extents, node_id);
  // Sentinel `(UINT32_MAX, 0)` → not recorded.
  uint32_t stmt_len = p->stmt_end_offset > p->stmt_start_offset
                          ? p->stmt_end_offset - p->stmt_start_offset
                          : 0;
  if (r.root_start > r.root_end || r.root_end > stmt_len) {
    return NULL;
  }
  if (out_len) {
    *out_len = r.root_end - r.root_start;
  }
  if (out_offset) {
    *out_offset = r.root_start;
  }
  return p->stmt_source + r.root_start;
}

#ifndef SYNTAQLITE_OMIT_MACROS
static void append_expanded_range(SyntaqliteParser* p,
                                  uint32_t layer_id,
                                  const char* buf,
                                  uint32_t buf_len,
                                  uint32_t start,
                                  uint32_t end) {
  if (start > buf_len)
    start = buf_len;
  if (end > buf_len)
    end = buf_len;
  uint32_t cursor = start;
  uint32_t nlayers = syntaqlite_vec_len(&p->macro.layers);
  for (;;) {
    // Find the next child layer (parent == layer_id) whose call site
    // begins at or after `cursor` and lies fully within `[start, end)`.
    uint32_t best_child = 0;
    uint32_t best_offset = UINT32_MAX;
    for (uint32_t i = 1; i < nlayers; i++) {
      const SynqExpansionLayer* lyr = &p->macro.layers.data[i];
      if (lyr->parent_layer_id != layer_id)
        continue;
      if (lyr->call_offset < cursor)
        continue;
      if (lyr->call_offset + lyr->call_length > end)
        continue;
      if (lyr->call_offset < best_offset) {
        best_offset = lyr->call_offset;
        best_child = i;
      }
    }
    if (best_child == 0)
      break;
    const SynqExpansionLayer* child = &p->macro.layers.data[best_child];
    if (best_offset > cursor) {
      uint32_t n = best_offset - cursor;
      syntaqlite_vec_push_n(&p->macro.node_expanded_buf, buf + cursor, n,
                            p->mem);
    }
    append_expanded_range(p, best_child, child->expansion_data,
                          child->expansion_len, 0, child->expansion_len);
    cursor = best_offset + child->call_length;
  }
  if (end > cursor) {
    uint32_t n = end - cursor;
    syntaqlite_vec_push_n(&p->macro.node_expanded_buf, buf + cursor, n, p->mem);
  }
}
#endif  // !SYNTAQLITE_OMIT_MACROS

SYNTAQLITE_API const char* syntaqlite_parser_node_expanded_text(
    SyntaqliteParser* p,
    uint32_t node_id,
    uint32_t* out_len) {
#ifdef SYNTAQLITE_OMIT_MACROS
  // Without macros, expanded text == authored text.
  return syntaqlite_parser_node_text(p, node_id, out_len, NULL);
#else
  if (out_len) {
    *out_len = 0;
  }
  if (!p->ctx.collect_node_extents) {
    return NULL;
  }
  if (node_id >= syntaqlite_vec_len(&p->ctx.node_expanded_extents)) {
    return NULL;
  }

  SynqNodeExpandedExtent e =
      syntaqlite_vec_at(&p->ctx.node_expanded_extents, node_id);
  if (e.length > 0) {
    const char* buf = e.layer_id == 0
                          ? p->stmt_source
                          : p->macro.layers.data[e.layer_id].expansion_data;
    if (out_len) {
      *out_len = e.length;
    }
    return buf + e.offset;
  }

  if (node_id >= syntaqlite_vec_len(&p->ctx.node_extents)) {
    return NULL;
  }
  SynqExtentRange r = syntaqlite_vec_at(&p->ctx.node_extents, node_id);
  uint32_t stmt_len = p->stmt_end_offset > p->stmt_start_offset
                          ? p->stmt_end_offset - p->stmt_start_offset
                          : 0;
  if (r.root_start > r.root_end || r.root_end > stmt_len) {
    return NULL;
  }
  syntaqlite_vec_clear(&p->macro.node_expanded_buf);
  append_expanded_range(p, 0, p->stmt_source, stmt_len, r.root_start,
                        r.root_end);
  if (out_len) {
    *out_len = syntaqlite_vec_len(&p->macro.node_expanded_buf);
  }
  return (const char*)p->macro.node_expanded_buf.data;
#endif
}

SYNTAQLITE_API int syntaqlite_node_is_macro_free(SyntaqliteParser* p,
                                                 uint32_t node_id) {
#ifdef SYNTAQLITE_OMIT_MACROS
  (void)p;
  (void)node_id;
  return 1;  // No macros → all nodes are macro-free.
#else
  if (!p->ctx.collect_node_extents) {
    return 0;
  }
  if (node_id >= syntaqlite_vec_len(&p->ctx.node_expanded_extents)) {
    return 0;
  }
  SynqNodeExpandedExtent e =
      syntaqlite_vec_at(&p->ctx.node_expanded_extents, node_id);
  // length == 0 is the sentinel for epsilon or multi-layer nodes.
  return e.length > 0 && e.layer_id == 0;
#endif
}

// ---------------------------------------------------------------------------
// SYNTAQLITE_OMIT_MACROS stubs for span/traceback APIs
// ---------------------------------------------------------------------------
//
// When macros are compiled out, parser_spans.c is empty.  These stubs
// provide the same public API with trivial implementations: all spans
// are in layer 0 (source), so span_text is a direct source slice and
// traceback returns NULL (no expansion frames to report).

#ifdef SYNTAQLITE_OMIT_MACROS

SYNTAQLITE_API const char* syntaqlite_parser_span_text(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    uint32_t* out_len,
    uint32_t* out_offset) {
  if (out_offset)
    *out_offset = 0;
  if (!span || span->length == 0) {
    *out_len = 0;
    return NULL;
  }
  uint32_t stmt_len = p->stmt_end_offset > p->stmt_start_offset
                          ? p->stmt_end_offset - p->stmt_start_offset
                          : 0;
  if (span->offset + span->length > stmt_len) {
    *out_len = 0;
    return NULL;
  }
  *out_len = span->length;
  if (out_offset)
    *out_offset = span->offset;
  return p->stmt_source + span->offset;
}

SYNTAQLITE_API const char* syntaqlite_parser_span_expanded_text(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* span,
    uint32_t* out_len) {
  return syntaqlite_parser_span_text(p, span, out_len, NULL);
}

SYNTAQLITE_API const SyntaqliteTracebackFrame* syntaqlite_parser_traceback(
    SyntaqliteParser* p,
    const SyntaqliteTextSpan* sp,
    uint32_t* out_count) {
  (void)p;
  (void)sp;
  if (out_count)
    *out_count = 0;
  return NULL;
}

SYNTAQLITE_API int32_t
syntaqlite_parser_set_macro_lookup(SyntaqliteParser* p,
                                   SyntaqliteMacroLookupFn fn,
                                   void* user_data) {
  (void)p;
  (void)fn;
  (void)user_data;
  return SYNTAQLITE_ERR_OMITTED;
}

#endif  // SYNTAQLITE_OMIT_MACROS
