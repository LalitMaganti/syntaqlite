// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Test binary for formatter diff tests.
// Usage: fmt_test [--trace] [--debug-ir] [--width=N] [--help] < input.sql
//
// Reads SQL from stdin, parses it, formats it, and prints the result.
// Use --trace to enable Lemon parser tracing on stderr.
// Use --debug-ir to print the doc IR tree instead of formatted output.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/sq_getopt.h"
#include "src/synq_sqlite_defs.h"

// External definition for inline function (C99/C11).
extern inline int synq_getopt_long(struct synq_getopt_state *st, int argc,
    char *const *argv, const char *shortopts,
    const struct synq_option *longopts, int *longindex);
#include "src/sqlite_tokens.h"
#include "src/token_list.h"
#include "src/ast/ast_base.h"
#include "src/fmt/fmt.h"
#include "src/fmt/fmt_options.h"

// External parser functions (defined in sqlite_parse.c)
void *synq_sqlite3ParserAlloc(void *(*mallocProc)(size_t),
                                     SynqParseContext *pCtx);
void synq_sqlite3Parser(void *parser, int tokenType, SynqToken token,
                               SynqParseContext *pCtx);
void synq_sqlite3ParserFree(void *parser, void (*freeProc)(void *));
#ifndef NDEBUG
void synq_sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt);
#endif

// External tokenizer function (defined in sqlite_tokenize.c)
i64 synq_sqlite3GetToken(const unsigned char *z, int *tokenType);

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
static void on_syntax_error(SynqParseContext *ctx) {
  (void)ctx;
  g_syntax_error = 1;
}

static void on_stack_overflow(SynqParseContext *ctx) {
  (void)ctx;
  fprintf(stderr, "Error: Parser stack overflow\n");
}

int main(int argc, char **argv) {
  int trace = 0;
  int debug_ir = 0;
  uint32_t width = 80;

  static const struct synq_option long_options[] = {
      {"trace", SYNQ_NO_ARGUMENT, NULL, 'T'},
      {"debug-ir", SYNQ_NO_ARGUMENT, NULL, 'D'},
      {"width", SYNQ_REQUIRED_ARGUMENT, NULL, 'w'},
      {"help", SYNQ_NO_ARGUMENT, NULL, 'h'},
      {NULL, 0, NULL, 0},
  };

  struct synq_getopt_state opt = SYNQ_GETOPT_INIT;
  int c;
  while ((c = synq_getopt_long(&opt, argc, argv, "hw:", long_options, NULL)) !=
         -1) {
    switch (c) {
      case 'T':
        trace = 1;
        break;
      case 'D':
        debug_ir = 1;
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
                "  --trace      Enable Lemon parser trace output on stderr (debug builds only)\n"
                "  --debug-ir   Print doc IR tree instead of formatted output\n"
                "  --width=N    Target line width (default: 80)\n"
                "  --help       Show this help message and exit\n",
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
  SynqAstContext astCtx;
  synq_ast_context_init(&astCtx, sql, (uint32_t)len);

  // Initialize token list
  SynqTokenList tokenList;
  synq_vec_init(&tokenList);

  // Initialize parse context
  SynqParseContext parseCtx;
  parseCtx.userData = NULL;
  parseCtx.onSyntaxError = on_syntax_error;
  parseCtx.onStackOverflow = on_stack_overflow;
  parseCtx.astCtx = &astCtx;
  parseCtx.zSql = sql;
  parseCtx.root = SYNQ_NULL_NODE;
  parseCtx.token_list = &tokenList;

  // Create parser
  void *parser = synq_sqlite3ParserAlloc(malloc, &parseCtx);
  if (!parser) {
    fprintf(stderr, "Error: Failed to allocate parser\n");
    synq_vec_free(&tokenList);
    synq_ast_context_cleanup(&astCtx);

    free(sql);
    return 1;
  }

#ifndef NDEBUG
  if (trace) {
    synq_sqlite3ParserTrace(stderr, "PARSER: ");
  }
#else
  (void)trace;
#endif

  // Tokenize and parse
  const unsigned char *z = (const unsigned char *)sql;
  int tokenType;
  int lastTokenType = 0;

  while (*z) {
    i64 tokenLen = synq_sqlite3GetToken(z, &tokenType);
    if (tokenLen <= 0) break;

    // Collect all tokens (including whitespace and comments)
    synq_token_list_append(&tokenList,
                                 (uint32_t)((const char *)z - sql),
                                 (uint16_t)tokenLen, (uint16_t)tokenType);

    // Skip whitespace and comments for parser
    if (tokenType == TK_SPACE || tokenType == TK_COMMENT) {
      z += tokenLen;
      continue;
    }

    // Feed token to parser
    SynqToken token;
    token.z = (const char *)z;
    token.n = (int)tokenLen;
    token.type = tokenType;

    synq_sqlite3Parser(parser, tokenType, token, &parseCtx);
    lastTokenType = tokenType;

    if (g_syntax_error) {
      fprintf(stderr, "Error: Syntax error near '%.*s'\n", (int)tokenLen, z);
      synq_sqlite3ParserFree(parser, free);
      synq_vec_free(&tokenList);
      synq_ast_context_cleanup(&astCtx);
  
      free(sql);
      return 1;
    }

    z += tokenLen;
  }

  // If the statement didn't end with SEMI, synthesize one
  if (lastTokenType != TK_SEMI) {
    SynqToken semiToken = {NULL, 0, TK_SEMI};
    synq_sqlite3Parser(parser, TK_SEMI, semiToken, &parseCtx);
  }

  // Send end-of-input (token 0)
  SynqToken endToken = {NULL, 0, 0};
  synq_sqlite3Parser(parser, 0, endToken, &parseCtx);

  // Flush any pending list accumulator before reading the AST
  synq_ast_list_flush(&astCtx);

  // Check for syntax error
  if (g_syntax_error) {
    fprintf(stderr, "Error: Incomplete statement\n");
    synq_sqlite3ParserFree(parser, free);
    synq_vec_free(&tokenList);
    synq_ast_context_cleanup(&astCtx);

    free(sql);
    return 1;
  }

  // Get root from parse context
  uint32_t root_id = parseCtx.root;

  if (root_id == SYNQ_NULL_NODE) {
    synq_sqlite3ParserFree(parser, free);
    synq_vec_free(&tokenList);
    synq_ast_context_cleanup(&astCtx);

    free(sql);
    return 0;
  }

  // Format and print
  SynqFmtOptions fmtOpts = {.target_width = width, .indent_width = 2};
  char *formatted;
  if (debug_ir) {
    formatted = synq_format_debug_ir(&astCtx, root_id, sql, &tokenList, &fmtOpts);
  } else {
    formatted = synq_format(&astCtx, root_id, sql, &tokenList, &fmtOpts);
  }
  if (formatted) {
    fputs(formatted, stdout);
    free(formatted);
  }

  // Cleanup
  synq_sqlite3ParserFree(parser, free);
  synq_vec_free(&tokenList);
  synq_ast_context_cleanup(&astCtx);
  free(sql);

  return 0;
}
