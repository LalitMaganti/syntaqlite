// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Abort-on-failure allocation wrappers (like Git's xmalloc/xrealloc).

#ifndef SYNQ_SRC_BASE_XMALLOC_H
#define SYNQ_SRC_BASE_XMALLOC_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *synq_xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) abort();
    return p;
}

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_BASE_XMALLOC_H
