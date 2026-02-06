// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQL formatter: converts AST to formatted SQL string.

#ifndef SYNQ_SRC_FMT_FMT_H
#define SYNQ_SRC_FMT_FMT_H

#include "src/ast/ast_base.h"
#include "src/fmt/fmt_options.h"
#include "src/token_list.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Format an AST node to a SQL string.
// Returns malloc'd string (caller frees), or NULL on error.
// token_list may be NULL (comments will be omitted).
char *synq_format(SynqAstContext *astCtx, uint32_t root_id,
                        const char *source, SynqTokenList *token_list,
                        SynqFmtOptions *options);

// Build the doc IR and print it in debug format instead of laying it out.
// Returns malloc'd string (caller frees), or NULL on error.
char *synq_format_debug_ir(SynqAstContext *astCtx, uint32_t root_id,
                                  const char *source, SynqTokenList *token_list,
                                  SynqFmtOptions *options);

#ifdef __cplusplus
}
#endif

#endif // SYNQ_SRC_FMT_FMT_H
