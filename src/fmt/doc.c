// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model implementation for Wadler-Lindig pretty printer.

#include "src/fmt/doc.h"

#include <stdlib.h>
#include <string.h>

void syntaqlite_doc_arena_init(SyntaqliteDocArena *arena) {
    arena->arena = NULL;
    arena->arena_size = 0;
    arena->arena_capacity = 0;
    arena->offsets = NULL;
    arena->node_count = 0;
    arena->node_capacity = 0;
}

void syntaqlite_doc_arena_free(SyntaqliteDocArena *arena) {
    free(arena->arena);
    free(arena->offsets);
    arena->arena = NULL;
    arena->offsets = NULL;
    arena->arena_size = 0;
    arena->arena_capacity = 0;
    arena->node_count = 0;
    arena->node_capacity = 0;
}

// Allocate space in the arena for a doc node, return its ID
static uint32_t doc_alloc(SyntaqliteDocArena *arena, uint8_t tag, size_t size) {
    // Grow arena if needed
    if (arena->arena_size + size > arena->arena_capacity) {
        size_t new_capacity = arena->arena_capacity * 2 + size + 1024;
        uint8_t *new_arena = (uint8_t*)realloc(arena->arena, new_capacity);
        if (!new_arena) {
            return SYNTAQLITE_NULL_DOC;
        }
        arena->arena = new_arena;
        arena->arena_capacity = (uint32_t)new_capacity;
    }

    // Grow offset table if needed
    if (arena->node_count >= arena->node_capacity) {
        size_t new_capacity = arena->node_capacity * 2 + 64;
        uint32_t *new_offsets = (uint32_t*)realloc(arena->offsets, new_capacity * sizeof(uint32_t));
        if (!new_offsets) {
            return SYNTAQLITE_NULL_DOC;
        }
        arena->offsets = new_offsets;
        arena->node_capacity = (uint32_t)new_capacity;
    }

    uint32_t node_id = arena->node_count++;
    arena->offsets[node_id] = arena->arena_size;
    arena->arena[arena->arena_size] = tag;
    arena->arena_size += (uint32_t)size;

    return node_id;
}

// Ensure the arena has room for additional bytes (without allocating a new node)
static int doc_arena_grow(SyntaqliteDocArena *arena, size_t additional) {
    if (arena->arena_size + additional > arena->arena_capacity) {
        size_t new_capacity = arena->arena_capacity * 2 + additional + 1024;
        uint8_t *new_arena = (uint8_t*)realloc(arena->arena, new_capacity);
        if (!new_arena) {
            return 0;
        }
        arena->arena = new_arena;
        arena->arena_capacity = (uint32_t)new_capacity;
    }
    return 1;
}

uint32_t doc_text(SyntaqliteDocArena *arena, const char *text, uint32_t length) {
    uint32_t id = doc_alloc(arena, SYNTAQLITE_DOC_TEXT, sizeof(SyntaqliteDocText));
    if (id == SYNTAQLITE_NULL_DOC) return id;
    SyntaqliteDocText *node = &((SyntaqliteDoc*)(arena->arena + arena->offsets[id]))->text;
    node->length = length;
    node->text = text;
    return id;
}

uint32_t doc_line(SyntaqliteDocArena *arena) {
    return doc_alloc(arena, SYNTAQLITE_DOC_LINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_softline(SyntaqliteDocArena *arena) {
    return doc_alloc(arena, SYNTAQLITE_DOC_SOFTLINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_hardline(SyntaqliteDocArena *arena) {
    return doc_alloc(arena, SYNTAQLITE_DOC_HARDLINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_nest(SyntaqliteDocArena *arena, int32_t indent, uint32_t child) {
    uint32_t id = doc_alloc(arena, SYNTAQLITE_DOC_NEST, sizeof(SyntaqliteDocNest));
    if (id == SYNTAQLITE_NULL_DOC) return id;
    SyntaqliteDocNest *node = &((SyntaqliteDoc*)(arena->arena + arena->offsets[id]))->nest;
    node->indent = indent;
    node->child = child;
    return id;
}

uint32_t doc_group(SyntaqliteDocArena *arena, uint32_t child) {
    uint32_t id = doc_alloc(arena, SYNTAQLITE_DOC_GROUP, sizeof(SyntaqliteDocGroup));
    if (id == SYNTAQLITE_NULL_DOC) return id;
    SyntaqliteDocGroup *node = &((SyntaqliteDoc*)(arena->arena + arena->offsets[id]))->group;
    node->child = child;
    return id;
}

uint32_t doc_concat_empty(SyntaqliteDocArena *arena) {
    // Allocate base struct (no children yet)
    return doc_alloc(arena, SYNTAQLITE_DOC_CONCAT, offsetof(SyntaqliteDocConcat, children));
}

uint32_t doc_concat_append(SyntaqliteDocArena *arena, uint32_t concat_id, uint32_t child) {
    uint32_t offset = arena->offsets[concat_id];
    SyntaqliteDocConcat *concat = &((SyntaqliteDoc*)(arena->arena + offset))->concat;
    uint32_t old_count = concat->count;
    size_t existing_size = offsetof(SyntaqliteDocConcat, children) + old_count * sizeof(uint32_t);

    // Check if this concat is the last allocation in the arena (can extend in place)
    if (offset + existing_size == arena->arena_size) {
        // Extend in place
        if (!doc_arena_grow(arena, sizeof(uint32_t))) {
            return SYNTAQLITE_NULL_DOC;
        }
        // Re-derive pointer (realloc may have moved the arena)
        concat = &((SyntaqliteDoc*)(arena->arena + offset))->concat;
        concat->children[old_count] = child;
        concat->count = old_count + 1;
        arena->arena_size += (uint32_t)sizeof(uint32_t);
        return concat_id;
    }

    // Not at end — allocate a fresh concat with room for old children + new child
    uint32_t new_count = old_count + 1;
    size_t new_size = offsetof(SyntaqliteDocConcat, children) + new_count * sizeof(uint32_t);
    uint32_t new_id = doc_alloc(arena, SYNTAQLITE_DOC_CONCAT, new_size);
    if (new_id == SYNTAQLITE_NULL_DOC) return new_id;

    // Re-derive old pointer (doc_alloc may have reallocated)
    concat = &((SyntaqliteDoc*)(arena->arena + arena->offsets[concat_id]))->concat;
    SyntaqliteDocConcat *new_concat = &((SyntaqliteDoc*)(arena->arena + arena->offsets[new_id]))->concat;

    // Copy old children and add new one
    memcpy(new_concat->children, concat->children, old_count * sizeof(uint32_t));
    new_concat->children[old_count] = child;
    new_concat->count = new_count;

    return new_id;
}
