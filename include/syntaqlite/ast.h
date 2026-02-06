// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Fixed AST type definitions (not auto-generated).
// Included transitively via parser.h — users don't need to include this
// directly.

#ifndef SYNTAQLITE_AST_H
#define SYNTAQLITE_AST_H

#include <stdint.h>
#include <stdio.h>

// Sentinel value for optional node fields. Check against this before passing
// a node ID to syntaqlite_parser_node().
#define SYNTAQLITE_NULL_NODE 0xFFFFFFFFu

// Byte range within the source text (passed to syntaqlite_parser_reset()).
// Retrieve the corresponding text with:
//   syntaqlite_parser_source(p) + span.offset
typedef struct SyntaqliteSourceSpan {
  uint32_t offset;  // Byte offset from the start of the source text.
  uint16_t length;  // Length in bytes.
} SyntaqliteSourceSpan;

// Forward declaration — full definition in parser.h.
typedef struct SyntaqliteParser SyntaqliteParser;

// Print an AST subtree rooted at node_id to a file stream (e.g. stderr).
// Needs the parser to resolve child node IDs and access source text.
void syntaqlite_ast_print(SyntaqliteParser* p, uint32_t node_id, FILE* out);

#endif  // SYNTAQLITE_AST_H
