// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Test binary for AST diff tests.
// Usage: ast_test < input.sql
//
// Reads SQL from stdin, parses it using the real parser, and prints the AST.

#include "src/ast/ast_builder.h"
#include "src/ast/ast_print.h"
#include "src/syntaqlite_sqlite_defs.h"
#include "src/sqlite_tokens.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External parser functions (defined in sqlite_parse.c)
void *syntaqlite_sqlite3ParserAlloc(void *(*mallocProc)(size_t),
                                     SyntaqliteParseContext *pCtx);
void syntaqlite_sqlite3Parser(void *parser, int tokenType, SyntaqliteToken token,
                               SyntaqliteParseContext *pCtx);
void syntaqlite_sqlite3ParserFree(void *parser, void (*freeProc)(void *));

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

int main(void) {
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

  // Initialize AST
  SyntaqliteAst ast;
  syntaqlite_ast_init(&ast);

  SyntaqliteAstContext astCtx;
  astCtx.ast = &ast;
  astCtx.source = sql;
  astCtx.source_length = (uint32_t)len;
  astCtx.error_code = 0;
  astCtx.error_msg = NULL;

  // Initialize parse context
  SyntaqliteParseContext parseCtx;
  parseCtx.userData = NULL;
  parseCtx.onSyntaxError = on_syntax_error;
  parseCtx.onStackOverflow = on_stack_overflow;
  parseCtx.astCtx = &astCtx;
  parseCtx.zSql = sql;
  parseCtx.root = SYNTAQLITE_NULL_NODE;

  // Create parser
  void *parser = syntaqlite_sqlite3ParserAlloc(malloc, &parseCtx);
  if (!parser) {
    fprintf(stderr, "Error: Failed to allocate parser\n");
    syntaqlite_ast_free(&ast);
    free(sql);
    return 1;
  }

  // Tokenize and parse
  // Following Perfetto's approach: synthesize SEMI at EOF if needed
  const unsigned char *z = (const unsigned char *)sql;
  int tokenType;
  int lastTokenType = 0;

  while (*z) {
    i64 tokenLen = syntaqlite_sqlite3GetToken(z, &tokenType);
    if (tokenLen <= 0) break;

    // Skip whitespace and comments
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
      syntaqlite_ast_free(&ast);
      free(sql);
      return 1;
    }

    z += tokenLen;
  }

  // If the statement didn't end with SEMI, synthesize one (like Perfetto does)
  if (lastTokenType != TK_SEMI) {
    SyntaqliteToken semiToken = {NULL, 0, TK_SEMI};
    syntaqlite_sqlite3Parser(parser, TK_SEMI, semiToken, &parseCtx);
  }

  // Send end-of-input (token 0)
  SyntaqliteToken endToken = {NULL, 0, 0};
  syntaqlite_sqlite3Parser(parser, 0, endToken, &parseCtx);

  // Check for syntax error
  if (g_syntax_error) {
    fprintf(stderr, "Error: Incomplete statement\n");
    syntaqlite_sqlite3ParserFree(parser, free);
    syntaqlite_ast_free(&ast);
    free(sql);
    return 1;
  }

  // Get root from parse context
  uint32_t root_id = parseCtx.root;

  if (root_id == SYNTAQLITE_NULL_NODE) {
    // No AST produced (empty input or semicolon only)
    syntaqlite_sqlite3ParserFree(parser, free);
    syntaqlite_ast_free(&ast);
    free(sql);
    return 0;
  }

  // Print AST
  syntaqlite_ast_print(stdout, &ast, root_id, sql);

  // Cleanup
  syntaqlite_sqlite3ParserFree(parser, free);
  syntaqlite_ast_free(&ast);
  free(sql);

  return 0;
}
