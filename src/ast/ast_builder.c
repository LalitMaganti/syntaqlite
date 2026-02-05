// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#include "src/ast/ast_builder.h"

#include <stdlib.h>
#include <string.h>

// ============ Node Size Table ============

static const size_t node_base_sizes[] = {
    [SYNTAQLITE_NODE_NULL] = 0,
    [SYNTAQLITE_NODE_BINARY_EXPR] = sizeof(SyntaqliteBinaryExpr),
    [SYNTAQLITE_NODE_UNARY_EXPR] = sizeof(SyntaqliteUnaryExpr),
    [SYNTAQLITE_NODE_LITERAL] = sizeof(SyntaqliteLiteral),
    [SYNTAQLITE_NODE_EXPR_LIST] = sizeof(SyntaqliteExprList),
    [SYNTAQLITE_NODE_RESULT_COLUMN] = sizeof(SyntaqliteResultColumn),
    [SYNTAQLITE_NODE_RESULT_COLUMN_LIST] = sizeof(SyntaqliteResultColumnList),
    [SYNTAQLITE_NODE_SELECT_STMT] = sizeof(SyntaqliteSelectStmt),
    [SYNTAQLITE_NODE_ORDERING_TERM] = sizeof(SyntaqliteOrderingTerm),
    [SYNTAQLITE_NODE_ORDER_BY_LIST] = sizeof(SyntaqliteOrderByList),
    [SYNTAQLITE_NODE_LIMIT_CLAUSE] = sizeof(SyntaqliteLimitClause),
    [SYNTAQLITE_NODE_COLUMN_REF] = sizeof(SyntaqliteColumnRef),
    [SYNTAQLITE_NODE_FUNCTION_CALL] = sizeof(SyntaqliteFunctionCall),
    [SYNTAQLITE_NODE_IS_EXPR] = sizeof(SyntaqliteIsExpr),
    [SYNTAQLITE_NODE_BETWEEN_EXPR] = sizeof(SyntaqliteBetweenExpr),
    [SYNTAQLITE_NODE_LIKE_EXPR] = sizeof(SyntaqliteLikeExpr),
    [SYNTAQLITE_NODE_CASE_EXPR] = sizeof(SyntaqliteCaseExpr),
    [SYNTAQLITE_NODE_CASE_WHEN] = sizeof(SyntaqliteCaseWhen),
    [SYNTAQLITE_NODE_CASE_WHEN_LIST] = sizeof(SyntaqliteCaseWhenList),
    [SYNTAQLITE_NODE_COMPOUND_SELECT] = sizeof(SyntaqliteCompoundSelect),
    [SYNTAQLITE_NODE_SUBQUERY_EXPR] = sizeof(SyntaqliteSubqueryExpr),
    [SYNTAQLITE_NODE_EXISTS_EXPR] = sizeof(SyntaqliteExistsExpr),
    [SYNTAQLITE_NODE_IN_EXPR] = sizeof(SyntaqliteInExpr),
    [SYNTAQLITE_NODE_VARIABLE] = sizeof(SyntaqliteVariable),
    [SYNTAQLITE_NODE_COLLATE_EXPR] = sizeof(SyntaqliteCollateExpr),
};

size_t syntaqlite_node_base_size(uint8_t tag) {
    if (tag >= SYNTAQLITE_NODE_COUNT) return 0;
    return node_base_sizes[tag];
}

// ============ Builder Functions ============

uint32_t ast_binary_expr(SyntaqliteAstContext *ctx, SyntaqliteBinaryOp op, uint32_t left, uint32_t right) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_BINARY_EXPR, sizeof(SyntaqliteBinaryExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteBinaryExpr *node = (SyntaqliteBinaryExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;
    return id;
}

uint32_t ast_unary_expr(SyntaqliteAstContext *ctx, SyntaqliteUnaryOp op, uint32_t operand) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_UNARY_EXPR, sizeof(SyntaqliteUnaryExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteUnaryExpr *node = (SyntaqliteUnaryExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->operand = operand;
    return id;
}

uint32_t ast_literal(
    SyntaqliteAstContext *ctx,
    SyntaqliteLiteralType literal_type,
    SyntaqliteSourceSpan source
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_LITERAL, sizeof(SyntaqliteLiteral));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteLiteral *node = (SyntaqliteLiteral*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->literal_type = literal_type;
    node->source = source;
    return id;
}

uint32_t ast_expr_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_EXPR_LIST, sizeof(SyntaqliteExprList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteExprList *node = (SyntaqliteExprList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_expr_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_EXPR_LIST, sizeof(SyntaqliteExprList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteExprList *node = (SyntaqliteExprList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_expr_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_expr_list(ctx, child);
    }

    // Get current list
    SyntaqliteExprList *old_node = (SyntaqliteExprList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteExprList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_EXPR_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteExprList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteExprList *new_node = (SyntaqliteExprList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_result_column(
    SyntaqliteAstContext *ctx,
    uint8_t flags,
    SyntaqliteSourceSpan alias,
    uint32_t expr
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN, sizeof(SyntaqliteResultColumn));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteResultColumn *node = (SyntaqliteResultColumn*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->flags = flags;
    node->alias = alias;
    node->expr = expr;
    return id;
}

uint32_t ast_result_column_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, sizeof(SyntaqliteResultColumnList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteResultColumnList *node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_result_column_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, sizeof(SyntaqliteResultColumnList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteResultColumnList *node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_result_column_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_result_column_list(ctx, child);
    }

    // Get current list
    SyntaqliteResultColumnList *old_node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteResultColumnList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteResultColumnList *new_node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_select_stmt(
    SyntaqliteAstContext *ctx,
    uint8_t flags,
    uint32_t columns,
    uint32_t where,
    uint32_t groupby,
    uint32_t having,
    uint32_t orderby,
    uint32_t limit_clause
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SELECT_STMT, sizeof(SyntaqliteSelectStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSelectStmt *node = (SyntaqliteSelectStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->flags = flags;
    node->columns = columns;
    node->where = where;
    node->groupby = groupby;
    node->having = having;
    node->orderby = orderby;
    node->limit_clause = limit_clause;
    return id;
}

uint32_t ast_ordering_term(
    SyntaqliteAstContext *ctx,
    uint32_t expr,
    SyntaqliteSortOrder sort_order,
    SyntaqliteNullsOrder nulls_order
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_ORDERING_TERM, sizeof(SyntaqliteOrderingTerm));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteOrderingTerm *node = (SyntaqliteOrderingTerm*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->expr = expr;
    node->sort_order = sort_order;
    node->nulls_order = nulls_order;
    return id;
}

uint32_t ast_order_by_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_ORDER_BY_LIST, sizeof(SyntaqliteOrderByList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteOrderByList *node = (SyntaqliteOrderByList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_order_by_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_ORDER_BY_LIST, sizeof(SyntaqliteOrderByList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteOrderByList *node = (SyntaqliteOrderByList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_order_by_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_order_by_list(ctx, child);
    }

    // Get current list
    SyntaqliteOrderByList *old_node = (SyntaqliteOrderByList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteOrderByList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_ORDER_BY_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteOrderByList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteOrderByList *new_node = (SyntaqliteOrderByList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_limit_clause(SyntaqliteAstContext *ctx, uint32_t limit, uint32_t offset) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_LIMIT_CLAUSE, sizeof(SyntaqliteLimitClause));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteLimitClause *node = (SyntaqliteLimitClause*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->limit = limit;
    node->offset = offset;
    return id;
}

uint32_t ast_column_ref(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan column,
    SyntaqliteSourceSpan table,
    SyntaqliteSourceSpan schema
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_REF, sizeof(SyntaqliteColumnRef));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnRef *node = (SyntaqliteColumnRef*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->column = column;
    node->table = table;
    node->schema = schema;
    return id;
}

uint32_t ast_function_call(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan func_name,
    uint8_t flags,
    uint32_t args
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_FUNCTION_CALL, sizeof(SyntaqliteFunctionCall));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteFunctionCall *node = (SyntaqliteFunctionCall*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->func_name = func_name;
    node->flags = flags;
    node->args = args;
    return id;
}

uint32_t ast_is_expr(SyntaqliteAstContext *ctx, SyntaqliteIsOp op, uint32_t left, uint32_t right) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_IS_EXPR, sizeof(SyntaqliteIsExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteIsExpr *node = (SyntaqliteIsExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;
    return id;
}

uint32_t ast_between_expr(
    SyntaqliteAstContext *ctx,
    uint8_t negated,
    uint32_t operand,
    uint32_t low,
    uint32_t high
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_BETWEEN_EXPR, sizeof(SyntaqliteBetweenExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteBetweenExpr *node = (SyntaqliteBetweenExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->negated = negated;
    node->operand = operand;
    node->low = low;
    node->high = high;
    return id;
}

uint32_t ast_like_expr(
    SyntaqliteAstContext *ctx,
    uint8_t negated,
    uint32_t operand,
    uint32_t pattern,
    uint32_t escape
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_LIKE_EXPR, sizeof(SyntaqliteLikeExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteLikeExpr *node = (SyntaqliteLikeExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->negated = negated;
    node->operand = operand;
    node->pattern = pattern;
    node->escape = escape;
    return id;
}

uint32_t ast_case_expr(SyntaqliteAstContext *ctx, uint32_t operand, uint32_t else_expr, uint32_t whens) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CASE_EXPR, sizeof(SyntaqliteCaseExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCaseExpr *node = (SyntaqliteCaseExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->operand = operand;
    node->else_expr = else_expr;
    node->whens = whens;
    return id;
}

uint32_t ast_case_when(SyntaqliteAstContext *ctx, uint32_t when_expr, uint32_t then_expr) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CASE_WHEN, sizeof(SyntaqliteCaseWhen));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCaseWhen *node = (SyntaqliteCaseWhen*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->when_expr = when_expr;
    node->then_expr = then_expr;
    return id;
}

uint32_t ast_case_when_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CASE_WHEN_LIST, sizeof(SyntaqliteCaseWhenList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCaseWhenList *node = (SyntaqliteCaseWhenList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_case_when_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CASE_WHEN_LIST, sizeof(SyntaqliteCaseWhenList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCaseWhenList *node = (SyntaqliteCaseWhenList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_case_when_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_case_when_list(ctx, child);
    }

    // Get current list
    SyntaqliteCaseWhenList *old_node = (SyntaqliteCaseWhenList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteCaseWhenList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_CASE_WHEN_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteCaseWhenList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteCaseWhenList *new_node = (SyntaqliteCaseWhenList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_compound_select(
    SyntaqliteAstContext *ctx,
    SyntaqliteCompoundOp op,
    uint32_t left,
    uint32_t right
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COMPOUND_SELECT, sizeof(SyntaqliteCompoundSelect));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCompoundSelect *node = (SyntaqliteCompoundSelect*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;
    return id;
}

uint32_t ast_subquery_expr(SyntaqliteAstContext *ctx, uint32_t select) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SUBQUERY_EXPR, sizeof(SyntaqliteSubqueryExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSubqueryExpr *node = (SyntaqliteSubqueryExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->select = select;
    return id;
}

uint32_t ast_exists_expr(SyntaqliteAstContext *ctx, uint32_t select) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_EXISTS_EXPR, sizeof(SyntaqliteExistsExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteExistsExpr *node = (SyntaqliteExistsExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->select = select;
    return id;
}

uint32_t ast_in_expr(SyntaqliteAstContext *ctx, uint8_t negated, uint32_t operand, uint32_t source) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_IN_EXPR, sizeof(SyntaqliteInExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteInExpr *node = (SyntaqliteInExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->negated = negated;
    node->operand = operand;
    node->source = source;
    return id;
}

uint32_t ast_variable(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan source) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_VARIABLE, sizeof(SyntaqliteVariable));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteVariable *node = (SyntaqliteVariable*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->source = source;
    return id;
}

uint32_t ast_collate_expr(SyntaqliteAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan collation) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLLATE_EXPR, sizeof(SyntaqliteCollateExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCollateExpr *node = (SyntaqliteCollateExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->expr = expr;
    node->collation = collation;
    return id;
}

