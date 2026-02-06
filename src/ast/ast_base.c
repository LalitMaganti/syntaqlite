// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base AST infrastructure implementations.

#include "src/ast/ast_base.h"
#include "src/common/synq_sqlite_defs.h"

#include <string.h>


// External definitions for inline functions (C99/C11).
extern inline void synq_ast_ranges_sync(SynqAstContext *ctx);
extern inline void synq_ast_set_range(SynqAstContext *ctx, uint32_t node_id, uint32_t first, uint32_t last);
extern inline SynqSourceRange synq_ast_get_range(SynqAstContext *ctx, uint32_t node_id);
extern inline void synq_ast_range_union(SynqAstContext *ctx, SynqSourceRange *acc, uint32_t child_id);
extern inline void synq_ast_range_union_span(SynqSourceRange *acc, SynqSourceSpan span);

// ============ AST Context Lifecycle ============

void synq_ast_context_init(SynqAstContext *ctx,
                                  const char *source, uint32_t source_length) {
    synq_arena_init(&ctx->ast);
    ctx->source = source;
    ctx->source_length = source_length;
    ctx->error_code = 0;
    ctx->error_msg = NULL;
    synq_vec_init(&ctx->list_acc);
    ctx->list_acc_node_id = SYNQ_NULL_NODE;
    ctx->list_acc_tag = 0;
    synq_vec_init(&ctx->ranges);
}

void synq_ast_context_cleanup(SynqAstContext *ctx) {
    synq_ast_list_flush(ctx);
    synq_vec_free(&ctx->list_acc);
    synq_vec_free(&ctx->ranges);
    synq_arena_free(&ctx->ast);
}

// ============ List Accumulator ============

void synq_ast_list_flush(SynqAstContext *ctx) {
    if (ctx->list_acc_node_id == SYNQ_NULL_NODE) return;

    SynqArena *ast = &ctx->ast;
    uint32_t count = ctx->list_acc.count;
    // All list structs: tag(1) + pad(3) + count(4) + children(4*N) = 8 + 4*N
    size_t size = 8 + count * sizeof(uint32_t);

    synq_arena_ensure(ast, size);

    uint32_t offset = ast->size;
    ast->offsets[ctx->list_acc_node_id] = offset;

    // Write list struct: tag + pad[3] + count + children[]
    uint8_t *dest = ast->data + offset;
    dest[0] = ctx->list_acc_tag;
    dest[1] = 0;
    dest[2] = 0;
    dest[3] = 0;
    memcpy(dest + 4, &count, sizeof(uint32_t));
    if (count > 0) {
        memcpy(dest + 8, ctx->list_acc.data, count * sizeof(uint32_t));
    }
    ast->size += (uint32_t)size;

    // Compute source range as union of all children's ranges
    synq_ast_ranges_sync(ctx);
    SynqSourceRange list_range = {UINT32_MAX, 0};
    for (uint32_t i = 0; i < count; i++) {
        synq_ast_range_union(ctx, &list_range, ctx->list_acc.data[i]);
    }
    if (list_range.first != UINT32_MAX) {
        ctx->ranges.data[ctx->list_acc_node_id] = list_range;
    }

    ctx->list_acc_node_id = SYNQ_NULL_NODE;
    ctx->list_acc.count = 0;
}

uint32_t synq_ast_list_start(SynqAstContext *ctx, uint8_t tag,
                         uint32_t first_child) {
    // Flush any in-progress list
    synq_ast_list_flush(ctx);

    uint32_t node_id = synq_arena_reserve_id(&ctx->ast);
    synq_ast_ranges_sync(ctx);

    synq_vec_ensure(&ctx->list_acc, 1);
    ctx->list_acc.data[0] = first_child;
    ctx->list_acc.count = 1;
    ctx->list_acc_node_id = node_id;
    ctx->list_acc_tag = tag;

    return node_id;
}

uint32_t synq_ast_list_append(SynqAstContext *ctx, uint32_t list_id,
                          uint32_t child, uint8_t tag) {
    // Fast path: appending to the list currently in the accumulator
    if (list_id == ctx->list_acc_node_id) {
        synq_vec_push(&ctx->list_acc, child);
        return list_id;
    }

    // Slow path: switching to a different list.
    // Flush the current accumulator, then reload this list from the arena.
    synq_ast_list_flush(ctx);

    uint8_t *data = ctx->ast.data + ctx->ast.offsets[list_id];
    uint32_t old_count;
    memcpy(&old_count, data + 4, sizeof(uint32_t));

    uint32_t needed = old_count + 1;
    synq_vec_ensure(&ctx->list_acc, needed);

    // Re-fetch after potential realloc in ensure
    data = ctx->ast.data + ctx->ast.offsets[list_id];
    if (old_count > 0) {
        memcpy(ctx->list_acc.data, data + 8, old_count * sizeof(uint32_t));
    }
    ctx->list_acc.data[old_count] = child;
    ctx->list_acc.count = needed;
    ctx->list_acc_node_id = list_id;
    ctx->list_acc_tag = tag;

    return list_id;
}

void synq_ast_print_indent(FILE *out, int depth) {
    for (int i = 0; i < depth; i++) {
        fprintf(out, "  ");
    }
}

void synq_ast_print_source_span(FILE *out, const char *source, SynqSourceSpan span) {
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

SynqSourceSpan synq_span(SynqParseContext *ctx, SynqToken tok) {
    return (SynqSourceSpan){
        (uint32_t)(tok.z - ctx->zSql),
        (uint16_t)tok.n
    };
}
