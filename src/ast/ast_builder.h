// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

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

// Create empty ExprList
uint32_t ast_expr_list_empty(SyntaqliteAstContext *ctx);

// Create ExprList with single child
uint32_t ast_expr_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to ExprList (may reallocate, returns new list ID)
uint32_t ast_expr_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

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

uint32_t ast_select_stmt(
    SyntaqliteAstContext *ctx,
    uint8_t flags,
    uint32_t columns,
    uint32_t where,
    uint32_t groupby,
    uint32_t having,
    uint32_t orderby,
    uint32_t limit_clause
);

uint32_t ast_ordering_term(
    SyntaqliteAstContext *ctx,
    uint32_t expr,
    SyntaqliteSortOrder sort_order,
    SyntaqliteNullsOrder nulls_order
);

// Create empty OrderByList
uint32_t ast_order_by_list_empty(SyntaqliteAstContext *ctx);

// Create OrderByList with single child
uint32_t ast_order_by_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to OrderByList (may reallocate, returns new list ID)
uint32_t ast_order_by_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_limit_clause(SyntaqliteAstContext *ctx, uint32_t limit, uint32_t offset);

uint32_t ast_column_ref(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan column,
    SyntaqliteSourceSpan table,
    SyntaqliteSourceSpan schema
);

uint32_t ast_function_call(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan func_name,
    uint8_t flags,
    uint32_t args
);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_BUILDER_H
