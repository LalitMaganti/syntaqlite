// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base AST infrastructure implementations.

#include "src/ast/ast_base.h"
#include "src/syntaqlite_sqlite_defs.h"

#include <string.h>

// ============ AST Context Lifecycle ============

void syntaqlite_ast_context_init(SyntaqliteAstContext *ctx,
                                  const char *source, uint32_t source_length) {
    syntaqlite_arena_init(&ctx->ast);
    ctx->source = source;
    ctx->source_length = source_length;
    ctx->error_code = 0;
    ctx->error_msg = NULL;
    syntaqlite_vec_init(&ctx->list_acc);
    ctx->list_acc_node_id = SYNTAQLITE_NULL_NODE;
    ctx->list_acc_tag = 0;
    syntaqlite_vec_init(&ctx->ranges);
}

void syntaqlite_ast_context_cleanup(SyntaqliteAstContext *ctx) {
    ast_list_flush(ctx);
    syntaqlite_vec_free(&ctx->list_acc);
    syntaqlite_vec_free(&ctx->ranges);
    syntaqlite_arena_free(&ctx->ast);
}

// ============ List Accumulator ============

void ast_list_flush(SyntaqliteAstContext *ctx) {
    if (ctx->list_acc_node_id == SYNTAQLITE_NULL_NODE) return;

    SyntaqliteArena *ast = &ctx->ast;
    uint32_t count = ctx->list_acc.count;
    // All list structs: tag(1) + pad(3) + count(4) + children(4*N) = 8 + 4*N
    size_t size = 8 + count * sizeof(uint32_t);

    syntaqlite_arena_ensure(ast, size);

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
    ast_ranges_sync(ctx);
    SyntaqliteSourceRange list_range = {UINT32_MAX, 0};
    for (uint32_t i = 0; i < count; i++) {
        ast_range_union(ctx, &list_range, ctx->list_acc.data[i]);
    }
    if (list_range.first != UINT32_MAX) {
        ctx->ranges.data[ctx->list_acc_node_id] = list_range;
    }

    ctx->list_acc_node_id = SYNTAQLITE_NULL_NODE;
    ctx->list_acc.count = 0;
}

uint32_t ast_list_start(SyntaqliteAstContext *ctx, uint8_t tag,
                         uint32_t first_child) {
    // Flush any in-progress list
    ast_list_flush(ctx);

    uint32_t node_id = syntaqlite_arena_reserve_id(&ctx->ast);
    ast_ranges_sync(ctx);

    syntaqlite_vec_ensure(&ctx->list_acc, 1);
    ctx->list_acc.data[0] = first_child;
    ctx->list_acc.count = 1;
    ctx->list_acc_node_id = node_id;
    ctx->list_acc_tag = tag;

    return node_id;
}

uint32_t ast_list_append(SyntaqliteAstContext *ctx, uint32_t list_id,
                          uint32_t child, uint8_t tag) {
    // Fast path: appending to the list currently in the accumulator
    if (list_id == ctx->list_acc_node_id) {
        syntaqlite_vec_push(&ctx->list_acc, child);
        return list_id;
    }

    // Slow path: switching to a different list.
    // Flush the current accumulator, then reload this list from the arena.
    ast_list_flush(ctx);

    uint8_t *data = ctx->ast.data + ctx->ast.offsets[list_id];
    uint32_t old_count;
    memcpy(&old_count, data + 4, sizeof(uint32_t));

    uint32_t needed = old_count + 1;
    syntaqlite_vec_ensure(&ctx->list_acc, needed);

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
