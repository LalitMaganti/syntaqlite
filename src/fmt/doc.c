// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model implementation for Wadler-Lindig pretty printer.

#include "src/fmt/doc.h"

#include <string.h>

void syntaqlite_doc_context_init(SyntaqliteDocContext *ctx) {
    syntaqlite_arena_init(&ctx->arena);
}

void syntaqlite_doc_context_cleanup(SyntaqliteDocContext *ctx) {
    syntaqlite_arena_free(&ctx->arena);
}

uint32_t doc_text(SyntaqliteDocContext *ctx, const char *text, uint32_t length) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_TEXT, sizeof(SyntaqliteDocText));
    SyntaqliteDocText *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->text;
    node->length = length;
    node->text = text;
    return id;
}

uint32_t doc_line(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_LINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_softline(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_SOFTLINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_hardline(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_HARDLINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_nest(SyntaqliteDocContext *ctx, int32_t indent, uint32_t child) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_NEST, sizeof(SyntaqliteDocNest));
    SyntaqliteDocNest *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->nest;
    node->indent = indent;
    node->child = child;
    return id;
}

uint32_t doc_group(SyntaqliteDocContext *ctx, uint32_t child) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_GROUP, sizeof(SyntaqliteDocGroup));
    SyntaqliteDocGroup *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->group;
    node->child = child;
    return id;
}

uint32_t doc_concat(SyntaqliteDocContext *ctx, uint32_t *children, uint32_t count) {
    size_t size = offsetof(SyntaqliteDocConcat, children) + count * sizeof(uint32_t);
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_CONCAT, size);
    SyntaqliteDocConcat *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->concat;
    memcpy(node->children, children, count * sizeof(uint32_t));
    node->count = count;
    return id;
}
