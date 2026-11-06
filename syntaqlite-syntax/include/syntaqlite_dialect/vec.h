// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Header-only type-generic dynamic array.
// Works on any struct with {T *data; uint32_t count; uint32_t capacity;}.
//
// All mutating operations take a SyntaqliteMemMethods parameter for
// allocation. This lets the vec use the caller's configured allocator.

#ifndef SYNTAQLITE_EXT_VEC_H
#define SYNTAQLITE_EXT_VEC_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type constructor
#define SYNQ_VEC(T)    \
  struct {             \
    T* data;           \
    uint32_t count;    \
    uint32_t capacity; \
  }

// Zero-init
#define syntaqlite_vec_init(v) \
  do {                         \
    (v)->data = NULL;          \
    (v)->count = 0;            \
    (v)->capacity = 0;         \
  } while (0)

// Free + zero
#define syntaqlite_vec_free(v, mem) \
  do {                              \
    (mem).xFree((v)->data);         \
    (v)->data = NULL;               \
    (v)->count = 0;                 \
    (v)->capacity = 0;              \
  } while (0)

// Reset count, keep allocation
#define syntaqlite_vec_clear(v) \
  do {                          \
    (v)->count = 0;             \
  } while (0)

// gcc rejects `inline` with `noinline`, and clang warns when something marked
// `unused` is used, so each gets its own spelling.
#if defined(__clang__)
#define SYNQ_VEC_COLD_FN static inline __attribute__((noinline, cold))
#elif defined(__GNUC__)
#define SYNQ_VEC_COLD_FN static __attribute__((noinline, cold, unused))
#elif defined(_MSC_VER)
#define SYNQ_VEC_COLD_FN static __inline __declspec(noinline)
#else
#define SYNQ_VEC_COLD_FN static inline
#endif

// Out of line so a push's common path has no call in it, and stays small
// enough to inline.
SYNQ_VEC_COLD_FN void* syntaqlite_vec_grow_(void* data,
                                            uint32_t* capacity,
                                            uint32_t needed,
                                            size_t elem_size,
                                            void* (*xRealloc)(void*, size_t)) {
  uint32_t cap = *capacity ? *capacity : 16;
  while (cap < needed)
    cap *= 2;
  *capacity = cap;
  return xRealloc(data, (size_t)cap * elem_size);
}

// Ensure capacity >= needed (capacity is always a power of two).
#define syntaqlite_vec_ensure(v, needed, mem)                      \
  do {                                                             \
    if ((needed) > (v)->capacity) {                                \
      (v)->data = (__typeof__((v)->data))syntaqlite_vec_grow_(     \
          (v)->data, &(v)->capacity, (needed), sizeof(*(v)->data), \
          (mem).xRealloc);                                         \
    }                                                              \
  } while (0)

// Append one element, grow if needed
#define syntaqlite_vec_push(v, val, mem)             \
  do {                                               \
    syntaqlite_vec_ensure((v), (v)->count + 1, mem); \
    (v)->data[(v)->count++] = (val);                 \
  } while (0)

// Element count
#define syntaqlite_vec_len(v) ((v)->count)

// Lvalue access to element at index
#define syntaqlite_vec_at(v, i) ((v)->data[i])

// Set count to n, discarding trailing elements
#define syntaqlite_vec_truncate(v, n) \
  do {                                \
    (v)->count = (n);                 \
  } while (0)

// Decrement count, evaluate to last element
#define syntaqlite_vec_pop(v) ((v)->data[--(v)->count])

// Bulk append via memcpy
#define syntaqlite_vec_push_n(v, src, n, mem)                               \
  do {                                                                      \
    uint32_t _n = (n);                                                      \
    syntaqlite_vec_ensure((v), (v)->count + _n, mem);                       \
    memcpy((v)->data + (v)->count, (src), (size_t)_n * sizeof(*(v)->data)); \
    (v)->count += _n;                                                       \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_EXT_VEC_H
