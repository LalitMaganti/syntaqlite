// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "csrc/dialect_dispatch.h"
#include "csrc/hashmap.h"
#include "csrc/token_wrapped.h"
#include "csrc/tokens.h"
#include "syntaqlite/grammar.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/dialect_types.h"

// ---------------------------------------------------------------------------
// Macro expansion types
// ---------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
#define SYNQ_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define SYNQ_NOINLINE __declspec(noinline)
#else
#define SYNQ_NOINLINE
#endif

#define SYNQ_MAX_MACRO_DEPTH 16
#define SYNQ_MACRO_TABLE_INITIAL_SIZE 16

// A single registered macro.
typedef struct SyntaqliteMacroEntry {
  char* name;  // Owned copy of the macro name.
  uint32_t name_len;

  // --- Template macros ---
  char* body;  // Body text with $param placeholders. Owned.
  uint32_t body_len;
  char** param_names;  // Array of param name strings. Owned.
  uint32_t* param_name_lens;
  uint32_t param_count;

  uint8_t state;  // SYNQ_MAP_EMPTY / LIVE / TOMBSTONE
} SyntaqliteMacroEntry;

// A comma-separated argument extracted from a macro call site.
typedef struct SynqMacroArg {
  uint32_t offset;  // Byte offset in the source buffer.
  uint32_t length;  // Byte length of the argument text.
} SynqMacroArg;

// An owned expansion buffer kept alive for AST span references.
typedef struct SynqOwnedBuf {
  char* data;
  uint32_t len;
} SynqOwnedBuf;

// ---------------------------------------------------------------------------
// Parser struct
// ---------------------------------------------------------------------------

struct SyntaqliteParser {
  SyntaqliteMemMethods mem;
  SyntaqliteGrammar grammar;
  void* lemon;
  SynqParseCtx ctx;
  const char* source;
  uint32_t source_len;
  uint32_t offset;           // Tokenizer cursor into source.
  uint32_t last_token_type;  // Last non-whitespace token fed to Lemon.
  uint32_t finished;         // 1 after EOF has been sent to Lemon.
  uint32_t had_comment;      // 1 if any comment token was seen this stmt.
  uint32_t had_error;        // Sticky error flag for current result.
  int32_t last_status;       // Last SYNTAQLITE_PARSE_* status returned.
  char error_msg[256];       // Error message buffer.
  uint32_t trace;
  uint32_t collect_tokens;
  uint32_t macro_fallback;  // 1 = unregistered name!(args) becomes TK_ID.
  uint32_t sealed;
  uint32_t pending_reset;  // 1 after feed_token signals completion; cleared on
                           // the next feed_token call (arena reset deferred).
  SYNQ_VEC(SyntaqliteComment) comments;
  SYNQ_VEC(SyntaqliteParserToken) tokens;
  uint32_t macro_depth;  // Nesting depth (0 = not in macro).
  SYNQ_VEC(SyntaqliteMacroRegion) macros;

  // ── Macro registry (open-addressing hashmap) ──────────────────────────
  SyntaqliteMacroEntry* macro_table;
  uint32_t macro_table_size;   // Capacity (power of 2).
  uint32_t macro_table_count;  // Number of live entries.

  // ── Expansion state ───────────────────────────────────────────────────
  // Blue-paint recursion detection: names of macros currently being expanded.
  const char* expansion_names[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_name_lens[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_depth;

  // Owned expansion buffers kept alive so AST spans remain valid until
  // the next reset_stmt().
  SYNQ_VEC(SynqOwnedBuf) owned_bufs;
};

static int32_t set_result_status(SyntaqliteParser* p, int32_t rc) {
  p->last_status = rc;
  return rc;
}

// Forward declaration — defined after feed_one_token.
static int check_macro_straddle(SyntaqliteParser* p);

// Forward declaration — macro expansion in a buffer (defined later).
static int try_expand_macro_in_buf(SyntaqliteParser* p,
                                   const char* buf,
                                   uint32_t buf_len,
                                   uint32_t id_offset,
                                   uint32_t id_len,
                                   uint32_t bang_offset,
                                   uint32_t depth);

// Forward declaration — balanced-paren scanning for macro args (defined later).
static uint32_t scan_macro_args(SyntaqliteParser* p,
                                const char* source,
                                uint32_t source_len,
                                uint32_t bang_offset,
                                SynqMacroArg* out_args,
                                uint32_t max_args,
                                uint32_t* out_end_offset);

// Forward declaration — free macro entry strings (defined in Cleanup section).
static void free_macro_entry(SyntaqliteParser* p, SyntaqliteMacroEntry* e);

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
  SYNQ_PARSER_FINALIZE(p->grammar.tmpl, p->lemon);
  SYNQ_PARSER_INIT(p->grammar.tmpl, p->lemon, &p->ctx);
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
  syntaqlite_vec_clear(&p->macros);
  // Free owned expansion buffers from previous statement.
  for (uint32_t i = 0; i < syntaqlite_vec_len(&p->owned_bufs); i++) {
    p->mem.xFree(p->owned_bufs.data[i].data);
  }
  syntaqlite_vec_clear(&p->owned_bufs);
  p->expansion_depth = 0;
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

  if (check_macro_straddle(p) < 0)
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

SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create_with_grammar(
    const SyntaqliteMemMethods* mem,
    const SyntaqliteGrammar grammar) {
  SyntaqliteMemMethods m = mem ? *mem : SYNTAQLITE_MEM_METHODS_DEFAULT;
  SyntaqliteParser* p = m.xMalloc(sizeof(SyntaqliteParser));
  memset(p, 0, sizeof(*p));
  p->mem = m;
  p->grammar = grammar;
  p->lemon = SYNQ_PARSER_ALLOC(grammar.tmpl, m.xMalloc, &p->ctx);
  synq_parse_ctx_init(&p->ctx, m);
  syntaqlite_vec_init(&p->comments);
  syntaqlite_vec_init(&p->tokens);
  syntaqlite_vec_init(&p->macros);
  syntaqlite_vec_init(&p->owned_bufs);
  // macro_table, expansion state already zeroed by memset
  return p;
}

#ifndef SYNTAQLITE_OMIT_SQLITE_API
SYNTAQLITE_API SyntaqliteParser* syntaqlite_parser_create(
    const SyntaqliteMemMethods* mem) {
  SyntaqliteGrammar grammar = syntaqlite_sqlite_grammar();
  return syntaqlite_parser_create_with_grammar(mem, grammar);
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

  p->ctx.source = source;
  p->ctx.env = &p->grammar;
}

// ---------------------------------------------------------------------------
// Internal: feed one real token to Lemon.
// Returns: 0 = keep going, 1 = statement completed, -1 = unrecoverable error.
// ---------------------------------------------------------------------------

static int feed_one_token(SyntaqliteParser* p,
                          uint32_t token_type,
                          const char* text,
                          uint32_t len,
                          uint32_t token_idx) {
  SynqParseToken minor = {
      .z = text, .n = len, .type = token_type, .token_idx = token_idx};
  SYNQ_PARSER_FEED(p->grammar.tmpl, p->lemon, (int)token_type, minor);
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

// ---------------------------------------------------------------------------
// Internal: check macro straddle after statement completion.
// ---------------------------------------------------------------------------

static int check_macro_straddle(SyntaqliteParser* p) {
  uint32_t macro_count = syntaqlite_vec_len(&p->macros);
  if (macro_count == 0)
    return 0;
  if (!p->grammar.tmpl->range_meta) {
    snprintf(p->error_msg, sizeof(p->error_msg),
             "internal error: grammar has no range_meta but macros were used");
    p->had_error = 1;
    return -1;
  }

  uint32_t node_count = syntaqlite_vec_len(&p->ctx.ast.offsets);
  const SyntaqliteMacroRegion* macros = p->macros.data;

  for (uint32_t nid = 0; nid < node_count; nid++) {
    const uint8_t* raw = (const uint8_t*)synq_arena_ptr(&p->ctx.ast, nid);
    uint32_t tag;
    memcpy(&tag, raw, sizeof(tag));
    if (tag == 0 || tag >= p->grammar.tmpl->node_count)
      continue;

    const SyntaqliteRangeMetaEntry* entry = &p->grammar.tmpl->range_meta[tag];
    if (entry->fields == NULL || entry->count == 0)
      continue;

    for (uint32_t mi = 0; mi < macro_count; mi++) {
      uint32_t r_start = macros[mi].call_offset;
      uint32_t r_end = r_start + macros[mi].call_length;

      int has_inside = 0;
      int has_outside = 0;

      for (uint8_t fi = 0; fi < entry->count; fi++) {
        if (entry->fields[fi].kind != 1)
          continue;  // Not a SourceSpan.
        const SyntaqliteSourceSpan* sp =
            (const SyntaqliteSourceSpan*)(raw + entry->fields[fi].offset);
        if (sp->length == 0)
          continue;

        uint32_t s_start = sp->offset;
        uint32_t s_end = sp->offset + sp->length;

        if (s_start >= r_start && s_end <= r_end) {
          has_inside = 1;
        } else {
          has_outside = 1;
        }
      }

      if (has_inside && has_outside) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "macro expansion straddles node boundary");
        p->had_error = 1;
        return -1;
      }
    }
  }
  return 0;
}

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
  SynqParseToken eof = {.z = NULL, .n = 0, .type = 0, .token_idx = 0xFFFFFFFF};
  SYNQ_PARSER_FEED(p->grammar.tmpl, p->lemon, 0, eof);
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
    if (check_macro_straddle(p) < 0)
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
static int record_and_feed(SyntaqliteParser* p,
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

// Try to expand a Rust-style macro call: ID!(args).
// Requires macro_style == RUST and a matching registry entry (or fallback
// mode). Returns 0 if consumed, -1 if not a macro call, 1 if statement
// boundary.
SYNQ_NOINLINE
static int try_macro_call(SyntaqliteParser* p,
                          uint32_t id_offset,
                          uint32_t id_len,
                          uint32_t bang_offset) {
  const unsigned char* z = (const unsigned char*)p->source;
  if (z[bang_offset] != '!')
    return -1;
  if (p->grammar.tmpl->macro_style != SYNQ_MACRO_STYLE_RUST &&
      !p->macro_fallback)
    return -1;

  // Look up macro in registry.
  SyntaqliteMacroEntry* entry = NULL;
  if (p->macro_table_size > 0) {
    SYNQ_MAP_FIND(p->macro_table, p->macro_table_size, p->source + id_offset,
                  id_len, entry);
  }

  if (entry) {
    // Registered macro — expand as before.
    int rc = try_expand_macro_in_buf(p, p->source, p->source_len, id_offset,
                                     id_len, bang_offset, 0);
    if (rc < 0)
      return -1;
    uint32_t call_end = (uint32_t)rc;
    syntaqlite_parser_begin_macro(p, id_offset, call_end - id_offset);
    syntaqlite_parser_end_macro(p);
    p->offset = call_end;
    return 0;
  }

  // Unregistered macro — fallback to TK_ID.  Always allowed when the
  // grammar declares RUST-style macros; otherwise only when macro_fallback
  // is explicitly set (e.g. embedded-SQL hole placeholders).
  if (p->grammar.tmpl->macro_style != SYNQ_MACRO_STYLE_RUST &&
      !p->macro_fallback)
    return -1;

  // Scan balanced parens to find the end of name!(args).
  uint32_t end_offset = 0;
  scan_macro_args(p, p->source, p->source_len, bang_offset, NULL, 0,
                  &end_offset);
  if (end_offset == 0)
    return -1;  // Unbalanced parens — still an error.

  uint32_t call_length = end_offset - id_offset;

  // Record macro region so formatter emits verbatim.
  syntaqlite_parser_begin_macro(p, id_offset, call_length);
  syntaqlite_parser_end_macro(p);

  // Feed the whole name!(args) span as a single TK_ID to Lemon.
  int rc = record_and_feed(p, SYNTAQLITE_TK_ID, id_offset, call_length);
  p->offset = end_offset;
  return rc;
}

// ---------------------------------------------------------------------------
// Macro expansion
// ---------------------------------------------------------------------------

// Scan balanced parens after '!' and split into comma-separated args.
// Returns arg count on success, 0 if not a valid macro call.
// `source`/`source_len` is the buffer being scanned (may be original source
// or an expansion buffer for nested macros).
static uint32_t scan_macro_args(SyntaqliteParser* p,
                                const char* source,
                                uint32_t source_len,
                                uint32_t bang_offset,
                                SynqMacroArg* out_args,
                                uint32_t max_args,
                                uint32_t* out_end_offset) {
  const unsigned char* z = (const unsigned char*)source;
  uint32_t pos = bang_offset + 1;  // skip '!'

  // Expect LP.
  uint32_t ttype = 0;
  int64_t tlen = SynqSqliteGetTokenVersionWrapped(&p->grammar, z + pos, &ttype);
  if (tlen <= 0 || ttype != SYNTAQLITE_TK_LP)
    return 0;
  pos += (uint32_t)tlen;

  // Check for empty args: macro!()
  ttype = 0;
  tlen = SynqSqliteGetTokenVersionWrapped(&p->grammar, z + pos, &ttype);
  if (tlen > 0 && ttype == SYNTAQLITE_TK_RP) {
    *out_end_offset = pos + (uint32_t)tlen;
    return 0;
  }

  uint32_t arg_count = 0;
  uint32_t depth = 1;
  uint32_t arg_start = pos;

  while (pos < source_len && depth > 0) {
    ttype = 0;
    tlen = SynqSqliteGetTokenVersionWrapped(&p->grammar, z + pos, &ttype);
    if (tlen <= 0)
      return 0;

    if (ttype == SYNTAQLITE_TK_LP) {
      depth++;
    } else if (ttype == SYNTAQLITE_TK_RP) {
      depth--;
      if (depth == 0) {
        if (arg_count < max_args) {
          out_args[arg_count].offset = arg_start;
          out_args[arg_count].length = pos - arg_start;
        }
        arg_count++;
        *out_end_offset = pos + (uint32_t)tlen;
        return arg_count;
      }
    } else if (depth == 1 && ttype == SYNTAQLITE_TK_COMMA) {
      if (arg_count < max_args) {
        out_args[arg_count].offset = arg_start;
        out_args[arg_count].length = pos - arg_start;
      }
      arg_count++;
      arg_start = pos + (uint32_t)tlen;
    } else if (ttype == SYNTAQLITE_TK_SEMI) {
      return 0;
    }

    pos += (uint32_t)tlen;
  }

  return 0;  // Unbalanced parens.
}

// ---------------------------------------------------------------------------
// Template expansion ($param substitution)
// ---------------------------------------------------------------------------

// Expand a template macro body by substituting $param references.
// Uses the tokenizer to identify TK_VARIABLE tokens rather than
// hand-rolling identifier scanning.
// Allocates `*out_buf` via p->mem; caller owns the result.
// Returns 0 on success, -1 on error (unknown $param).
static int expand_template(SyntaqliteParser* p,
                           const SyntaqliteMacroEntry* entry,
                           const SynqMacroArg* args,
                           uint32_t arg_count,
                           const char* arg_source,
                           char** out_buf,
                           uint32_t* out_len) {
  // Pre-size: body length + some slack for arg text.
  uint32_t cap = entry->body_len + 64;
  char* buf = p->mem.xMalloc(cap);
  uint32_t len = 0;
  const char* body = entry->body;
  uint32_t blen = entry->body_len;
  const unsigned char* z = (const unsigned char*)body;

  uint32_t pos = 0;
  while (pos < blen) {
    uint32_t ttype = 0;
    int64_t tlen =
        SynqSqliteGetTokenVersionWrapped(&p->grammar, z + pos, &ttype);
    if (tlen <= 0)
      break;

    if (ttype == SYNTAQLITE_TK_VARIABLE && body[pos] == '$' && tlen > 1) {
      // $param — look up the name after '$'.
      const char* pname = body + pos + 1;
      uint32_t pname_len = (uint32_t)tlen - 1;

      int found = -1;
      for (uint32_t pi = 0; pi < entry->param_count; pi++) {
        if (entry->param_name_lens[pi] == pname_len &&
            memcmp(entry->param_names[pi], pname, pname_len) == 0) {
          found = (int)pi;
          break;
        }
      }

      if (found < 0) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "unknown macro parameter '$%.*s'", (int)pname_len, pname);
        p->mem.xFree(buf);
        return -1;
      }

      // Substitute the arg text.
      if ((uint32_t)found < arg_count) {
        uint32_t alen = args[found].length;
        while (len + alen > cap) {
          cap *= 2;
          buf = p->mem.xRealloc(buf, cap);
        }
        memcpy(buf + len, arg_source + args[found].offset, alen);
        len += alen;
      }
      // else: arg not provided — substitute empty string.
    } else {
      // Copy token verbatim.
      while (len + (uint32_t)tlen > cap) {
        cap *= 2;
        buf = p->mem.xRealloc(buf, cap);
      }
      memcpy(buf + len, body + pos, (uint32_t)tlen);
      len += (uint32_t)tlen;
    }

    pos += (uint32_t)tlen;
  }

  // Null-terminate so the tokenizer has a sentinel when scanning ahead.
  while (len + 1 > cap) {
    cap *= 2;
    buf = p->mem.xRealloc(buf, cap);
  }
  buf[len] = '\0';

  *out_buf = buf;
  *out_len = len;
  return 0;
}

// Tokenize `buf` and feed each token to Lemon.
// `depth` is the current expansion nesting (for recursion limit).
// Returns: 0 = ok, 1 = statement boundary, -1 = error.
static int expand_and_feed(SyntaqliteParser* p,
                           const char* buf,
                           uint32_t buf_len,
                           uint32_t depth) {
  if (depth >= SYNQ_MAX_MACRO_DEPTH) {
    snprintf(p->error_msg, sizeof(p->error_msg),
             "macro expansion depth limit exceeded (%d)", SYNQ_MAX_MACRO_DEPTH);
    p->had_error = 1;
    return -1;
  }

  // Temporarily swap ctx.source so Lemon action offset computations are
  // relative to the expansion buffer.
  const char* saved_source = p->ctx.source;
  p->ctx.source = buf;

  const unsigned char* z = (const unsigned char*)buf;
  uint32_t pos = 0;

  while (pos < buf_len) {
    uint32_t ttype = 0;
    int64_t tlen =
        SynqSqliteGetTokenVersionWrapped(&p->grammar, z + pos, &ttype);
    if (tlen <= 0)
      break;

    if (ttype == SYNTAQLITE_TK_SPACE || ttype == SYNTAQLITE_TK_COMMENT) {
      pos += (uint32_t)tlen;
      continue;
    }

    // Check for nested macro call: ID followed by '!'.
    uint32_t next_pos = pos + (uint32_t)tlen;
    if (ttype == SYNTAQLITE_TK_ID && next_pos < buf_len && z[next_pos] == '!') {
      int mrc = try_expand_macro_in_buf(p, buf, buf_len, pos, (uint32_t)tlen,
                                        next_pos, depth);
      if (mrc >= 0) {
        // Nested macro handled (mrc is the new pos).
        pos = (uint32_t)mrc;
        continue;
      }
      // mrc == -1: not a macro or error — feed ID normally below.
      if (p->had_error) {
        p->ctx.source = saved_source;
        return -1;
      }
    }

    // Feed token to Lemon.
    SynqParseToken minor = {.z = buf + pos,
                            .n = (uint32_t)tlen,
                            .type = ttype,
                            .token_idx = 0xFFFFFFFF};
    SYNQ_PARSER_FEED(p->grammar.tmpl, p->lemon, (int)ttype, minor);
    p->last_token_type = ttype;

    if (p->ctx.error) {
      p->had_error = 1;
      if (p->error_msg[0] == '\0') {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "syntax error in macro expansion near '%.*s'", (int)tlen,
                 buf + pos);
      }
      p->ctx.error = 0;
    }

    if (p->ctx.stmt_completed) {
      p->ctx.stmt_completed = 0;
      p->ctx.source = saved_source;
      return 1;
    }

    pos += (uint32_t)tlen;
  }

  p->ctx.source = saved_source;
  return 0;
}

// Try to expand a macro call within an expansion buffer.
// Returns: new position past the call on success, -1 if not a macro.
static int try_expand_macro_in_buf(SyntaqliteParser* p,
                                   const char* buf,
                                   uint32_t buf_len,
                                   uint32_t id_offset,
                                   uint32_t id_len,
                                   uint32_t bang_offset,
                                   uint32_t depth) {
  SyntaqliteMacroEntry* entry;
  SYNQ_MAP_FIND(p->macro_table, p->macro_table_size, buf + id_offset, id_len,
                entry);
  if (!entry)
    return -1;

  // Check blue-paint: recursion detection.
  for (uint32_t i = 0; i < p->expansion_depth; i++) {
    if (synq_name_eq_ci(p->expansion_names[i], p->expansion_name_lens[i],
                        entry->name, entry->name_len)) {
      snprintf(p->error_msg, sizeof(p->error_msg),
               "recursive macro expansion: '%.*s'", (int)entry->name_len,
               entry->name);
      p->had_error = 1;
      return -1;
    }
  }

  // Parse args.
  SynqMacroArg args[64];
  uint32_t end_offset = 0;
  uint32_t arg_count =
      scan_macro_args(p, buf, buf_len, bang_offset, args, 64, &end_offset);

  // Check arg count.
  if (entry->param_count > 0 && arg_count != entry->param_count) {
    snprintf(p->error_msg, sizeof(p->error_msg),
             "macro '%.*s' expects %u args, got %u", (int)entry->name_len,
             entry->name, entry->param_count, arg_count);
    p->had_error = 1;
    return -1;
  }

  // Expand.
  char* expanded = NULL;
  uint32_t expanded_len = 0;

  if (expand_template(p, entry, args, arg_count, buf, &expanded,
                      &expanded_len) < 0) {
    p->had_error = 1;
    return -1;
  }

  // Keep the expansion buffer alive for AST spans.
  SynqOwnedBuf ob = {expanded, expanded_len};
  syntaqlite_vec_push(&p->owned_bufs, ob, p->mem);

  // Push blue-paint.
  p->expansion_names[p->expansion_depth] = entry->name;
  p->expansion_name_lens[p->expansion_depth] = entry->name_len;
  p->expansion_depth++;

  // Feed expanded tokens.
  int rc = expand_and_feed(p, expanded, expanded_len, depth + 1);

  // Pop blue-paint.
  p->expansion_depth--;

  if (rc < 0)
    return -1;

  return (int)end_offset;
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
    int64_t len = SynqSqliteGetTokenVersionWrapped(&p->grammar, z + pos, &type);
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

    // Macro detection: ID followed by TK_ILLEGAL ('!').
    if (cur_type == SYNTAQLITE_TK_ID && la_type == SYNTAQLITE_TK_ILLEGAL) {
      int mrc = try_macro_call(p, cur_offset, (uint32_t)cur_len, la_offset);
      if (mrc == 1)
        return set_result_status(p, stmt_boundary(p));
      if (mrc == 0) {
        // Macro consumed tokens past the lookahead — re-tokenize.
        cur_len = next_token(p, z, p->offset, &cur_offset, &cur_type);
        continue;
      }
    }

    // Normal token (or macro fallthrough): record + feed to Lemon.
    if (record_and_feed(p, cur_type, cur_offset, (uint32_t)cur_len))
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
  *count = syntaqlite_vec_len(&p->macros);
  return p->macros.data;
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
  if (p == NULL || p->grammar.tmpl == NULL ||
      p->grammar.tmpl->parser_expected_tokens == NULL) {
    return 0;
  }
  return p->grammar.tmpl->parser_expected_tokens(p->lemon, out_tokens, out_cap);
}

SYNTAQLITE_API SyntaqliteCompletionContext
syntaqlite_parser_completion_context(SyntaqliteParser* p) {
  if (p == NULL || p->grammar.tmpl == NULL ||
      p->grammar.tmpl->parser_completion_context == NULL) {
    return SYNTAQLITE_COMPLETION_CONTEXT_UNKNOWN;
  }
  return (SyntaqliteCompletionContext)
      p->grammar.tmpl->parser_completion_context(p->lemon);
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
// Macro region tracking
// ---------------------------------------------------------------------------

SYNTAQLITE_API void syntaqlite_parser_begin_macro(SyntaqliteParser* p,
                                                  uint32_t call_offset,
                                                  uint32_t call_length) {
  SyntaqliteMacroRegion region = {call_offset, call_length};
  syntaqlite_vec_push(&p->macros, region, p->mem);
  p->macro_depth++;
}

SYNTAQLITE_API void syntaqlite_parser_end_macro(SyntaqliteParser* p) {
  if (p->macro_depth > 0) {
    p->macro_depth--;
  }
}

// ---------------------------------------------------------------------------
// AST dump
// ---------------------------------------------------------------------------

typedef SYNQ_VEC(char) DumpBuf;

static void dump_append(DumpBuf* b,
                        SyntaqliteMemMethods mem,
                        const char* s,
                        uint32_t n) {
  syntaqlite_vec_push_n(b, s, n, mem);
}

static void dump_printf(DumpBuf* b,
                        SyntaqliteMemMethods mem,
                        const char* fmt,
                        ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (n <= 0)
    return;
  syntaqlite_vec_ensure(b, b->count + (uint32_t)n + 1, mem);
  va_start(ap, fmt);
  vsnprintf(b->data + b->count, (uint32_t)n + 1, fmt, ap);
  va_end(ap);
  b->count += (uint32_t)n;
}

static void dump_indent(DumpBuf* b, SyntaqliteMemMethods mem, uint32_t indent) {
  for (uint32_t i = 0; i < indent; i++)
    dump_append(b, mem, "  ", 2);
}

static void dump_node_recursive(DumpBuf* b,
                                SyntaqliteParser* p,
                                uint32_t node_id,
                                uint32_t indent) {
  if (node_id == SYNTAQLITE_NULL_NODE)
    return;
  uint32_t count = syntaqlite_vec_len(&p->ctx.ast.offsets);
  if (node_id >= count)
    return;

  const uint8_t* raw = (const uint8_t*)synq_arena_ptr(&p->ctx.ast, node_id);
  uint32_t tag;
  memcpy(&tag, raw, sizeof(tag));

  const SyntaqliteGrammarTemplate* g = p->grammar.tmpl;
  if (tag >= g->node_count)
    return;

  const char* name = g->node_names[tag];
  uint8_t field_count = g->field_meta_counts[tag];
  SyntaqliteMemMethods mem = p->mem;

  // List node: no field descriptors, has tag + count header.
  if (field_count == 0 && tag != 0) {
    SynqListHeader hdr;
    memcpy(&hdr, raw, sizeof(hdr));
    dump_indent(b, mem, indent);
    dump_printf(b, mem, "%s [%u items]\n", name, hdr.count);
    const uint32_t* children = (const uint32_t*)(raw + sizeof(SynqListHeader));
    for (uint32_t i = 0; i < hdr.count; i++) {
      dump_node_recursive(b, p, children[i], indent + 1);
    }
    return;
  }

  dump_indent(b, mem, indent);
  dump_printf(b, mem, "%s\n", name);

  if (field_count == 0)
    return;
  const SyntaqliteFieldMeta* fields = g->field_meta[tag];

  for (uint8_t fi = 0; fi < field_count; fi++) {
    const SyntaqliteFieldMeta* fm = &fields[fi];
    const uint8_t* field_ptr = raw + fm->offset;

    switch (fm->kind) {
      case SYNTAQLITE_FIELD_NODE_ID: {
        uint32_t child_id;
        memcpy(&child_id, field_ptr, sizeof(child_id));
        dump_indent(b, mem, indent + 1);
        if (child_id == SYNTAQLITE_NULL_NODE) {
          dump_printf(b, mem, "%s: (none)\n", fm->name);
        } else {
          dump_printf(b, mem, "%s:\n", fm->name);
          dump_node_recursive(b, p, child_id, indent + 2);
        }
        break;
      }
      case SYNTAQLITE_FIELD_SPAN: {
        SyntaqliteSourceSpan sp;
        memcpy(&sp, field_ptr, sizeof(sp));
        dump_indent(b, mem, indent + 1);
        if (sp.length == 0) {
          dump_printf(b, mem, "%s: (none)\n", fm->name);
        } else {
          dump_printf(b, mem, "%s: \"%.*s\"\n", fm->name, (int)sp.length,
                      p->source + sp.offset);
        }
        break;
      }
      case SYNTAQLITE_FIELD_BOOL: {
        uint32_t val;
        memcpy(&val, field_ptr, sizeof(val));
        dump_indent(b, mem, indent + 1);
        dump_printf(b, mem, "%s: %s\n", fm->name, val ? "TRUE" : "FALSE");
        break;
      }
      case SYNTAQLITE_FIELD_FLAGS: {
        uint8_t val = *field_ptr;
        // Mask to defined bits only — upper bits may contain struct padding.
        if (fm->display_count < 8)
          val &= (uint8_t)((1u << fm->display_count) - 1);
        dump_indent(b, mem, indent + 1);
        dump_printf(b, mem, "%s: ", fm->name);
        if (val == 0) {
          dump_append(b, mem, "(none)", 6);
        } else {
          int first = 1;
          for (int bit = 0; bit < fm->display_count; bit++) {
            if (val & (1 << bit)) {
              const char* flag_name = fm->display ? fm->display[bit] : "?";
              if (flag_name[0] == '\0')
                continue;
              if (!first)
                dump_append(b, mem, " ", 1);
              dump_append(b, mem, flag_name, (uint32_t)strlen(flag_name));
              first = 0;
            }
          }
          if (first)
            dump_append(b, mem, "(none)", 6);
        }
        dump_append(b, mem, "\n", 1);
        break;
      }
      case SYNTAQLITE_FIELD_ENUM: {
        uint32_t val;
        memcpy(&val, field_ptr, sizeof(val));
        dump_indent(b, mem, indent + 1);
        const char* label =
            (val < fm->display_count && fm->display) ? fm->display[val] : "?";
        dump_printf(b, mem, "%s: %s\n", fm->name, label);
        break;
      }
    }
  }
}

SYNTAQLITE_API char* syntaqlite_dump_node(SyntaqliteParser* p,
                                          uint32_t node_id,
                                          uint32_t indent) {
  DumpBuf buf;
  syntaqlite_vec_init(&buf);
  dump_node_recursive(&buf, p, node_id, indent);
  syntaqlite_vec_push(&buf, '\0', p->mem);
  return buf.data;
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

// Free a single macro entry's owned strings.
static void free_macro_entry(SyntaqliteParser* p, SyntaqliteMacroEntry* e) {
  p->mem.xFree(e->name);
  p->mem.xFree(e->body);
  if (e->param_names) {
    for (uint32_t i = 0; i < e->param_count; i++)
      p->mem.xFree(e->param_names[i]);
    p->mem.xFree(e->param_names);
    p->mem.xFree(e->param_name_lens);
  }
  e->name = NULL;
  e->body = NULL;
  e->param_names = NULL;
  e->param_name_lens = NULL;
  e->state = SYNQ_MAP_EMPTY;
}

SYNTAQLITE_API void syntaqlite_parser_destroy(SyntaqliteParser* p) {
  if (p) {
    SYNQ_PARSER_FREE(p->grammar.tmpl, p->lemon, p->mem.xFree);
    synq_parse_ctx_free(&p->ctx);
    syntaqlite_vec_free(&p->comments, p->mem);
    syntaqlite_vec_free(&p->tokens, p->mem);
    syntaqlite_vec_free(&p->macros, p->mem);
    // Free owned expansion buffers.
    for (uint32_t i = 0; i < syntaqlite_vec_len(&p->owned_bufs); i++)
      p->mem.xFree(p->owned_bufs.data[i].data);
    syntaqlite_vec_free(&p->owned_bufs, p->mem);
    // Free macro registry.
    if (p->macro_table) {
      for (uint32_t i = 0; i < p->macro_table_size; i++) {
        if (p->macro_table[i].state == SYNQ_MAP_LIVE)
          free_macro_entry(p, &p->macro_table[i]);
      }
      p->mem.xFree(p->macro_table);
    }
    p->mem.xFree(p);
  }
}

// ---------------------------------------------------------------------------
// Reading results
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
// Configuration
// ---------------------------------------------------------------------------

SYNTAQLITE_API int32_t syntaqlite_parser_set_trace(SyntaqliteParser* p,
                                                   uint32_t enable) {
  if (p->sealed)
    return -1;
  p->trace = enable;
  if (enable) {
    SYNQ_PARSER_TRACE(p->grammar.tmpl, stderr, "parser> ");
  } else {
    SYNQ_PARSER_TRACE(p->grammar.tmpl, NULL, NULL);
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

// ---------------------------------------------------------------------------
// Macro registration API
// ---------------------------------------------------------------------------

// Helper: duplicate a string via the parser's allocator.
static char* synq_strdup(SyntaqliteMemMethods mem,
                         const char* s,
                         uint32_t len) {
  char* d = mem.xMalloc(len + 1);
  memcpy(d, s, len);
  d[len] = '\0';
  return d;
}

SYNTAQLITE_API int syntaqlite_parser_register_macro(
    SyntaqliteParser* p,
    const char* name,
    uint32_t name_len,
    const char* const* param_names,
    uint32_t param_count,
    const char* body,
    uint32_t body_len) {
  SyntaqliteMacroEntry* slot;
  SYNQ_MAP_INSERT(p->macro_table, p->macro_table_size, p->macro_table_count,
                  name, name_len, p->mem, SYNQ_MACRO_TABLE_INITIAL_SIZE, slot);

  // If the slot already has a live entry, free old data first.
  if (slot->name) {
    free_macro_entry(p, slot);
    slot->state = SYNQ_MAP_LIVE;
  }

  slot->name = synq_strdup(p->mem, name, name_len);
  slot->name_len = name_len;
  slot->body = synq_strdup(p->mem, body, body_len);
  slot->body_len = body_len;
  slot->param_count = param_count;

  if (param_count > 0) {
    slot->param_names = p->mem.xMalloc(param_count * sizeof(char*));
    slot->param_name_lens = p->mem.xMalloc(param_count * sizeof(uint32_t));
    for (uint32_t i = 0; i < param_count; i++) {
      uint32_t plen = (uint32_t)strlen(param_names[i]);
      slot->param_names[i] = synq_strdup(p->mem, param_names[i], plen);
      slot->param_name_lens[i] = plen;
    }
  } else {
    slot->param_names = NULL;
    slot->param_name_lens = NULL;
  }

  return 0;
}

SYNTAQLITE_API int syntaqlite_parser_deregister_macro(SyntaqliteParser* p,
                                                      const char* name,
                                                      uint32_t name_len) {
  SyntaqliteMacroEntry* entry;
  SYNQ_MAP_FIND(p->macro_table, p->macro_table_size, name, name_len, entry);
  if (!entry)
    return -1;
  free_macro_entry(p, entry);
  entry->state = SYNQ_MAP_TOMBSTONE;
  p->macro_table_count--;
  return 0;
}
