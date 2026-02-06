// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Wadler-Lindig layout engine: renders a doc tree to a formatted string.

#ifndef SYNQ_SRC_FMT_DOC_LAYOUT_H
#define SYNQ_SRC_FMT_DOC_LAYOUT_H

#include "src/fmt/doc.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Render a document tree to a formatted string.
// Returns malloc'd string (caller frees), or NULL on error.
char *synq_doc_layout(SynqDocContext *ctx, uint32_t root_id, uint32_t target_width);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_FMT_DOC_LAYOUT_H
