// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/extract_sqlite.py

#ifndef SYNTAQLITE_AST_NODES_GEN_H
#define SYNTAQLITE_AST_NODES_GEN_H

#ifdef SYNTAQLITE_CUSTOM_NODES
#include SYNTAQLITE_CUSTOM_NODES
#else

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Null node ID (used for nullable fields)
#define SYNQ_NULL_NODE 0xFFFFFFFFu

// Source location span (offset + length into source text)
typedef struct SynqSourceSpan {
    uint32_t offset;
    uint16_t length;
} SynqSourceSpan;

// ============ Value Enums ============

typedef enum {
    SYNQ_BOOL_FALSE = 0,
    SYNQ_BOOL_TRUE = 1,
} SynqBool;

typedef enum {
    SYNQ_LITERAL_TYPE_INTEGER = 0,
    SYNQ_LITERAL_TYPE_FLOAT = 1,
    SYNQ_LITERAL_TYPE_STRING = 2,
    SYNQ_LITERAL_TYPE_BLOB = 3,
    SYNQ_LITERAL_TYPE_NULL = 4,
    SYNQ_LITERAL_TYPE_CURRENT = 5,
    SYNQ_LITERAL_TYPE_QNUMBER = 6,
} SynqLiteralType;

typedef enum {
    SYNQ_BINARY_OP_PLUS = 0,
    SYNQ_BINARY_OP_MINUS = 1,
    SYNQ_BINARY_OP_STAR = 2,
    SYNQ_BINARY_OP_SLASH = 3,
    SYNQ_BINARY_OP_REM = 4,
    SYNQ_BINARY_OP_LT = 5,
    SYNQ_BINARY_OP_GT = 6,
    SYNQ_BINARY_OP_LE = 7,
    SYNQ_BINARY_OP_GE = 8,
    SYNQ_BINARY_OP_EQ = 9,
    SYNQ_BINARY_OP_NE = 10,
    SYNQ_BINARY_OP_AND = 11,
    SYNQ_BINARY_OP_OR = 12,
    SYNQ_BINARY_OP_BITAND = 13,
    SYNQ_BINARY_OP_BITOR = 14,
    SYNQ_BINARY_OP_LSHIFT = 15,
    SYNQ_BINARY_OP_RSHIFT = 16,
    SYNQ_BINARY_OP_CONCAT = 17,
    SYNQ_BINARY_OP_PTR = 18,
} SynqBinaryOp;

typedef enum {
    SYNQ_UNARY_OP_MINUS = 0,
    SYNQ_UNARY_OP_PLUS = 1,
    SYNQ_UNARY_OP_BITNOT = 2,
    SYNQ_UNARY_OP_NOT = 3,
} SynqUnaryOp;

typedef enum {
    SYNQ_SORT_ORDER_ASC = 0,
    SYNQ_SORT_ORDER_DESC = 1,
} SynqSortOrder;

typedef enum {
    SYNQ_NULLS_ORDER_NONE = 0,
    SYNQ_NULLS_ORDER_FIRST = 1,
    SYNQ_NULLS_ORDER_LAST = 2,
} SynqNullsOrder;

typedef enum {
    SYNQ_IS_OP_IS = 0,
    SYNQ_IS_OP_IS_NOT = 1,
    SYNQ_IS_OP_ISNULL = 2,
    SYNQ_IS_OP_NOTNULL = 3,
    SYNQ_IS_OP_IS_NOT_DISTINCT = 4,
    SYNQ_IS_OP_IS_DISTINCT = 5,
} SynqIsOp;

typedef enum {
    SYNQ_COMPOUND_OP_UNION = 0,
    SYNQ_COMPOUND_OP_UNION_ALL = 1,
    SYNQ_COMPOUND_OP_INTERSECT = 2,
    SYNQ_COMPOUND_OP_EXCEPT = 3,
} SynqCompoundOp;

typedef enum {
    SYNQ_MATERIALIZED_DEFAULT = 0,
    SYNQ_MATERIALIZED_MATERIALIZED = 1,
    SYNQ_MATERIALIZED_NOT_MATERIALIZED = 2,
} SynqMaterialized;

typedef enum {
    SYNQ_RAISE_TYPE_IGNORE = 0,
    SYNQ_RAISE_TYPE_ROLLBACK = 1,
    SYNQ_RAISE_TYPE_ABORT = 2,
    SYNQ_RAISE_TYPE_FAIL = 3,
} SynqRaiseType;

typedef enum {
    SYNQ_JOIN_TYPE_COMMA = 0,
    SYNQ_JOIN_TYPE_INNER = 1,
    SYNQ_JOIN_TYPE_LEFT = 2,
    SYNQ_JOIN_TYPE_RIGHT = 3,
    SYNQ_JOIN_TYPE_FULL = 4,
    SYNQ_JOIN_TYPE_CROSS = 5,
    SYNQ_JOIN_TYPE_NATURAL_INNER = 6,
    SYNQ_JOIN_TYPE_NATURAL_LEFT = 7,
    SYNQ_JOIN_TYPE_NATURAL_RIGHT = 8,
    SYNQ_JOIN_TYPE_NATURAL_FULL = 9,
} SynqJoinType;

typedef enum {
    SYNQ_CONFLICT_ACTION_DEFAULT = 0,
    SYNQ_CONFLICT_ACTION_ROLLBACK = 1,
    SYNQ_CONFLICT_ACTION_ABORT = 2,
    SYNQ_CONFLICT_ACTION_FAIL = 3,
    SYNQ_CONFLICT_ACTION_IGNORE = 4,
    SYNQ_CONFLICT_ACTION_REPLACE = 5,
} SynqConflictAction;

typedef enum {
    SYNQ_DROP_OBJECT_TYPE_TABLE = 0,
    SYNQ_DROP_OBJECT_TYPE_INDEX = 1,
    SYNQ_DROP_OBJECT_TYPE_VIEW = 2,
    SYNQ_DROP_OBJECT_TYPE_TRIGGER = 3,
} SynqDropObjectType;

typedef enum {
    SYNQ_ALTER_OP_RENAME_TABLE = 0,
    SYNQ_ALTER_OP_RENAME_COLUMN = 1,
    SYNQ_ALTER_OP_DROP_COLUMN = 2,
    SYNQ_ALTER_OP_ADD_COLUMN = 3,
} SynqAlterOp;

typedef enum {
    SYNQ_TRANSACTION_TYPE_DEFERRED = 0,
    SYNQ_TRANSACTION_TYPE_IMMEDIATE = 1,
    SYNQ_TRANSACTION_TYPE_EXCLUSIVE = 2,
} SynqTransactionType;

typedef enum {
    SYNQ_TRANSACTION_OP_BEGIN = 0,
    SYNQ_TRANSACTION_OP_COMMIT = 1,
    SYNQ_TRANSACTION_OP_ROLLBACK = 2,
} SynqTransactionOp;

typedef enum {
    SYNQ_SAVEPOINT_OP_SAVEPOINT = 0,
    SYNQ_SAVEPOINT_OP_RELEASE = 1,
    SYNQ_SAVEPOINT_OP_ROLLBACK_TO = 2,
} SynqSavepointOp;

typedef enum {
    SYNQ_EXPLAIN_MODE_EXPLAIN = 0,
    SYNQ_EXPLAIN_MODE_QUERY_PLAN = 1,
} SynqExplainMode;

typedef enum {
    SYNQ_PRAGMA_FORM_BARE = 0,
    SYNQ_PRAGMA_FORM_EQ = 1,
    SYNQ_PRAGMA_FORM_CALL = 2,
} SynqPragmaForm;

typedef enum {
    SYNQ_ANALYZE_KIND_ANALYZE = 0,
    SYNQ_ANALYZE_KIND_REINDEX = 1,
} SynqAnalyzeKind;

typedef enum {
    SYNQ_FOREIGN_KEY_ACTION_NO_ACTION = 0,
    SYNQ_FOREIGN_KEY_ACTION_SET_NULL = 1,
    SYNQ_FOREIGN_KEY_ACTION_SET_DEFAULT = 2,
    SYNQ_FOREIGN_KEY_ACTION_CASCADE = 3,
    SYNQ_FOREIGN_KEY_ACTION_RESTRICT = 4,
} SynqForeignKeyAction;

typedef enum {
    SYNQ_GENERATED_COLUMN_STORAGE_VIRTUAL = 0,
    SYNQ_GENERATED_COLUMN_STORAGE_STORED = 1,
} SynqGeneratedColumnStorage;

typedef enum {
    SYNQ_COLUMN_CONSTRAINT_KIND_DEFAULT = 0,
    SYNQ_COLUMN_CONSTRAINT_KIND_NOT_NULL = 1,
    SYNQ_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY = 2,
    SYNQ_COLUMN_CONSTRAINT_KIND_UNIQUE = 3,
    SYNQ_COLUMN_CONSTRAINT_KIND_CHECK = 4,
    SYNQ_COLUMN_CONSTRAINT_KIND_REFERENCES = 5,
    SYNQ_COLUMN_CONSTRAINT_KIND_COLLATE = 6,
    SYNQ_COLUMN_CONSTRAINT_KIND_GENERATED = 7,
    SYNQ_COLUMN_CONSTRAINT_KIND_NULL = 8,
} SynqColumnConstraintKind;

typedef enum {
    SYNQ_TABLE_CONSTRAINT_KIND_PRIMARY_KEY = 0,
    SYNQ_TABLE_CONSTRAINT_KIND_UNIQUE = 1,
    SYNQ_TABLE_CONSTRAINT_KIND_CHECK = 2,
    SYNQ_TABLE_CONSTRAINT_KIND_FOREIGN_KEY = 3,
} SynqTableConstraintKind;

typedef enum {
    SYNQ_FRAME_TYPE_NONE = 0,
    SYNQ_FRAME_TYPE_RANGE = 1,
    SYNQ_FRAME_TYPE_ROWS = 2,
    SYNQ_FRAME_TYPE_GROUPS = 3,
} SynqFrameType;

typedef enum {
    SYNQ_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING = 0,
    SYNQ_FRAME_BOUND_TYPE_EXPR_PRECEDING = 1,
    SYNQ_FRAME_BOUND_TYPE_CURRENT_ROW = 2,
    SYNQ_FRAME_BOUND_TYPE_EXPR_FOLLOWING = 3,
    SYNQ_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING = 4,
} SynqFrameBoundType;

typedef enum {
    SYNQ_FRAME_EXCLUDE_NONE = 0,
    SYNQ_FRAME_EXCLUDE_NO_OTHERS = 1,
    SYNQ_FRAME_EXCLUDE_CURRENT_ROW = 2,
    SYNQ_FRAME_EXCLUDE_GROUP = 3,
    SYNQ_FRAME_EXCLUDE_TIES = 4,
} SynqFrameExclude;

typedef enum {
    SYNQ_TRIGGER_TIMING_BEFORE = 0,
    SYNQ_TRIGGER_TIMING_AFTER = 1,
    SYNQ_TRIGGER_TIMING_INSTEAD_OF = 2,
} SynqTriggerTiming;

typedef enum {
    SYNQ_TRIGGER_EVENT_TYPE_DELETE = 0,
    SYNQ_TRIGGER_EVENT_TYPE_INSERT = 1,
    SYNQ_TRIGGER_EVENT_TYPE_UPDATE = 2,
} SynqTriggerEventType;

// ============ Flags Types ============

typedef union SynqResultColumnFlags {
    uint8_t raw;
    struct {
        uint8_t star : 1;
    };
} SynqResultColumnFlags;

typedef union SynqSelectStmtFlags {
    uint8_t raw;
    struct {
        uint8_t distinct : 1;
    };
} SynqSelectStmtFlags;

typedef union SynqFunctionCallFlags {
    uint8_t raw;
    struct {
        uint8_t distinct : 1;
        uint8_t star : 1;
    };
} SynqFunctionCallFlags;

typedef union SynqAggregateFunctionCallFlags {
    uint8_t raw;
    struct {
        uint8_t distinct : 1;
    };
} SynqAggregateFunctionCallFlags;

typedef union SynqCreateTableStmtFlags {
    uint8_t raw;
    struct {
        uint8_t without_rowid : 1;
        uint8_t strict : 1;
    };
} SynqCreateTableStmtFlags;

// ============ Node Tags ============

typedef enum {
    SYNQ_NODE_NULL = 0,
    SYNQ_NODE_BINARY_EXPR,
    SYNQ_NODE_UNARY_EXPR,
    SYNQ_NODE_LITERAL,
    SYNQ_NODE_EXPR_LIST,
    SYNQ_NODE_RESULT_COLUMN,
    SYNQ_NODE_RESULT_COLUMN_LIST,
    SYNQ_NODE_SELECT_STMT,
    SYNQ_NODE_ORDERING_TERM,
    SYNQ_NODE_ORDER_BY_LIST,
    SYNQ_NODE_LIMIT_CLAUSE,
    SYNQ_NODE_COLUMN_REF,
    SYNQ_NODE_FUNCTION_CALL,
    SYNQ_NODE_IS_EXPR,
    SYNQ_NODE_BETWEEN_EXPR,
    SYNQ_NODE_LIKE_EXPR,
    SYNQ_NODE_CASE_EXPR,
    SYNQ_NODE_CASE_WHEN,
    SYNQ_NODE_CASE_WHEN_LIST,
    SYNQ_NODE_COMPOUND_SELECT,
    SYNQ_NODE_SUBQUERY_EXPR,
    SYNQ_NODE_EXISTS_EXPR,
    SYNQ_NODE_IN_EXPR,
    SYNQ_NODE_VARIABLE,
    SYNQ_NODE_COLLATE_EXPR,
    SYNQ_NODE_CAST_EXPR,
    SYNQ_NODE_VALUES_ROW_LIST,
    SYNQ_NODE_VALUES_CLAUSE,
    SYNQ_NODE_CTE_DEFINITION,
    SYNQ_NODE_CTE_LIST,
    SYNQ_NODE_WITH_CLAUSE,
    SYNQ_NODE_AGGREGATE_FUNCTION_CALL,
    SYNQ_NODE_RAISE_EXPR,
    SYNQ_NODE_TABLE_REF,
    SYNQ_NODE_SUBQUERY_TABLE_SOURCE,
    SYNQ_NODE_JOIN_CLAUSE,
    SYNQ_NODE_JOIN_PREFIX,
    SYNQ_NODE_DELETE_STMT,
    SYNQ_NODE_SET_CLAUSE,
    SYNQ_NODE_SET_CLAUSE_LIST,
    SYNQ_NODE_UPDATE_STMT,
    SYNQ_NODE_INSERT_STMT,
    SYNQ_NODE_QUALIFIED_NAME,
    SYNQ_NODE_DROP_STMT,
    SYNQ_NODE_ALTER_TABLE_STMT,
    SYNQ_NODE_TRANSACTION_STMT,
    SYNQ_NODE_SAVEPOINT_STMT,
    SYNQ_NODE_PRAGMA_STMT,
    SYNQ_NODE_ANALYZE_STMT,
    SYNQ_NODE_ATTACH_STMT,
    SYNQ_NODE_DETACH_STMT,
    SYNQ_NODE_VACUUM_STMT,
    SYNQ_NODE_EXPLAIN_STMT,
    SYNQ_NODE_CREATE_INDEX_STMT,
    SYNQ_NODE_CREATE_VIEW_STMT,
    SYNQ_NODE_FOREIGN_KEY_CLAUSE,
    SYNQ_NODE_COLUMN_CONSTRAINT,
    SYNQ_NODE_COLUMN_CONSTRAINT_LIST,
    SYNQ_NODE_COLUMN_DEF,
    SYNQ_NODE_COLUMN_DEF_LIST,
    SYNQ_NODE_TABLE_CONSTRAINT,
    SYNQ_NODE_TABLE_CONSTRAINT_LIST,
    SYNQ_NODE_CREATE_TABLE_STMT,
    SYNQ_NODE_FRAME_BOUND,
    SYNQ_NODE_FRAME_SPEC,
    SYNQ_NODE_WINDOW_DEF,
    SYNQ_NODE_WINDOW_DEF_LIST,
    SYNQ_NODE_NAMED_WINDOW_DEF,
    SYNQ_NODE_NAMED_WINDOW_DEF_LIST,
    SYNQ_NODE_FILTER_OVER,
    SYNQ_NODE_TRIGGER_EVENT,
    SYNQ_NODE_TRIGGER_CMD_LIST,
    SYNQ_NODE_CREATE_TRIGGER_STMT,
    SYNQ_NODE_CREATE_VIRTUAL_TABLE_STMT,
    SYNQ_NODE_COUNT
} SynqNodeTag;

// ============ Node Structs (variable sizes) ============

typedef struct SynqBinaryExpr {
    uint8_t tag;
    SynqBinaryOp op;
    uint32_t left;
    uint32_t right;
} SynqBinaryExpr;

typedef struct SynqUnaryExpr {
    uint8_t tag;
    SynqUnaryOp op;
    uint32_t operand;
} SynqUnaryExpr;

typedef struct SynqLiteral {
    uint8_t tag;
    SynqLiteralType literal_type;
    SynqSourceSpan source;
} SynqLiteral;

// List of Expr
typedef struct SynqExprList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqExprList;

typedef struct SynqResultColumn {
    uint8_t tag;
    SynqResultColumnFlags flags;
    SynqSourceSpan alias;
    uint32_t expr;
} SynqResultColumn;

// List of ResultColumn
typedef struct SynqResultColumnList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqResultColumnList;

typedef struct SynqSelectStmt {
    uint8_t tag;
    SynqSelectStmtFlags flags;
    uint32_t columns;
    uint32_t from_clause;
    uint32_t where;
    uint32_t groupby;
    uint32_t having;
    uint32_t orderby;
    uint32_t limit_clause;
    uint32_t window_clause;
} SynqSelectStmt;

typedef struct SynqOrderingTerm {
    uint8_t tag;
    uint32_t expr;
    SynqSortOrder sort_order;
    SynqNullsOrder nulls_order;
} SynqOrderingTerm;

// List of OrderingTerm
typedef struct SynqOrderByList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqOrderByList;

typedef struct SynqLimitClause {
    uint8_t tag;
    uint32_t limit;
    uint32_t offset;
} SynqLimitClause;

typedef struct SynqColumnRef {
    uint8_t tag;
    SynqSourceSpan column;
    SynqSourceSpan table;
    SynqSourceSpan schema;
} SynqColumnRef;

typedef struct SynqFunctionCall {
    uint8_t tag;
    SynqSourceSpan func_name;
    SynqFunctionCallFlags flags;
    uint32_t args;
    uint32_t filter_clause;
    uint32_t over_clause;
} SynqFunctionCall;

typedef struct SynqIsExpr {
    uint8_t tag;
    SynqIsOp op;
    uint32_t left;
    uint32_t right;
} SynqIsExpr;

typedef struct SynqBetweenExpr {
    uint8_t tag;
    SynqBool negated;
    uint32_t operand;
    uint32_t low;
    uint32_t high;
} SynqBetweenExpr;

typedef struct SynqLikeExpr {
    uint8_t tag;
    SynqBool negated;
    uint32_t operand;
    uint32_t pattern;
    uint32_t escape;
} SynqLikeExpr;

typedef struct SynqCaseExpr {
    uint8_t tag;
    uint32_t operand;
    uint32_t else_expr;
    uint32_t whens;
} SynqCaseExpr;

typedef struct SynqCaseWhen {
    uint8_t tag;
    uint32_t when_expr;
    uint32_t then_expr;
} SynqCaseWhen;

// List of CaseWhen
typedef struct SynqCaseWhenList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqCaseWhenList;

typedef struct SynqCompoundSelect {
    uint8_t tag;
    SynqCompoundOp op;
    uint32_t left;
    uint32_t right;
} SynqCompoundSelect;

typedef struct SynqSubqueryExpr {
    uint8_t tag;
    uint32_t select;
} SynqSubqueryExpr;

typedef struct SynqExistsExpr {
    uint8_t tag;
    uint32_t select;
} SynqExistsExpr;

typedef struct SynqInExpr {
    uint8_t tag;
    SynqBool negated;
    uint32_t operand;
    uint32_t source;
} SynqInExpr;

typedef struct SynqVariable {
    uint8_t tag;
    SynqSourceSpan source;
} SynqVariable;

typedef struct SynqCollateExpr {
    uint8_t tag;
    uint32_t expr;
    SynqSourceSpan collation;
} SynqCollateExpr;

typedef struct SynqCastExpr {
    uint8_t tag;
    uint32_t expr;
    SynqSourceSpan type_name;
} SynqCastExpr;

// List of ExprList
typedef struct SynqValuesRowList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqValuesRowList;

typedef struct SynqValuesClause {
    uint8_t tag;
    uint32_t rows;
} SynqValuesClause;

typedef struct SynqCteDefinition {
    uint8_t tag;
    SynqSourceSpan cte_name;
    SynqMaterialized materialized;
    uint32_t columns;
    uint32_t select;
} SynqCteDefinition;

// List of CteDefinition
typedef struct SynqCteList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqCteList;

typedef struct SynqWithClause {
    uint8_t tag;
    SynqBool recursive;
    uint32_t ctes;
    uint32_t select;
} SynqWithClause;

typedef struct SynqAggregateFunctionCall {
    uint8_t tag;
    SynqSourceSpan func_name;
    SynqAggregateFunctionCallFlags flags;
    uint32_t args;
    uint32_t orderby;
    uint32_t filter_clause;
    uint32_t over_clause;
} SynqAggregateFunctionCall;

typedef struct SynqRaiseExpr {
    uint8_t tag;
    SynqRaiseType raise_type;
    uint32_t error_message;
} SynqRaiseExpr;

typedef struct SynqTableRef {
    uint8_t tag;
    SynqSourceSpan table_name;
    SynqSourceSpan schema;
    SynqSourceSpan alias;
} SynqTableRef;

typedef struct SynqSubqueryTableSource {
    uint8_t tag;
    uint32_t select;
    SynqSourceSpan alias;
} SynqSubqueryTableSource;

typedef struct SynqJoinClause {
    uint8_t tag;
    SynqJoinType join_type;
    uint32_t left;
    uint32_t right;
    uint32_t on_expr;
    uint32_t using_columns;
} SynqJoinClause;

typedef struct SynqJoinPrefix {
    uint8_t tag;
    uint32_t source;
    SynqJoinType join_type;
} SynqJoinPrefix;

typedef struct SynqDeleteStmt {
    uint8_t tag;
    uint32_t table;
    uint32_t where;
} SynqDeleteStmt;

typedef struct SynqSetClause {
    uint8_t tag;
    SynqSourceSpan column;
    uint32_t columns;
    uint32_t value;
} SynqSetClause;

// List of SetClause
typedef struct SynqSetClauseList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqSetClauseList;

typedef struct SynqUpdateStmt {
    uint8_t tag;
    SynqConflictAction conflict_action;
    uint32_t table;
    uint32_t setlist;
    uint32_t from_clause;
    uint32_t where;
} SynqUpdateStmt;

typedef struct SynqInsertStmt {
    uint8_t tag;
    SynqConflictAction conflict_action;
    uint32_t table;
    uint32_t columns;
    uint32_t source;
} SynqInsertStmt;

typedef struct SynqQualifiedName {
    uint8_t tag;
    SynqSourceSpan object_name;
    SynqSourceSpan schema;
} SynqQualifiedName;

typedef struct SynqDropStmt {
    uint8_t tag;
    SynqDropObjectType object_type;
    SynqBool if_exists;
    uint32_t target;
} SynqDropStmt;

typedef struct SynqAlterTableStmt {
    uint8_t tag;
    SynqAlterOp op;
    uint32_t target;
    SynqSourceSpan new_name;
    SynqSourceSpan old_name;
} SynqAlterTableStmt;

typedef struct SynqTransactionStmt {
    uint8_t tag;
    SynqTransactionOp op;
    SynqTransactionType trans_type;
} SynqTransactionStmt;

typedef struct SynqSavepointStmt {
    uint8_t tag;
    SynqSavepointOp op;
    SynqSourceSpan savepoint_name;
} SynqSavepointStmt;

typedef struct SynqPragmaStmt {
    uint8_t tag;
    SynqSourceSpan pragma_name;
    SynqSourceSpan schema;
    SynqSourceSpan value;
    SynqPragmaForm pragma_form;
} SynqPragmaStmt;

typedef struct SynqAnalyzeStmt {
    uint8_t tag;
    SynqSourceSpan target_name;
    SynqSourceSpan schema;
    SynqAnalyzeKind kind;
} SynqAnalyzeStmt;

typedef struct SynqAttachStmt {
    uint8_t tag;
    uint32_t filename;
    uint32_t db_name;
    uint32_t key;
} SynqAttachStmt;

typedef struct SynqDetachStmt {
    uint8_t tag;
    uint32_t db_name;
} SynqDetachStmt;

typedef struct SynqVacuumStmt {
    uint8_t tag;
    SynqSourceSpan schema;
    uint32_t into_expr;
} SynqVacuumStmt;

typedef struct SynqExplainStmt {
    uint8_t tag;
    SynqExplainMode explain_mode;
    uint32_t stmt;
} SynqExplainStmt;

typedef struct SynqCreateIndexStmt {
    uint8_t tag;
    SynqSourceSpan index_name;
    SynqSourceSpan schema;
    SynqSourceSpan table_name;
    SynqBool is_unique;
    SynqBool if_not_exists;
    uint32_t columns;
    uint32_t where;
} SynqCreateIndexStmt;

typedef struct SynqCreateViewStmt {
    uint8_t tag;
    SynqSourceSpan view_name;
    SynqSourceSpan schema;
    SynqBool is_temp;
    SynqBool if_not_exists;
    uint32_t column_names;
    uint32_t select;
} SynqCreateViewStmt;

typedef struct SynqForeignKeyClause {
    uint8_t tag;
    SynqSourceSpan ref_table;
    uint32_t ref_columns;
    SynqForeignKeyAction on_delete;
    SynqForeignKeyAction on_update;
    SynqBool is_deferred;
} SynqForeignKeyClause;

typedef struct SynqColumnConstraint {
    uint8_t tag;
    SynqColumnConstraintKind kind;
    SynqSourceSpan constraint_name;
    SynqConflictAction onconf;
    SynqSortOrder sort_order;
    SynqBool is_autoincrement;
    SynqSourceSpan collation_name;
    SynqGeneratedColumnStorage generated_storage;
    uint32_t default_expr;
    uint32_t check_expr;
    uint32_t generated_expr;
    uint32_t fk_clause;
} SynqColumnConstraint;

// List of ColumnConstraint
typedef struct SynqColumnConstraintList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqColumnConstraintList;

typedef struct SynqColumnDef {
    uint8_t tag;
    SynqSourceSpan column_name;
    SynqSourceSpan type_name;
    uint32_t constraints;
} SynqColumnDef;

// List of ColumnDef
typedef struct SynqColumnDefList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqColumnDefList;

typedef struct SynqTableConstraint {
    uint8_t tag;
    SynqTableConstraintKind kind;
    SynqSourceSpan constraint_name;
    SynqConflictAction onconf;
    SynqBool is_autoincrement;
    uint32_t columns;
    uint32_t check_expr;
    uint32_t fk_clause;
} SynqTableConstraint;

// List of TableConstraint
typedef struct SynqTableConstraintList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqTableConstraintList;

typedef struct SynqCreateTableStmt {
    uint8_t tag;
    SynqSourceSpan table_name;
    SynqSourceSpan schema;
    SynqBool is_temp;
    SynqBool if_not_exists;
    SynqCreateTableStmtFlags flags;
    uint32_t columns;
    uint32_t table_constraints;
    uint32_t as_select;
} SynqCreateTableStmt;

typedef struct SynqFrameBound {
    uint8_t tag;
    SynqFrameBoundType bound_type;
    uint32_t expr;
} SynqFrameBound;

typedef struct SynqFrameSpec {
    uint8_t tag;
    SynqFrameType frame_type;
    SynqFrameExclude exclude;
    uint32_t start_bound;
    uint32_t end_bound;
} SynqFrameSpec;

typedef struct SynqWindowDef {
    uint8_t tag;
    SynqSourceSpan base_window_name;
    uint32_t partition_by;
    uint32_t orderby;
    uint32_t frame;
} SynqWindowDef;

// List of WindowDef
typedef struct SynqWindowDefList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqWindowDefList;

typedef struct SynqNamedWindowDef {
    uint8_t tag;
    SynqSourceSpan window_name;
    uint32_t window_def;
} SynqNamedWindowDef;

// List of NamedWindowDef
typedef struct SynqNamedWindowDefList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqNamedWindowDefList;

typedef struct SynqFilterOver {
    uint8_t tag;
    uint32_t filter_expr;
    uint32_t over_def;
    SynqSourceSpan over_name;
} SynqFilterOver;

typedef struct SynqTriggerEvent {
    uint8_t tag;
    SynqTriggerEventType event_type;
    uint32_t columns;
} SynqTriggerEvent;

// List of Stmt
typedef struct SynqTriggerCmdList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SynqTriggerCmdList;

typedef struct SynqCreateTriggerStmt {
    uint8_t tag;
    SynqSourceSpan trigger_name;
    SynqSourceSpan schema;
    SynqBool is_temp;
    SynqBool if_not_exists;
    SynqTriggerTiming timing;
    uint32_t event;
    uint32_t table;
    uint32_t when_expr;
    uint32_t body;
} SynqCreateTriggerStmt;

typedef struct SynqCreateVirtualTableStmt {
    uint8_t tag;
    SynqSourceSpan table_name;
    SynqSourceSpan schema;
    SynqSourceSpan module_name;
    SynqBool if_not_exists;
    SynqSourceSpan module_args;
} SynqCreateVirtualTableStmt;

// ============ Node Union ============

typedef union SynqNode {
    uint8_t tag;
    SynqBinaryExpr binary_expr;
    SynqUnaryExpr unary_expr;
    SynqLiteral literal;
    SynqExprList expr_list;
    SynqResultColumn result_column;
    SynqResultColumnList result_column_list;
    SynqSelectStmt select_stmt;
    SynqOrderingTerm ordering_term;
    SynqOrderByList order_by_list;
    SynqLimitClause limit_clause;
    SynqColumnRef column_ref;
    SynqFunctionCall function_call;
    SynqIsExpr is_expr;
    SynqBetweenExpr between_expr;
    SynqLikeExpr like_expr;
    SynqCaseExpr case_expr;
    SynqCaseWhen case_when;
    SynqCaseWhenList case_when_list;
    SynqCompoundSelect compound_select;
    SynqSubqueryExpr subquery_expr;
    SynqExistsExpr exists_expr;
    SynqInExpr in_expr;
    SynqVariable variable;
    SynqCollateExpr collate_expr;
    SynqCastExpr cast_expr;
    SynqValuesRowList values_row_list;
    SynqValuesClause values_clause;
    SynqCteDefinition cte_definition;
    SynqCteList cte_list;
    SynqWithClause with_clause;
    SynqAggregateFunctionCall aggregate_function_call;
    SynqRaiseExpr raise_expr;
    SynqTableRef table_ref;
    SynqSubqueryTableSource subquery_table_source;
    SynqJoinClause join_clause;
    SynqJoinPrefix join_prefix;
    SynqDeleteStmt delete_stmt;
    SynqSetClause set_clause;
    SynqSetClauseList set_clause_list;
    SynqUpdateStmt update_stmt;
    SynqInsertStmt insert_stmt;
    SynqQualifiedName qualified_name;
    SynqDropStmt drop_stmt;
    SynqAlterTableStmt alter_table_stmt;
    SynqTransactionStmt transaction_stmt;
    SynqSavepointStmt savepoint_stmt;
    SynqPragmaStmt pragma_stmt;
    SynqAnalyzeStmt analyze_stmt;
    SynqAttachStmt attach_stmt;
    SynqDetachStmt detach_stmt;
    SynqVacuumStmt vacuum_stmt;
    SynqExplainStmt explain_stmt;
    SynqCreateIndexStmt create_index_stmt;
    SynqCreateViewStmt create_view_stmt;
    SynqForeignKeyClause foreign_key_clause;
    SynqColumnConstraint column_constraint;
    SynqColumnConstraintList column_constraint_list;
    SynqColumnDef column_def;
    SynqColumnDefList column_def_list;
    SynqTableConstraint table_constraint;
    SynqTableConstraintList table_constraint_list;
    SynqCreateTableStmt create_table_stmt;
    SynqFrameBound frame_bound;
    SynqFrameSpec frame_spec;
    SynqWindowDef window_def;
    SynqWindowDefList window_def_list;
    SynqNamedWindowDef named_window_def;
    SynqNamedWindowDefList named_window_def_list;
    SynqFilterOver filter_over;
    SynqTriggerEvent trigger_event;
    SynqTriggerCmdList trigger_cmd_list;
    SynqCreateTriggerStmt create_trigger_stmt;
    SynqCreateVirtualTableStmt create_virtual_table_stmt;
} SynqNode;

#ifdef __cplusplus
}
#endif

#endif /* SYNTAQLITE_CUSTOM_NODES */

#endif  // SYNTAQLITE_AST_NODES_GEN_H
