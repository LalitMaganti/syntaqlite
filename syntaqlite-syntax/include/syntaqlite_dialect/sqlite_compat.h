// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQLite compatibility definitions for syntaqlite.
// Provides type aliases, macros, and structures needed by tokenizer and parser.

#ifndef SYNTAQLITE_INTERNAL_SQLITE_COMPAT_H
#define SYNTAQLITE_INTERNAL_SQLITE_COMPAT_H

#include <stdint.h>

// SQLite type aliases
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;

// Default to SQLITE_ASCII if not defined and SQLITE_EBCDIC also not defined
#if !defined(SQLITE_ASCII) && !defined(SQLITE_EBCDIC)
#define SQLITE_ASCII 1
#endif

// Default digit separator for numeric literals (e.g., 1_000_000)
#ifndef SQLITE_DIGIT_SEPARATOR
#define SQLITE_DIGIT_SEPARATOR '_'
#endif

// Case-insensitive comparison over ASCII, which is all SQL keywords need.
// Deliberately not libc's strncasecmp: the generated parser is compiled into
// embeddings that do not necessarily provide it, such as an Emscripten side
// module, where the unresolved import aborts the engine mid-parse.
#include <stddef.h>

static inline int synq_strncasecmp_ascii(const char* a,
                                         const char* b,
                                         size_t n) {
  for (size_t i = 0; i < n; i++) {
    unsigned char ca = (unsigned char)a[i];
    unsigned char cb = (unsigned char)b[i];
    if (ca >= 'A' && ca <= 'Z') {
      ca = (unsigned char)(ca - 'A' + 'a');
    }
    if (cb >= 'A' && cb <= 'Z') {
      cb = (unsigned char)(cb - 'A' + 'a');
    }
    if (ca != cb) {
      return (int)ca - (int)cb;
    }
    if (ca == 0) {
      return 0;
    }
  }
  return 0;
}

#define SYNQ_STRNCASECMP synq_strncasecmp_ascii

// C++17 fallthrough, C no-op
#ifdef __cplusplus
#define deliberate_fall_through [[fallthrough]]
#else
#define deliberate_fall_through
#endif  // SYNTAQLITE_INTERNAL_SQLITE_COMPAT_H

// No-op for testcase macro if not defined
#ifndef testcase
#define testcase(X)
#endif

// No-op for assert macro if not defined
#ifndef assert
#define assert(X)
#endif

#endif  // SYNTAQLITE_INTERNAL_SQLITE_COMPAT_H
