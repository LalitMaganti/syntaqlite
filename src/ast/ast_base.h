// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base types and functions for AST infrastructure.

#ifndef SYNQ_AST_BASE_H
#define SYNQ_AST_BASE_H

#include "src/base/arena.h"
#include "src/base/vec.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for token/context types
struct SynqToken;
struct SynqParseContext;

// Null node ID (used for nullable fields)
#define SYNQ_NULL_NODE 0xFFFFFFFFu

// Source location span (offset + length into source text)
typedef struct SynqSourceSpan {
    uint32_t offset;
    uint16_t length;
} SynqSourceSpan;

// Empty source span
#define SYNQ_NO_SPAN ((SynqSourceSpan){0, 0})

// Source range: byte offsets [first, last) covering a full AST node
typedef struct SynqSourceRange {
    uint32_t first;  // byte offset of first token
    uint32_t last;   // byte offset past last token
} SynqSourceRange;

// Grammar stack value: column name + type token (threaded through columnname rule)
typedef struct SynqColumnNameValue {
    SynqSourceSpan name;
    SynqSourceSpan typetoken;
} SynqColumnNameValue;

// Grammar stack value: constraint node + optional pending CONSTRAINT name
typedef struct SynqConstraintValue {
    uint32_t node;
    SynqSourceSpan pending_name;
} SynqConstraintValue;

// Grammar stack value: constraint list + pending CONSTRAINT name
typedef struct SynqConstraintListValue {
    uint32_t list;
    SynqSourceSpan pending_name;
} SynqConstraintListValue;

// Grammar stack value: ON expr / USING column-list discriminator
typedef struct SynqOnUsingValue {
    uint32_t on_expr;
    uint32_t using_cols;
} SynqOnUsingValue;

// Grammar stack value: WITH clause (carries recursive flag without a wrapper node)
typedef struct SynqWithValue {
    uint32_t cte_list;
    uint8_t is_recursive;
} SynqWithValue;

// Build context passed through parser
typedef struct SynqAstContext {
    SynqArena ast;
    const char *source;
    uint32_t source_length;
    int error_code;
    const char *error_msg;
    // List accumulator: avoids O(n^2) copy-on-every-append.
    // Children are collected here, then flushed to the arena in one shot.
    SYNQ_VEC(uint32_t) list_acc;
    uint32_t list_acc_node_id;   // node ID of the list being built
    uint8_t  list_acc_tag;       // tag of the list being built
    // Source ranges parallel array (indexed by node ID)
    SYNQ_VEC(SynqSourceRange) ranges;
} SynqAstContext;

// AST context lifecycle
void synq_ast_context_init(SynqAstContext *ctx,
                                  const char *source, uint32_t source_length);
void synq_ast_context_cleanup(SynqAstContext *ctx);

// Generic list building (O(n) amortized via accumulator)
uint32_t synq_ast_list_start(SynqAstContext *ctx, uint8_t tag, uint32_t first_child);
uint32_t synq_ast_list_append(SynqAstContext *ctx, uint32_t list_id,
                          uint32_t child, uint8_t tag);
void synq_ast_list_flush(SynqAstContext *ctx);

// Print helpers
void synq_ast_print_indent(FILE *out, int depth);
void synq_ast_print_source_span(FILE *out, const char *source, SynqSourceSpan span);

// Create source span from token
SynqSourceSpan synq_span(struct SynqParseContext *ctx,
                                     struct SynqToken tok);

// ============ Source Range Helpers ============

// Sync ranges vec to match arena node count
inline void synq_ast_ranges_sync(SynqAstContext *ctx) {
    while (ctx->ranges.count < ctx->ast.count) {
        SynqSourceRange zero = {0, 0};
        synq_vec_push(&ctx->ranges, zero);
    }
}

// Set source range for a node
inline void synq_ast_set_range(SynqAstContext *ctx, uint32_t node_id,
                                 uint32_t first, uint32_t last) {
    if (node_id == SYNQ_NULL_NODE) return;
    synq_ast_ranges_sync(ctx);
    if (node_id < ctx->ranges.count) {
        ctx->ranges.data[node_id].first = first;
        ctx->ranges.data[node_id].last = last;
    }
}

// Get source range of a node
inline SynqSourceRange synq_ast_get_range(SynqAstContext *ctx,
                                                   uint32_t node_id) {
    if (node_id == SYNQ_NULL_NODE || node_id >= ctx->ranges.count)
        return (SynqSourceRange){0, 0};
    return ctx->ranges.data[node_id];
}

// Union a child node's range into an accumulator
inline void synq_ast_range_union(SynqAstContext *ctx,
                                   SynqSourceRange *acc,
                                   uint32_t child_id) {
    if (child_id == SYNQ_NULL_NODE) return;
    SynqSourceRange child = synq_ast_get_range(ctx, child_id);
    if (child.first == 0 && child.last == 0) return;
    if (child.first < acc->first) acc->first = child.first;
    if (child.last > acc->last) acc->last = child.last;
}

// Union a SynqSourceSpan into an accumulator
inline void synq_ast_range_union_span(SynqSourceRange *acc,
                                        SynqSourceSpan span) {
    if (span.length == 0) return;
    if (span.offset < acc->first) acc->first = span.offset;
    uint32_t end = span.offset + span.length;
    if (end > acc->last) acc->last = end;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_AST_BASE_H
