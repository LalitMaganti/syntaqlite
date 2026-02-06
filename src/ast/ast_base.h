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

// Grammar stack value: column name + type token (threaded through columnname rule)
typedef struct SyntaqliteColumnNameValue {
    SyntaqliteSourceSpan name;
    SyntaqliteSourceSpan typetoken;
} SyntaqliteColumnNameValue;

// Grammar stack value: constraint node + optional pending CONSTRAINT name
typedef struct SyntaqliteConstraintValue {
    uint32_t node;
    SyntaqliteSourceSpan pending_name;
} SyntaqliteConstraintValue;

// Grammar stack value: constraint list + pending CONSTRAINT name
typedef struct SyntaqliteConstraintListValue {
    uint32_t list;
    SyntaqliteSourceSpan pending_name;
} SyntaqliteConstraintListValue;

// Grammar stack value: ON expr / USING column-list discriminator
typedef struct SyntaqliteOnUsingValue {
    uint32_t on_expr;
    uint32_t using_cols;
} SyntaqliteOnUsingValue;

// Grammar stack value: WITH clause (carries recursive flag without a wrapper node)
typedef struct SyntaqliteWithValue {
    uint32_t cte_list;
    uint8_t is_recursive;
} SyntaqliteWithValue;

// Build context passed through parser
typedef struct SyntaqliteAstContext {
    SyntaqliteAst *ast;
    const char *source;
    uint32_t source_length;
    int error_code;
    const char *error_msg;
    // List accumulator: avoids O(n^2) copy-on-every-append.
    // Children are collected here, then flushed to the arena in one shot.
    uint32_t *list_acc;          // heap-allocated buffer for child node IDs
    uint32_t list_acc_count;     // number of children in accumulator
    uint32_t list_acc_cap;       // buffer capacity
    uint32_t list_acc_node_id;   // node ID of the list being built
    uint8_t  list_acc_tag;       // tag of the list being built
} SyntaqliteAstContext;

// Arena management
void syntaqlite_ast_init(SyntaqliteAst *ast);
void syntaqlite_ast_free(SyntaqliteAst *ast);
uint32_t ast_alloc(SyntaqliteAstContext *ctx, uint8_t tag, size_t size);

// AST context lifecycle
void syntaqlite_ast_context_init(SyntaqliteAstContext *ctx, SyntaqliteAst *ast,
                                  const char *source, uint32_t source_length);
void syntaqlite_ast_context_cleanup(SyntaqliteAstContext *ctx);

// Generic list building (O(n) amortized via accumulator)
uint32_t ast_list_start(SyntaqliteAstContext *ctx, uint8_t tag, uint32_t first_child);
uint32_t ast_list_append(SyntaqliteAstContext *ctx, uint32_t list_id,
                          uint32_t child, uint8_t tag);
void ast_list_flush(SyntaqliteAstContext *ctx);

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
