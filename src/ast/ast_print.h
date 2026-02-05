// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST print function declarations.
// Implementations are generated in ast_print.c.

#ifndef SYNTAQLITE_AST_PRINT_H
#define SYNTAQLITE_AST_PRINT_H

#include "src/ast/ast_base.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Print AST node and its children to a file stream
void syntaqlite_ast_print(FILE *out, SyntaqliteAst *ast, uint32_t node_id,
                          const char *source);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_PRINT_H
