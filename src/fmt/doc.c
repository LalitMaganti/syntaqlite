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

uint32_t doc_concat_empty(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_CONCAT, offsetof(SyntaqliteDocConcat, children));
}

uint32_t doc_concat_append(SyntaqliteDocContext *ctx, uint32_t concat_id, uint32_t child) {
    SyntaqliteArena *arena = &ctx->arena;
    uint32_t offset = arena->offsets[concat_id];
    SyntaqliteDocConcat *concat = &((SyntaqliteDoc*)(arena->data + offset))->concat;
    uint32_t old_count = concat->count;
    size_t existing_size = offsetof(SyntaqliteDocConcat, children) + old_count * sizeof(uint32_t);

    // Check if this concat is the last allocation in the arena (can extend in place)
    if (offset + existing_size == arena->size) {
        // Extend in place
        syntaqlite_arena_ensure(arena, sizeof(uint32_t));
        // Re-derive pointer (realloc may have moved the arena)
        concat = &((SyntaqliteDoc*)(arena->data + offset))->concat;
        concat->children[old_count] = child;
        concat->count = old_count + 1;
        arena->size += (uint32_t)sizeof(uint32_t);
        return concat_id;
    }

    // Not at end — allocate a fresh concat with room for old children + new child
    uint32_t new_count = old_count + 1;
    size_t new_size = offsetof(SyntaqliteDocConcat, children) + new_count * sizeof(uint32_t);
    uint32_t new_id = syntaqlite_arena_alloc(arena, SYNTAQLITE_DOC_CONCAT, new_size);

    // Re-derive old pointer (arena_alloc may have reallocated)
    concat = &((SyntaqliteDoc*)(arena->data + arena->offsets[concat_id]))->concat;
    SyntaqliteDocConcat *new_concat = &((SyntaqliteDoc*)(arena->data + arena->offsets[new_id]))->concat;

    // Copy old children and add new one
    memcpy(new_concat->children, concat->children, old_count * sizeof(uint32_t));
    new_concat->children[old_count] = child;
    new_concat->count = new_count;

    return new_id;
}
