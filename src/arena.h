// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Arena allocator with offset table for node-based data structures.

#ifndef SYNQ_SRC_ARENA_H
#define SYNQ_SRC_ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynqArena {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
    uint32_t *offsets;
    uint32_t count;
    uint32_t offset_capacity;
} SynqArena;

void synq_arena_init(SynqArena *a);
void synq_arena_free(SynqArena *a);

// Allocate space in the arena for a node with the given tag and size.
// Returns the node ID. Aborts on OOM.
uint32_t synq_arena_alloc(SynqArena *a, uint8_t tag, size_t size);

// Ensure the arena has room for additional bytes (without allocating a node).
void synq_arena_ensure(SynqArena *a, size_t additional);

// Reserve a node ID in the offset table without allocating arena bytes.
// The offset is written later (e.g., by the list accumulator flush).
uint32_t synq_arena_reserve_id(SynqArena *a);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_ARENA_H
