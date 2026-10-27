// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Shared types for syntaqlite.
//
// SyntaqliteMemMethods lets callers override the allocator used for all
// internal allocations. Pass a pointer to one when creating a parser or
// tokenizer, or pass NULL to use the default (malloc/free).
//
// Usage:
//   // Pure defaults — no allocator needed:
//   SyntaqliteParser* p = syntaqlite_parser_create(NULL);  // see
//   parser.h
//
//   // Custom allocator:
//   SyntaqliteMemMethods mem = { my_malloc, my_free };
//   SyntaqliteParser* p = syntaqlite_parser_create(&mem);  // see
//   parser.h

#ifndef SYNTAQLITE_CONFIG_H
#define SYNTAQLITE_CONFIG_H

#include "syntaqlite/symbol_prefix.h"

#include <stddef.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Shared library export
// ---------------------------------------------------------------------------

#ifndef SYNTAQLITE_API
#if defined(SYNTAQLITE_SHARED)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(SYNTAQLITE_BUILDING)
#define SYNTAQLITE_API __declspec(dllexport)
#else
#define SYNTAQLITE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define SYNTAQLITE_API __attribute__((visibility("default")))
#else
#define SYNTAQLITE_API
#endif
#else
#define SYNTAQLITE_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Memory methods
// ---------------------------------------------------------------------------

// Allocator for all syntaqlite-internal allocations. All function
// pointers must be non-NULL. Pass a pointer to one of these to
// syntaqlite_*_create() functions, or pass NULL for malloc/realloc/free.
typedef struct SyntaqliteMemMethods {
  void* (*xMalloc)(size_t);  // Allocate size bytes. Must not return NULL.
  void* (*xRealloc)(void*,
                    size_t);  // Resize allocation. May return new pointer.
  void (*xFree)(void*);       // Free a pointer returned by xMalloc/xRealloc.
} SyntaqliteMemMethods;

// Default allocator (system malloc/realloc/free). Use this when you need
// an explicit allocator but have no custom one:
//   SyntaqliteParser* p =
//   syntaqlite_parser_create(&SYNTAQLITE_MEM_METHODS_DEFAULT);
#define SYNTAQLITE_MEM_METHODS_DEFAULT \
  ((SyntaqliteMemMethods){malloc, realloc, free})

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_CONFIG_H
