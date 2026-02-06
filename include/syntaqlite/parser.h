// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Streaming parser for SQLite SQL.
//
// Usage:
//   SyntaqliteParser *p = syntaqlite_parser_create(NULL);
//   syntaqlite_parser_reset(p, sql, len);
//   SyntaqliteParseResult result;
//   while ((result = syntaqlite_parser_next(p)).root != SYNQ_NULL_NODE) {
//     // process result.root
//   }
//   if (result.error) { /* handle error */ }
//   syntaqlite_parser_destroy(p);

#ifndef SYNTAQLITE_PARSER_H
#define SYNTAQLITE_PARSER_H

#include <stdint.h>
#include <stdio.h>

// AST node types (public, self-contained).
#include "syntaqlite/ast_nodes_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque parser handle (heap-allocated).
typedef struct SyntaqliteParser SyntaqliteParser;

// Parser configuration.
typedef struct SyntaqliteParserConfig {
    int collect_tokens;  // If nonzero, collect all tokens for formatter use.
    int trace;           // If nonzero, enable Lemon parser tracing on stderr
                         // (debug builds only; ignored in release).
} SyntaqliteParserConfig;

// Result from syntaqlite_parser_next().
typedef struct SyntaqliteParseResult {
    uint32_t root;       // Root node ID, or SYNQ_NULL_NODE at end-of-input.
    int error;           // Nonzero if a parse error occurred.
    const char *error_msg;  // Error message (owned by parser), or NULL.
} SyntaqliteParseResult;

// Create a parser. Call reset() to bind a source buffer before use.
// config may be NULL for defaults (no token collection).
SyntaqliteParser *syntaqlite_parser_create(const SyntaqliteParserConfig *config);

// Bind (or rebind) a source buffer and reset parser state.
// The source must remain valid until the next reset or destroy.
void syntaqlite_parser_reset(SyntaqliteParser *p, const char *source,
                              uint32_t len);

// Parse the next statement. Returns the root node ID.
// Returns SYNQ_NULL_NODE with error=0 at end-of-input.
// Bare semicolons are skipped automatically.
SyntaqliteParseResult syntaqlite_parser_next(SyntaqliteParser *p);

// Access a node by ID (valid until the next syntaqlite_parser_reset call).
const SynqNode *syntaqlite_parser_node(SyntaqliteParser *p, uint32_t node_id);

// Access the source text that was passed to syntaqlite_parser_reset().
const char *syntaqlite_parser_source(SyntaqliteParser *p);

// Length of the source text.
uint32_t syntaqlite_parser_source_length(SyntaqliteParser *p);

// Print AST node tree to a file stream (for debugging/inspection).
void syntaqlite_parser_print_ast(SyntaqliteParser *p, uint32_t node_id,
                                 FILE *out);

// Destroy the parser and free all resources.
// All nodes are invalidated.
void syntaqlite_parser_destroy(SyntaqliteParser *p);

#ifdef __cplusplus
}
#endif

#endif // SYNTAQLITE_PARSER_H
