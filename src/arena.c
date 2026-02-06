// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Shared arena allocator with offset table.

#include "src/arena.h"
#include "src/vec.h"

#include <stdlib.h>

void syntaqlite_arena_init(SyntaqliteArena *a) {
    a->data = NULL;
    a->size = 0;
    a->capacity = 0;
    a->offsets = NULL;
    a->count = 0;
    a->offset_capacity = 0;
}

void syntaqlite_arena_free(SyntaqliteArena *a) {
    free(a->data);
    free(a->offsets);
    a->data = NULL;
    a->offsets = NULL;
    a->size = 0;
    a->capacity = 0;
    a->count = 0;
    a->offset_capacity = 0;
}

uint32_t syntaqlite_arena_alloc(SyntaqliteArena *a, uint8_t tag, size_t size) {
    // Grow arena if needed
    if (a->size + size > a->capacity) {
        size_t new_capacity = a->capacity * 2 + size + 1024;
        a->data = (uint8_t *)syntaqlite_xrealloc(a->data, new_capacity);
        a->capacity = (uint32_t)new_capacity;
    }

    // Grow offset table if needed
    if (a->count >= a->offset_capacity) {
        size_t new_capacity = a->offset_capacity * 2 + 64;
        a->offsets = (uint32_t *)syntaqlite_xrealloc(
            a->offsets, new_capacity * sizeof(uint32_t));
        a->offset_capacity = (uint32_t)new_capacity;
    }

    uint32_t node_id = a->count++;
    a->offsets[node_id] = a->size;
    a->data[a->size] = tag;
    a->size += (uint32_t)size;

    return node_id;
}

void syntaqlite_arena_ensure(SyntaqliteArena *a, size_t additional) {
    if (a->size + additional > a->capacity) {
        size_t new_capacity = a->capacity * 2 + additional + 1024;
        a->data = (uint8_t *)syntaqlite_xrealloc(a->data, new_capacity);
        a->capacity = (uint32_t)new_capacity;
    }
}

uint32_t syntaqlite_arena_reserve_id(SyntaqliteArena *a) {
    if (a->count >= a->offset_capacity) {
        size_t new_capacity = a->offset_capacity * 2 + 64;
        a->offsets = (uint32_t *)syntaqlite_xrealloc(
            a->offsets, new_capacity * sizeof(uint32_t));
        a->offset_capacity = (uint32_t)new_capacity;
    }
    return a->count++;
}
