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

uint32_t ast_is_expr(SyntaqliteAstContext *ctx, SyntaqliteIsOp op, uint32_t left, uint32_t right);

uint32_t ast_between_expr(
    SyntaqliteAstContext *ctx,
    uint8_t negated,
    uint32_t operand,
    uint32_t low,
    uint32_t high
);

uint32_t ast_like_expr(
    SyntaqliteAstContext *ctx,
    uint8_t negated,
    uint32_t operand,
    uint32_t pattern,
    uint32_t escape
);

uint32_t ast_case_expr(SyntaqliteAstContext *ctx, uint32_t operand, uint32_t else_expr, uint32_t whens);

uint32_t ast_case_when(SyntaqliteAstContext *ctx, uint32_t when_expr, uint32_t then_expr);

// Create empty CaseWhenList
uint32_t ast_case_when_list_empty(SyntaqliteAstContext *ctx);

// Create CaseWhenList with single child
uint32_t ast_case_when_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to CaseWhenList (may reallocate, returns new list ID)
uint32_t ast_case_when_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_compound_select(
    SyntaqliteAstContext *ctx,
    SyntaqliteCompoundOp op,
    uint32_t left,
    uint32_t right
);

uint32_t ast_subquery_expr(SyntaqliteAstContext *ctx, uint32_t select);

uint32_t ast_exists_expr(SyntaqliteAstContext *ctx, uint32_t select);

uint32_t ast_in_expr(SyntaqliteAstContext *ctx, uint8_t negated, uint32_t operand, uint32_t source);

uint32_t ast_variable(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan source);

uint32_t ast_collate_expr(SyntaqliteAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan collation);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_BUILDER_H
