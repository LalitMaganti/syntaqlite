// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST print function declarations.
// Implementations are generated in ast_print.c.

#ifndef SYNQ_SRC_AST_AST_PRINT_H
#define SYNQ_SRC_AST_AST_PRINT_H

#include "src/base/arena.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  // SYNQ_SRC_AST_AST_PRINT_H

// Print AST node and its children to a file stream
void synq_ast_print(FILE *out, SynqArena *ast, uint32_t node_id,
                          const char *source);

#ifdef __cplusplus
}
#endif  // SYNQ_SRC_AST_AST_PRINT_H

#endif  // SYNQ_SRC_AST_AST_PRINT_H
