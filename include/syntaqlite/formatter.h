// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQL formatter: pretty-prints SQL statements.
//
// This is the highest-level entry point. For most users, syntaqlite_format()
// is all you need — pass in SQL text, get back formatted SQL.
//
// Simple API (string in, string out):
//   char *formatted = syntaqlite_format(sql, strlen(sql), NULL);
//   // use formatted...
//   free(formatted);
//
// If you already have a parsed AST (e.g. for selective formatting), use
// syntaqlite_format_stmt() instead. The parser must have been created with
// collect_tokens=1 so comments are available to the formatter.
//
// Low-level (already-parsed statement):
//   SyntaqliteParser *p = syntaqlite_parser_create(
//       (SyntaqliteParserConfig){.collect_tokens = 1});
//   syntaqlite_parser_reset(p, sql, len);
//   SyntaqliteParseResult result = syntaqlite_parser_next(p);
//   char *formatted = syntaqlite_format_stmt(p, result.root, NULL);
//   free(formatted);

#ifndef SYNTAQLITE_FORMATTER_H
#define SYNTAQLITE_FORMATTER_H

#include <stdint.h>

#include "syntaqlite/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

// Formatting options. Pass NULL anywhere an options pointer is accepted to
// use the defaults (80-column width, 2-space indent).
typedef struct SyntaqliteFormatOptions {
  uint32_t target_width;  // Target line width (default 80).
  uint32_t indent_width;  // Spaces per indent level (default 2).
} SyntaqliteFormatOptions;

#define SYNTAQLITE_FORMAT_OPTIONS_DEFAULT {80, 2}

// --- Simple API (most users only need this) ---

// Format a SQL string end-to-end. Internally creates a parser, parses all
// statements, formats them, and returns the result as a single string.
// Returns a malloc'd string the caller must free(), or NULL on error.
// options may be NULL for defaults.
char* syntaqlite_format(const char* sql,
                        uint32_t len,
                        const SyntaqliteFormatOptions* options);

// --- Low-level API (for pre-parsed statements) ---

// Format a single statement that was already parsed. Use this when you have
// a parser open and want to format individual statements selectively.
// The parser must have been created with collect_tokens=1, otherwise
// comments cannot be preserved in the output.
// Returns a malloc'd string the caller must free(), or NULL on error.
char* syntaqlite_format_stmt(SyntaqliteParser* parser,
                             uint32_t root_id,
                             const SyntaqliteFormatOptions* options);

// --- Debugging ---

// Like syntaqlite_format_stmt(), but returns the internal document IR tree
// as a string instead of formatted SQL. Useful for debugging formatter
// output decisions.
// Returns a malloc'd string the caller must free(), or NULL on error.
char* syntaqlite_format_stmt_debug_ir(SyntaqliteParser* parser,
                                      uint32_t root_id,
                                      const SyntaqliteFormatOptions* options);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_FORMATTER_H
