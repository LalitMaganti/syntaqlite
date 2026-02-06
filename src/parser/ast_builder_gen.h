// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#ifndef SYNQ_SRC_AST_AST_BUILDER_GEN_H
#define SYNQ_SRC_AST_AST_BUILDER_GEN_H

#include "src/parser/ast_nodes_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============ Builder Functions ============

uint32_t synq_ast_binary_expr(SynqAstContext *ctx, SyntaqliteBinaryOp op, uint32_t left, uint32_t right);

uint32_t synq_ast_unary_expr(SynqAstContext *ctx, SyntaqliteUnaryOp op, uint32_t operand);

uint32_t synq_ast_literal(
    SynqAstContext *ctx,
    SyntaqliteLiteralType literal_type,
    SyntaqliteSourceSpan source
);

// Create empty ExprList
uint32_t synq_ast_expr_list_empty(SynqAstContext *ctx);

// Create ExprList with single child
uint32_t synq_ast_expr_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to ExprList (may reallocate, returns new list ID)
uint32_t synq_ast_expr_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_result_column(
    SynqAstContext *ctx,
    SyntaqliteResultColumnFlags flags,
    SyntaqliteSourceSpan alias,
    uint32_t expr
);

// Create empty ResultColumnList
uint32_t synq_ast_result_column_list_empty(SynqAstContext *ctx);

// Create ResultColumnList with single child
uint32_t synq_ast_result_column_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to ResultColumnList (may reallocate, returns new list ID)
uint32_t synq_ast_result_column_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

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
);

uint32_t synq_ast_ordering_term(
    SynqAstContext *ctx,
    uint32_t expr,
    SyntaqliteSortOrder sort_order,
    SyntaqliteNullsOrder nulls_order
);

// Create empty OrderByList
uint32_t synq_ast_order_by_list_empty(SynqAstContext *ctx);

// Create OrderByList with single child
uint32_t synq_ast_order_by_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to OrderByList (may reallocate, returns new list ID)
uint32_t synq_ast_order_by_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_limit_clause(SynqAstContext *ctx, uint32_t limit, uint32_t offset);

uint32_t synq_ast_column_ref(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan column,
    SyntaqliteSourceSpan table,
    SyntaqliteSourceSpan schema
);

uint32_t synq_ast_function_call(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan func_name,
    SyntaqliteFunctionCallFlags flags,
    uint32_t args,
    uint32_t filter_clause,
    uint32_t over_clause
);

uint32_t synq_ast_is_expr(SynqAstContext *ctx, SyntaqliteIsOp op, uint32_t left, uint32_t right);

uint32_t synq_ast_between_expr(
    SynqAstContext *ctx,
    SyntaqliteBool negated,
    uint32_t operand,
    uint32_t low,
    uint32_t high
);

uint32_t synq_ast_like_expr(
    SynqAstContext *ctx,
    SyntaqliteBool negated,
    uint32_t operand,
    uint32_t pattern,
    uint32_t escape
);

uint32_t synq_ast_case_expr(SynqAstContext *ctx, uint32_t operand, uint32_t else_expr, uint32_t whens);

uint32_t synq_ast_case_when(SynqAstContext *ctx, uint32_t when_expr, uint32_t then_expr);

// Create empty CaseWhenList
uint32_t synq_ast_case_when_list_empty(SynqAstContext *ctx);

// Create CaseWhenList with single child
uint32_t synq_ast_case_when_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to CaseWhenList (may reallocate, returns new list ID)
uint32_t synq_ast_case_when_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_compound_select(SynqAstContext *ctx, SyntaqliteCompoundOp op, uint32_t left, uint32_t right);

uint32_t synq_ast_subquery_expr(SynqAstContext *ctx, uint32_t select);

uint32_t synq_ast_exists_expr(SynqAstContext *ctx, uint32_t select);

uint32_t synq_ast_in_expr(SynqAstContext *ctx, SyntaqliteBool negated, uint32_t operand, uint32_t source);

uint32_t synq_ast_variable(SynqAstContext *ctx, SyntaqliteSourceSpan source);

uint32_t synq_ast_collate_expr(SynqAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan collation);

uint32_t synq_ast_cast_expr(SynqAstContext *ctx, uint32_t expr, SyntaqliteSourceSpan type_name);

// Create empty ValuesRowList
uint32_t synq_ast_values_row_list_empty(SynqAstContext *ctx);

// Create ValuesRowList with single child
uint32_t synq_ast_values_row_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to ValuesRowList (may reallocate, returns new list ID)
uint32_t synq_ast_values_row_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_values_clause(SynqAstContext *ctx, uint32_t rows);

uint32_t synq_ast_cte_definition(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan cte_name,
    SyntaqliteMaterialized materialized,
    uint32_t columns,
    uint32_t select
);

// Create empty CteList
uint32_t synq_ast_cte_list_empty(SynqAstContext *ctx);

// Create CteList with single child
uint32_t synq_ast_cte_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to CteList (may reallocate, returns new list ID)
uint32_t synq_ast_cte_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_with_clause(SynqAstContext *ctx, SyntaqliteBool recursive, uint32_t ctes, uint32_t select);

uint32_t synq_ast_aggregate_function_call(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan func_name,
    SyntaqliteAggregateFunctionCallFlags flags,
    uint32_t args,
    uint32_t orderby,
    uint32_t filter_clause,
    uint32_t over_clause
);

uint32_t synq_ast_raise_expr(SynqAstContext *ctx, SyntaqliteRaiseType raise_type, uint32_t error_message);

uint32_t synq_ast_table_ref(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan alias
);

uint32_t synq_ast_subquery_table_source(SynqAstContext *ctx, uint32_t select, SyntaqliteSourceSpan alias);

uint32_t synq_ast_join_clause(
    SynqAstContext *ctx,
    SyntaqliteJoinType join_type,
    uint32_t left,
    uint32_t right,
    uint32_t on_expr,
    uint32_t using_columns
);

uint32_t synq_ast_join_prefix(SynqAstContext *ctx, uint32_t source, SyntaqliteJoinType join_type);

uint32_t synq_ast_delete_stmt(SynqAstContext *ctx, uint32_t table, uint32_t where);

uint32_t synq_ast_set_clause(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan column,
    uint32_t columns,
    uint32_t value
);

// Create empty SetClauseList
uint32_t synq_ast_set_clause_list_empty(SynqAstContext *ctx);

// Create SetClauseList with single child
uint32_t synq_ast_set_clause_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to SetClauseList (may reallocate, returns new list ID)
uint32_t synq_ast_set_clause_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_update_stmt(
    SynqAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t setlist,
    uint32_t from_clause,
    uint32_t where
);

uint32_t synq_ast_insert_stmt(
    SynqAstContext *ctx,
    SyntaqliteConflictAction conflict_action,
    uint32_t table,
    uint32_t columns,
    uint32_t source
);

uint32_t synq_ast_qualified_name(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan object_name,
    SyntaqliteSourceSpan schema
);

uint32_t synq_ast_drop_stmt(
    SynqAstContext *ctx,
    SyntaqliteDropObjectType object_type,
    SyntaqliteBool if_exists,
    uint32_t target
);

uint32_t synq_ast_alter_table_stmt(
    SynqAstContext *ctx,
    SyntaqliteAlterOp op,
    uint32_t target,
    SyntaqliteSourceSpan new_name,
    SyntaqliteSourceSpan old_name
);

uint32_t synq_ast_transaction_stmt(
    SynqAstContext *ctx,
    SyntaqliteTransactionOp op,
    SyntaqliteTransactionType trans_type
);

uint32_t synq_ast_savepoint_stmt(
    SynqAstContext *ctx,
    SyntaqliteSavepointOp op,
    SyntaqliteSourceSpan savepoint_name
);

uint32_t synq_ast_pragma_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan pragma_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan value,
    SyntaqlitePragmaForm pragma_form
);

uint32_t synq_ast_analyze_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan target_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteAnalyzeKind kind
);

uint32_t synq_ast_attach_stmt(SynqAstContext *ctx, uint32_t filename, uint32_t db_name, uint32_t key);

uint32_t synq_ast_detach_stmt(SynqAstContext *ctx, uint32_t db_name);

uint32_t synq_ast_vacuum_stmt(SynqAstContext *ctx, SyntaqliteSourceSpan schema, uint32_t into_expr);

uint32_t synq_ast_explain_stmt(SynqAstContext *ctx, SyntaqliteExplainMode explain_mode, uint32_t stmt);

uint32_t synq_ast_create_index_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan index_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan table_name,
    SyntaqliteBool is_unique,
    SyntaqliteBool if_not_exists,
    uint32_t columns,
    uint32_t where
);

uint32_t synq_ast_create_view_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan view_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteBool is_temp,
    SyntaqliteBool if_not_exists,
    uint32_t column_names,
    uint32_t select
);

uint32_t synq_ast_foreign_key_clause(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan ref_table,
    uint32_t ref_columns,
    SyntaqliteForeignKeyAction on_delete,
    SyntaqliteForeignKeyAction on_update,
    SyntaqliteBool is_deferred
);

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
);

// Create empty ColumnConstraintList
uint32_t synq_ast_column_constraint_list_empty(SynqAstContext *ctx);

// Create ColumnConstraintList with single child
uint32_t synq_ast_column_constraint_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to ColumnConstraintList (may reallocate, returns new list ID)
uint32_t synq_ast_column_constraint_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_column_def(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan column_name,
    SyntaqliteSourceSpan type_name,
    uint32_t constraints
);

// Create empty ColumnDefList
uint32_t synq_ast_column_def_list_empty(SynqAstContext *ctx);

// Create ColumnDefList with single child
uint32_t synq_ast_column_def_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to ColumnDefList (may reallocate, returns new list ID)
uint32_t synq_ast_column_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_table_constraint(
    SynqAstContext *ctx,
    SyntaqliteTableConstraintKind kind,
    SyntaqliteSourceSpan constraint_name,
    SyntaqliteConflictAction onconf,
    SyntaqliteBool is_autoincrement,
    uint32_t columns,
    uint32_t check_expr,
    uint32_t fk_clause
);

// Create empty TableConstraintList
uint32_t synq_ast_table_constraint_list_empty(SynqAstContext *ctx);

// Create TableConstraintList with single child
uint32_t synq_ast_table_constraint_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to TableConstraintList (may reallocate, returns new list ID)
uint32_t synq_ast_table_constraint_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

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
);

uint32_t synq_ast_frame_bound(SynqAstContext *ctx, SyntaqliteFrameBoundType bound_type, uint32_t expr);

uint32_t synq_ast_frame_spec(
    SynqAstContext *ctx,
    SyntaqliteFrameType frame_type,
    SyntaqliteFrameExclude exclude,
    uint32_t start_bound,
    uint32_t end_bound
);

uint32_t synq_ast_window_def(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan base_window_name,
    uint32_t partition_by,
    uint32_t orderby,
    uint32_t frame
);

// Create empty WindowDefList
uint32_t synq_ast_window_def_list_empty(SynqAstContext *ctx);

// Create WindowDefList with single child
uint32_t synq_ast_window_def_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to WindowDefList (may reallocate, returns new list ID)
uint32_t synq_ast_window_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_named_window_def(SynqAstContext *ctx, SyntaqliteSourceSpan window_name, uint32_t window_def);

// Create empty NamedWindowDefList
uint32_t synq_ast_named_window_def_list_empty(SynqAstContext *ctx);

// Create NamedWindowDefList with single child
uint32_t synq_ast_named_window_def_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to NamedWindowDefList (may reallocate, returns new list ID)
uint32_t synq_ast_named_window_def_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

uint32_t synq_ast_filter_over(
    SynqAstContext *ctx,
    uint32_t filter_expr,
    uint32_t over_def,
    SyntaqliteSourceSpan over_name
);

uint32_t synq_ast_trigger_event(SynqAstContext *ctx, SyntaqliteTriggerEventType event_type, uint32_t columns);

// Create empty TriggerCmdList
uint32_t synq_ast_trigger_cmd_list_empty(SynqAstContext *ctx);

// Create TriggerCmdList with single child
uint32_t synq_ast_trigger_cmd_list(SynqAstContext *ctx, uint32_t first_child);

// Append child to TriggerCmdList (may reallocate, returns new list ID)
uint32_t synq_ast_trigger_cmd_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);

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
);

uint32_t synq_ast_create_virtual_table_stmt(
    SynqAstContext *ctx,
    SyntaqliteSourceSpan table_name,
    SyntaqliteSourceSpan schema,
    SyntaqliteSourceSpan module_name,
    SyntaqliteBool if_not_exists,
    SyntaqliteSourceSpan module_args
);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_AST_AST_BUILDER_GEN_H
