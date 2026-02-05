// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#ifndef SYNTAQLITE_AST_BUILDER_H
#define SYNTAQLITE_AST_BUILDER_H

#include "src/ast/ast_nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============ Builder Functions ============

uint32_t ast_binary_expr(SyntaqliteAstContext *ctx, SyntaqliteBinaryOp op, uint32_t left, uint32_t right);

uint32_t ast_unary_expr(SyntaqliteAstContext *ctx, SyntaqliteUnaryOp op, uint32_t operand);

uint32_t ast_literal(
    SyntaqliteAstContext *ctx,
    SyntaqliteLiteralType literal_type,
    SyntaqliteSourceSpan source
);

uint32_t ast_result_column(
    SyntaqliteAstContext *ctx,
    uint8_t flags,
    SyntaqliteSourceSpan alias,
    uint32_t expr
);

// Create empty ResultColumnList
uint32_t ast_result_column_list_empty(SyntaqliteAstContext *ctx);

// Create ResultColumnList with single child
uint32_t ast_result_column_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to ResultColumnList (may reallocate, returns new list ID)
uint32_t ast_result_column_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_select_stmt(SyntaqliteAstContext *ctx, uint8_t flags, uint32_t columns);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_BUILDER_H
