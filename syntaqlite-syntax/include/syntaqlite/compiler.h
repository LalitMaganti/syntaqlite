// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Compiler abstraction macros.
//
// Wrappers for features that differ across compilers and C/C++ dialects,
// letting the rest of the codebase use a single spelling.

#ifndef SYNTAQLITE_COMPILER_H
#define SYNTAQLITE_COMPILER_H

// Portable compile-time assertion. Use in file scope.
//
// MSVC does not support C's `_Static_assert` keyword in C mode unless built
// with `/std:c11` or newer, so we need dialect-specific fallbacks:
//   - C++: `static_assert` is a keyword since C++11.
//   - C23: `static_assert` is a keyword.
//   - C11+: `_Static_assert` is the keyword form.
//   - Older C (including MSVC without /std:c11): typedef a 1- or -1-element
//     char array, which fails to compile on false.
#if defined(__cplusplus)
#define SYNQ_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define SYNQ_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define SYNQ_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define SYNQ_STATIC_ASSERT_CAT_(a, b) a##b
#define SYNQ_STATIC_ASSERT_CAT(a, b) SYNQ_STATIC_ASSERT_CAT_(a, b)
#define SYNQ_STATIC_ASSERT(cond, msg)                      \
  typedef char SYNQ_STATIC_ASSERT_CAT(synq_static_assert_, \
                                      __LINE__)[(cond) ? 1 : -1]
#endif

#endif  // SYNTAQLITE_COMPILER_H
