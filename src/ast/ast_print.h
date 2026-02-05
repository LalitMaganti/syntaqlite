// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#ifndef SYNTAQLITE_AST_PRINT_H
#define SYNTAQLITE_AST_PRINT_H

#include "src/ast/ast_nodes.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Print AST node and its children to a file stream.
// Uses a simple indented text format suitable for diff tests.
void syntaqlite_ast_print(FILE *out, SyntaqliteAst *ast, uint32_t node_id,
                          const char *source);

// Print AST to stdout
void syntaqlite_ast_dump(SyntaqliteAst *ast, uint32_t root_id,
                         const char *source);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_PRINT_H
