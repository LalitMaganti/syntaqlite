
// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Arena allocator with offset table for node-based data structures.

#ifndef SYNTAQLITE_EXT_ARENA_H
#define SYNTAQLITE_EXT_ARENA_H

#include <stdint.h>

#include "syntaqlite/config.h"
#include "syntaqlite_dialect/vec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynqArena {
  SYNQ_VEC(uint8_t) data;
  SYNQ_VEC(uint32_t) offsets;
} SynqArena;

// Get pointer to node data by offset-table ID.
#define synq_arena_ptr(a, id) \
  (&syntaqlite_vec_at(&(a)->data, syntaqlite_vec_at(&(a)->offsets, id)))

// Const-correct variant for read-only access.
#define synq_arena_cptr(a, id) ((const uint8_t*)synq_arena_ptr((a), (id)))

static inline void synq_arena_init(SynqArena* a) {
  syntaqlite_vec_init(&a->data);
  syntaqlite_vec_init(&a->offsets);
}

static inline void synq_arena_free(SynqArena* a, SyntaqliteMemMethods mem) {
  syntaqlite_vec_free(&a->data, mem);
  syntaqlite_vec_free(&a->offsets, mem);
}

// Reset counts to zero, keeping allocated memory for reuse.
static inline void synq_arena_clear(SynqArena* a) {
  syntaqlite_vec_clear(&a->data);
  syntaqlite_vec_clear(&a->offsets);
}

// Copy data into the arena and register in the offset table.
// Returns the node ID.
static inline uint32_t synq_arena_alloc(SynqArena* a,
                                        const void* data,
                                        uint32_t size,
                                        SyntaqliteMemMethods mem) {
  uint32_t node_id = syntaqlite_vec_len(&a->offsets);
  syntaqlite_vec_push(&a->offsets, syntaqlite_vec_len(&a->data), mem);
  syntaqlite_vec_push_n(&a->data, data, size, mem);
  return node_id;
}

// Reserve a node ID in the offset table without allocating arena bytes.
// The offset is written later by synq_arena_commit.
static inline uint32_t synq_arena_reserve_id(SynqArena* a,
                                             SyntaqliteMemMethods mem) {
  uint32_t node_id = syntaqlite_vec_len(&a->offsets);
  syntaqlite_vec_push(&a->offsets, 0, mem);
  return node_id;
}

// Commit data at a previously reserved node ID.
static inline void synq_arena_commit(SynqArena* a,
                                     uint32_t node_id,
                                     const void* data,
                                     uint32_t size,
                                     SyntaqliteMemMethods mem) {
  syntaqlite_vec_at(&a->offsets, node_id) = syntaqlite_vec_len(&a->data);
  syntaqlite_vec_push_n(&a->data, data, size, mem);
}

// Append raw bytes to the arena without registering an offset entry.
static inline void synq_arena_append(SynqArena* a,
                                     const void* data,
                                     uint32_t size,
                                     SyntaqliteMemMethods mem) {
  syntaqlite_vec_push_n(&a->data, data, size, mem);
}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_EXT_ARENA_H
