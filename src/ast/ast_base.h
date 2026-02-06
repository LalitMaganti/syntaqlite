// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base types and functions for AST infrastructure.

#ifndef SYNTAQLITE_AST_BASE_H
#define SYNTAQLITE_AST_BASE_H

#include "src/arena.h"
#include "src/vec.h"

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

// Source range: byte offsets [first, last) covering a full AST node
typedef struct SyntaqliteSourceRange {
    uint32_t first;  // byte offset of first token
    uint32_t last;   // byte offset past last token
} SyntaqliteSourceRange;

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
    SyntaqliteArena ast;
    const char *source;
    uint32_t source_length;
    int error_code;
    const char *error_msg;
    // List accumulator: avoids O(n^2) copy-on-every-append.
    // Children are collected here, then flushed to the arena in one shot.
    SYNTAQLITE_VEC(uint32_t) list_acc;
    uint32_t list_acc_node_id;   // node ID of the list being built
    uint8_t  list_acc_tag;       // tag of the list being built
    // Source ranges parallel array (indexed by node ID)
    SYNTAQLITE_VEC(SyntaqliteSourceRange) ranges;
} SyntaqliteAstContext;

// AST context lifecycle
void syntaqlite_ast_context_init(SyntaqliteAstContext *ctx,
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

// ============ Source Range Helpers ============

// Sync ranges vec to match arena node count
static inline void ast_ranges_sync(SyntaqliteAstContext *ctx) {
    while (ctx->ranges.count < ctx->ast.count) {
        SyntaqliteSourceRange zero = {0, 0};
        syntaqlite_vec_push(&ctx->ranges, zero);
    }
}

// Set source range for a node
static inline void ast_set_range(SyntaqliteAstContext *ctx, uint32_t node_id,
                                 uint32_t first, uint32_t last) {
    if (node_id == SYNTAQLITE_NULL_NODE) return;
    ast_ranges_sync(ctx);
    if (node_id < ctx->ranges.count) {
        ctx->ranges.data[node_id].first = first;
        ctx->ranges.data[node_id].last = last;
    }
}

// Get source range of a node
static inline SyntaqliteSourceRange ast_get_range(SyntaqliteAstContext *ctx,
                                                   uint32_t node_id) {
    if (node_id == SYNTAQLITE_NULL_NODE || node_id >= ctx->ranges.count)
        return (SyntaqliteSourceRange){0, 0};
    return ctx->ranges.data[node_id];
}

// Union a child node's range into an accumulator
static inline void ast_range_union(SyntaqliteAstContext *ctx,
                                   SyntaqliteSourceRange *acc,
                                   uint32_t child_id) {
    if (child_id == SYNTAQLITE_NULL_NODE) return;
    SyntaqliteSourceRange child = ast_get_range(ctx, child_id);
    if (child.first == 0 && child.last == 0) return;
    if (child.first < acc->first) acc->first = child.first;
    if (child.last > acc->last) acc->last = child.last;
}

// Union a SyntaqliteSourceSpan into an accumulator
static inline void ast_range_union_span(SyntaqliteSourceRange *acc,
                                        SyntaqliteSourceSpan span) {
    if (span.length == 0) return;
    if (span.offset < acc->first) acc->first = span.offset;
    uint32_t end = span.offset + span.length;
    if (end > acc->last) acc->last = end;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_BASE_H
