// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base types and functions for AST infrastructure.

#ifndef SYNTAQLITE_AST_BASE_H
#define SYNTAQLITE_AST_BASE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for token/context types
struct SyntaqliteToken;
struct SyntaqliteParseContext;

// Null node ID (used for nullable fields)
#define SYNTAQLITE_NULL_NODE 0xFFFFFFFFu

// Source location span (offset + length into source text)
typedef struct SyntaqliteSourceSpan {
    uint32_t offset;
    uint16_t length;
} SyntaqliteSourceSpan;

// Empty source span
#define SYNTAQLITE_NO_SPAN ((SyntaqliteSourceSpan){0, 0})

// Arena-based AST storage
typedef struct SyntaqliteAst {
    uint8_t *arena;
    uint32_t arena_size;
    uint32_t arena_capacity;
    uint32_t *offsets;
    uint32_t node_count;
    uint32_t node_capacity;
} SyntaqliteAst;

// Build context passed through parser
typedef struct SyntaqliteAstContext {
    SyntaqliteAst *ast;
    const char *source;
    uint32_t source_length;
    int error_code;
    const char *error_msg;
    // CREATE TABLE parser state
    SyntaqliteSourceSpan typetoken_span;        // current column type token
    SyntaqliteSourceSpan constraint_name;       // current column CONSTRAINT name
    SyntaqliteSourceSpan tcons_constraint_name; // current table CONSTRAINT name
    uint32_t tcons_list;                        // accumulated table constraint list
} SyntaqliteAstContext;

// Arena management
void syntaqlite_ast_init(SyntaqliteAst *ast);
void syntaqlite_ast_free(SyntaqliteAst *ast);
uint32_t ast_alloc(SyntaqliteAstContext *ctx, uint8_t tag, size_t size);

// Print helpers
void ast_print_indent(FILE *out, int depth);
void ast_print_source_span(FILE *out, const char *source, SyntaqliteSourceSpan span);

// Create source span from token
SyntaqliteSourceSpan syntaqlite_span(struct SyntaqliteParseContext *ctx,
                                     struct SyntaqliteToken tok);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_BASE_H
