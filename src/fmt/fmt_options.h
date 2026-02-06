// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Formatting options for the SQL pretty printer.

#ifndef SYNTAQLITE_SRC_FMT_FMT_OPTIONS_H
#define SYNTAQLITE_SRC_FMT_FMT_OPTIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SyntaqliteFmtOptions {
    uint32_t target_width;   // default 80
    uint32_t indent_width;   // default 2
} SyntaqliteFmtOptions;

#define SYNTAQLITE_FMT_OPTIONS_DEFAULT {80, 2}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_FMT_FMT_OPTIONS_H
