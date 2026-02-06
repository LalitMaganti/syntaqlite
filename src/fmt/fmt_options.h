// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Formatting options for the SQL pretty printer.

#ifndef SYNQ_SRC_FMT_FMT_OPTIONS_H
#define SYNQ_SRC_FMT_FMT_OPTIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynqFmtOptions {
    uint32_t target_width;   // default 80
    uint32_t indent_width;   // default 2
} SynqFmtOptions;

#define SYNQ_FMT_OPTIONS_DEFAULT {80, 2}

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_FMT_FMT_OPTIONS_H
