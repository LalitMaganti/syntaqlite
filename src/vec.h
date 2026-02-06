// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Header-only type-generic dynamic array.
// Works on any struct with {T *data; uint32_t count; uint32_t capacity;}.

#ifndef SYNTAQLITE_SRC_VEC_H
#define SYNTAQLITE_SRC_VEC_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Abort-on-failure realloc (like Git's xrealloc).
static inline void *syntaqlite_xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) abort();
    return p;
}

// Type constructor
#define SYNTAQLITE_VEC(T) struct { T *data; uint32_t count; uint32_t capacity; }

// Zero-init
#define syntaqlite_vec_init(v) \
    do { (v)->data = NULL; (v)->count = 0; (v)->capacity = 0; } while (0)

// Free + zero
#define syntaqlite_vec_free(v) \
    do { free((v)->data); (v)->data = NULL; (v)->count = 0; (v)->capacity = 0; } while (0)

// Reset count, keep allocation
#define syntaqlite_vec_clear(v) \
    do { (v)->count = 0; } while (0)

// Ensure capacity >= needed
#define syntaqlite_vec_ensure(v, needed) \
    do { \
        if ((needed) > (v)->capacity) { \
            uint32_t _cap = (v)->capacity * 2; \
            if (_cap < (needed)) _cap = (needed); \
            if (_cap < 16) _cap = 16; \
            (v)->data = syntaqlite_xrealloc((v)->data, (size_t)_cap * sizeof(*(v)->data)); \
            (v)->capacity = _cap; \
        } \
    } while (0)

// Append one element, grow if needed
#define syntaqlite_vec_push(v, val) \
    do { \
        syntaqlite_vec_ensure((v), (v)->count + 1); \
        (v)->data[(v)->count++] = (val); \
    } while (0)

// Decrement count, evaluate to last element
#define syntaqlite_vec_pop(v) ((v)->data[--(v)->count])

// Bulk append via memcpy
#define syntaqlite_vec_push_n(v, src, n) \
    do { \
        uint32_t _n = (n); \
        syntaqlite_vec_ensure((v), (v)->count + _n); \
        memcpy((v)->data + (v)->count, (src), (size_t)_n * sizeof(*(v)->data)); \
        (v)->count += _n; \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_VEC_H
