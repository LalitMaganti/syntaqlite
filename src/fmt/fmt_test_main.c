// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Test binary for formatter diff tests.
// Usage: fmt_test [--trace] [--width=N] [--help] < input.sql
//
// Reads SQL from stdin, parses it, formats it, and prints the result.
// Use --trace to enable Lemon parser tracing on stderr.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/sq_getopt.h"
#include "src/syntaqlite_sqlite_defs.h"
#include "src/sqlite_tokens.h"
#include "src/token_list.h"
#include "src/ast/ast_base.h"
#include "src/fmt/fmt.h"
#include "src/fmt/fmt_options.h"

// External parser functions (defined in sqlite_parse.c)
void *syntaqlite_sqlite3ParserAlloc(void *(*mallocProc)(size_t),
                                     SyntaqliteParseContext *pCtx);
void syntaqlite_sqlite3Parser(void *parser, int tokenType, SyntaqliteToken token,
                               SyntaqliteParseContext *pCtx);
void syntaqlite_sqlite3ParserFree(void *parser, void (*freeProc)(void *));
#ifndef NDEBUG
void syntaqlite_sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt);
#endif

// External tokenizer function (defined in sqlite_tokenize.c)
i64 syntaqlite_sqlite3GetToken(const unsigned char *z, int *tokenType);

// Read entire stdin into a buffer
static char *read_stdin(size_t *out_len) {
  size_t capacity = 4096;
  size_t len = 0;
  char *buf = (char *)malloc(capacity);
  if (!buf) return NULL;

  int c;
  while ((c = getchar()) != EOF) {
    if (len + 1 >= capacity) {
      capacity *= 2;
      char *new_buf = (char *)realloc(buf, capacity);
      if (!new_buf) {
        free(buf);
        return NULL;
      }
      buf = new_buf;
    }
    buf[len++] = (char)c;
  }
  buf[len] = '\0';
  *out_len = len;
  return buf;
}

// Error handlers
static int g_syntax_error = 0;
static void on_syntax_error(SyntaqliteParseContext *ctx) {
  (void)ctx;
  g_syntax_error = 1;
}

static void on_stack_overflow(SyntaqliteParseContext *ctx) {
  (void)ctx;
  fprintf(stderr, "Error: Parser stack overflow\n");
}

int main(int argc, char **argv) {
  int trace = 0;
  uint32_t width = 80;

  static const struct sq_option long_options[] = {
      {"trace", SQ_NO_ARGUMENT, NULL, 'T'},
      {"width", SQ_REQUIRED_ARGUMENT, NULL, 'w'},
      {"help", SQ_NO_ARGUMENT, NULL, 'h'},
      {NULL, 0, NULL, 0},
  };

  struct sq_getopt_state opt = SQ_GETOPT_INIT;
  int c;
  while ((c = sq_getopt_long(&opt, argc, argv, "hw:", long_options, NULL)) !=
         -1) {
    switch (c) {
      case 'T':
        trace = 1;
        break;
      case 'w':
        width = (uint32_t)atoi(opt.arg);
        break;
      case 'h':
        fprintf(stdout,
                "Usage: %s [OPTIONS] < input.sql\n"
                "\n"
                "Reads SQL from stdin, formats it, and prints the result.\n"
                "\n"
                "Options:\n"
                "  --trace    Enable Lemon parser trace output on stderr (debug builds only)\n"
                "  --width=N  Target line width (default: 80)\n"
                "  --help     Show this help message and exit\n",
                argv[0]);
        return 0;
      default:
        fprintf(stderr, "Try '%s --help' for usage information.\n", argv[0]);
        return 1;
    }
  }

  // Read input
  size_t len;
  char *sql = read_stdin(&len);
  if (!sql) {
    fprintf(stderr, "Error: Failed to read input\n");
    return 1;
  }

  if (len == 0) {
    free(sql);
    return 0;
  }

  // Initialize AST context
  SyntaqliteAstContext astCtx;
  syntaqlite_ast_context_init(&astCtx, sql, (uint32_t)len);

  // Initialize token list
  SyntaqliteTokenList tokenList;
  syntaqlite_vec_init(&tokenList);

  // Initialize parse context
  SyntaqliteParseContext parseCtx;
  parseCtx.userData = NULL;
  parseCtx.onSyntaxError = on_syntax_error;
  parseCtx.onStackOverflow = on_stack_overflow;
  parseCtx.astCtx = &astCtx;
  parseCtx.zSql = sql;
  parseCtx.root = SYNTAQLITE_NULL_NODE;
  parseCtx.token_list = &tokenList;

  // Create parser
  void *parser = syntaqlite_sqlite3ParserAlloc(malloc, &parseCtx);
  if (!parser) {
    fprintf(stderr, "Error: Failed to allocate parser\n");
    syntaqlite_vec_free(&tokenList);
    syntaqlite_ast_context_cleanup(&astCtx);

    free(sql);
    return 1;
  }

#ifndef NDEBUG
  if (trace) {
    syntaqlite_sqlite3ParserTrace(stderr, "PARSER: ");
  }
#else
  (void)trace;
#endif

  // Tokenize and parse
  const unsigned char *z = (const unsigned char *)sql;
  int tokenType;
  int lastTokenType = 0;

  while (*z) {
    i64 tokenLen = syntaqlite_sqlite3GetToken(z, &tokenType);
    if (tokenLen <= 0) break;

    // Collect all tokens (including whitespace and comments)
    syntaqlite_token_list_append(&tokenList,
                                 (uint32_t)((const char *)z - sql),
                                 (uint16_t)tokenLen, (uint16_t)tokenType);

    // Skip whitespace and comments for parser
    if (tokenType == TK_SPACE || tokenType == TK_COMMENT) {
      z += tokenLen;
      continue;
    }

    // Feed token to parser
    SyntaqliteToken token;
    token.z = (const char *)z;
    token.n = (int)tokenLen;
    token.type = tokenType;

    syntaqlite_sqlite3Parser(parser, tokenType, token, &parseCtx);
    lastTokenType = tokenType;

    if (g_syntax_error) {
      fprintf(stderr, "Error: Syntax error near '%.*s'\n", (int)tokenLen, z);
      syntaqlite_sqlite3ParserFree(parser, free);
      syntaqlite_vec_free(&tokenList);
      syntaqlite_ast_context_cleanup(&astCtx);
  
      free(sql);
      return 1;
    }

    z += tokenLen;
  }

  // If the statement didn't end with SEMI, synthesize one
  if (lastTokenType != TK_SEMI) {
    SyntaqliteToken semiToken = {NULL, 0, TK_SEMI};
    syntaqlite_sqlite3Parser(parser, TK_SEMI, semiToken, &parseCtx);
  }

  // Send end-of-input (token 0)
  SyntaqliteToken endToken = {NULL, 0, 0};
  syntaqlite_sqlite3Parser(parser, 0, endToken, &parseCtx);

  // Flush any pending list accumulator before reading the AST
  ast_list_flush(&astCtx);

  // Check for syntax error
  if (g_syntax_error) {
    fprintf(stderr, "Error: Incomplete statement\n");
    syntaqlite_sqlite3ParserFree(parser, free);
    syntaqlite_vec_free(&tokenList);
    syntaqlite_ast_context_cleanup(&astCtx);

    free(sql);
    return 1;
  }

  // Get root from parse context
  uint32_t root_id = parseCtx.root;

  if (root_id == SYNTAQLITE_NULL_NODE) {
    syntaqlite_sqlite3ParserFree(parser, free);
    syntaqlite_vec_free(&tokenList);
    syntaqlite_ast_context_cleanup(&astCtx);

    free(sql);
    return 0;
  }

  // Format and print
  SyntaqliteFmtOptions fmtOpts = {.target_width = width, .indent_width = 2};
  char *formatted = syntaqlite_format(&astCtx.ast, root_id, sql, &tokenList, &fmtOpts);
  if (formatted) {
    fputs(formatted, stdout);
    free(formatted);
  }

  // Cleanup
  syntaqlite_sqlite3ParserFree(parser, free);
  syntaqlite_vec_free(&tokenList);
  syntaqlite_ast_context_cleanup(&astCtx);
  free(sql);

  return 0;
}
