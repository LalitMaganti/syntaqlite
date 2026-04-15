// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Small shared helpers used across the parser C sources.

#ifndef SYNTAQLITE_INTERNAL_UTIL_H
#define SYNTAQLITE_INTERNAL_UTIL_H

#include <stdint.h>

// Case-insensitive name comparison.
static inline int synq_name_eq_ci(const char* a,
                                  uint32_t alen,
                                  const char* b,
                                  uint32_t blen) {
  if (alen != blen)
    return 0;
  for (uint32_t i = 0; i < alen; i++) {
    uint8_t ca = (uint8_t)a[i], cb = (uint8_t)b[i];
    if (ca >= 'A' && ca <= 'Z')
      ca += 32;
    if (cb >= 'A' && cb <= 'Z')
      cb += 32;
    if (ca != cb)
      return 0;
  }
  return 1;
}

#endif  // SYNTAQLITE_INTERNAL_UTIL_H
