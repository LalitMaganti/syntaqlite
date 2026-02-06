// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Arena allocator with offset table for node-based data structures.

#ifndef SYNTAQLITE_SRC_ARENA_H
#define SYNTAQLITE_SRC_ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SyntaqliteArena {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
    uint32_t *offsets;
    uint32_t count;
    uint32_t offset_capacity;
} SyntaqliteArena;

void syntaqlite_arena_init(SyntaqliteArena *a);
void syntaqlite_arena_free(SyntaqliteArena *a);

// Allocate space in the arena for a node with the given tag and size.
// Returns the node ID. Aborts on OOM.
uint32_t syntaqlite_arena_alloc(SyntaqliteArena *a, uint8_t tag, size_t size);

// Ensure the arena has room for additional bytes (without allocating a node).
void syntaqlite_arena_ensure(SyntaqliteArena *a, size_t additional);

// Reserve a node ID in the offset table without allocating arena bytes.
// The offset is written later (e.g., by the list accumulator flush).
uint32_t syntaqlite_arena_reserve_id(SyntaqliteArena *a);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_ARENA_H
