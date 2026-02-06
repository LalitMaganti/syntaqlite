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

#include "src/base/synq_getopt.h"
#include "src/fmt/fmt.h"
#include "src/fmt/fmt_options.h"
#include "src/parser_internal.h"

#ifndef NDEBUG
void synq_sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt);
#endif

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

#ifndef NDEBUG
  if (trace) {
    synq_sqlite3ParserTrace(stderr, "PARSER: ");
  }
#else
  (void)trace;
#endif

  // Create parser with token collection enabled
  SyntaqliteParserConfig config = {.collect_tokens = 1};
  SyntaqliteParser *parser = syntaqlite_parser_create(sql, (uint32_t)len, &config);
  if (!parser) {
    fprintf(stderr, "Error: Failed to allocate parser\n");
    free(sql);
    return 1;
  }

  // Parse and format each statement
  SyntaqliteParseResult result;
  while ((result = syntaqlite_parser_next(parser)).root != SYNQ_NULL_NODE) {
    if (result.error) {
      fprintf(stderr, "Error: %s\n", result.error_msg ? result.error_msg : "Parse error");
      syntaqlite_parser_destroy(parser);
      free(sql);
      return 1;
    }

    SynqFmtOptions fmtOpts = {.target_width = width, .indent_width = 2};
    SynqAstContext *astCtx = syntaqlite_parser_ast_context(parser);
    SynqTokenList *tokenList = syntaqlite_parser_token_list(parser);
    char *formatted;
    if (debug_ir) {
      formatted = synq_format_debug_ir(astCtx, result.root, sql, tokenList, &fmtOpts);
    } else {
      formatted = synq_format(astCtx, result.root, sql, tokenList, &fmtOpts);
    }
    if (formatted) {
      fputs(formatted, stdout);
      free(formatted);
    }
  }

  if (result.error) {
    fprintf(stderr, "Error: %s\n", result.error_msg ? result.error_msg : "Parse error");
    syntaqlite_parser_destroy(parser);
    free(sql);
    return 1;
  }

  // Cleanup
  syntaqlite_parser_destroy(parser);
  free(sql);

  return 0;
}
