// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#include "src/parser/ast_builder_gen.h"

#include <stdlib.h>
#include <string.h>

// External definition for inline function (C99/C11).
extern inline SyntaqliteNode* synq_ast_node(SynqArena *ast, uint32_t id);

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

size_t synq_node_base_size(uint8_t tag) {
    if (tag >= SYNTAQLITE_NODE_COUNT) return 0;
    return node_base_sizes[tag];
}

// ============ Builder Functions ============

uint32_t synq_ast_binary_expr(SynqAstContext *ctx, SyntaqliteBinaryOp op, uint32_t left, uint32_t right) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_BINARY_EXPR, sizeof(SyntaqliteBinaryExpr));

    SyntaqliteBinaryExpr *node = (SyntaqliteBinaryExpr*)
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

uint32_t synq_ast_unary_expr(SynqAstContext *ctx, SyntaqliteUnaryOp op, uint32_t operand) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_UNARY_EXPR, sizeof(SyntaqliteUnaryExpr));

    SyntaqliteUnaryExpr *node = (SyntaqliteUnaryExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->operand = operand;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, operand);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_literal(
    SynqAstContext *ctx,
    SyntaqliteLiteralType literal_type,
    SyntaqliteSourceSpan source
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_LITERAL, sizeof(SyntaqliteLiteral));

    SyntaqliteLiteral *node = (SyntaqliteLiteral*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_EXPR_LIST, sizeof(SyntaqliteExprList));

    SyntaqliteExprList *node = (SyntaqliteExprList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_expr_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_EXPR_LIST, first_child);
}

uint32_t synq_ast_expr_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_expr_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_EXPR_LIST);
}

uint32_t synq_ast_result_column(
    SynqAstContext *ctx,
    SyntaqliteResultColumnFlags flags,
    SyntaqliteSourceSpan alias,
    uint32_t expr
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_RESULT_COLUMN, sizeof(SyntaqliteResultColumn));

    SyntaqliteResultColumn *node = (SyntaqliteResultColumn*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, sizeof(SyntaqliteResultColumnList));

    SyntaqliteResultColumnList *node = (SyntaqliteResultColumnList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_result_column_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, first_child);
}

uint32_t synq_ast_result_column_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_result_column_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_RESULT_COLUMN_LIST);
}

uint32_t synq_ast_select_stmt(
    SynqAstContext *ctx,
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_SELECT_STMT, sizeof(SyntaqliteSelectStmt));

    SyntaqliteSelectStmt *node = (SyntaqliteSelectStmt*)
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
    SyntaqliteSortOrder sort_order,
    SyntaqliteNullsOrder nulls_order
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_ORDERING_TERM, sizeof(SyntaqliteOrderingTerm));

    SyntaqliteOrderingTerm *node = (SyntaqliteOrderingTerm*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_ORDER_BY_LIST, sizeof(SyntaqliteOrderByList));

    SyntaqliteOrderByList *node = (SyntaqliteOrderByList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_order_by_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_ORDER_BY_LIST, first_child);
}

uint32_t synq_ast_order_by_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_order_by_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_ORDER_BY_LIST);
}

uint32_t synq_ast_limit_clause(SynqAstContext *ctx, uint32_t limit, uint32_t offset) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_LIMIT_CLAUSE, sizeof(SyntaqliteLimitClause));

    SyntaqliteLimitClause *node = (SyntaqliteLimitClause*)
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
    SyntaqliteSourceSpan column,
    SyntaqliteSourceSpan table,
    SyntaqliteSourceSpan schema
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COLUMN_REF, sizeof(SyntaqliteColumnRef));

    SyntaqliteColumnRef *node = (SyntaqliteColumnRef*)
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
    SyntaqliteSourceSpan func_name,
    SyntaqliteFunctionCallFlags flags,
    uint32_t args,
    uint32_t filter_clause,
    uint32_t over_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_FUNCTION_CALL, sizeof(SyntaqliteFunctionCall));

    SyntaqliteFunctionCall *node = (SyntaqliteFunctionCall*)
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

uint32_t synq_ast_is_expr(SynqAstContext *ctx, SyntaqliteIsOp op, uint32_t left, uint32_t right) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_IS_EXPR, sizeof(SyntaqliteIsExpr));

    SyntaqliteIsExpr *node = (SyntaqliteIsExpr*)
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
    SyntaqliteBool negated,
    uint32_t operand,
    uint32_t low,
    uint32_t high
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_BETWEEN_EXPR, sizeof(SyntaqliteBetweenExpr));

    SyntaqliteBetweenExpr *node = (SyntaqliteBetweenExpr*)
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
    SyntaqliteBool negated,
    uint32_t operand,
    uint32_t pattern,
    uint32_t escape
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_LIKE_EXPR, sizeof(SyntaqliteLikeExpr));

    SyntaqliteLikeExpr *node = (SyntaqliteLikeExpr*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CASE_EXPR, sizeof(SyntaqliteCaseExpr));

    SyntaqliteCaseExpr *node = (SyntaqliteCaseExpr*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CASE_WHEN, sizeof(SyntaqliteCaseWhen));

    SyntaqliteCaseWhen *node = (SyntaqliteCaseWhen*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CASE_WHEN_LIST, sizeof(SyntaqliteCaseWhenList));

    SyntaqliteCaseWhenList *node = (SyntaqliteCaseWhenList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_case_when_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_CASE_WHEN_LIST, first_child);
}

uint32_t synq_ast_case_when_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_case_when_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_CASE_WHEN_LIST);
}

uint32_t synq_ast_compound_select(SynqAstContext *ctx, SyntaqliteCompoundOp op, uint32_t left, uint32_t right) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COMPOUND_SELECT, sizeof(SyntaqliteCompoundSelect));

    SyntaqliteCompoundSelect *node = (SyntaqliteCompoundSelect*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_SUBQUERY_EXPR, sizeof(SyntaqliteSubqueryExpr));

    SyntaqliteSubqueryExpr *node = (SyntaqliteSubqueryExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, select);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_exists_expr(SynqAstContext *ctx, uint32_t select) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_EXISTS_EXPR, sizeof(SyntaqliteExistsExpr));

    SyntaqliteExistsExpr *node = (SyntaqliteExistsExpr*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->select = select;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, select);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_in_expr(SynqAstContext *ctx, SyntaqliteBool negated, uint32_t operand, uint32_t source) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_IN_EXPR, sizeof(SyntaqliteInExpr));

    SyntaqliteInExpr *node = (SyntaqliteInExpr*)
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

uint32_t synq_ast_variable(SynqAstContext *ctx, SyntaqliteSourceSpan source) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_VARIABLE, sizeof(SyntaqliteVariable));

    SyntaqliteVariable *node = (SyntaqliteVariable*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->source = source;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union_span(&_r, source);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_collate_expr(SynqAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan collation) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COLLATE_EXPR, sizeof(SyntaqliteCollateExpr));

    SyntaqliteCollateExpr *node = (SyntaqliteCollateExpr*)
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

uint32_t synq_ast_cast_expr(SynqAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan type_name) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CAST_EXPR, sizeof(SyntaqliteCastExpr));

    SyntaqliteCastExpr *node = (SyntaqliteCastExpr*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_VALUES_ROW_LIST, sizeof(SyntaqliteValuesRowList));

    SyntaqliteValuesRowList *node = (SyntaqliteValuesRowList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_values_row_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_VALUES_ROW_LIST, first_child);
}

uint32_t synq_ast_values_row_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_values_row_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_VALUES_ROW_LIST);
}

uint32_t synq_ast_values_clause(SynqAstContext *ctx, uint32_t rows) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_VALUES_CLAUSE, sizeof(SyntaqliteValuesClause));

    SyntaqliteValuesClause *node = (SyntaqliteValuesClause*)
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
    SyntaqliteSourceSpan cte_name,
    SyntaqliteMaterialized materialized,
    uint32_t columns,
    uint32_t select
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CTE_DEFINITION, sizeof(SyntaqliteCteDefinition));

    SyntaqliteCteDefinition *node = (SyntaqliteCteDefinition*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CTE_LIST, sizeof(SyntaqliteCteList));

    SyntaqliteCteList *node = (SyntaqliteCteList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_cte_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_CTE_LIST, first_child);
}

uint32_t synq_ast_cte_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_cte_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_CTE_LIST);
}

uint32_t synq_ast_with_clause(SynqAstContext *ctx, SyntaqliteBool recursive, uint32_t ctes, uint32_t select) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_WITH_CLAUSE, sizeof(SyntaqliteWithClause));

    SyntaqliteWithClause *node = (SyntaqliteWithClause*)
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
    SyntaqliteSourceSpan func_name,
    SyntaqliteAggregateFunctionCallFlags flags,
    uint32_t args,
    uint32_t orderby,
    uint32_t filter_clause,
    uint32_t over_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL, sizeof(SyntaqliteAggregateFunctionCall));

    SyntaqliteAggregateFunctionCall *node = (SyntaqliteAggregateFunctionCall*)
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

uint32_t synq_ast_raise_expr(SynqAstContext *ctx, SyntaqliteRaiseType raise_type, uint32_t error_message) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_RAISE_EXPR, sizeof(SyntaqliteRaiseExpr));

    SyntaqliteRaiseExpr *node = (SyntaqliteRaiseExpr*)
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
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan alias
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_TABLE_REF, sizeof(SyntaqliteTableRef));

    SyntaqliteTableRef *node = (SyntaqliteTableRef*)
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

uint32_t synq_ast_subquery_table_source(SynqAstContext *ctx, uint32_t select, SyntaqliteSourceSpan alias) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE, sizeof(SyntaqliteSubqueryTableSource));

    SyntaqliteSubqueryTableSource *node = (SyntaqliteSubqueryTableSource*)
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
    SyntaqliteJoinType join_type,
    uint32_t left,
    uint32_t right,
    uint32_t on_expr,
    uint32_t using_columns
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_JOIN_CLAUSE, sizeof(SyntaqliteJoinClause));

    SyntaqliteJoinClause *node = (SyntaqliteJoinClause*)
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

uint32_t synq_ast_join_prefix(SynqAstContext *ctx, uint32_t source, SyntaqliteJoinType join_type) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_JOIN_PREFIX, sizeof(SyntaqliteJoinPrefix));

    SyntaqliteJoinPrefix *node = (SyntaqliteJoinPrefix*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_DELETE_STMT, sizeof(SyntaqliteDeleteStmt));

    SyntaqliteDeleteStmt *node = (SyntaqliteDeleteStmt*)
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

uint32_t synq_ast_set_clause(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan column,
    uint32_t columns,
    uint32_t value
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_SET_CLAUSE, sizeof(SyntaqliteSetClause));

    SyntaqliteSetClause *node = (SyntaqliteSetClause*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_SET_CLAUSE_LIST, sizeof(SyntaqliteSetClauseList));

    SyntaqliteSetClauseList *node = (SyntaqliteSetClauseList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_set_clause_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_SET_CLAUSE_LIST, first_child);
}

uint32_t synq_ast_set_clause_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_set_clause_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_SET_CLAUSE_LIST);
}

uint32_t synq_ast_update_stmt(
    SynqAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t setlist,
    uint32_t from_clause,
    uint32_t where
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_UPDATE_STMT, sizeof(SyntaqliteUpdateStmt));

    SyntaqliteUpdateStmt *node = (SyntaqliteUpdateStmt*)
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
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t columns,
    uint32_t source
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_INSERT_STMT, sizeof(SyntaqliteInsertStmt));

    SyntaqliteInsertStmt *node = (SyntaqliteInsertStmt*)
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

uint32_t synq_ast_qualified_name(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan object_name,
    SyntaqliteSourceSpan schema
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_QUALIFIED_NAME, sizeof(SyntaqliteQualifiedName));

    SyntaqliteQualifiedName *node = (SyntaqliteQualifiedName*)
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
    SyntaqliteDropObjectType object_type,
    SyntaqliteBool if_exists,
    uint32_t target
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_DROP_STMT, sizeof(SyntaqliteDropStmt));

    SyntaqliteDropStmt *node = (SyntaqliteDropStmt*)
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
    SyntaqliteAlterOp op,
    uint32_t target,
    SyntaqliteSourceSpan new_name,
    SyntaqliteSourceSpan old_name
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_ALTER_TABLE_STMT, sizeof(SyntaqliteAlterTableStmt));

    SyntaqliteAlterTableStmt *node = (SyntaqliteAlterTableStmt*)
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

uint32_t synq_ast_transaction_stmt(
    SynqAstContext *ctx,
    SyntaqliteTransactionOp op,
    SyntaqliteTransactionType trans_type
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_TRANSACTION_STMT, sizeof(SyntaqliteTransactionStmt));

    SyntaqliteTransactionStmt *node = (SyntaqliteTransactionStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->op = op;
    node->trans_type = trans_type;
    return id;
}

uint32_t synq_ast_savepoint_stmt(
    SynqAstContext *ctx,
    SyntaqliteSavepointOp op,
    SyntaqliteSourceSpan savepoint_name
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_SAVEPOINT_STMT, sizeof(SyntaqliteSavepointStmt));

    SyntaqliteSavepointStmt *node = (SyntaqliteSavepointStmt*)
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
    SyntaqliteSourceSpan pragma_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan value,
    SyntaqlitePragmaForm pragma_form
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_PRAGMA_STMT, sizeof(SyntaqlitePragmaStmt));

    SyntaqlitePragmaStmt *node = (SyntaqlitePragmaStmt*)
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
    SyntaqliteSourceSpan target_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteAnalyzeKind kind
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_ANALYZE_STMT, sizeof(SyntaqliteAnalyzeStmt));

    SyntaqliteAnalyzeStmt *node = (SyntaqliteAnalyzeStmt*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_ATTACH_STMT, sizeof(SyntaqliteAttachStmt));

    SyntaqliteAttachStmt *node = (SyntaqliteAttachStmt*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_DETACH_STMT, sizeof(SyntaqliteDetachStmt));

    SyntaqliteDetachStmt *node = (SyntaqliteDetachStmt*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->db_name = db_name;

    synq_ast_ranges_sync(ctx);
    SynqSourceRange _r = {UINT32_MAX, 0};
    synq_ast_range_union(ctx, &_r, db_name);
    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
    return id;
}

uint32_t synq_ast_vacuum_stmt(SynqAstContext *ctx, SyntaqliteSourceSpan schema, uint32_t into_expr) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_VACUUM_STMT, sizeof(SyntaqliteVacuumStmt));

    SyntaqliteVacuumStmt *node = (SyntaqliteVacuumStmt*)
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

uint32_t synq_ast_explain_stmt(SynqAstContext *ctx, SyntaqliteExplainMode explain_mode, uint32_t stmt) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_EXPLAIN_STMT, sizeof(SyntaqliteExplainStmt));

    SyntaqliteExplainStmt *node = (SyntaqliteExplainStmt*)
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
    SyntaqliteSourceSpan index_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan table_name,
    SyntaqliteBool is_unique,
    SyntaqliteBool if_not_exists,
    uint32_t columns,
    uint32_t where
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CREATE_INDEX_STMT, sizeof(SyntaqliteCreateIndexStmt));

    SyntaqliteCreateIndexStmt *node = (SyntaqliteCreateIndexStmt*)
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
    SyntaqliteSourceSpan view_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    uint32_t column_names,
    uint32_t select
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CREATE_VIEW_STMT, sizeof(SyntaqliteCreateViewStmt));

    SyntaqliteCreateViewStmt *node = (SyntaqliteCreateViewStmt*)
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
    SyntaqliteSourceSpan ref_table,
    uint32_t ref_columns,
    SyntaqliteForeignKeyAction on_delete,
    SyntaqliteForeignKeyAction on_update,
    SyntaqliteBool is_deferred
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE, sizeof(SyntaqliteForeignKeyClause));

    SyntaqliteForeignKeyClause *node = (SyntaqliteForeignKeyClause*)
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
    SyntaqliteColumnConstraintKind kind,
    SyntaqliteSourceSpan constraint_name,
    SyntaqliteConflictAction onconf,
    SyntaqliteSortOrder sort_order,
    SyntaqliteBool is_autoincrement,
    SyntaqliteSourceSpan collation_name,
    SyntaqliteGeneratedColumnStorage generated_storage,
    uint32_t default_expr,
    uint32_t check_expr,
    uint32_t generated_expr,
    uint32_t fk_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COLUMN_CONSTRAINT, sizeof(SyntaqliteColumnConstraint));

    SyntaqliteColumnConstraint *node = (SyntaqliteColumnConstraint*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST, sizeof(SyntaqliteColumnConstraintList));

    SyntaqliteColumnConstraintList *node = (SyntaqliteColumnConstraintList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_column_constraint_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST, first_child);
}

uint32_t synq_ast_column_constraint_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_column_constraint_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST);
}

uint32_t synq_ast_column_def(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan column_name,
    SyntaqliteSourceSpan type_name,
    uint32_t constraints
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COLUMN_DEF, sizeof(SyntaqliteColumnDef));

    SyntaqliteColumnDef *node = (SyntaqliteColumnDef*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_COLUMN_DEF_LIST, sizeof(SyntaqliteColumnDefList));

    SyntaqliteColumnDefList *node = (SyntaqliteColumnDefList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_column_def_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_COLUMN_DEF_LIST, first_child);
}

uint32_t synq_ast_column_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_column_def_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_COLUMN_DEF_LIST);
}

uint32_t synq_ast_table_constraint(
    SynqAstContext *ctx,
    SyntaqliteTableConstraintKind kind,
    SyntaqliteSourceSpan constraint_name,
    SyntaqliteConflictAction onconf,
    SyntaqliteBool is_autoincrement,
    uint32_t columns,
    uint32_t check_expr,
    uint32_t fk_clause
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_TABLE_CONSTRAINT, sizeof(SyntaqliteTableConstraint));

    SyntaqliteTableConstraint *node = (SyntaqliteTableConstraint*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST, sizeof(SyntaqliteTableConstraintList));

    SyntaqliteTableConstraintList *node = (SyntaqliteTableConstraintList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_table_constraint_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST, first_child);
}

uint32_t synq_ast_table_constraint_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_table_constraint_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST);
}

uint32_t synq_ast_create_table_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    SyntaqliteCreateTableStmtFlags flags,
    uint32_t columns,
    uint32_t table_constraints,
    uint32_t as_select
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CREATE_TABLE_STMT, sizeof(SyntaqliteCreateTableStmt));

    SyntaqliteCreateTableStmt *node = (SyntaqliteCreateTableStmt*)
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

uint32_t synq_ast_frame_bound(SynqAstContext *ctx, SyntaqliteFrameBoundType bound_type, uint32_t expr) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_FRAME_BOUND, sizeof(SyntaqliteFrameBound));

    SyntaqliteFrameBound *node = (SyntaqliteFrameBound*)
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
    SyntaqliteFrameType frame_type,
    SyntaqliteFrameExclude exclude,
    uint32_t start_bound,
    uint32_t end_bound
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_FRAME_SPEC, sizeof(SyntaqliteFrameSpec));

    SyntaqliteFrameSpec *node = (SyntaqliteFrameSpec*)
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
    SyntaqliteSourceSpan base_window_name,
    uint32_t partition_by,
    uint32_t orderby,
    uint32_t frame
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_WINDOW_DEF, sizeof(SyntaqliteWindowDef));

    SyntaqliteWindowDef *node = (SyntaqliteWindowDef*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_WINDOW_DEF_LIST, sizeof(SyntaqliteWindowDefList));

    SyntaqliteWindowDefList *node = (SyntaqliteWindowDefList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_window_def_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_WINDOW_DEF_LIST, first_child);
}

uint32_t synq_ast_window_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_window_def_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_WINDOW_DEF_LIST);
}

uint32_t synq_ast_named_window_def(SynqAstContext *ctx, SyntaqliteSourceSpan window_name, uint32_t window_def) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_NAMED_WINDOW_DEF, sizeof(SyntaqliteNamedWindowDef));

    SyntaqliteNamedWindowDef *node = (SyntaqliteNamedWindowDef*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST, sizeof(SyntaqliteNamedWindowDefList));

    SyntaqliteNamedWindowDefList *node = (SyntaqliteNamedWindowDefList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_named_window_def_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST, first_child);
}

uint32_t synq_ast_named_window_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_named_window_def_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST);
}

uint32_t synq_ast_filter_over(
    SynqAstContext *ctx,
    uint32_t filter_expr,
    uint32_t over_def,
    SyntaqliteSourceSpan over_name
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_FILTER_OVER, sizeof(SyntaqliteFilterOver));

    SyntaqliteFilterOver *node = (SyntaqliteFilterOver*)
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

uint32_t synq_ast_trigger_event(SynqAstContext *ctx, SyntaqliteTriggerEventType event_type, uint32_t columns) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_TRIGGER_EVENT, sizeof(SyntaqliteTriggerEvent));

    SyntaqliteTriggerEvent *node = (SyntaqliteTriggerEvent*)
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
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_TRIGGER_CMD_LIST, sizeof(SyntaqliteTriggerCmdList));

    SyntaqliteTriggerCmdList *node = (SyntaqliteTriggerCmdList*)
        (ctx->ast.data + ctx->ast.offsets[id]);
    node->count = 0;
    synq_ast_ranges_sync(ctx);
    return id;
}

uint32_t synq_ast_trigger_cmd_list(SynqAstContext *ctx, uint32_t first_child) {
    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_TRIGGER_CMD_LIST, first_child);
}

uint32_t synq_ast_trigger_cmd_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return synq_ast_trigger_cmd_list(ctx, child);
    }
    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_TRIGGER_CMD_LIST);
}

uint32_t synq_ast_create_trigger_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan trigger_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    SyntaqliteTriggerTiming timing,
    uint32_t event,
    uint32_t table,
    uint32_t when_expr,
    uint32_t body
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CREATE_TRIGGER_STMT, sizeof(SyntaqliteCreateTriggerStmt));

    SyntaqliteCreateTriggerStmt *node = (SyntaqliteCreateTriggerStmt*)
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
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan module_name,
    SyntaqliteBool if_not_exists,
    SyntaqliteSourceSpan module_args
) {
    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT, sizeof(SyntaqliteCreateVirtualTableStmt));

    SyntaqliteCreateVirtualTableStmt *node = (SyntaqliteCreateVirtualTableStmt*)
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

