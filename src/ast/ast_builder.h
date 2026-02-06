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
    SyntaqliteResultColumnFlags flags,
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
    SyntaqliteSelectStmtFlags flags,
    uint32_t columns,
    uint32_t from_clause,
    uint32_t where,
    uint32_t groupby,
    uint32_t having,
    uint32_t orderby,
    uint32_t limit_clause,
    uint32_t window_clause
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
    SyntaqliteFunctionCallFlags flags,
    uint32_t args,
    uint32_t filter_clause,
    uint32_t over_clause
);

uint32_t ast_is_expr(SyntaqliteAstContext *ctx, SyntaqliteIsOp op, uint32_t left, uint32_t right);

uint32_t ast_between_expr(
    SyntaqliteAstContext *ctx,
    SyntaqliteBool negated,
    uint32_t operand,
    uint32_t low,
    uint32_t high
);

uint32_t ast_like_expr(
    SyntaqliteAstContext *ctx,
    SyntaqliteBool negated,
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

uint32_t ast_in_expr(
    SyntaqliteAstContext *ctx,
    SyntaqliteBool negated,
    uint32_t operand,
    uint32_t source
);

uint32_t ast_variable(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan source);

uint32_t ast_collate_expr(SyntaqliteAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan collation);

uint32_t ast_cast_expr(SyntaqliteAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan type_name);

// Create empty ValuesRowList
uint32_t ast_values_row_list_empty(SyntaqliteAstContext *ctx);

// Create ValuesRowList with single child
uint32_t ast_values_row_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to ValuesRowList (may reallocate, returns new list ID)
uint32_t ast_values_row_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_values_clause(SyntaqliteAstContext *ctx, uint32_t rows);

uint32_t ast_cte_definition(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan cte_name,
    SyntaqliteMaterialized materialized,
    uint32_t columns,
    uint32_t select
);

// Create empty CteList
uint32_t ast_cte_list_empty(SyntaqliteAstContext *ctx);

// Create CteList with single child
uint32_t ast_cte_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to CteList (may reallocate, returns new list ID)
uint32_t ast_cte_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_with_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteBool recursive,
    uint32_t ctes,
    uint32_t select
);

uint32_t ast_aggregate_function_call(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan func_name,
    SyntaqliteAggregateFunctionCallFlags flags,
    uint32_t args,
    uint32_t orderby,
    uint32_t filter_clause,
    uint32_t over_clause
);

uint32_t ast_raise_expr(
    SyntaqliteAstContext *ctx,
    SyntaqliteRaiseType raise_type,
    uint32_t error_message
);

uint32_t ast_table_ref(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan alias
);

uint32_t ast_subquery_table_source(SyntaqliteAstContext *ctx, uint32_t select, SyntaqliteSourceSpan alias);

uint32_t ast_join_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteJoinType join_type,
    uint32_t left,
    uint32_t right,
    uint32_t on_expr,
    uint32_t using_columns
);

uint32_t ast_join_prefix(SyntaqliteAstContext *ctx, uint32_t source, SyntaqliteJoinType join_type);

uint32_t ast_delete_stmt(SyntaqliteAstContext *ctx, uint32_t table, uint32_t where);

uint32_t ast_set_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan column,
    uint32_t columns,
    uint32_t value
);

// Create empty SetClauseList
uint32_t ast_set_clause_list_empty(SyntaqliteAstContext *ctx);

// Create SetClauseList with single child
uint32_t ast_set_clause_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to SetClauseList (may reallocate, returns new list ID)
uint32_t ast_set_clause_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_update_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t setlist,
    uint32_t from_clause,
    uint32_t where
);

uint32_t ast_insert_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t columns,
    uint32_t source
);

uint32_t ast_qualified_name(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan object_name,
    SyntaqliteSourceSpan schema
);

uint32_t ast_drop_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteDropObjectType object_type,
    SyntaqliteBool if_exists,
    uint32_t target
);

uint32_t ast_alter_table_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteAlterOp op,
    uint32_t target,
    SyntaqliteSourceSpan new_name,
    SyntaqliteSourceSpan old_name
);

uint32_t ast_transaction_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteTransactionOp op,
    SyntaqliteTransactionType trans_type
);

uint32_t ast_savepoint_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSavepointOp op,
    SyntaqliteSourceSpan savepoint_name
);

uint32_t ast_pragma_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan pragma_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan value,
    SyntaqlitePragmaForm pragma_form
);

uint32_t ast_analyze_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan target_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteAnalyzeKind kind
);

uint32_t ast_attach_stmt(SyntaqliteAstContext *ctx, uint32_t filename, uint32_t db_name, uint32_t key);

uint32_t ast_detach_stmt(SyntaqliteAstContext *ctx, uint32_t db_name);

uint32_t ast_vacuum_stmt(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan schema, uint32_t into_expr);

uint32_t ast_explain_stmt(SyntaqliteAstContext *ctx, SyntaqliteExplainMode explain_mode, uint32_t stmt);

uint32_t ast_create_index_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan index_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan table_name,
    SyntaqliteBool is_unique,
    SyntaqliteBool if_not_exists,
    uint32_t columns,
    uint32_t where
);

uint32_t ast_create_view_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan view_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    uint32_t column_names,
    uint32_t select
);

uint32_t ast_foreign_key_clause(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan ref_table,
    uint32_t ref_columns,
    SyntaqliteForeignKeyAction on_delete,
    SyntaqliteForeignKeyAction on_update,
    SyntaqliteBool is_deferred
);

uint32_t ast_column_constraint(
    SyntaqliteAstContext *ctx,
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
);

// Create empty ColumnConstraintList
uint32_t ast_column_constraint_list_empty(SyntaqliteAstContext *ctx);

// Create ColumnConstraintList with single child
uint32_t ast_column_constraint_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to ColumnConstraintList (may reallocate, returns new list ID)
uint32_t ast_column_constraint_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_column_def(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan column_name,
    SyntaqliteSourceSpan type_name,
    uint32_t constraints
);

// Create empty ColumnDefList
uint32_t ast_column_def_list_empty(SyntaqliteAstContext *ctx);

// Create ColumnDefList with single child
uint32_t ast_column_def_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to ColumnDefList (may reallocate, returns new list ID)
uint32_t ast_column_def_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_table_constraint(
    SyntaqliteAstContext *ctx,
    SyntaqliteTableConstraintKind kind,
    SyntaqliteSourceSpan constraint_name,
    SyntaqliteConflictAction onconf,
    SyntaqliteBool is_autoincrement,
    uint32_t columns,
    uint32_t check_expr,
    uint32_t fk_clause
);

// Create empty TableConstraintList
uint32_t ast_table_constraint_list_empty(SyntaqliteAstContext *ctx);

// Create TableConstraintList with single child
uint32_t ast_table_constraint_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to TableConstraintList (may reallocate, returns new list ID)
uint32_t ast_table_constraint_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_create_table_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    SyntaqliteCreateTableStmtFlags flags,
    uint32_t columns,
    uint32_t table_constraints,
    uint32_t as_select
);

uint32_t ast_frame_bound(SyntaqliteAstContext *ctx, SyntaqliteFrameBoundType bound_type, uint32_t expr);

uint32_t ast_frame_spec(
    SyntaqliteAstContext *ctx,
    SyntaqliteFrameType frame_type,
    SyntaqliteFrameExclude exclude,
    uint32_t start_bound,
    uint32_t end_bound
);

uint32_t ast_window_def(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan base_window_name,
    uint32_t partition_by,
    uint32_t orderby,
    uint32_t frame
);

// Create empty WindowDefList
uint32_t ast_window_def_list_empty(SyntaqliteAstContext *ctx);

// Create WindowDefList with single child
uint32_t ast_window_def_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to WindowDefList (may reallocate, returns new list ID)
uint32_t ast_window_def_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_named_window_def(SyntaqliteAstContext *ctx, SyntaqliteSourceSpan window_name, uint32_t window_def);

// Create empty NamedWindowDefList
uint32_t ast_named_window_def_list_empty(SyntaqliteAstContext *ctx);

// Create NamedWindowDefList with single child
uint32_t ast_named_window_def_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to NamedWindowDefList (may reallocate, returns new list ID)
uint32_t ast_named_window_def_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_filter_over(
    SyntaqliteAstContext *ctx,
    uint32_t filter_expr,
    uint32_t over_def,
    SyntaqliteSourceSpan over_name
);

uint32_t ast_trigger_event(
    SyntaqliteAstContext *ctx,
    SyntaqliteTriggerEventType event_type,
    uint32_t columns
);

// Create empty TriggerCmdList
uint32_t ast_trigger_cmd_list_empty(SyntaqliteAstContext *ctx);

// Create TriggerCmdList with single child
uint32_t ast_trigger_cmd_list(SyntaqliteAstContext *ctx, uint32_t first_child);

// Append child to TriggerCmdList (may reallocate, returns new list ID)
uint32_t ast_trigger_cmd_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t ast_create_trigger_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan trigger_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    SyntaqliteTriggerTiming timing,
    uint32_t event,
    uint32_t table,
    uint32_t when_expr,
    uint32_t body
);

uint32_t ast_create_virtual_table_stmt(
    SyntaqliteAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan module_name,
    SyntaqliteBool if_not_exists,
    SyntaqliteSourceSpan module_args
);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_BUILDER_H
