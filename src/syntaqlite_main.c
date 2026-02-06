// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Combined syntaqlite CLI tool.
// Usage: syntaqlite <command> [options] < input.sql
//
// Commands:
//   ast   Parse SQL and print AST tree
//   fmt   Parse SQL and print formatted output
//   doc   Parse SQL and print document IR tree (debug)
//
// Also built as `synqfmt` which defaults to fmt mode (no subcommand needed).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/base/synq_getopt.h"
#include "syntaqlite/formatter.h"
#include "syntaqlite/parser.h"

enum synq_mode {
  SYNQ_MODE_AST,
  SYNQ_MODE_FMT,
  SYNQ_MODE_DOC,
};

static char* read_stdin(size_t* out_len) {
  size_t capacity = 4096;
  size_t len = 0;
  char* buf = (char*)malloc(capacity);
  if (!buf)
    return NULL;

  int c;
  while ((c = getchar()) != EOF) {
    if (len + 1 >= capacity) {
      capacity *= 2;
      char* new_buf = (char*)realloc(buf, capacity);
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

static void print_usage(const char* prog, int is_synqfmt) {
  if (is_synqfmt) {
    fprintf(stdout,
            "Usage: %s [options] < input.sql\n"
            "\n"
            "Reads SQL from stdin and prints formatted output.\n"
            "\n"
            "Options:\n"
            "  --trace      Enable Lemon parser trace on stderr (debug builds "
            "only)\n"
            "  --width=N    Target line width (default: 80)\n"
            "  --help       Show this help message and exit\n",
            prog);
  } else {
    fprintf(stdout,
            "Usage: %s <command> [options] < input.sql\n"
            "\n"
            "Reads SQL from stdin and processes it.\n"
            "\n"
            "Commands:\n"
            "  ast       Parse SQL and print AST tree\n"
            "  fmt       Parse SQL and print formatted output\n"
            "  doc       Parse SQL and print document IR tree (debug)\n"
            "\n"
            "Options:\n"
            "  --trace      Enable Lemon parser trace on stderr (debug builds "
            "only)\n"
            "  --width=N    Target line width (default: 80, applies to "
            "fmt/doc)\n"
            "  --help       Show this help message and exit\n",
            prog);
  }
}

int main(int argc, char** argv) {
  const char* prog = argv[0];

  // Detect basename to check if invoked as synqfmt
  const char* basename = prog;
  const char* slash = strrchr(prog, '/');
  if (slash)
    basename = slash + 1;
#ifdef _WIN32
  const char* bslash = strrchr(basename, '\\');
  if (bslash)
    basename = bslash + 1;
#endif
  int is_synqfmt = strcmp(basename, "synqfmt") == 0;

  enum synq_mode mode;

  if (is_synqfmt) {
    // synqfmt defaults to fmt mode, no subcommand needed
    mode = SYNQ_MODE_FMT;
  } else {
    if (argc < 2) {
      print_usage(prog, 0);
      return 1;
    }

    // Handle --help before subcommand
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
      print_usage(prog, 0);
      return 0;
    }

    // Parse subcommand
    if (strcmp(argv[1], "ast") == 0) {
      mode = SYNQ_MODE_AST;
    } else if (strcmp(argv[1], "fmt") == 0) {
      mode = SYNQ_MODE_FMT;
    } else if (strcmp(argv[1], "doc") == 0) {
      mode = SYNQ_MODE_DOC;
    } else {
      fprintf(stderr, "Unknown command: %s\n", argv[1]);
      fprintf(stderr, "Try '%s --help' for usage information.\n", prog);
      return 1;
    }

    // Shift argv past subcommand for option parsing
    argc--;
    argv++;
  }

  int trace = 0;
  uint32_t width = 80;

  static const struct synq_option long_options[] = {
      {"trace", SYNQ_NO_ARGUMENT, NULL, 'T'},
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
      case 'w':
        width = (uint32_t)atoi(opt.arg);
        break;
      case 'h':
        print_usage(prog, is_synqfmt);
        return 0;
      default:
        fprintf(stderr, "Try '%s --help' for usage information.\n", prog);
        return 1;
    }
  }

  // Read input
  size_t len;
  char* sql = read_stdin(&len);
  if (!sql) {
    fprintf(stderr, "Error: Failed to read input\n");
    return 1;
  }

  if (len == 0) {
    free(sql);
    return 0;
  }

  // Create parser (enable token collection for fmt/doc)
  int collect = (mode == SYNQ_MODE_FMT || mode == SYNQ_MODE_DOC);
  SyntaqliteParser* parser = syntaqlite_parser_create(
      (SyntaqliteParserConfig){.collect_tokens = collect, .trace = trace});
  if (!parser) {
    fprintf(stderr, "Error: Failed to allocate parser\n");
    free(sql);
    return 1;
  }
  syntaqlite_parser_reset(parser, sql, (uint32_t)len);

  // Parse and process each statement
  SyntaqliteParseResult result;
  while ((result = syntaqlite_parser_next(parser)).root !=
         SYNTAQLITE_NULL_NODE) {
    if (result.error) {
      fprintf(stderr, "Error: %s\n",
              result.error_msg ? result.error_msg : "Parse error");
      syntaqlite_parser_destroy(parser);
      free(sql);
      return 1;
    }

    switch (mode) {
      case SYNQ_MODE_AST:
        syntaqlite_ast_print(parser, result.root, stdout);
        break;
      case SYNQ_MODE_FMT: {
        SyntaqliteFormatOptions fmt_opts = {.target_width = width,
                                            .indent_width = 2};
        char* formatted =
            syntaqlite_format_stmt(parser, result.root, &fmt_opts);
        if (formatted) {
          fputs(formatted, stdout);
          free(formatted);
        }
        break;
      }
      case SYNQ_MODE_DOC: {
        SyntaqliteFormatOptions fmt_opts = {.target_width = width,
                                            .indent_width = 2};
        char* formatted =
            syntaqlite_format_stmt_debug_ir(parser, result.root, &fmt_opts);
        if (formatted) {
          fputs(formatted, stdout);
          free(formatted);
        }
        break;
      }
    }
  }

  if (result.error) {
    fprintf(stderr, "Error: %s\n",
            result.error_msg ? result.error_msg : "Parse error");
    syntaqlite_parser_destroy(parser);
    free(sql);
    return 1;
  }

  syntaqlite_parser_destroy(parser);
  free(sql);
  return 0;
}
