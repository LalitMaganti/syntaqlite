// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/extract_sqlite.py

#ifndef SYNQ_SRC_AST_AST_NODES_GEN_H
#define SYNQ_SRC_AST_AST_NODES_GEN_H

// Public types (enums, node structs, union)
#include "syntaqlite/ast_nodes_gen.h"

// Internal dependencies (SynqArena, etc.)
#include "src/parser/ast_base.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char* const synq_bool_names[] = {
    "FALSE",
    "TRUE",
};

static const char* const synq_literal_type_names[] = {
    "INTEGER",
    "FLOAT",
    "STRING",
    "BLOB",
    "NULL",
    "CURRENT",
    "QNUMBER",
};

static const char* const synq_binary_op_names[] = {
    "PLUS",
    "MINUS",
    "STAR",
    "SLASH",
    "REM",
    "LT",
    "GT",
    "LE",
    "GE",
    "EQ",
    "NE",
    "AND",
    "OR",
    "BITAND",
    "BITOR",
    "LSHIFT",
    "RSHIFT",
    "CONCAT",
    "PTR",
};

static const char* const synq_unary_op_names[] = {
    "MINUS",
    "PLUS",
    "BITNOT",
    "NOT",
};

static const char* const synq_sort_order_names[] = {
    "ASC",
    "DESC",
};

static const char* const synq_nulls_order_names[] = {
    "NONE",
    "FIRST",
    "LAST",
};

static const char* const synq_is_op_names[] = {
    "IS",
    "IS_NOT",
    "ISNULL",
    "NOTNULL",
    "IS_NOT_DISTINCT",
    "IS_DISTINCT",
};

static const char* const synq_compound_op_names[] = {
    "UNION",
    "UNION_ALL",
    "INTERSECT",
    "EXCEPT",
};

static const char* const synq_materialized_names[] = {
    "DEFAULT",
    "MATERIALIZED",
    "NOT_MATERIALIZED",
};

static const char* const synq_raise_type_names[] = {
    "IGNORE",
    "ROLLBACK",
    "ABORT",
    "FAIL",
};

static const char* const synq_join_type_names[] = {
    "COMMA",
    "INNER",
    "LEFT",
    "RIGHT",
    "FULL",
    "CROSS",
    "NATURAL_INNER",
    "NATURAL_LEFT",
    "NATURAL_RIGHT",
    "NATURAL_FULL",
};

static const char* const synq_conflict_action_names[] = {
    "DEFAULT",
    "ROLLBACK",
    "ABORT",
    "FAIL",
    "IGNORE",
    "REPLACE",
};

static const char* const synq_drop_object_type_names[] = {
    "TABLE",
    "INDEX",
    "VIEW",
    "TRIGGER",
};

static const char* const synq_alter_op_names[] = {
    "RENAME_TABLE",
    "RENAME_COLUMN",
    "DROP_COLUMN",
    "ADD_COLUMN",
};

static const char* const synq_transaction_type_names[] = {
    "DEFERRED",
    "IMMEDIATE",
    "EXCLUSIVE",
};

static const char* const synq_transaction_op_names[] = {
    "BEGIN",
    "COMMIT",
    "ROLLBACK",
};

static const char* const synq_savepoint_op_names[] = {
    "SAVEPOINT",
    "RELEASE",
    "ROLLBACK_TO",
};

static const char* const synq_explain_mode_names[] = {
    "EXPLAIN",
    "QUERY_PLAN",
};

static const char* const synq_pragma_form_names[] = {
    "BARE",
    "EQ",
    "CALL",
};

static const char* const synq_analyze_kind_names[] = {
    "ANALYZE",
    "REINDEX",
};

static const char* const synq_foreign_key_action_names[] = {
    "NO_ACTION",
    "SET_NULL",
    "SET_DEFAULT",
    "CASCADE",
    "RESTRICT",
};

static const char* const synq_generated_column_storage_names[] = {
    "VIRTUAL",
    "STORED",
};

static const char* const synq_column_constraint_kind_names[] = {
    "DEFAULT",
    "NOT_NULL",
    "PRIMARY_KEY",
    "UNIQUE",
    "CHECK",
    "REFERENCES",
    "COLLATE",
    "GENERATED",
    "NULL",
};

static const char* const synq_table_constraint_kind_names[] = {
    "PRIMARY_KEY",
    "UNIQUE",
    "CHECK",
    "FOREIGN_KEY",
};

static const char* const synq_frame_type_names[] = {
    "NONE",
    "RANGE",
    "ROWS",
    "GROUPS",
};

static const char* const synq_frame_bound_type_names[] = {
    "UNBOUNDED_PRECEDING",
    "EXPR_PRECEDING",
    "CURRENT_ROW",
    "EXPR_FOLLOWING",
    "UNBOUNDED_FOLLOWING",
};

static const char* const synq_frame_exclude_names[] = {
    "NONE",
    "NO_OTHERS",
    "CURRENT_ROW",
    "GROUP",
    "TIES",
};

static const char* const synq_trigger_timing_names[] = {
    "BEFORE",
    "AFTER",
    "INSTEAD_OF",
};

static const char* const synq_trigger_event_type_names[] = {
    "DELETE",
    "INSERT",
    "UPDATE",
};

// Access node by ID
inline SynqNode* synq_ast_node(SynqArena *ast, uint32_t id) {
    if (id == SYNQ_NULL_NODE) return NULL;
    return (SynqNode*)(ast->data + ast->offsets[id]);
}

#define AST_NODE(ast, id) synq_ast_node(ast, id)

// ============ Node Size Table ============

// Returns the fixed size of a node type (0 for variable-size nodes like lists)
size_t synq_node_base_size(uint8_t tag);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_AST_AST_NODES_GEN_H
