// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQL formatter: converts AST to formatted SQL string.

#ifndef SYNTAQLITE_SRC_FMT_FMT_H
#define SYNTAQLITE_SRC_FMT_FMT_H

#include "src/arena.h"
#include "src/fmt/fmt_options.h"
#include "src/token_list.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Format an AST node to a SQL string.
// Returns malloc'd string (caller frees), or NULL on error.
// token_list may be NULL (comments will be omitted).
char *syntaqlite_format(SyntaqliteArena *ast, uint32_t root_id,
                        const char *source, SyntaqliteTokenList *token_list,
                        SyntaqliteFmtOptions *options);

#ifdef __cplusplus
}
#endif

#endif // SYNTAQLITE_SRC_FMT_FMT_H
