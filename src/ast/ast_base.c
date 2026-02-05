// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Base AST infrastructure implementations.

#include "src/ast/ast_base.h"
#include "src/syntaqlite_sqlite_defs.h"

#include <stdlib.h>

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

