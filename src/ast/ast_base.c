// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base AST infrastructure implementations.

#include "src/ast/ast_base.h"
#include "src/syntaqlite_sqlite_defs.h"

#include <stdlib.h>
#include <string.h>

void syntaqlite_ast_init(SyntaqliteAst *ast) {
    ast->arena = NULL;
    ast->arena_size = 0;
    ast->arena_capacity = 0;
    ast->offsets = NULL;
    ast->node_count = 0;
    ast->node_capacity = 0;
}

void syntaqlite_ast_free(SyntaqliteAst *ast) {
    free(ast->arena);
    free(ast->offsets);
    ast->arena = NULL;
    ast->offsets = NULL;
    ast->arena_size = 0;
    ast->arena_capacity = 0;
    ast->node_count = 0;
    ast->node_capacity = 0;
}

uint32_t ast_alloc(SyntaqliteAstContext *ctx, uint8_t tag, size_t size) {
    SyntaqliteAst *ast = ctx->ast;

    // Grow arena if needed
    if (ast->arena_size + size > ast->arena_capacity) {
        size_t new_capacity = ast->arena_capacity * 2 + size + 1024;
        uint8_t *new_arena = (uint8_t*)realloc(ast->arena, new_capacity);
        if (!new_arena) {
            ctx->error_code = 1;
            ctx->error_msg = "Out of memory (arena)";
            return SYNTAQLITE_NULL_NODE;
        }
        ast->arena = new_arena;
        ast->arena_capacity = (uint32_t)new_capacity;
    }

    // Grow offset table if needed
    if (ast->node_count >= ast->node_capacity) {
        size_t new_capacity = ast->node_capacity * 2 + 64;
        uint32_t *new_offsets = (uint32_t*)realloc(ast->offsets, new_capacity * sizeof(uint32_t));
        if (!new_offsets) {
            ctx->error_code = 1;
            ctx->error_msg = "Out of memory (offsets)";
            return SYNTAQLITE_NULL_NODE;
        }
        ast->offsets = new_offsets;
        ast->node_capacity = (uint32_t)new_capacity;
    }

    uint32_t node_id = ast->node_count++;
    ast->offsets[node_id] = ast->arena_size;
    ast->arena[ast->arena_size] = tag;
    ast->arena_size += (uint32_t)size;

    return node_id;
}

// ============ AST Context Lifecycle ============

void syntaqlite_ast_context_init(SyntaqliteAstContext *ctx, SyntaqliteAst *ast,
                                  const char *source, uint32_t source_length) {
    ctx->ast = ast;
    ctx->source = source;
    ctx->source_length = source_length;
    ctx->error_code = 0;
    ctx->error_msg = NULL;
    ctx->list_acc = NULL;
    ctx->list_acc_count = 0;
    ctx->list_acc_cap = 0;
    ctx->list_acc_node_id = SYNTAQLITE_NULL_NODE;
    ctx->list_acc_tag = 0;
}

void syntaqlite_ast_context_cleanup(SyntaqliteAstContext *ctx) {
    ast_list_flush(ctx);
    free(ctx->list_acc);
    ctx->list_acc = NULL;
    ctx->list_acc_count = 0;
    ctx->list_acc_cap = 0;
}

// ============ List Accumulator ============

// Ensure the accumulator buffer has room for at least `needed` entries.
static int ast_list_acc_ensure(SyntaqliteAstContext *ctx, uint32_t needed) {
    if (needed <= ctx->list_acc_cap) return 0;
    uint32_t new_cap = ctx->list_acc_cap * 2;
    if (new_cap < needed) new_cap = needed;
    if (new_cap < 16) new_cap = 16;
    uint32_t *buf = (uint32_t*)realloc(ctx->list_acc, new_cap * sizeof(uint32_t));
    if (!buf) {
        ctx->error_code = 1;
        ctx->error_msg = "Out of memory (list accumulator)";
        return -1;
    }
    ctx->list_acc = buf;
    ctx->list_acc_cap = new_cap;
    return 0;
}

void ast_list_flush(SyntaqliteAstContext *ctx) {
    if (ctx->list_acc_node_id == SYNTAQLITE_NULL_NODE) return;

    SyntaqliteAst *ast = ctx->ast;
    uint32_t count = ctx->list_acc_count;
    // All list structs: tag(1) + pad(3) + count(4) + children(4*N) = 8 + 4*N
    size_t size = 8 + count * sizeof(uint32_t);

    // Grow arena if needed
    if (ast->arena_size + size > ast->arena_capacity) {
        size_t new_capacity = ast->arena_capacity * 2 + size + 1024;
        uint8_t *new_arena = (uint8_t*)realloc(ast->arena, new_capacity);
        if (!new_arena) {
            ctx->error_code = 1;
            ctx->error_msg = "Out of memory (arena)";
            return;
        }
        ast->arena = new_arena;
        ast->arena_capacity = (uint32_t)new_capacity;
    }

    uint32_t offset = ast->arena_size;
    ast->offsets[ctx->list_acc_node_id] = offset;

    // Write list struct: tag + pad[3] + count + children[]
    uint8_t *dest = ast->arena + offset;
    dest[0] = ctx->list_acc_tag;
    dest[1] = 0;
    dest[2] = 0;
    dest[3] = 0;
    memcpy(dest + 4, &count, sizeof(uint32_t));
    if (count > 0) {
        memcpy(dest + 8, ctx->list_acc, count * sizeof(uint32_t));
    }
    ast->arena_size += (uint32_t)size;

    ctx->list_acc_node_id = SYNTAQLITE_NULL_NODE;
    ctx->list_acc_count = 0;
}

uint32_t ast_list_start(SyntaqliteAstContext *ctx, uint8_t tag,
                         uint32_t first_child) {
    // Flush any in-progress list
    ast_list_flush(ctx);

    // Allocate a header on the arena so the tag is readable via AST_NODE().
    // Grammar actions may inspect n->tag before the list is fully built
    // (e.g. JOIN USING checks tag to distinguish ExprList from ON expr).
    // The header will be replaced with the full list when the accumulator
    // is flushed; these 8 bytes become dead space.
    uint32_t node_id = ast_alloc(ctx, tag, 8);
    if (node_id == SYNTAQLITE_NULL_NODE) return node_id;

    if (ast_list_acc_ensure(ctx, 1) < 0) return SYNTAQLITE_NULL_NODE;

    ctx->list_acc[0] = first_child;
    ctx->list_acc_count = 1;
    ctx->list_acc_node_id = node_id;
    ctx->list_acc_tag = tag;

    return node_id;
}

uint32_t ast_list_append(SyntaqliteAstContext *ctx, uint32_t list_id,
                          uint32_t child, uint8_t tag) {
    // Fast path: appending to the list currently in the accumulator
    if (list_id == ctx->list_acc_node_id) {
        if (ast_list_acc_ensure(ctx, ctx->list_acc_count + 1) < 0)
            return SYNTAQLITE_NULL_NODE;
        ctx->list_acc[ctx->list_acc_count++] = child;
        return list_id;
    }

    // Slow path: switching to a different list.
    // Flush the current accumulator, then reload this list from the arena.
    ast_list_flush(ctx);

    uint8_t *data = ctx->ast->arena + ctx->ast->offsets[list_id];
    uint32_t old_count;
    memcpy(&old_count, data + 4, sizeof(uint32_t));

    uint32_t needed = old_count + 1;
    if (ast_list_acc_ensure(ctx, needed) < 0) return SYNTAQLITE_NULL_NODE;

    // Re-fetch after potential realloc in ensure
    data = ctx->ast->arena + ctx->ast->offsets[list_id];
    if (old_count > 0) {
        memcpy(ctx->list_acc, data + 8, old_count * sizeof(uint32_t));
    }
    ctx->list_acc[old_count] = child;
    ctx->list_acc_count = needed;
    ctx->list_acc_node_id = list_id;
    ctx->list_acc_tag = tag;

    return list_id;
}

void ast_print_indent(FILE *out, int depth) {
    for (int i = 0; i < depth; i++) {
        fprintf(out, "  ");
    }
}

void ast_print_source_span(FILE *out, const char *source, SyntaqliteSourceSpan span) {
    if (source && span.length > 0) {
        fprintf(out, "\"");
        for (uint16_t i = 0; i < span.length; i++) {
            char c = source[span.offset + i];
            if (c == '"') {
                fprintf(out, "\\\"");
            } else if (c == '\\') {
                fprintf(out, "\\\\");
            } else if (c == '\n') {
                fprintf(out, "\\n");
            } else {
                fprintf(out, "%c", c);
            }
        }
        fprintf(out, "\"");
    } else {
        fprintf(out, "null");
    }
}

SyntaqliteSourceSpan syntaqlite_span(SyntaqliteParseContext *ctx, SyntaqliteToken tok) {
    return (SyntaqliteSourceSpan){
        (uint32_t)(tok.z - ctx->zSql),
        (uint16_t)tok.n
    };
}

