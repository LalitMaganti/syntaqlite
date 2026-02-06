// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#include "src/parser/ast_builder_gen.h"

#include <stdlib.h>
#include <string.h>

// External definition for inline function (C99/C11).
extern inline SynqNode* synq_ast_node(SynqArena *ast, uint32_t id);

// ============ Node Size Table ============

static const size_t node_base_sizes[] = {
    [SYNQ_NODE_NULL] = 0,
    [SYNQ_NODE_BINARY_EXPR] = sizeof(SynqBinaryExpr),
    [SYNQ_NODE_UNARY_EXPR] = sizeof(SynqUnaryExpr),
    [SYNQ_NODE_LITERAL] = sizeof(SynqLiteral),
    [SYNQ_NODE_EXPR_LIST] = sizeof(SynqExprList),
    [SYNQ_NODE_RESULT_COLUMN] = sizeof(SynqResultColumn),
    [SYNQ_NODE_RESULT_COLUMN_LIST] = sizeof(SynqResultColumnList),
    [SYNQ_NODE_SELECT_STMT] = sizeof(SynqSelectStmt),
    [SYNQ_NODE_ORDERING_TERM] = sizeof(SynqOrderingTerm),
    [SYNQ_NODE_ORDER_BY_LIST] = sizeof(SynqOrderByList),
    [SYNQ_NODE_LIMIT_CLAUSE] = sizeof(SynqLimitClause),
    [SYNQ_NODE_COLUMN_REF] = sizeof(SynqColumnRef),
    [SYNQ_NODE_FUNCTION_CALL] = sizeof(SynqFunctionCall),
    [SYNQ_NODE_IS_EXPR] = sizeof(SynqIsExpr),
    [SYNQ_NODE_BETWEEN_EXPR] = sizeof(SynqBetweenExpr),
    [SYNQ_NODE_LIKE_EXPR] = sizeof(SynqLikeExpr),
    [SYNQ_NODE_CASE_EXPR] = sizeof(SynqCaseExpr),
    [SYNQ_NODE_CASE_WHEN] = sizeof(SynqCaseWhen),
    [SYNQ_NODE_CASE_WHEN_LIST] = sizeof(SynqCaseWhenList),
    [SYNQ_NODE_COMPOUND_SELECT] = sizeof(SynqCompoundSelect),
    [SYNQ_NODE_SUBQUERY_EXPR] = sizeof(SynqSubqueryExpr),
    [SYNQ_NODE_EXISTS_EXPR] = sizeof(SynqExistsExpr),
    [SYNQ_NODE_IN_EXPR] = sizeof(SynqInExpr),
    [SYNQ_NODE_VARIABLE] = sizeof(SynqVariable),
    [SYNQ_NODE_COLLATE_EXPR] = sizeof(SynqCollateExpr),
    [SYNQ_NODE_CAST_EXPR] = sizeof(SynqCastExpr),
    [SYNQ_NODE_VALUES_ROW_LIST] = sizeof(SynqValuesRowList),
    [SYNQ_NODE_VALUES_CLAUSE] = sizeof(SynqValuesClause),
    [SYNQ_NODE_CTE_DEFINITION] = sizeof(SynqCteDefinition),
    [SYNQ_NODE_CTE_LIST] = sizeof(SynqCteList),
    [SYNQ_NODE_WITH_CLAUSE] = sizeof(SynqWithClause),
    [SYNQ_NODE_AGGREGATE_FUNCTION_CALL] = sizeof(SynqAggregateFunctionCall),
    [SYNQ_NODE_RAISE_EXPR] = sizeof(SynqRaiseExpr),
    [SYNQ_NODE_TABLE_REF] = sizeof(SynqTableRef),
    [SYNQ_NODE_SUBQUERY_TABLE_SOURCE] = sizeof(SynqSubqueryTableSource),
    [SYNQ_NODE_JOIN_CLAUSE] = sizeof(SynqJoinClause),
    [SYNQ_NODE_JOIN_PREFIX] = sizeof(SynqJoinPrefix),
    [SYNQ_NODE_DELETE_STMT] = sizeof(SynqDeleteStmt),
    [SYNQ_NODE_SET_CLAUSE] = sizeof(SynqSetClause),
    [SYNQ_NODE_SET_CLAUSE_LIST] = sizeof(SynqSetClauseList),
    [SYNQ_NODE_UPDATE_STMT] = sizeof(SynqUpdateStmt),
    [SYNQ_NODE_INSERT_STMT] = sizeof(SynqInsertStmt),
    [SYNQ_NODE_QUALIFIED_NAME] = sizeof(SynqQualifiedName),
    [SYNQ_NODE_DROP_STMT] = sizeof(SynqDropStmt),
    [SYNQ_NODE_ALTER_TABLE_STMT] = sizeof(SynqAlterTableStmt),
    [SYNQ_NODE_TRANSACTION_STMT] = sizeof(SynqTransactionStmt),
    [SYNQ_NODE_SAVEPOINT_STMT] = sizeof(SynqSavepointStmt),
    [SYNQ_NODE_PRAGMA_STMT] = sizeof(SynqPragmaStmt),
    [SYNQ_NODE_ANALYZE_STMT] = sizeof(SynqAnalyzeStmt),
    [SYNQ_NODE_ATTACH_STMT] = sizeof(SynqAttachStmt),
    [SYNQ_NODE_DETACH_STMT] = sizeof(SynqDetachStmt),
    [SYNQ_NODE_VACUUM_STMT] = sizeof(SynqVacuumStmt),
    [SYNQ_NODE_EXPLAIN_STMT] = sizeof(SynqExplainStmt),
    [SYNQ_NODE_CREATE_INDEX_STMT] = sizeof(SynqCreateIndexStmt),
    [SYNQ_NODE_CREATE_VIEW_STMT] = sizeof(SynqCreateViewStmt),
    [SYNQ_NODE_FOREIGN_KEY_CLAUSE] = sizeof(SynqForeignKeyClause),
    [SYNQ_NODE_COLUMN_CONSTRAINT] = sizeof(SynqColumnConstraint),
    [SYNQ_NODE_COLUMN_CONSTRAINT_LIST] = sizeof(SynqColumnConstraintList),
    [SYNQ_NODE_COLUMN_DEF] = sizeof(SynqColumnDef),
    [SYNQ_NODE_COLUMN_DEF_LIST] = sizeof(SynqColumnDefList),
    [SYNQ_NODE_TABLE_CONSTRAINT] = sizeof(SynqTableConstraint),
    [SYNQ_NODE_TABLE_CONSTRAINT_LIST] = sizeof(SynqTableConstraintList),
    [SYNQ_NODE_CREATE_TABLE_STMT] = sizeof(SynqCreateTableStmt),
    [SYNQ_NODE_FRAME_BOUND] = sizeof(SynqFrameBound),
    [SYNQ_NODE_FRAME_SPEC] = sizeof(SynqFrameSpec),
    [SYNQ_NODE_WINDOW_DEF] = sizeof(SynqWindowDef),
    [SYNQ_NODE_WINDOW_DEF_LIST] = sizeof(SynqWindowDefList),
    [SYNQ_NODE_NAMED_WINDOW_DEF] = sizeof(SynqNamedWindowDef),
    [SYNQ_NODE_NAMED_WINDOW_DEF_LIST] = sizeof(SynqNamedWindowDefList),
    [SYNQ_NODE_FILTER_OVER] = sizeof(SynqFilterOver),
    [SYNQ_NODE_TRIGGER_EVENT] = sizeof(SynqTriggerEvent),
    [SYNQ_NODE_TRIGGER_CMD_LIST] = sizeof(SynqTriggerCmdList),
    [SYNQ_NODE_CREATE_TRIGGER_STMT] = sizeof(SynqCreateTriggerStmt),
    [SYNQ_NODE_CREATE_VIRTUAL_TABLE_STMT] = sizeof(SynqCreateVirtualTableStmt),
};

size_t synq_node_base_size(uint8_t tag) {
    if (tag >= SYNQ_NODE_COUNT) return 0;
    return node_base_sizes[tag];
}

// ============ Builder Functions ============

uint32_t synq_ast_binary_expr(SynqAstContext *ctx, SynqBinaryOp op, uint32_t left, uint32_t right) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_BINARY_EXPR, sizeof(SynqBinaryExpr));

    SynqBinaryExpr *node = (SynqBinaryExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, left);
    synq_ast_range_union(ctx, &_r, right);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_unary_expr(SynqAstContext *ctx, SynqUnaryOp op, uint32_t operand) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_UNARY_EXPR, sizeof(SynqUnaryExpr));

    SynqUnaryExpr *node = (SynqUnaryExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->operand = operand;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, operand);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_literal(SynqAstContext *ctx, SynqLiteralType literal_type, SynqSourceSpan source) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_LITERAL, sizeof(SynqLiteral));

    SynqLiteral *node = (SynqLiteral*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->literal_type = literal_type;
    node->source = source;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, source);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_expr_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_EXPR_LIST, sizeof(SynqExprList));

    SynqExprList *node = (SynqExprList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_expr_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_EXPR_LIST, first_child);
}

uint32_t synq_ast_expr_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_expr_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_EXPR_LIST);
}

uint32_t synq_ast_result_column(
    SynqAstContext *ctx,
    SynqResultColumnFlags flags,
    SynqSourceSpan alias,
    uint32_t expr
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_RESULT_COLUMN, sizeof(SynqResultColumn));

    SynqResultColumn *node = (SynqResultColumn*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->flags = flags;
    node->alias = alias;
    node->expr = expr;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, expr);
    synq_ast_range_union_span(&_r, alias);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_result_column_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_RESULT_COLUMN_LIST, sizeof(SynqResultColumnList));

    SynqResultColumnList *node = (SynqResultColumnList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_result_column_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_RESULT_COLUMN_LIST, first_child);
}

uint32_t synq_ast_result_column_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_result_column_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_RESULT_COLUMN_LIST);
}

uint32_t synq_ast_select_stmt(
    SynqAstContext *ctx,
    SynqSelectStmtFlags flags,
    uint32_t columns,
    uint32_t from_clause,
    uint32_t where,
    uint32_t groupby,
    uint32_t having,
    uint32_t orderby,
    uint32_t limit_clause,
    uint32_t window_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_SELECT_STMT, sizeof(SynqSelectStmt));

    SynqSelectStmt *node = (SynqSelectStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->flags = flags;
    node->columns = columns;
    node->from_clause = from_clause;
    node->where = where;
    node->groupby = groupby;
    node->having = having;
    node->orderby = orderby;
    node->limit_clause = limit_clause;
    node->window_clause = window_clause;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, from_clause);
    synq_ast_range_union(ctx, &_r, where);
    synq_ast_range_union(ctx, &_r, groupby);
    synq_ast_range_union(ctx, &_r, having);
    synq_ast_range_union(ctx, &_r, orderby);
    synq_ast_range_union(ctx, &_r, limit_clause);
    synq_ast_range_union(ctx, &_r, window_clause);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_ordering_term(
    SynqAstContext *ctx,
    uint32_t expr,
    SynqSortOrder sort_order,
    SynqNullsOrder nulls_order
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_ORDERING_TERM, sizeof(SynqOrderingTerm));

    SynqOrderingTerm *node = (SynqOrderingTerm*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->expr = expr;
    node->sort_order = sort_order;
    node->nulls_order = nulls_order;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, expr);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_order_by_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_ORDER_BY_LIST, sizeof(SynqOrderByList));

    SynqOrderByList *node = (SynqOrderByList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_order_by_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_ORDER_BY_LIST, first_child);
}

uint32_t synq_ast_order_by_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_order_by_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_ORDER_BY_LIST);
}

uint32_t synq_ast_limit_clause(SynqAstContext *ctx, uint32_t limit, uint32_t offset) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_LIMIT_CLAUSE, sizeof(SynqLimitClause));

    SynqLimitClause *node = (SynqLimitClause*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->limit = limit;
    node->offset = offset;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, limit);
    synq_ast_range_union(ctx, &_r, offset);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_column_ref(
    SynqAstContext *ctx,
    SynqSourceSpan column,
    SynqSourceSpan table,
    SynqSourceSpan schema
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COLUMN_REF, sizeof(SynqColumnRef));

    SynqColumnRef *node = (SynqColumnRef*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->column = column;
    node->table = table;
    node->schema = schema;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, column);
    synq_ast_range_union_span(&_r, table);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_function_call(
    SynqAstContext *ctx,
    SynqSourceSpan func_name,
    SynqFunctionCallFlags flags,
    uint32_t args,
    uint32_t filter_clause,
    uint32_t over_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_FUNCTION_CALL, sizeof(SynqFunctionCall));

    SynqFunctionCall *node = (SynqFunctionCall*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->func_name = func_name;
    node->flags = flags;
    node->args = args;
    node->filter_clause = filter_clause;
    node->over_clause = over_clause;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, args);
    synq_ast_range_union(ctx, &_r, filter_clause);
    synq_ast_range_union(ctx, &_r, over_clause);
    synq_ast_range_union_span(&_r, func_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_is_expr(SynqAstContext *ctx, SynqIsOp op, uint32_t left, uint32_t right) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_IS_EXPR, sizeof(SynqIsExpr));

    SynqIsExpr *node = (SynqIsExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, left);
    synq_ast_range_union(ctx, &_r, right);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_between_expr(
    SynqAstContext *ctx,
    SynqBool negated,
    uint32_t operand,
    uint32_t low,
    uint32_t high
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_BETWEEN_EXPR, sizeof(SynqBetweenExpr));

    SynqBetweenExpr *node = (SynqBetweenExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->negated = negated;
    node->operand = operand;
    node->low = low;
    node->high = high;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, operand);
    synq_ast_range_union(ctx, &_r, low);
    synq_ast_range_union(ctx, &_r, high);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_like_expr(
    SynqAstContext *ctx,
    SynqBool negated,
    uint32_t operand,
    uint32_t pattern,
    uint32_t escape
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_LIKE_EXPR, sizeof(SynqLikeExpr));

    SynqLikeExpr *node = (SynqLikeExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->negated = negated;
    node->operand = operand;
    node->pattern = pattern;
    node->escape = escape;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, operand);
    synq_ast_range_union(ctx, &_r, pattern);
    synq_ast_range_union(ctx, &_r, escape);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_case_expr(SynqAstContext *ctx, uint32_t operand, uint32_t else_expr, uint32_t whens) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CASE_EXPR, sizeof(SynqCaseExpr));

    SynqCaseExpr *node = (SynqCaseExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->operand = operand;
    node->else_expr = else_expr;
    node->whens = whens;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, operand);
    synq_ast_range_union(ctx, &_r, else_expr);
    synq_ast_range_union(ctx, &_r, whens);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_case_when(SynqAstContext *ctx, uint32_t when_expr, uint32_t then_expr) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CASE_WHEN, sizeof(SynqCaseWhen));

    SynqCaseWhen *node = (SynqCaseWhen*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->when_expr = when_expr;
    node->then_expr = then_expr;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, when_expr);
    synq_ast_range_union(ctx, &_r, then_expr);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_case_when_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CASE_WHEN_LIST, sizeof(SynqCaseWhenList));

    SynqCaseWhenList *node = (SynqCaseWhenList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_case_when_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_CASE_WHEN_LIST, first_child);
}

uint32_t synq_ast_case_when_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_case_when_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_CASE_WHEN_LIST);
}

uint32_t synq_ast_compound_select(SynqAstContext *ctx, SynqCompoundOp op, uint32_t left, uint32_t right) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COMPOUND_SELECT, sizeof(SynqCompoundSelect));

    SynqCompoundSelect *node = (SynqCompoundSelect*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, left);
    synq_ast_range_union(ctx, &_r, right);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_subquery_expr(SynqAstContext *ctx, uint32_t select) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_SUBQUERY_EXPR, sizeof(SynqSubqueryExpr));

    SynqSubqueryExpr *node = (SynqSubqueryExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, select);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_exists_expr(SynqAstContext *ctx, uint32_t select) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_EXISTS_EXPR, sizeof(SynqExistsExpr));

    SynqExistsExpr *node = (SynqExistsExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, select);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_in_expr(SynqAstContext *ctx, SynqBool negated, uint32_t operand, uint32_t source) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_IN_EXPR, sizeof(SynqInExpr));

    SynqInExpr *node = (SynqInExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->negated = negated;
    node->operand = operand;
    node->source = source;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, operand);
    synq_ast_range_union(ctx, &_r, source);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_variable(SynqAstContext *ctx, SynqSourceSpan source) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_VARIABLE, sizeof(SynqVariable));

    SynqVariable *node = (SynqVariable*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->source = source;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, source);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_collate_expr(SynqAstContext *ctx, uint32_t expr, SynqSourceSpan collation) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COLLATE_EXPR, sizeof(SynqCollateExpr));

    SynqCollateExpr *node = (SynqCollateExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->expr = expr;
    node->collation = collation;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, expr);
    synq_ast_range_union_span(&_r, collation);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_cast_expr(SynqAstContext *ctx, uint32_t expr, SynqSourceSpan type_name) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CAST_EXPR, sizeof(SynqCastExpr));

    SynqCastExpr *node = (SynqCastExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->expr = expr;
    node->type_name = type_name;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, expr);
    synq_ast_range_union_span(&_r, type_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_values_row_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_VALUES_ROW_LIST, sizeof(SynqValuesRowList));

    SynqValuesRowList *node = (SynqValuesRowList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_values_row_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_VALUES_ROW_LIST, first_child);
}

uint32_t synq_ast_values_row_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_values_row_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_VALUES_ROW_LIST);
}

uint32_t synq_ast_values_clause(SynqAstContext *ctx, uint32_t rows) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_VALUES_CLAUSE, sizeof(SynqValuesClause));

    SynqValuesClause *node = (SynqValuesClause*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->rows = rows;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, rows);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_cte_definition(
    SynqAstContext *ctx,
    SynqSourceSpan cte_name,
    SynqMaterialized materialized,
    uint32_t columns,
    uint32_t select
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CTE_DEFINITION, sizeof(SynqCteDefinition));

    SynqCteDefinition *node = (SynqCteDefinition*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->cte_name = cte_name;
    node->materialized = materialized;
    node->columns = columns;
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, select);
    synq_ast_range_union_span(&_r, cte_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_cte_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CTE_LIST, sizeof(SynqCteList));

    SynqCteList *node = (SynqCteList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_cte_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_CTE_LIST, first_child);
}

uint32_t synq_ast_cte_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_cte_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_CTE_LIST);
}

uint32_t synq_ast_with_clause(SynqAstContext *ctx, SynqBool recursive, uint32_t ctes, uint32_t select) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_WITH_CLAUSE, sizeof(SynqWithClause));

    SynqWithClause *node = (SynqWithClause*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->recursive = recursive;
    node->ctes = ctes;
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, ctes);
    synq_ast_range_union(ctx, &_r, select);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_aggregate_function_call(
    SynqAstContext *ctx,
    SynqSourceSpan func_name,
    SynqAggregateFunctionCallFlags flags,
    uint32_t args,
    uint32_t orderby,
    uint32_t filter_clause,
    uint32_t over_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_AGGREGATE_FUNCTION_CALL, sizeof(SynqAggregateFunctionCall));

    SynqAggregateFunctionCall *node = (SynqAggregateFunctionCall*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->func_name = func_name;
    node->flags = flags;
    node->args = args;
    node->orderby = orderby;
    node->filter_clause = filter_clause;
    node->over_clause = over_clause;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, args);
    synq_ast_range_union(ctx, &_r, orderby);
    synq_ast_range_union(ctx, &_r, filter_clause);
    synq_ast_range_union(ctx, &_r, over_clause);
    synq_ast_range_union_span(&_r, func_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_raise_expr(SynqAstContext *ctx, SynqRaiseType raise_type, uint32_t error_message) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_RAISE_EXPR, sizeof(SynqRaiseExpr));

    SynqRaiseExpr *node = (SynqRaiseExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->raise_type = raise_type;
    node->error_message = error_message;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, error_message);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_table_ref(
    SynqAstContext *ctx,
    SynqSourceSpan table_name,
    SynqSourceSpan schema,
    SynqSourceSpan alias
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_TABLE_REF, sizeof(SynqTableRef));

    SynqTableRef *node = (SynqTableRef*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->table_name = table_name;
    node->schema = schema;
    node->alias = alias;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, table_name);
    synq_ast_range_union_span(&_r, schema);
    synq_ast_range_union_span(&_r, alias);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_subquery_table_source(SynqAstContext *ctx, uint32_t select, SynqSourceSpan alias) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_SUBQUERY_TABLE_SOURCE, sizeof(SynqSubqueryTableSource));

    SynqSubqueryTableSource *node = (SynqSubqueryTableSource*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->select = select;
    node->alias = alias;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, select);
    synq_ast_range_union_span(&_r, alias);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_join_clause(
    SynqAstContext *ctx,
    SynqJoinType join_type,
    uint32_t left,
    uint32_t right,
    uint32_t on_expr,
    uint32_t using_columns
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_JOIN_CLAUSE, sizeof(SynqJoinClause));

    SynqJoinClause *node = (SynqJoinClause*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->join_type = join_type;
    node->left = left;
    node->right = right;
    node->on_expr = on_expr;
    node->using_columns = using_columns;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, left);
    synq_ast_range_union(ctx, &_r, right);
    synq_ast_range_union(ctx, &_r, on_expr);
    synq_ast_range_union(ctx, &_r, using_columns);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_join_prefix(SynqAstContext *ctx, uint32_t source, SynqJoinType join_type) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_JOIN_PREFIX, sizeof(SynqJoinPrefix));

    SynqJoinPrefix *node = (SynqJoinPrefix*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->source = source;
    node->join_type = join_type;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, source);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_delete_stmt(SynqAstContext *ctx, uint32_t table, uint32_t where) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_DELETE_STMT, sizeof(SynqDeleteStmt));

    SynqDeleteStmt *node = (SynqDeleteStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->table = table;
    node->where = where;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, table);
    synq_ast_range_union(ctx, &_r, where);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_set_clause(SynqAstContext *ctx, SynqSourceSpan column, uint32_t columns, uint32_t value) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_SET_CLAUSE, sizeof(SynqSetClause));

    SynqSetClause *node = (SynqSetClause*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->column = column;
    node->columns = columns;
    node->value = value;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, value);
    synq_ast_range_union_span(&_r, column);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_set_clause_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_SET_CLAUSE_LIST, sizeof(SynqSetClauseList));

    SynqSetClauseList *node = (SynqSetClauseList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_set_clause_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_SET_CLAUSE_LIST, first_child);
}

uint32_t synq_ast_set_clause_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_set_clause_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_SET_CLAUSE_LIST);
}

uint32_t synq_ast_update_stmt(
    SynqAstContext *ctx,
    SynqConflictAction conflict_action,
    uint32_t table,
    uint32_t setlist,
    uint32_t from_clause,
    uint32_t where
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_UPDATE_STMT, sizeof(SynqUpdateStmt));

    SynqUpdateStmt *node = (SynqUpdateStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->conflict_action = conflict_action;
    node->table = table;
    node->setlist = setlist;
    node->from_clause = from_clause;
    node->where = where;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, table);
    synq_ast_range_union(ctx, &_r, setlist);
    synq_ast_range_union(ctx, &_r, from_clause);
    synq_ast_range_union(ctx, &_r, where);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_insert_stmt(
    SynqAstContext *ctx,
    SynqConflictAction conflict_action,
    uint32_t table,
    uint32_t columns,
    uint32_t source
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_INSERT_STMT, sizeof(SynqInsertStmt));

    SynqInsertStmt *node = (SynqInsertStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->conflict_action = conflict_action;
    node->table = table;
    node->columns = columns;
    node->source = source;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, table);
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, source);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_qualified_name(SynqAstContext *ctx, SynqSourceSpan object_name, SynqSourceSpan schema) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_QUALIFIED_NAME, sizeof(SynqQualifiedName));

    SynqQualifiedName *node = (SynqQualifiedName*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->object_name = object_name;
    node->schema = schema;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, object_name);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_drop_stmt(
    SynqAstContext *ctx,
    SynqDropObjectType object_type,
    SynqBool if_exists,
    uint32_t target
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_DROP_STMT, sizeof(SynqDropStmt));

    SynqDropStmt *node = (SynqDropStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->object_type = object_type;
    node->if_exists = if_exists;
    node->target = target;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, target);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_alter_table_stmt(
    SynqAstContext *ctx,
    SynqAlterOp op,
    uint32_t target,
    SynqSourceSpan new_name,
    SynqSourceSpan old_name
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_ALTER_TABLE_STMT, sizeof(SynqAlterTableStmt));

    SynqAlterTableStmt *node = (SynqAlterTableStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->target = target;
    node->new_name = new_name;
    node->old_name = old_name;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, target);
    synq_ast_range_union_span(&_r, new_name);
    synq_ast_range_union_span(&_r, old_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_transaction_stmt(SynqAstContext *ctx, SynqTransactionOp op, SynqTransactionType trans_type) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_TRANSACTION_STMT, sizeof(SynqTransactionStmt));

    SynqTransactionStmt *node = (SynqTransactionStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->trans_type = trans_type;
    return id;
}

uint32_t synq_ast_savepoint_stmt(SynqAstContext *ctx, SynqSavepointOp op, SynqSourceSpan savepoint_name) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_SAVEPOINT_STMT, sizeof(SynqSavepointStmt));

    SynqSavepointStmt *node = (SynqSavepointStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->savepoint_name = savepoint_name;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, savepoint_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_pragma_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan pragma_name,
    SynqSourceSpan schema,
    SynqSourceSpan value,
    SynqPragmaForm pragma_form
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_PRAGMA_STMT, sizeof(SynqPragmaStmt));

    SynqPragmaStmt *node = (SynqPragmaStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->pragma_name = pragma_name;
    node->schema = schema;
    node->value = value;
    node->pragma_form = pragma_form;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, pragma_name);
    synq_ast_range_union_span(&_r, schema);
    synq_ast_range_union_span(&_r, value);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_analyze_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan target_name,
    SynqSourceSpan schema,
    SynqAnalyzeKind kind
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_ANALYZE_STMT, sizeof(SynqAnalyzeStmt));

    SynqAnalyzeStmt *node = (SynqAnalyzeStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->target_name = target_name;
    node->schema = schema;
    node->kind = kind;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, target_name);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_attach_stmt(SynqAstContext *ctx, uint32_t filename, uint32_t db_name, uint32_t key) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_ATTACH_STMT, sizeof(SynqAttachStmt));

    SynqAttachStmt *node = (SynqAttachStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->filename = filename;
    node->db_name = db_name;
    node->key = key;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, filename);
    synq_ast_range_union(ctx, &_r, db_name);
    synq_ast_range_union(ctx, &_r, key);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_detach_stmt(SynqAstContext *ctx, uint32_t db_name) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_DETACH_STMT, sizeof(SynqDetachStmt));

    SynqDetachStmt *node = (SynqDetachStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->db_name = db_name;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, db_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_vacuum_stmt(SynqAstContext *ctx, SynqSourceSpan schema, uint32_t into_expr) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_VACUUM_STMT, sizeof(SynqVacuumStmt));

    SynqVacuumStmt *node = (SynqVacuumStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->schema = schema;
    node->into_expr = into_expr;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, into_expr);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_explain_stmt(SynqAstContext *ctx, SynqExplainMode explain_mode, uint32_t stmt) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_EXPLAIN_STMT, sizeof(SynqExplainStmt));

    SynqExplainStmt *node = (SynqExplainStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->explain_mode = explain_mode;
    node->stmt = stmt;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, stmt);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_create_index_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan index_name,
    SynqSourceSpan schema,
    SynqSourceSpan table_name,
    SynqBool is_unique,
    SynqBool if_not_exists,
    uint32_t columns,
    uint32_t where
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CREATE_INDEX_STMT, sizeof(SynqCreateIndexStmt));

    SynqCreateIndexStmt *node = (SynqCreateIndexStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->index_name = index_name;
    node->schema = schema;
    node->table_name = table_name;
    node->is_unique = is_unique;
    node->if_not_exists = if_not_exists;
    node->columns = columns;
    node->where = where;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, where);
    synq_ast_range_union_span(&_r, index_name);
    synq_ast_range_union_span(&_r, schema);
    synq_ast_range_union_span(&_r, table_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_create_view_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan view_name,
    SynqSourceSpan schema,
    SynqBool is_temp,
    SynqBool if_not_exists,
    uint32_t column_names,
    uint32_t select
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CREATE_VIEW_STMT, sizeof(SynqCreateViewStmt));

    SynqCreateViewStmt *node = (SynqCreateViewStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->view_name = view_name;
    node->schema = schema;
    node->is_temp = is_temp;
    node->if_not_exists = if_not_exists;
    node->column_names = column_names;
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, column_names);
    synq_ast_range_union(ctx, &_r, select);
    synq_ast_range_union_span(&_r, view_name);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_foreign_key_clause(
    SynqAstContext *ctx,
    SynqSourceSpan ref_table,
    uint32_t ref_columns,
    SynqForeignKeyAction on_delete,
    SynqForeignKeyAction on_update,
    SynqBool is_deferred
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_FOREIGN_KEY_CLAUSE, sizeof(SynqForeignKeyClause));

    SynqForeignKeyClause *node = (SynqForeignKeyClause*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->ref_table = ref_table;
    node->ref_columns = ref_columns;
    node->on_delete = on_delete;
    node->on_update = on_update;
    node->is_deferred = is_deferred;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, ref_columns);
    synq_ast_range_union_span(&_r, ref_table);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_column_constraint(
    SynqAstContext *ctx,
    SynqColumnConstraintKind kind,
    SynqSourceSpan constraint_name,
    SynqConflictAction onconf,
    SynqSortOrder sort_order,
    SynqBool is_autoincrement,
    SynqSourceSpan collation_name,
    SynqGeneratedColumnStorage generated_storage,
    uint32_t default_expr,
    uint32_t check_expr,
    uint32_t generated_expr,
    uint32_t fk_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COLUMN_CONSTRAINT, sizeof(SynqColumnConstraint));

    SynqColumnConstraint *node = (SynqColumnConstraint*)
        (ctx->ast.data + ctx->ast.offsets[id]);
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

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, default_expr);
    synq_ast_range_union(ctx, &_r, check_expr);
    synq_ast_range_union(ctx, &_r, generated_expr);
    synq_ast_range_union(ctx, &_r, fk_clause);
    synq_ast_range_union_span(&_r, constraint_name);
    synq_ast_range_union_span(&_r, collation_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_column_constraint_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COLUMN_CONSTRAINT_LIST, sizeof(SynqColumnConstraintList));

    SynqColumnConstraintList *node = (SynqColumnConstraintList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_column_constraint_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_COLUMN_CONSTRAINT_LIST, first_child);
}

uint32_t synq_ast_column_constraint_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_column_constraint_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_COLUMN_CONSTRAINT_LIST);
}

uint32_t synq_ast_column_def(
    SynqAstContext *ctx,
    SynqSourceSpan column_name,
    SynqSourceSpan type_name,
    uint32_t constraints
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COLUMN_DEF, sizeof(SynqColumnDef));

    SynqColumnDef *node = (SynqColumnDef*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->column_name = column_name;
    node->type_name = type_name;
    node->constraints = constraints;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, constraints);
    synq_ast_range_union_span(&_r, column_name);
    synq_ast_range_union_span(&_r, type_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_column_def_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_COLUMN_DEF_LIST, sizeof(SynqColumnDefList));

    SynqColumnDefList *node = (SynqColumnDefList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_column_def_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_COLUMN_DEF_LIST, first_child);
}

uint32_t synq_ast_column_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_column_def_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_COLUMN_DEF_LIST);
}

uint32_t synq_ast_table_constraint(
    SynqAstContext *ctx,
    SynqTableConstraintKind kind,
    SynqSourceSpan constraint_name,
    SynqConflictAction onconf,
    SynqBool is_autoincrement,
    uint32_t columns,
    uint32_t check_expr,
    uint32_t fk_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_TABLE_CONSTRAINT, sizeof(SynqTableConstraint));

    SynqTableConstraint *node = (SynqTableConstraint*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->kind = kind;
    node->constraint_name = constraint_name;
    node->onconf = onconf;
    node->is_autoincrement = is_autoincrement;
    node->columns = columns;
    node->check_expr = check_expr;
    node->fk_clause = fk_clause;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, check_expr);
    synq_ast_range_union(ctx, &_r, fk_clause);
    synq_ast_range_union_span(&_r, constraint_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_table_constraint_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_TABLE_CONSTRAINT_LIST, sizeof(SynqTableConstraintList));

    SynqTableConstraintList *node = (SynqTableConstraintList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_table_constraint_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_TABLE_CONSTRAINT_LIST, first_child);
}

uint32_t synq_ast_table_constraint_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_table_constraint_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_TABLE_CONSTRAINT_LIST);
}

uint32_t synq_ast_create_table_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan table_name,
    SynqSourceSpan schema,
    SynqBool is_temp,
    SynqBool if_not_exists,
    SynqCreateTableStmtFlags flags,
    uint32_t columns,
    uint32_t table_constraints,
    uint32_t as_select
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CREATE_TABLE_STMT, sizeof(SynqCreateTableStmt));

    SynqCreateTableStmt *node = (SynqCreateTableStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->table_name = table_name;
    node->schema = schema;
    node->is_temp = is_temp;
    node->if_not_exists = if_not_exists;
    node->flags = flags;
    node->columns = columns;
    node->table_constraints = table_constraints;
    node->as_select = as_select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    synq_ast_range_union(ctx, &_r, table_constraints);
    synq_ast_range_union(ctx, &_r, as_select);
    synq_ast_range_union_span(&_r, table_name);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_frame_bound(SynqAstContext *ctx, SynqFrameBoundType bound_type, uint32_t expr) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_FRAME_BOUND, sizeof(SynqFrameBound));

    SynqFrameBound *node = (SynqFrameBound*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->bound_type = bound_type;
    node->expr = expr;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, expr);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_frame_spec(
    SynqAstContext *ctx,
    SynqFrameType frame_type,
    SynqFrameExclude exclude,
    uint32_t start_bound,
    uint32_t end_bound
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_FRAME_SPEC, sizeof(SynqFrameSpec));

    SynqFrameSpec *node = (SynqFrameSpec*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->frame_type = frame_type;
    node->exclude = exclude;
    node->start_bound = start_bound;
    node->end_bound = end_bound;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, start_bound);
    synq_ast_range_union(ctx, &_r, end_bound);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_window_def(
    SynqAstContext *ctx,
    SynqSourceSpan base_window_name,
    uint32_t partition_by,
    uint32_t orderby,
    uint32_t frame
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_WINDOW_DEF, sizeof(SynqWindowDef));

    SynqWindowDef *node = (SynqWindowDef*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->base_window_name = base_window_name;
    node->partition_by = partition_by;
    node->orderby = orderby;
    node->frame = frame;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, partition_by);
    synq_ast_range_union(ctx, &_r, orderby);
    synq_ast_range_union(ctx, &_r, frame);
    synq_ast_range_union_span(&_r, base_window_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_window_def_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_WINDOW_DEF_LIST, sizeof(SynqWindowDefList));

    SynqWindowDefList *node = (SynqWindowDefList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_window_def_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_WINDOW_DEF_LIST, first_child);
}

uint32_t synq_ast_window_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_window_def_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_WINDOW_DEF_LIST);
}

uint32_t synq_ast_named_window_def(SynqAstContext *ctx, SynqSourceSpan window_name, uint32_t window_def) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_NAMED_WINDOW_DEF, sizeof(SynqNamedWindowDef));

    SynqNamedWindowDef *node = (SynqNamedWindowDef*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->window_name = window_name;
    node->window_def = window_def;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, window_def);
    synq_ast_range_union_span(&_r, window_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_named_window_def_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_NAMED_WINDOW_DEF_LIST, sizeof(SynqNamedWindowDefList));

    SynqNamedWindowDefList *node = (SynqNamedWindowDefList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_named_window_def_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_NAMED_WINDOW_DEF_LIST, first_child);
}

uint32_t synq_ast_named_window_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_named_window_def_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_NAMED_WINDOW_DEF_LIST);
}

uint32_t synq_ast_filter_over(
    SynqAstContext *ctx,
    uint32_t filter_expr,
    uint32_t over_def,
    SynqSourceSpan over_name
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_FILTER_OVER, sizeof(SynqFilterOver));

    SynqFilterOver *node = (SynqFilterOver*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->filter_expr = filter_expr;
    node->over_def = over_def;
    node->over_name = over_name;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, filter_expr);
    synq_ast_range_union(ctx, &_r, over_def);
    synq_ast_range_union_span(&_r, over_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_trigger_event(SynqAstContext *ctx, SynqTriggerEventType event_type, uint32_t columns) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_TRIGGER_EVENT, sizeof(SynqTriggerEvent));

    SynqTriggerEvent *node = (SynqTriggerEvent*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->event_type = event_type;
    node->columns = columns;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, columns);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_trigger_cmd_list_empty(SynqAstContext *ctx) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_TRIGGER_CMD_LIST, sizeof(SynqTriggerCmdList));

    SynqTriggerCmdList *node = (SynqTriggerCmdList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_trigger_cmd_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNQ_NODE_TRIGGER_CMD_LIST, first_child);
}

uint32_t synq_ast_trigger_cmd_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNQ_NULL_NODE) {
        return synq_ast_trigger_cmd_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNQ_NODE_TRIGGER_CMD_LIST);
}

uint32_t synq_ast_create_trigger_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan trigger_name,
    SynqSourceSpan schema,
    SynqBool is_temp,
    SynqBool if_not_exists,
    SynqTriggerTiming timing,
    uint32_t event,
    uint32_t table,
    uint32_t when_expr,
    uint32_t body
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CREATE_TRIGGER_STMT, sizeof(SynqCreateTriggerStmt));

    SynqCreateTriggerStmt *node = (SynqCreateTriggerStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->trigger_name = trigger_name;
    node->schema = schema;
    node->is_temp = is_temp;
    node->if_not_exists = if_not_exists;
    node->timing = timing;
    node->event = event;
    node->table = table;
    node->when_expr = when_expr;
    node->body = body;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, event);
    synq_ast_range_union(ctx, &_r, table);
    synq_ast_range_union(ctx, &_r, when_expr);
    synq_ast_range_union(ctx, &_r, body);
    synq_ast_range_union_span(&_r, trigger_name);
    synq_ast_range_union_span(&_r, schema);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_create_virtual_table_stmt(
    SynqAstContext *ctx,
    SynqSourceSpan table_name,
    SynqSourceSpan schema,
    SynqSourceSpan module_name,
    SynqBool if_not_exists,
    SynqSourceSpan module_args
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNQ_NODE_CREATE_VIRTUAL_TABLE_STMT, sizeof(SynqCreateVirtualTableStmt));

    SynqCreateVirtualTableStmt *node = (SynqCreateVirtualTableStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->table_name = table_name;
    node->schema = schema;
    node->module_name = module_name;
    node->if_not_exists = if_not_exists;
    node->module_args = module_args;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, table_name);
    synq_ast_range_union_span(&_r, schema);
    synq_ast_range_union_span(&_r, module_name);
    synq_ast_range_union_span(&_r, module_args);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

