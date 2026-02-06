// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/parser.h"

#include "src/parser/ast_nodes_gen.h"
#include "src/parser/ast_print.h"
#include "src/parser/parser.h"
#include "src/parser/sqlite_parse_gen.h"
#include "src/tokenizer/sqlite_tokenize_gen.h"
#include "syntaqlite/sqlite_tokens_gen.h"

#include <stdlib.h>
#include <string.h>

static void on_syntax_error(SynqParseContext* ctx) {
  SyntaqliteParser* p = (SyntaqliteParser*)ctx->userData;
  p->had_error = 1;
}

static void on_stack_overflow(SynqParseContext* ctx) {
  SyntaqliteParser* p = (SyntaqliteParser*)ctx->userData;
  p->had_error = 1;
  snprintf(p->error_msg, sizeof(p->error_msg), "Parser stack overflow");
}

SyntaqliteParser* syntaqlite_parser_create(SyntaqliteParserConfig config) {
  SyntaqliteParser* p = (SyntaqliteParser*)calloc(1, sizeof(SyntaqliteParser));
  if (!p)
    return NULL;

  // Initialize AST context to valid empty state (reset will reinit)
  synq_ast_context_init(&p->astCtx, NULL, 0);

  // Token collection
  if (config.collect_tokens) {
    synq_vec_init(&p->token_list_storage);
    p->token_list = &p->token_list_storage;
  } else {
    p->token_list = NULL;
  }

  // Initialize parse context (stable pointers that survive reset)
  p->parseCtx.userData = p;
  p->parseCtx.onSyntaxError = on_syntax_error;
  p->parseCtx.onStackOverflow = on_stack_overflow;
  p->parseCtx.astCtx = &p->astCtx;
  p->parseCtx.token_list = p->token_list;

  // Create Lemon parser
  p->lemon = synq_sqlite3ParserAlloc(malloc, &p->parseCtx);
  if (!p->lemon) {
    if (p->token_list)
      synq_vec_free(&p->token_list_storage);
    free(p);
    return NULL;
  }

  // Parser tracing (debug builds only)
#ifndef NDEBUG
  if (config.trace) {
    synq_sqlite3ParserTrace(stderr, "PARSER: ");
  }
#endif

  return p;
}

void syntaqlite_parser_reset(SyntaqliteParser* p,
                             const char* source,
                             uint32_t len) {
  // Clean up previous parse state
  synq_ast_context_cleanup(&p->astCtx);
  synq_sqlite3ParserFinalize(p->lemon);

  // Reset source
  p->source = source;
  p->length = len;
  p->pos = 0;
  p->last_token_type = 0;
  p->had_error = 0;
  p->finished = 0;
  p->error_msg[0] = '\0';

  // Re-initialize AST context with new source
  synq_ast_context_init(&p->astCtx, source, len);

  // Reset token list
  if (p->token_list) {
    synq_vec_clear(&p->token_list_storage);
  }

  // Reset parse context per-statement state
  p->parseCtx.zSql = source;
  p->parseCtx.root = SYNTAQLITE_NULL_NODE;
  p->parseCtx.stmt_completed = 0;

  // Re-initialize Lemon parser (reuse allocation)
  synq_sqlite3ParserInit(p->lemon, &p->parseCtx);
}

SyntaqliteParseResult syntaqlite_parser_next(SyntaqliteParser* p) {
  SyntaqliteParseResult result = {SYNTAQLITE_NULL_NODE, 0, NULL};

  if (p->finished || p->had_error) {
    result.error = p->had_error;
    result.error_msg = p->had_error ? p->error_msg : NULL;
    return result;
  }

  // Reset statement state
  p->parseCtx.stmt_completed = 0;
  p->parseCtx.root = SYNTAQLITE_NULL_NODE;

  const unsigned char* z = (const unsigned char*)p->source;

  while (p->pos < p->length && z[p->pos] != '\0') {
    int tokenType;
    i64 tokenLen = synq_sqlite3GetToken(z + p->pos, &tokenType);
    if (tokenLen <= 0)
      break;

    // Collect token if enabled
    if (p->token_list) {
      synq_token_list_append(p->token_list, p->pos, (uint16_t)tokenLen,
                             (uint16_t)tokenType);
    }

    // Skip whitespace and comments for parser.
    // NOTE: Upstream SQLite uses `tokenType >= TK_SPACE` as a range check
    // optimization, which requires SPACE/COMMENT/ILLEGAL to have the highest
    // token IDs. We intentionally use explicit equality checks instead, so
    // that token ID ordering is not a correctness constraint. This allows
    // dialect extensions to add tokens without worrying about ID ranges.
    if (tokenType == SYNTAQLITE_TOKEN_SPACE ||
        tokenType == SYNTAQLITE_TOKEN_COMMENT) {
      p->pos += (uint32_t)tokenLen;
      continue;
    }

    // Feed token to parser
    SynqToken token;
    token.z = (const char*)(z + p->pos);
    token.n = (int)tokenLen;
    token.type = tokenType;

    synq_sqlite3Parser(p->lemon, tokenType, token);
    p->last_token_type = tokenType;
    p->pos += (uint32_t)tokenLen;

    if (p->had_error) {
      if (p->error_msg[0] == '\0') {
        snprintf(p->error_msg, sizeof(p->error_msg), "Syntax error near '%.*s'",
                 (int)tokenLen, token.z);
      }
      result.error = 1;
      result.error_msg = p->error_msg;
      return result;
    }

    // Check if a statement was completed by the grammar
    if (p->parseCtx.stmt_completed) {
      p->parseCtx.stmt_completed = 0;
      uint32_t root = p->parseCtx.root;
      if (root != SYNTAQLITE_NULL_NODE) {
        result.root = root;
        return result;
      }
      // Bare semicolon — skip and continue
      p->parseCtx.root = SYNTAQLITE_NULL_NODE;
    }
  }

  // Reached end of input. Synthesize SEMI if last token wasn't one.
  if (p->last_token_type != 0 && p->last_token_type != SYNTAQLITE_TOKEN_SEMI) {
    SynqToken semiToken = {NULL, 0, SYNTAQLITE_TOKEN_SEMI};
    synq_sqlite3Parser(p->lemon, SYNTAQLITE_TOKEN_SEMI, semiToken);

    if (p->had_error) {
      if (p->error_msg[0] == '\0') {
        snprintf(p->error_msg, sizeof(p->error_msg), "Incomplete statement");
      }
      result.error = 1;
      result.error_msg = p->error_msg;
      return result;
    }

    if (p->parseCtx.stmt_completed &&
        p->parseCtx.root != SYNTAQLITE_NULL_NODE) {
      result.root = p->parseCtx.root;
      p->finished = 1;
      return result;
    }
  }

  // Send end-of-input to finalize parser
  SynqToken endToken = {NULL, 0, 0};
  synq_sqlite3Parser(p->lemon, 0, endToken);
  p->finished = 1;

  if (p->had_error) {
    if (p->error_msg[0] == '\0') {
      snprintf(p->error_msg, sizeof(p->error_msg), "Incomplete statement");
    }
    result.error = 1;
    result.error_msg = p->error_msg;
    return result;
  }

  // Check if the final reduction produced a root
  if (p->parseCtx.root != SYNTAQLITE_NULL_NODE) {
    result.root = p->parseCtx.root;
  }

  return result;
}

const SyntaqliteNode* syntaqlite_parser_node(SyntaqliteParser* p,
                                             uint32_t node_id) {
  return synq_ast_node(&p->astCtx.ast, node_id);
}

const char* syntaqlite_parser_source(SyntaqliteParser* p) {
  return p->source;
}

uint32_t syntaqlite_parser_source_length(SyntaqliteParser* p) {
  return p->length;
}

void syntaqlite_ast_print(SyntaqliteParser* p, uint32_t node_id, FILE* out) {
  synq_ast_print(out, &p->astCtx.ast, node_id, p->source);
}

void syntaqlite_parser_destroy(SyntaqliteParser* p) {
  if (!p)
    return;
  synq_sqlite3ParserFree(p->lemon, free);
  synq_ast_context_cleanup(&p->astCtx);
  if (p->token_list)
    synq_vec_free(&p->token_list_storage);
  free(p);
}
