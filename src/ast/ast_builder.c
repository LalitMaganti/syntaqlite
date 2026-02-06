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
    [SYNTAQLITE_NODE_CAST_EXPR] = sizeof(SyntaqliteCastExpr),
    [SYNTAQLITE_NODE_VALUES_ROW_LIST] = sizeof(SyntaqliteValuesRowList),
    [SYNTAQLITE_NODE_VALUES_CLAUSE] = sizeof(SyntaqliteValuesClause),
    [SYNTAQLITE_NODE_CTE_DEFINITION] = sizeof(SyntaqliteCteDefinition),
    [SYNTAQLITE_NODE_CTE_LIST] = sizeof(SyntaqliteCteList),
    [SYNTAQLITE_NODE_WITH_CLAUSE] = sizeof(SyntaqliteWithClause),
    [SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL] = sizeof(SyntaqliteAggregateFunctionCall),
    [SYNTAQLITE_NODE_RAISE_EXPR] = sizeof(SyntaqliteRaiseExpr),
    [SYNTAQLITE_NODE_TABLE_REF] = sizeof(SyntaqliteTableRef),
    [SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE] = sizeof(SyntaqliteSubqueryTableSource),
    [SYNTAQLITE_NODE_JOIN_CLAUSE] = sizeof(SyntaqliteJoinClause),
    [SYNTAQLITE_NODE_JOIN_PREFIX] = sizeof(SyntaqliteJoinPrefix),
    [SYNTAQLITE_NODE_DELETE_STMT] = sizeof(SyntaqliteDeleteStmt),
    [SYNTAQLITE_NODE_SET_CLAUSE] = sizeof(SyntaqliteSetClause),
    [SYNTAQLITE_NODE_SET_CLAUSE_LIST] = sizeof(SyntaqliteSetClauseList),
    [SYNTAQLITE_NODE_UPDATE_STMT] = sizeof(SyntaqliteUpdateStmt),
    [SYNTAQLITE_NODE_INSERT_STMT] = sizeof(SyntaqliteInsertStmt),
    [SYNTAQLITE_NODE_QUALIFIED_NAME] = sizeof(SyntaqliteQualifiedName),
    [SYNTAQLITE_NODE_DROP_STMT] = sizeof(SyntaqliteDropStmt),
    [SYNTAQLITE_NODE_ALTER_TABLE_STMT] = sizeof(SyntaqliteAlterTableStmt),
    [SYNTAQLITE_NODE_TRANSACTION_STMT] = sizeof(SyntaqliteTransactionStmt),
    [SYNTAQLITE_NODE_SAVEPOINT_STMT] = sizeof(SyntaqliteSavepointStmt),
    [SYNTAQLITE_NODE_PRAGMA_STMT] = sizeof(SyntaqlitePragmaStmt),
    [SYNTAQLITE_NODE_ANALYZE_STMT] = sizeof(SyntaqliteAnalyzeStmt),
    [SYNTAQLITE_NODE_ATTACH_STMT] = sizeof(SyntaqliteAttachStmt),
    [SYNTAQLITE_NODE_DETACH_STMT] = sizeof(SyntaqliteDetachStmt),
    [SYNTAQLITE_NODE_VACUUM_STMT] = sizeof(SyntaqliteVacuumStmt),
    [SYNTAQLITE_NODE_EXPLAIN_STMT] = sizeof(SyntaqliteExplainStmt),
    [SYNTAQLITE_NODE_CREATE_INDEX_STMT] = sizeof(SyntaqliteCreateIndexStmt),
    [SYNTAQLITE_NODE_CREATE_VIEW_STMT] = sizeof(SyntaqliteCreateViewStmt),
    [SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE] = sizeof(SyntaqliteForeignKeyClause),
    [SYNTAQLITE_NODE_COLUMN_CONSTRAINT] = sizeof(SyntaqliteColumnConstraint),
    [SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST] = sizeof(SyntaqliteColumnConstraintList),
    [SYNTAQLITE_NODE_COLUMN_DEF] = sizeof(SyntaqliteColumnDef),
    [SYNTAQLITE_NODE_COLUMN_DEF_LIST] = sizeof(SyntaqliteColumnDefList),
    [SYNTAQLITE_NODE_TABLE_CONSTRAINT] = sizeof(SyntaqliteTableConstraint),
    [SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST] = sizeof(SyntaqliteTableConstraintList),
    [SYNTAQLITE_NODE_CREATE_TABLE_STMT] = sizeof(SyntaqliteCreateTableStmt),
    [SYNTAQLITE_NODE_FRAME_BOUND] = sizeof(SyntaqliteFrameBound),
    [SYNTAQLITE_NODE_FRAME_SPEC] = sizeof(SyntaqliteFrameSpec),
    [SYNTAQLITE_NODE_WINDOW_DEF] = sizeof(SyntaqliteWindowDef),
    [SYNTAQLITE_NODE_WINDOW_DEF_LIST] = sizeof(SyntaqliteWindowDefList),
    [SYNTAQLITE_NODE_NAMED_WINDOW_DEF] = sizeof(SyntaqliteNamedWindowDef),
    [SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST] = sizeof(SyntaqliteNamedWindowDefList),
    [SYNTAQLITE_NODE_FILTER_OVER] = sizeof(SyntaqliteFilterOver),
    [SYNTAQLITE_NODE_TRIGGER_EVENT] = sizeof(SyntaqliteTriggerEvent),
    [SYNTAQLITE_NODE_TRIGGER_CMD_LIST] = sizeof(SyntaqliteTriggerCmdList),
    [SYNTAQLITE_NODE_CREATE_TRIGGER_STMT] = sizeof(SyntaqliteCreateTriggerStmt),
    [SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT] = sizeof(SyntaqliteCreateVirtualTableStmt),
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
    SyntaqliteResultColumnFlags flags,
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
    SyntaqliteSelectStmtFlags flags,
    uint32_t columns,
    uint32_t from_clause,
    uint32_t where,
    uint32_t groupby,
    uint32_t having,
    uint32_t orderby,
    uint32_t limit_clause,
    uint32_t window_clause
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SELECT_STMT, sizeof(SyntaqliteSelectStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSelectStmt *node = (SyntaqliteSelectStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->flags = flags;
    node->columns = columns;
    node->from_clause = from_clause;
    node->where = where;
    node->groupby = groupby;
    node->having = having;
    node->orderby = orderby;
    node->limit_clause = limit_clause;
    node->window_clause = window_clause;
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
    SyntaqliteFunctionCallFlags flags,
    uint32_t args,
    uint32_t filter_clause,
    uint32_t over_clause
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_FUNCTION_CALL, sizeof(SyntaqliteFunctionCall));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteFunctionCall *node = (SyntaqliteFunctionCall*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->func_name = func_name;
    node->flags = flags;
    node->args = args;
    node->filter_clause = filter_clause;
    node->over_clause = over_clause;
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

uint32_t ast_cast_expr(SyntaqliteAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan type_name) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CAST_EXPR, sizeof(SyntaqliteCastExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCastExpr *node = (SyntaqliteCastExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->expr = expr;
    node->type_name = type_name;
    return id;
}

uint32_t ast_values_row_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_VALUES_ROW_LIST, sizeof(SyntaqliteValuesRowList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteValuesRowList *node = (SyntaqliteValuesRowList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_values_row_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_VALUES_ROW_LIST, sizeof(SyntaqliteValuesRowList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteValuesRowList *node = (SyntaqliteValuesRowList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_values_row_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_values_row_list(ctx, child);
    }

    // Get current list
    SyntaqliteValuesRowList *old_node = (SyntaqliteValuesRowList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteValuesRowList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_VALUES_ROW_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteValuesRowList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteValuesRowList *new_node = (SyntaqliteValuesRowList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_values_clause(SyntaqliteAstContext *ctx, uint32_t rows) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_VALUES_CLAUSE, sizeof(SyntaqliteValuesClause));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteValuesClause *node = (SyntaqliteValuesClause*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->rows = rows;
    return id;
}

uint32_t ast_cte_definition(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan cte_name,
    uint8_t materialized,
    uint32_t columns,
    uint32_t select
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CTE_DEFINITION, sizeof(SyntaqliteCteDefinition));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCteDefinition *node = (SyntaqliteCteDefinition*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->cte_name = cte_name;
    node->materialized = materialized;
    node->columns = columns;
    node->select = select;
    return id;
}

uint32_t ast_cte_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CTE_LIST, sizeof(SyntaqliteCteList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCteList *node = (SyntaqliteCteList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_cte_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CTE_LIST, sizeof(SyntaqliteCteList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCteList *node = (SyntaqliteCteList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_cte_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_cte_list(ctx, child);
    }

    // Get current list
    SyntaqliteCteList *old_node = (SyntaqliteCteList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteCteList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_CTE_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteCteList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteCteList *new_node = (SyntaqliteCteList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_with_clause(SyntaqliteAstContext *ctx, uint8_t recursive, uint32_t ctes, uint32_t select) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_WITH_CLAUSE, sizeof(SyntaqliteWithClause));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteWithClause *node = (SyntaqliteWithClause*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->recursive = recursive;
    node->ctes = ctes;
    node->select = select;
    return id;
}

uint32_t ast_aggregate_function_call(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan func_name,
    SyntaqliteAggregateFunctionCallFlags flags,
    uint32_t args,
    uint32_t orderby,
    uint32_t filter_clause,
    uint32_t over_clause
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL, sizeof(SyntaqliteAggregateFunctionCall));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteAggregateFunctionCall *node = (SyntaqliteAggregateFunctionCall*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->func_name = func_name;
    node->flags = flags;
    node->args = args;
    node->orderby = orderby;
    node->filter_clause = filter_clause;
    node->over_clause = over_clause;
    return id;
}

uint32_t ast_raise_expr(
    SyntaqliteAstContext *ctx,
    SyntaqliteRaiseType raise_type,
    uint32_t error_message
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RAISE_EXPR, sizeof(SyntaqliteRaiseExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteRaiseExpr *node = (SyntaqliteRaiseExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->raise_type = raise_type;
    node->error_message = error_message;
    return id;
}

uint32_t ast_table_ref(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan alias
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TABLE_REF, sizeof(SyntaqliteTableRef));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTableRef *node = (SyntaqliteTableRef*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->table_name = table_name;
    node->schema = schema;
    node->alias = alias;
    return id;
}

uint32_t ast_subquery_table_source(SyntaqliteAstContext *ctx, uint32_t select, SyntaqliteSourceSpan alias) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE, sizeof(SyntaqliteSubqueryTableSource));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSubqueryTableSource *node = (SyntaqliteSubqueryTableSource*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->select = select;
    node->alias = alias;
    return id;
}

uint32_t ast_join_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteJoinType join_type,
    uint32_t left,
    uint32_t right,
    uint32_t on_expr,
    uint32_t using_columns
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_JOIN_CLAUSE, sizeof(SyntaqliteJoinClause));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteJoinClause *node = (SyntaqliteJoinClause*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->join_type = join_type;
    node->left = left;
    node->right = right;
    node->on_expr = on_expr;
    node->using_columns = using_columns;
    return id;
}

uint32_t ast_join_prefix(SyntaqliteAstContext *ctx, uint32_t source, SyntaqliteJoinType join_type) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_JOIN_PREFIX, sizeof(SyntaqliteJoinPrefix));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteJoinPrefix *node = (SyntaqliteJoinPrefix*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->source = source;
    node->join_type = join_type;
    return id;
}

uint32_t ast_delete_stmt(SyntaqliteAstContext *ctx, uint32_t table, uint32_t where) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_DELETE_STMT, sizeof(SyntaqliteDeleteStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteDeleteStmt *node = (SyntaqliteDeleteStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->table = table;
    node->where = where;
    return id;
}

uint32_t ast_set_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan column,
    uint32_t columns,
    uint32_t value
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SET_CLAUSE, sizeof(SyntaqliteSetClause));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSetClause *node = (SyntaqliteSetClause*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->column = column;
    node->columns = columns;
    node->value = value;
    return id;
}

uint32_t ast_set_clause_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SET_CLAUSE_LIST, sizeof(SyntaqliteSetClauseList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSetClauseList *node = (SyntaqliteSetClauseList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_set_clause_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SET_CLAUSE_LIST, sizeof(SyntaqliteSetClauseList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSetClauseList *node = (SyntaqliteSetClauseList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_set_clause_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_set_clause_list(ctx, child);
    }

    // Get current list
    SyntaqliteSetClauseList *old_node = (SyntaqliteSetClauseList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteSetClauseList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_SET_CLAUSE_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteSetClauseList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteSetClauseList *new_node = (SyntaqliteSetClauseList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_update_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t setlist,
    uint32_t from_clause,
    uint32_t where
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_UPDATE_STMT, sizeof(SyntaqliteUpdateStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteUpdateStmt *node = (SyntaqliteUpdateStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->conflict_action = conflict_action;
    node->table = table;
    node->setlist = setlist;
    node->from_clause = from_clause;
    node->where = where;
    return id;
}

uint32_t ast_insert_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t columns,
    uint32_t source
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_INSERT_STMT, sizeof(SyntaqliteInsertStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteInsertStmt *node = (SyntaqliteInsertStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->conflict_action = conflict_action;
    node->table = table;
    node->columns = columns;
    node->source = source;
    return id;
}

uint32_t ast_qualified_name(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan object_name,
    SyntaqliteSourceSpan schema
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_QUALIFIED_NAME, sizeof(SyntaqliteQualifiedName));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteQualifiedName *node = (SyntaqliteQualifiedName*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->object_name = object_name;
    node->schema = schema;
    return id;
}

uint32_t ast_drop_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteDropObjectType object_type,
    uint8_t if_exists,
    uint32_t target
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_DROP_STMT, sizeof(SyntaqliteDropStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteDropStmt *node = (SyntaqliteDropStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->object_type = object_type;
    node->if_exists = if_exists;
    node->target = target;
    return id;
}

uint32_t ast_alter_table_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteAlterOp op,
    uint32_t target,
    SyntaqliteSourceSpan new_name,
    SyntaqliteSourceSpan old_name
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_ALTER_TABLE_STMT, sizeof(SyntaqliteAlterTableStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteAlterTableStmt *node = (SyntaqliteAlterTableStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->target = target;
    node->new_name = new_name;
    node->old_name = old_name;
    return id;
}

uint32_t ast_transaction_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteTransactionOp op,
    SyntaqliteTransactionType trans_type
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TRANSACTION_STMT, sizeof(SyntaqliteTransactionStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTransactionStmt *node = (SyntaqliteTransactionStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->trans_type = trans_type;
    return id;
}

uint32_t ast_savepoint_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSavepointOp op,
    SyntaqliteSourceSpan savepoint_name
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SAVEPOINT_STMT, sizeof(SyntaqliteSavepointStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSavepointStmt *node = (SyntaqliteSavepointStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->savepoint_name = savepoint_name;
    return id;
}

uint32_t ast_pragma_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan pragma_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan value,
    uint8_t pragma_form
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_PRAGMA_STMT, sizeof(SyntaqlitePragmaStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqlitePragmaStmt *node = (SyntaqlitePragmaStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->pragma_name = pragma_name;
    node->schema = schema;
    node->value = value;
    node->pragma_form = pragma_form;
    return id;
}

uint32_t ast_analyze_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan target_name,
    SyntaqliteSourceSpan schema,
    uint8_t is_reindex
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_ANALYZE_STMT, sizeof(SyntaqliteAnalyzeStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteAnalyzeStmt *node = (SyntaqliteAnalyzeStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->target_name = target_name;
    node->schema = schema;
    node->is_reindex = is_reindex;
    return id;
}

uint32_t ast_attach_stmt(SyntaqliteAstContext *ctx, uint32_t filename, uint32_t db_name, uint32_t key) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_ATTACH_STMT, sizeof(SyntaqliteAttachStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteAttachStmt *node = (SyntaqliteAttachStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->filename = filename;
    node->db_name = db_name;
    node->key = key;
    return id;
}

uint32_t ast_detach_stmt(SyntaqliteAstContext *ctx, uint32_t db_name) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_DETACH_STMT, sizeof(SyntaqliteDetachStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteDetachStmt *node = (SyntaqliteDetachStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->db_name = db_name;
    return id;
}

uint32_t ast_vacuum_stmt(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan schema, uint32_t into_expr) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_VACUUM_STMT, sizeof(SyntaqliteVacuumStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteVacuumStmt *node = (SyntaqliteVacuumStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->schema = schema;
    node->into_expr = into_expr;
    return id;
}

uint32_t ast_explain_stmt(SyntaqliteAstContext *ctx, SyntaqliteExplainMode explain_mode, uint32_t stmt) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_EXPLAIN_STMT, sizeof(SyntaqliteExplainStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteExplainStmt *node = (SyntaqliteExplainStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->explain_mode = explain_mode;
    node->stmt = stmt;
    return id;
}

uint32_t ast_create_index_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan index_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan table_name,
    uint8_t is_unique,
    uint8_t if_not_exists,
    uint32_t columns,
    uint32_t where
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CREATE_INDEX_STMT, sizeof(SyntaqliteCreateIndexStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCreateIndexStmt *node = (SyntaqliteCreateIndexStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->index_name = index_name;
    node->schema = schema;
    node->table_name = table_name;
    node->is_unique = is_unique;
    node->if_not_exists = if_not_exists;
    node->columns = columns;
    node->where = where;
    return id;
}

uint32_t ast_create_view_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan view_name,
    SyntaqliteSourceSpan schema,
    uint8_t is_temp,
    uint8_t if_not_exists,
    uint32_t column_names,
    uint32_t select
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CREATE_VIEW_STMT, sizeof(SyntaqliteCreateViewStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCreateViewStmt *node = (SyntaqliteCreateViewStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->view_name = view_name;
    node->schema = schema;
    node->is_temp = is_temp;
    node->if_not_exists = if_not_exists;
    node->column_names = column_names;
    node->select = select;
    return id;
}

uint32_t ast_foreign_key_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan ref_table,
    uint32_t ref_columns,
    SyntaqliteForeignKeyAction on_delete,
    SyntaqliteForeignKeyAction on_update,
    uint8_t is_deferred
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE, sizeof(SyntaqliteForeignKeyClause));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteForeignKeyClause *node = (SyntaqliteForeignKeyClause*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->ref_table = ref_table;
    node->ref_columns = ref_columns;
    node->on_delete = on_delete;
    node->on_update = on_update;
    node->is_deferred = is_deferred;
    return id;
}

uint32_t ast_column_constraint(
    SyntaqliteAstContext *ctx,
    SyntaqliteColumnConstraintKind kind,
    SyntaqliteSourceSpan constraint_name,
    uint8_t onconf,
    uint8_t sort_order,
    uint8_t is_autoincrement,
    SyntaqliteSourceSpan collation_name,
    SyntaqliteGeneratedColumnStorage generated_storage,
    uint32_t default_expr,
    uint32_t check_expr,
    uint32_t generated_expr,
    uint32_t fk_clause
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_CONSTRAINT, sizeof(SyntaqliteColumnConstraint));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnConstraint *node = (SyntaqliteColumnConstraint*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->kind = kind;
    node->constraint_name = constraint_name;
    node->onconf = onconf;
    node->sort_order = sort_order;
    node->is_autoincrement = is_autoincrement;
    node->collation_name = collation_name;
    node->generated_storage = generated_storage;
    node->default_expr = default_expr;
    node->check_expr = check_expr;
    node->generated_expr = generated_expr;
    node->fk_clause = fk_clause;
    return id;
}

uint32_t ast_column_constraint_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST, sizeof(SyntaqliteColumnConstraintList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnConstraintList *node = (SyntaqliteColumnConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_column_constraint_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST, sizeof(SyntaqliteColumnConstraintList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnConstraintList *node = (SyntaqliteColumnConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_column_constraint_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_column_constraint_list(ctx, child);
    }

    // Get current list
    SyntaqliteColumnConstraintList *old_node = (SyntaqliteColumnConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteColumnConstraintList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteColumnConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteColumnConstraintList *new_node = (SyntaqliteColumnConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_column_def(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan column_name,
    SyntaqliteSourceSpan type_name,
    uint32_t constraints
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_DEF, sizeof(SyntaqliteColumnDef));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnDef *node = (SyntaqliteColumnDef*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->column_name = column_name;
    node->type_name = type_name;
    node->constraints = constraints;
    return id;
}

uint32_t ast_column_def_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_DEF_LIST, sizeof(SyntaqliteColumnDefList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnDefList *node = (SyntaqliteColumnDefList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_column_def_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_DEF_LIST, sizeof(SyntaqliteColumnDefList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteColumnDefList *node = (SyntaqliteColumnDefList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_column_def_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_column_def_list(ctx, child);
    }

    // Get current list
    SyntaqliteColumnDefList *old_node = (SyntaqliteColumnDefList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteColumnDefList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_COLUMN_DEF_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteColumnDefList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteColumnDefList *new_node = (SyntaqliteColumnDefList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_table_constraint(
    SyntaqliteAstContext *ctx,
    SyntaqliteTableConstraintKind kind,
    SyntaqliteSourceSpan constraint_name,
    uint8_t onconf,
    uint8_t is_autoincrement,
    uint32_t columns,
    uint32_t check_expr,
    uint32_t fk_clause
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TABLE_CONSTRAINT, sizeof(SyntaqliteTableConstraint));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTableConstraint *node = (SyntaqliteTableConstraint*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->kind = kind;
    node->constraint_name = constraint_name;
    node->onconf = onconf;
    node->is_autoincrement = is_autoincrement;
    node->columns = columns;
    node->check_expr = check_expr;
    node->fk_clause = fk_clause;
    return id;
}

uint32_t ast_table_constraint_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST, sizeof(SyntaqliteTableConstraintList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTableConstraintList *node = (SyntaqliteTableConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_table_constraint_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST, sizeof(SyntaqliteTableConstraintList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTableConstraintList *node = (SyntaqliteTableConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_table_constraint_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_table_constraint_list(ctx, child);
    }

    // Get current list
    SyntaqliteTableConstraintList *old_node = (SyntaqliteTableConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteTableConstraintList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteTableConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteTableConstraintList *new_node = (SyntaqliteTableConstraintList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_create_table_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    uint8_t is_temp,
    uint8_t if_not_exists,
    SyntaqliteCreateTableStmtFlags flags,
    uint32_t columns,
    uint32_t table_constraints,
    uint32_t as_select
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CREATE_TABLE_STMT, sizeof(SyntaqliteCreateTableStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCreateTableStmt *node = (SyntaqliteCreateTableStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->table_name = table_name;
    node->schema = schema;
    node->is_temp = is_temp;
    node->if_not_exists = if_not_exists;
    node->flags = flags;
    node->columns = columns;
    node->table_constraints = table_constraints;
    node->as_select = as_select;
    return id;
}

uint32_t ast_frame_bound(SyntaqliteAstContext *ctx, SyntaqliteFrameBoundType bound_type, uint32_t expr) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_FRAME_BOUND, sizeof(SyntaqliteFrameBound));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteFrameBound *node = (SyntaqliteFrameBound*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->bound_type = bound_type;
    node->expr = expr;
    return id;
}

uint32_t ast_frame_spec(
    SyntaqliteAstContext *ctx,
    SyntaqliteFrameType frame_type,
    SyntaqliteFrameExclude exclude,
    uint32_t start_bound,
    uint32_t end_bound
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_FRAME_SPEC, sizeof(SyntaqliteFrameSpec));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteFrameSpec *node = (SyntaqliteFrameSpec*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->frame_type = frame_type;
    node->exclude = exclude;
    node->start_bound = start_bound;
    node->end_bound = end_bound;
    return id;
}

uint32_t ast_window_def(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan base_window_name,
    uint32_t partition_by,
    uint32_t orderby,
    uint32_t frame
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_WINDOW_DEF, sizeof(SyntaqliteWindowDef));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteWindowDef *node = (SyntaqliteWindowDef*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->base_window_name = base_window_name;
    node->partition_by = partition_by;
    node->orderby = orderby;
    node->frame = frame;
    return id;
}

uint32_t ast_window_def_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_WINDOW_DEF_LIST, sizeof(SyntaqliteWindowDefList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteWindowDefList *node = (SyntaqliteWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_window_def_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_WINDOW_DEF_LIST, sizeof(SyntaqliteWindowDefList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteWindowDefList *node = (SyntaqliteWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_window_def_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_window_def_list(ctx, child);
    }

    // Get current list
    SyntaqliteWindowDefList *old_node = (SyntaqliteWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteWindowDefList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_WINDOW_DEF_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteWindowDefList *new_node = (SyntaqliteWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_named_window_def(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan window_name, uint32_t window_def) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_NAMED_WINDOW_DEF, sizeof(SyntaqliteNamedWindowDef));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteNamedWindowDef *node = (SyntaqliteNamedWindowDef*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->window_name = window_name;
    node->window_def = window_def;
    return id;
}

uint32_t ast_named_window_def_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST, sizeof(SyntaqliteNamedWindowDefList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteNamedWindowDefList *node = (SyntaqliteNamedWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_named_window_def_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST, sizeof(SyntaqliteNamedWindowDefList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteNamedWindowDefList *node = (SyntaqliteNamedWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_named_window_def_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_named_window_def_list(ctx, child);
    }

    // Get current list
    SyntaqliteNamedWindowDefList *old_node = (SyntaqliteNamedWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteNamedWindowDefList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteNamedWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteNamedWindowDefList *new_node = (SyntaqliteNamedWindowDefList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_filter_over(
    SyntaqliteAstContext *ctx,
    uint32_t filter_expr,
    uint32_t over_def,
    SyntaqliteSourceSpan over_name
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_FILTER_OVER, sizeof(SyntaqliteFilterOver));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteFilterOver *node = (SyntaqliteFilterOver*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->filter_expr = filter_expr;
    node->over_def = over_def;
    node->over_name = over_name;
    return id;
}

uint32_t ast_trigger_event(
    SyntaqliteAstContext *ctx,
    SyntaqliteTriggerEventType event_type,
    uint32_t columns
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TRIGGER_EVENT, sizeof(SyntaqliteTriggerEvent));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTriggerEvent *node = (SyntaqliteTriggerEvent*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->event_type = event_type;
    node->columns = columns;
    return id;
}

uint32_t ast_trigger_cmd_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TRIGGER_CMD_LIST, sizeof(SyntaqliteTriggerCmdList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTriggerCmdList *node = (SyntaqliteTriggerCmdList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_trigger_cmd_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_TRIGGER_CMD_LIST, sizeof(SyntaqliteTriggerCmdList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteTriggerCmdList *node = (SyntaqliteTriggerCmdList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_trigger_cmd_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_trigger_cmd_list(ctx, child);
    }

    // Get current list
    SyntaqliteTriggerCmdList *old_node = (SyntaqliteTriggerCmdList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteTriggerCmdList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_TRIGGER_CMD_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteTriggerCmdList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteTriggerCmdList *new_node = (SyntaqliteTriggerCmdList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_create_trigger_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan trigger_name,
    SyntaqliteSourceSpan schema,
    uint8_t is_temp,
    uint8_t if_not_exists,
    SyntaqliteTriggerTiming timing,
    uint32_t event,
    uint32_t table,
    uint32_t when_expr,
    uint32_t body
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CREATE_TRIGGER_STMT, sizeof(SyntaqliteCreateTriggerStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCreateTriggerStmt *node = (SyntaqliteCreateTriggerStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->trigger_name = trigger_name;
    node->schema = schema;
    node->is_temp = is_temp;
    node->if_not_exists = if_not_exists;
    node->timing = timing;
    node->event = event;
    node->table = table;
    node->when_expr = when_expr;
    node->body = body;
    return id;
}

uint32_t ast_create_virtual_table_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan module_name,
    uint8_t if_not_exists,
    SyntaqliteSourceSpan module_args
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT, sizeof(SyntaqliteCreateVirtualTableStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteCreateVirtualTableStmt *node = (SyntaqliteCreateVirtualTableStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->table_name = table_name;
    node->schema = schema;
    node->module_name = module_name;
    node->if_not_exists = if_not_exists;
    node->module_args = module_args;
    return id;
}

