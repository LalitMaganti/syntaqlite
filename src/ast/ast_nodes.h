// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/extract_sqlite.py

#ifndef SYNTAQLITE_AST_NODES_H
#define SYNTAQLITE_AST_NODES_H

// Base types (SyntaqliteSourceSpan, SyntaqliteAst, etc.)
#include "src/ast/ast_base.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============ Value Enums ============

typedef enum {
    SYNTAQLITE_LITERAL_TYPE_INTEGER = 0,
    SYNTAQLITE_LITERAL_TYPE_FLOAT = 1,
    SYNTAQLITE_LITERAL_TYPE_STRING = 2,
    SYNTAQLITE_LITERAL_TYPE_BLOB = 3,
    SYNTAQLITE_LITERAL_TYPE_NULL = 4,
    SYNTAQLITE_LITERAL_TYPE_CURRENT = 5,
    SYNTAQLITE_LITERAL_TYPE_QNUMBER = 6,
} SyntaqliteLiteralType;

typedef enum {
    SYNTAQLITE_BINARY_OP_PLUS = 0,
    SYNTAQLITE_BINARY_OP_MINUS = 1,
    SYNTAQLITE_BINARY_OP_STAR = 2,
    SYNTAQLITE_BINARY_OP_SLASH = 3,
    SYNTAQLITE_BINARY_OP_REM = 4,
    SYNTAQLITE_BINARY_OP_LT = 5,
    SYNTAQLITE_BINARY_OP_GT = 6,
    SYNTAQLITE_BINARY_OP_LE = 7,
    SYNTAQLITE_BINARY_OP_GE = 8,
    SYNTAQLITE_BINARY_OP_EQ = 9,
    SYNTAQLITE_BINARY_OP_NE = 10,
    SYNTAQLITE_BINARY_OP_AND = 11,
    SYNTAQLITE_BINARY_OP_OR = 12,
    SYNTAQLITE_BINARY_OP_BITAND = 13,
    SYNTAQLITE_BINARY_OP_BITOR = 14,
    SYNTAQLITE_BINARY_OP_LSHIFT = 15,
    SYNTAQLITE_BINARY_OP_RSHIFT = 16,
    SYNTAQLITE_BINARY_OP_CONCAT = 17,
    SYNTAQLITE_BINARY_OP_PTR = 18,
} SyntaqliteBinaryOp;

typedef enum {
    SYNTAQLITE_UNARY_OP_MINUS = 0,
    SYNTAQLITE_UNARY_OP_PLUS = 1,
    SYNTAQLITE_UNARY_OP_BITNOT = 2,
    SYNTAQLITE_UNARY_OP_NOT = 3,
} SyntaqliteUnaryOp;

typedef enum {
    SYNTAQLITE_SORT_ORDER_ASC = 0,
    SYNTAQLITE_SORT_ORDER_DESC = 1,
} SyntaqliteSortOrder;

typedef enum {
    SYNTAQLITE_NULLS_ORDER_NONE = 0,
    SYNTAQLITE_NULLS_ORDER_FIRST = 1,
    SYNTAQLITE_NULLS_ORDER_LAST = 2,
} SyntaqliteNullsOrder;

typedef enum {
    SYNTAQLITE_IS_OP_IS = 0,
    SYNTAQLITE_IS_OP_IS_NOT = 1,
    SYNTAQLITE_IS_OP_ISNULL = 2,
    SYNTAQLITE_IS_OP_NOTNULL = 3,
    SYNTAQLITE_IS_OP_IS_NOT_DISTINCT = 4,
    SYNTAQLITE_IS_OP_IS_DISTINCT = 5,
} SyntaqliteIsOp;

typedef enum {
    SYNTAQLITE_COMPOUND_OP_UNION = 0,
    SYNTAQLITE_COMPOUND_OP_UNION_ALL = 1,
    SYNTAQLITE_COMPOUND_OP_INTERSECT = 2,
    SYNTAQLITE_COMPOUND_OP_EXCEPT = 3,
} SyntaqliteCompoundOp;

typedef enum {
    SYNTAQLITE_RAISE_TYPE_IGNORE = 0,
    SYNTAQLITE_RAISE_TYPE_ROLLBACK = 1,
    SYNTAQLITE_RAISE_TYPE_ABORT = 2,
    SYNTAQLITE_RAISE_TYPE_FAIL = 3,
} SyntaqliteRaiseType;

typedef enum {
    SYNTAQLITE_JOIN_TYPE_COMMA = 0,
    SYNTAQLITE_JOIN_TYPE_INNER = 1,
    SYNTAQLITE_JOIN_TYPE_LEFT = 2,
    SYNTAQLITE_JOIN_TYPE_RIGHT = 3,
    SYNTAQLITE_JOIN_TYPE_FULL = 4,
    SYNTAQLITE_JOIN_TYPE_CROSS = 5,
    SYNTAQLITE_JOIN_TYPE_NATURAL_INNER = 6,
    SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT = 7,
    SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT = 8,
    SYNTAQLITE_JOIN_TYPE_NATURAL_FULL = 9,
} SyntaqliteJoinType;

typedef enum {
    SYNTAQLITE_CONFLICT_ACTION_DEFAULT = 0,
    SYNTAQLITE_CONFLICT_ACTION_ROLLBACK = 1,
    SYNTAQLITE_CONFLICT_ACTION_ABORT = 2,
    SYNTAQLITE_CONFLICT_ACTION_FAIL = 3,
    SYNTAQLITE_CONFLICT_ACTION_IGNORE = 4,
    SYNTAQLITE_CONFLICT_ACTION_REPLACE = 5,
} SyntaqliteConflictAction;

typedef enum {
    SYNTAQLITE_DROP_OBJECT_TYPE_TABLE = 0,
    SYNTAQLITE_DROP_OBJECT_TYPE_INDEX = 1,
    SYNTAQLITE_DROP_OBJECT_TYPE_VIEW = 2,
    SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER = 3,
} SyntaqliteDropObjectType;

typedef enum {
    SYNTAQLITE_ALTER_OP_RENAME_TABLE = 0,
    SYNTAQLITE_ALTER_OP_RENAME_COLUMN = 1,
    SYNTAQLITE_ALTER_OP_DROP_COLUMN = 2,
    SYNTAQLITE_ALTER_OP_ADD_COLUMN = 3,
} SyntaqliteAlterOp;

typedef enum {
    SYNTAQLITE_TRANSACTION_TYPE_DEFERRED = 0,
    SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE = 1,
    SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE = 2,
} SyntaqliteTransactionType;

typedef enum {
    SYNTAQLITE_TRANSACTION_OP_BEGIN = 0,
    SYNTAQLITE_TRANSACTION_OP_COMMIT = 1,
    SYNTAQLITE_TRANSACTION_OP_ROLLBACK = 2,
} SyntaqliteTransactionOp;

typedef enum {
    SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT = 0,
    SYNTAQLITE_SAVEPOINT_OP_RELEASE = 1,
    SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO = 2,
} SyntaqliteSavepointOp;

typedef enum {
    SYNTAQLITE_EXPLAIN_MODE_EXPLAIN = 0,
    SYNTAQLITE_EXPLAIN_MODE_QUERY_PLAN = 1,
} SyntaqliteExplainMode;

typedef enum {
    SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION = 0,
    SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL = 1,
    SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT = 2,
    SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE = 3,
    SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT = 4,
} SyntaqliteForeignKeyAction;

typedef enum {
    SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL = 0,
    SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED = 1,
} SyntaqliteGeneratedColumnStorage;

typedef enum {
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT = 0,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL = 1,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY = 2,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE = 3,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK = 4,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES = 5,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE = 6,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED = 7,
    SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL = 8,
} SyntaqliteColumnConstraintKind;

typedef enum {
    SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY = 0,
    SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE = 1,
    SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK = 2,
    SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY = 3,
} SyntaqliteTableConstraintKind;

typedef enum {
    SYNTAQLITE_FRAME_TYPE_NONE = 0,
    SYNTAQLITE_FRAME_TYPE_RANGE = 1,
    SYNTAQLITE_FRAME_TYPE_ROWS = 2,
    SYNTAQLITE_FRAME_TYPE_GROUPS = 3,
} SyntaqliteFrameType;

typedef enum {
    SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING = 0,
    SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING = 1,
    SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW = 2,
    SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING = 3,
    SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING = 4,
} SyntaqliteFrameBoundType;

typedef enum {
    SYNTAQLITE_FRAME_EXCLUDE_NONE = 0,
    SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS = 1,
    SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW = 2,
    SYNTAQLITE_FRAME_EXCLUDE_GROUP = 3,
    SYNTAQLITE_FRAME_EXCLUDE_TIES = 4,
} SyntaqliteFrameExclude;

typedef enum {
    SYNTAQLITE_TRIGGER_TIMING_BEFORE = 0,
    SYNTAQLITE_TRIGGER_TIMING_AFTER = 1,
    SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF = 2,
} SyntaqliteTriggerTiming;

typedef enum {
    SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE = 0,
    SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT = 1,
    SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE = 2,
} SyntaqliteTriggerEventType;

static const char* const syntaqlite_literal_type_names[] = {
    "INTEGER",
    "FLOAT",
    "STRING",
    "BLOB",
    "NULL",
    "CURRENT",
    "QNUMBER",
};

static const char* const syntaqlite_binary_op_names[] = {
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

static const char* const syntaqlite_unary_op_names[] = {
    "MINUS",
    "PLUS",
    "BITNOT",
    "NOT",
};

static const char* const syntaqlite_sort_order_names[] = {
    "ASC",
    "DESC",
};

static const char* const syntaqlite_nulls_order_names[] = {
    "NONE",
    "FIRST",
    "LAST",
};

static const char* const syntaqlite_is_op_names[] = {
    "IS",
    "IS_NOT",
    "ISNULL",
    "NOTNULL",
    "IS_NOT_DISTINCT",
    "IS_DISTINCT",
};

static const char* const syntaqlite_compound_op_names[] = {
    "UNION",
    "UNION_ALL",
    "INTERSECT",
    "EXCEPT",
};

static const char* const syntaqlite_raise_type_names[] = {
    "IGNORE",
    "ROLLBACK",
    "ABORT",
    "FAIL",
};

static const char* const syntaqlite_join_type_names[] = {
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

static const char* const syntaqlite_conflict_action_names[] = {
    "DEFAULT",
    "ROLLBACK",
    "ABORT",
    "FAIL",
    "IGNORE",
    "REPLACE",
};

static const char* const syntaqlite_drop_object_type_names[] = {
    "TABLE",
    "INDEX",
    "VIEW",
    "TRIGGER",
};

static const char* const syntaqlite_alter_op_names[] = {
    "RENAME_TABLE",
    "RENAME_COLUMN",
    "DROP_COLUMN",
    "ADD_COLUMN",
};

static const char* const syntaqlite_transaction_type_names[] = {
    "DEFERRED",
    "IMMEDIATE",
    "EXCLUSIVE",
};

static const char* const syntaqlite_transaction_op_names[] = {
    "BEGIN",
    "COMMIT",
    "ROLLBACK",
};

static const char* const syntaqlite_savepoint_op_names[] = {
    "SAVEPOINT",
    "RELEASE",
    "ROLLBACK_TO",
};

static const char* const syntaqlite_explain_mode_names[] = {
    "EXPLAIN",
    "QUERY_PLAN",
};

static const char* const syntaqlite_foreign_key_action_names[] = {
    "NO_ACTION",
    "SET_NULL",
    "SET_DEFAULT",
    "CASCADE",
    "RESTRICT",
};

static const char* const syntaqlite_generated_column_storage_names[] = {
    "VIRTUAL",
    "STORED",
};

static const char* const syntaqlite_column_constraint_kind_names[] = {
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

static const char* const syntaqlite_table_constraint_kind_names[] = {
    "PRIMARY_KEY",
    "UNIQUE",
    "CHECK",
    "FOREIGN_KEY",
};

static const char* const syntaqlite_frame_type_names[] = {
    "NONE",
    "RANGE",
    "ROWS",
    "GROUPS",
};

static const char* const syntaqlite_frame_bound_type_names[] = {
    "UNBOUNDED_PRECEDING",
    "EXPR_PRECEDING",
    "CURRENT_ROW",
    "EXPR_FOLLOWING",
    "UNBOUNDED_FOLLOWING",
};

static const char* const syntaqlite_frame_exclude_names[] = {
    "NONE",
    "NO_OTHERS",
    "CURRENT_ROW",
    "GROUP",
    "TIES",
};

static const char* const syntaqlite_trigger_timing_names[] = {
    "BEFORE",
    "AFTER",
    "INSTEAD_OF",
};

static const char* const syntaqlite_trigger_event_type_names[] = {
    "DELETE",
    "INSERT",
    "UPDATE",
};

// ============ Flags Types ============

typedef union SyntaqliteResultColumnFlags {
    uint8_t raw;
    struct {
        uint8_t star : 1;
    };
} SyntaqliteResultColumnFlags;

typedef union SyntaqliteSelectStmtFlags {
    uint8_t raw;
    struct {
        uint8_t distinct : 1;
    };
} SyntaqliteSelectStmtFlags;

typedef union SyntaqliteFunctionCallFlags {
    uint8_t raw;
    struct {
        uint8_t distinct : 1;
        uint8_t star : 1;
    };
} SyntaqliteFunctionCallFlags;

typedef union SyntaqliteAggregateFunctionCallFlags {
    uint8_t raw;
    struct {
        uint8_t distinct : 1;
    };
} SyntaqliteAggregateFunctionCallFlags;

// ============ Node Tags ============

typedef enum {
    SYNTAQLITE_NODE_NULL = 0,
    SYNTAQLITE_NODE_BINARY_EXPR,
    SYNTAQLITE_NODE_UNARY_EXPR,
    SYNTAQLITE_NODE_LITERAL,
    SYNTAQLITE_NODE_EXPR_LIST,
    SYNTAQLITE_NODE_RESULT_COLUMN,
    SYNTAQLITE_NODE_RESULT_COLUMN_LIST,
    SYNTAQLITE_NODE_SELECT_STMT,
    SYNTAQLITE_NODE_ORDERING_TERM,
    SYNTAQLITE_NODE_ORDER_BY_LIST,
    SYNTAQLITE_NODE_LIMIT_CLAUSE,
    SYNTAQLITE_NODE_COLUMN_REF,
    SYNTAQLITE_NODE_FUNCTION_CALL,
    SYNTAQLITE_NODE_IS_EXPR,
    SYNTAQLITE_NODE_BETWEEN_EXPR,
    SYNTAQLITE_NODE_LIKE_EXPR,
    SYNTAQLITE_NODE_CASE_EXPR,
    SYNTAQLITE_NODE_CASE_WHEN,
    SYNTAQLITE_NODE_CASE_WHEN_LIST,
    SYNTAQLITE_NODE_COMPOUND_SELECT,
    SYNTAQLITE_NODE_SUBQUERY_EXPR,
    SYNTAQLITE_NODE_EXISTS_EXPR,
    SYNTAQLITE_NODE_IN_EXPR,
    SYNTAQLITE_NODE_VARIABLE,
    SYNTAQLITE_NODE_COLLATE_EXPR,
    SYNTAQLITE_NODE_CAST_EXPR,
    SYNTAQLITE_NODE_VALUES_ROW_LIST,
    SYNTAQLITE_NODE_VALUES_CLAUSE,
    SYNTAQLITE_NODE_CTE_DEFINITION,
    SYNTAQLITE_NODE_CTE_LIST,
    SYNTAQLITE_NODE_WITH_CLAUSE,
    SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL,
    SYNTAQLITE_NODE_RAISE_EXPR,
    SYNTAQLITE_NODE_TABLE_REF,
    SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE,
    SYNTAQLITE_NODE_JOIN_CLAUSE,
    SYNTAQLITE_NODE_JOIN_PREFIX,
    SYNTAQLITE_NODE_DELETE_STMT,
    SYNTAQLITE_NODE_SET_CLAUSE,
    SYNTAQLITE_NODE_SET_CLAUSE_LIST,
    SYNTAQLITE_NODE_UPDATE_STMT,
    SYNTAQLITE_NODE_INSERT_STMT,
    SYNTAQLITE_NODE_QUALIFIED_NAME,
    SYNTAQLITE_NODE_DROP_STMT,
    SYNTAQLITE_NODE_ALTER_TABLE_STMT,
    SYNTAQLITE_NODE_TRANSACTION_STMT,
    SYNTAQLITE_NODE_SAVEPOINT_STMT,
    SYNTAQLITE_NODE_PRAGMA_STMT,
    SYNTAQLITE_NODE_ANALYZE_STMT,
    SYNTAQLITE_NODE_ATTACH_STMT,
    SYNTAQLITE_NODE_DETACH_STMT,
    SYNTAQLITE_NODE_VACUUM_STMT,
    SYNTAQLITE_NODE_EXPLAIN_STMT,
    SYNTAQLITE_NODE_CREATE_INDEX_STMT,
    SYNTAQLITE_NODE_CREATE_VIEW_STMT,
    SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE,
    SYNTAQLITE_NODE_COLUMN_CONSTRAINT,
    SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST,
    SYNTAQLITE_NODE_COLUMN_DEF,
    SYNTAQLITE_NODE_COLUMN_DEF_LIST,
    SYNTAQLITE_NODE_TABLE_CONSTRAINT,
    SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST,
    SYNTAQLITE_NODE_CREATE_TABLE_STMT,
    SYNTAQLITE_NODE_FRAME_BOUND,
    SYNTAQLITE_NODE_FRAME_SPEC,
    SYNTAQLITE_NODE_WINDOW_DEF,
    SYNTAQLITE_NODE_WINDOW_DEF_LIST,
    SYNTAQLITE_NODE_NAMED_WINDOW_DEF,
    SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST,
    SYNTAQLITE_NODE_FILTER_OVER,
    SYNTAQLITE_NODE_TRIGGER_EVENT,
    SYNTAQLITE_NODE_TRIGGER_CMD_LIST,
    SYNTAQLITE_NODE_CREATE_TRIGGER_STMT,
    SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT,
    SYNTAQLITE_NODE_COUNT
} SyntaqliteNodeTag;

// ============ Node Structs (variable sizes) ============

typedef struct SyntaqliteBinaryExpr {
    uint8_t tag;
    SyntaqliteBinaryOp op;
    uint32_t left;
    uint32_t right;
} SyntaqliteBinaryExpr;

typedef struct SyntaqliteUnaryExpr {
    uint8_t tag;
    SyntaqliteUnaryOp op;
    uint32_t operand;
} SyntaqliteUnaryExpr;

typedef struct SyntaqliteLiteral {
    uint8_t tag;
    SyntaqliteLiteralType literal_type;
    SyntaqliteSourceSpan source;
} SyntaqliteLiteral;

// List of Expr
typedef struct SyntaqliteExprList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteExprList;

typedef struct SyntaqliteResultColumn {
    uint8_t tag;
    SyntaqliteResultColumnFlags flags;
    SyntaqliteSourceSpan alias;
    uint32_t expr;
} SyntaqliteResultColumn;

// List of ResultColumn
typedef struct SyntaqliteResultColumnList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteResultColumnList;

typedef struct SyntaqliteSelectStmt {
    uint8_t tag;
    SyntaqliteSelectStmtFlags flags;
    uint32_t columns;
    uint32_t from_clause;
    uint32_t where;
    uint32_t groupby;
    uint32_t having;
    uint32_t orderby;
    uint32_t limit_clause;
    uint32_t window_clause;
} SyntaqliteSelectStmt;

typedef struct SyntaqliteOrderingTerm {
    uint8_t tag;
    uint32_t expr;
    SyntaqliteSortOrder sort_order;
    SyntaqliteNullsOrder nulls_order;
} SyntaqliteOrderingTerm;

// List of OrderingTerm
typedef struct SyntaqliteOrderByList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteOrderByList;

typedef struct SyntaqliteLimitClause {
    uint8_t tag;
    uint32_t limit;
    uint32_t offset;
} SyntaqliteLimitClause;

typedef struct SyntaqliteColumnRef {
    uint8_t tag;
    SyntaqliteSourceSpan column;
    SyntaqliteSourceSpan table;
    SyntaqliteSourceSpan schema;
} SyntaqliteColumnRef;

typedef struct SyntaqliteFunctionCall {
    uint8_t tag;
    SyntaqliteSourceSpan func_name;
    SyntaqliteFunctionCallFlags flags;
    uint32_t args;
    uint32_t filter_clause;
    uint32_t over_clause;
} SyntaqliteFunctionCall;

typedef struct SyntaqliteIsExpr {
    uint8_t tag;
    SyntaqliteIsOp op;
    uint32_t left;
    uint32_t right;
} SyntaqliteIsExpr;

typedef struct SyntaqliteBetweenExpr {
    uint8_t tag;
    uint8_t negated;
    uint32_t operand;
    uint32_t low;
    uint32_t high;
} SyntaqliteBetweenExpr;

typedef struct SyntaqliteLikeExpr {
    uint8_t tag;
    uint8_t negated;
    uint32_t operand;
    uint32_t pattern;
    uint32_t escape;
} SyntaqliteLikeExpr;

typedef struct SyntaqliteCaseExpr {
    uint8_t tag;
    uint32_t operand;
    uint32_t else_expr;
    uint32_t whens;
} SyntaqliteCaseExpr;

typedef struct SyntaqliteCaseWhen {
    uint8_t tag;
    uint32_t when_expr;
    uint32_t then_expr;
} SyntaqliteCaseWhen;

// List of CaseWhen
typedef struct SyntaqliteCaseWhenList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteCaseWhenList;

typedef struct SyntaqliteCompoundSelect {
    uint8_t tag;
    SyntaqliteCompoundOp op;
    uint32_t left;
    uint32_t right;
} SyntaqliteCompoundSelect;

typedef struct SyntaqliteSubqueryExpr {
    uint8_t tag;
    uint32_t select;
} SyntaqliteSubqueryExpr;

typedef struct SyntaqliteExistsExpr {
    uint8_t tag;
    uint32_t select;
} SyntaqliteExistsExpr;

typedef struct SyntaqliteInExpr {
    uint8_t tag;
    uint8_t negated;
    uint32_t operand;
    uint32_t source;
} SyntaqliteInExpr;

typedef struct SyntaqliteVariable {
    uint8_t tag;
    SyntaqliteSourceSpan source;
} SyntaqliteVariable;

typedef struct SyntaqliteCollateExpr {
    uint8_t tag;
    uint32_t expr;
    SyntaqliteSourceSpan collation;
} SyntaqliteCollateExpr;

typedef struct SyntaqliteCastExpr {
    uint8_t tag;
    uint32_t expr;
    SyntaqliteSourceSpan type_name;
} SyntaqliteCastExpr;

// List of ExprList
typedef struct SyntaqliteValuesRowList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteValuesRowList;

typedef struct SyntaqliteValuesClause {
    uint8_t tag;
    uint32_t rows;
} SyntaqliteValuesClause;

typedef struct SyntaqliteCteDefinition {
    uint8_t tag;
    SyntaqliteSourceSpan cte_name;
    uint8_t materialized;
    uint32_t columns;
    uint32_t select;
} SyntaqliteCteDefinition;

// List of CteDefinition
typedef struct SyntaqliteCteList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteCteList;

typedef struct SyntaqliteWithClause {
    uint8_t tag;
    uint8_t recursive;
    uint32_t ctes;
    uint32_t select;
} SyntaqliteWithClause;

typedef struct SyntaqliteAggregateFunctionCall {
    uint8_t tag;
    SyntaqliteSourceSpan func_name;
    SyntaqliteAggregateFunctionCallFlags flags;
    uint32_t args;
    uint32_t orderby;
    uint32_t filter_clause;
    uint32_t over_clause;
} SyntaqliteAggregateFunctionCall;

typedef struct SyntaqliteRaiseExpr {
    uint8_t tag;
    SyntaqliteRaiseType raise_type;
    uint32_t error_message;
} SyntaqliteRaiseExpr;

typedef struct SyntaqliteTableRef {
    uint8_t tag;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    SyntaqliteSourceSpan alias;
} SyntaqliteTableRef;

typedef struct SyntaqliteSubqueryTableSource {
    uint8_t tag;
    uint32_t select;
    SyntaqliteSourceSpan alias;
} SyntaqliteSubqueryTableSource;

typedef struct SyntaqliteJoinClause {
    uint8_t tag;
    SyntaqliteJoinType join_type;
    uint32_t left;
    uint32_t right;
    uint32_t on_expr;
    uint32_t using_columns;
} SyntaqliteJoinClause;

typedef struct SyntaqliteJoinPrefix {
    uint8_t tag;
    uint32_t source;
    SyntaqliteJoinType join_type;
} SyntaqliteJoinPrefix;

typedef struct SyntaqliteDeleteStmt {
    uint8_t tag;
    uint32_t table;
    uint32_t where;
} SyntaqliteDeleteStmt;

typedef struct SyntaqliteSetClause {
    uint8_t tag;
    SyntaqliteSourceSpan column;
    uint32_t columns;
    uint32_t value;
} SyntaqliteSetClause;

// List of SetClause
typedef struct SyntaqliteSetClauseList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteSetClauseList;

typedef struct SyntaqliteUpdateStmt {
    uint8_t tag;
    SyntaqliteConflictAction conflict_action;
    uint32_t table;
    uint32_t setlist;
    uint32_t from_clause;
    uint32_t where;
} SyntaqliteUpdateStmt;

typedef struct SyntaqliteInsertStmt {
    uint8_t tag;
    SyntaqliteConflictAction conflict_action;
    uint32_t table;
    uint32_t columns;
    uint32_t source;
} SyntaqliteInsertStmt;

typedef struct SyntaqliteQualifiedName {
    uint8_t tag;
    SyntaqliteSourceSpan object_name;
    SyntaqliteSourceSpan schema;
} SyntaqliteQualifiedName;

typedef struct SyntaqliteDropStmt {
    uint8_t tag;
    SyntaqliteDropObjectType object_type;
    uint8_t if_exists;
    uint32_t target;
} SyntaqliteDropStmt;

typedef struct SyntaqliteAlterTableStmt {
    uint8_t tag;
    SyntaqliteAlterOp op;
    uint32_t target;
    SyntaqliteSourceSpan new_name;
    SyntaqliteSourceSpan old_name;
} SyntaqliteAlterTableStmt;

typedef struct SyntaqliteTransactionStmt {
    uint8_t tag;
    SyntaqliteTransactionOp op;
    SyntaqliteTransactionType trans_type;
} SyntaqliteTransactionStmt;

typedef struct SyntaqliteSavepointStmt {
    uint8_t tag;
    SyntaqliteSavepointOp op;
    SyntaqliteSourceSpan savepoint_name;
} SyntaqliteSavepointStmt;

typedef struct SyntaqlitePragmaStmt {
    uint8_t tag;
    SyntaqliteSourceSpan pragma_name;
    SyntaqliteSourceSpan schema;
    SyntaqliteSourceSpan value;
    uint8_t pragma_form;
} SyntaqlitePragmaStmt;

typedef struct SyntaqliteAnalyzeStmt {
    uint8_t tag;
    SyntaqliteSourceSpan target_name;
    SyntaqliteSourceSpan schema;
    uint8_t is_reindex;
} SyntaqliteAnalyzeStmt;

typedef struct SyntaqliteAttachStmt {
    uint8_t tag;
    uint32_t filename;
    uint32_t db_name;
    uint32_t key;
} SyntaqliteAttachStmt;

typedef struct SyntaqliteDetachStmt {
    uint8_t tag;
    uint32_t db_name;
} SyntaqliteDetachStmt;

typedef struct SyntaqliteVacuumStmt {
    uint8_t tag;
    SyntaqliteSourceSpan schema;
    uint32_t into_expr;
} SyntaqliteVacuumStmt;

typedef struct SyntaqliteExplainStmt {
    uint8_t tag;
    SyntaqliteExplainMode explain_mode;
    uint32_t stmt;
} SyntaqliteExplainStmt;

typedef struct SyntaqliteCreateIndexStmt {
    uint8_t tag;
    SyntaqliteSourceSpan index_name;
    SyntaqliteSourceSpan schema;
    SyntaqliteSourceSpan table_name;
    uint8_t is_unique;
    uint8_t if_not_exists;
    uint32_t columns;
    uint32_t where;
} SyntaqliteCreateIndexStmt;

typedef struct SyntaqliteCreateViewStmt {
    uint8_t tag;
    SyntaqliteSourceSpan view_name;
    SyntaqliteSourceSpan schema;
    uint8_t is_temp;
    uint8_t if_not_exists;
    uint32_t column_names;
    uint32_t select;
} SyntaqliteCreateViewStmt;

typedef struct SyntaqliteForeignKeyClause {
    uint8_t tag;
    SyntaqliteSourceSpan ref_table;
    uint32_t ref_columns;
    SyntaqliteForeignKeyAction on_delete;
    SyntaqliteForeignKeyAction on_update;
    uint8_t is_deferred;
} SyntaqliteForeignKeyClause;

typedef struct SyntaqliteColumnConstraint {
    uint8_t tag;
    SyntaqliteColumnConstraintKind kind;
    SyntaqliteSourceSpan constraint_name;
    uint8_t onconf;
    uint8_t sort_order;
    uint8_t is_autoincrement;
    SyntaqliteSourceSpan collation_name;
    SyntaqliteGeneratedColumnStorage generated_storage;
    uint32_t default_expr;
    uint32_t check_expr;
    uint32_t generated_expr;
    uint32_t fk_clause;
} SyntaqliteColumnConstraint;

// List of ColumnConstraint
typedef struct SyntaqliteColumnConstraintList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteColumnConstraintList;

typedef struct SyntaqliteColumnDef {
    uint8_t tag;
    SyntaqliteSourceSpan column_name;
    SyntaqliteSourceSpan type_name;
    uint32_t constraints;
} SyntaqliteColumnDef;

// List of ColumnDef
typedef struct SyntaqliteColumnDefList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteColumnDefList;

typedef struct SyntaqliteTableConstraint {
    uint8_t tag;
    SyntaqliteTableConstraintKind kind;
    SyntaqliteSourceSpan constraint_name;
    uint8_t onconf;
    uint8_t is_autoincrement;
    uint32_t columns;
    uint32_t check_expr;
    uint32_t fk_clause;
} SyntaqliteTableConstraint;

// List of TableConstraint
typedef struct SyntaqliteTableConstraintList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteTableConstraintList;

typedef struct SyntaqliteCreateTableStmt {
    uint8_t tag;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    uint8_t is_temp;
    uint8_t if_not_exists;
    uint8_t table_options;
    uint32_t columns;
    uint32_t table_constraints;
    uint32_t as_select;
} SyntaqliteCreateTableStmt;

typedef struct SyntaqliteFrameBound {
    uint8_t tag;
    SyntaqliteFrameBoundType bound_type;
    uint32_t expr;
} SyntaqliteFrameBound;

typedef struct SyntaqliteFrameSpec {
    uint8_t tag;
    SyntaqliteFrameType frame_type;
    SyntaqliteFrameExclude exclude;
    uint32_t start_bound;
    uint32_t end_bound;
} SyntaqliteFrameSpec;

typedef struct SyntaqliteWindowDef {
    uint8_t tag;
    SyntaqliteSourceSpan base_window_name;
    uint32_t partition_by;
    uint32_t orderby;
    uint32_t frame;
} SyntaqliteWindowDef;

// List of WindowDef
typedef struct SyntaqliteWindowDefList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteWindowDefList;

typedef struct SyntaqliteNamedWindowDef {
    uint8_t tag;
    SyntaqliteSourceSpan window_name;
    uint32_t window_def;
} SyntaqliteNamedWindowDef;

// List of NamedWindowDef
typedef struct SyntaqliteNamedWindowDefList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteNamedWindowDefList;

typedef struct SyntaqliteFilterOver {
    uint8_t tag;
    uint32_t filter_expr;
    uint32_t over_def;
    SyntaqliteSourceSpan over_name;
} SyntaqliteFilterOver;

typedef struct SyntaqliteTriggerEvent {
    uint8_t tag;
    SyntaqliteTriggerEventType event_type;
    uint32_t columns;
} SyntaqliteTriggerEvent;

// List of Stmt
typedef struct SyntaqliteTriggerCmdList {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of indices
} SyntaqliteTriggerCmdList;

typedef struct SyntaqliteCreateTriggerStmt {
    uint8_t tag;
    SyntaqliteSourceSpan trigger_name;
    SyntaqliteSourceSpan schema;
    uint8_t is_temp;
    uint8_t if_not_exists;
    SyntaqliteTriggerTiming timing;
    uint32_t event;
    uint32_t table;
    uint32_t when_expr;
    uint32_t body;
} SyntaqliteCreateTriggerStmt;

typedef struct SyntaqliteCreateVirtualTableStmt {
    uint8_t tag;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    SyntaqliteSourceSpan module_name;
    uint8_t if_not_exists;
    SyntaqliteSourceSpan module_args;
} SyntaqliteCreateVirtualTableStmt;

// ============ Node Union ============

typedef union SyntaqliteNode {
    uint8_t tag;
    SyntaqliteBinaryExpr binary_expr;
    SyntaqliteUnaryExpr unary_expr;
    SyntaqliteLiteral literal;
    SyntaqliteExprList expr_list;
    SyntaqliteResultColumn result_column;
    SyntaqliteResultColumnList result_column_list;
    SyntaqliteSelectStmt select_stmt;
    SyntaqliteOrderingTerm ordering_term;
    SyntaqliteOrderByList order_by_list;
    SyntaqliteLimitClause limit_clause;
    SyntaqliteColumnRef column_ref;
    SyntaqliteFunctionCall function_call;
    SyntaqliteIsExpr is_expr;
    SyntaqliteBetweenExpr between_expr;
    SyntaqliteLikeExpr like_expr;
    SyntaqliteCaseExpr case_expr;
    SyntaqliteCaseWhen case_when;
    SyntaqliteCaseWhenList case_when_list;
    SyntaqliteCompoundSelect compound_select;
    SyntaqliteSubqueryExpr subquery_expr;
    SyntaqliteExistsExpr exists_expr;
    SyntaqliteInExpr in_expr;
    SyntaqliteVariable variable;
    SyntaqliteCollateExpr collate_expr;
    SyntaqliteCastExpr cast_expr;
    SyntaqliteValuesRowList values_row_list;
    SyntaqliteValuesClause values_clause;
    SyntaqliteCteDefinition cte_definition;
    SyntaqliteCteList cte_list;
    SyntaqliteWithClause with_clause;
    SyntaqliteAggregateFunctionCall aggregate_function_call;
    SyntaqliteRaiseExpr raise_expr;
    SyntaqliteTableRef table_ref;
    SyntaqliteSubqueryTableSource subquery_table_source;
    SyntaqliteJoinClause join_clause;
    SyntaqliteJoinPrefix join_prefix;
    SyntaqliteDeleteStmt delete_stmt;
    SyntaqliteSetClause set_clause;
    SyntaqliteSetClauseList set_clause_list;
    SyntaqliteUpdateStmt update_stmt;
    SyntaqliteInsertStmt insert_stmt;
    SyntaqliteQualifiedName qualified_name;
    SyntaqliteDropStmt drop_stmt;
    SyntaqliteAlterTableStmt alter_table_stmt;
    SyntaqliteTransactionStmt transaction_stmt;
    SyntaqliteSavepointStmt savepoint_stmt;
    SyntaqlitePragmaStmt pragma_stmt;
    SyntaqliteAnalyzeStmt analyze_stmt;
    SyntaqliteAttachStmt attach_stmt;
    SyntaqliteDetachStmt detach_stmt;
    SyntaqliteVacuumStmt vacuum_stmt;
    SyntaqliteExplainStmt explain_stmt;
    SyntaqliteCreateIndexStmt create_index_stmt;
    SyntaqliteCreateViewStmt create_view_stmt;
    SyntaqliteForeignKeyClause foreign_key_clause;
    SyntaqliteColumnConstraint column_constraint;
    SyntaqliteColumnConstraintList column_constraint_list;
    SyntaqliteColumnDef column_def;
    SyntaqliteColumnDefList column_def_list;
    SyntaqliteTableConstraint table_constraint;
    SyntaqliteTableConstraintList table_constraint_list;
    SyntaqliteCreateTableStmt create_table_stmt;
    SyntaqliteFrameBound frame_bound;
    SyntaqliteFrameSpec frame_spec;
    SyntaqliteWindowDef window_def;
    SyntaqliteWindowDefList window_def_list;
    SyntaqliteNamedWindowDef named_window_def;
    SyntaqliteNamedWindowDefList named_window_def_list;
    SyntaqliteFilterOver filter_over;
    SyntaqliteTriggerEvent trigger_event;
    SyntaqliteTriggerCmdList trigger_cmd_list;
    SyntaqliteCreateTriggerStmt create_trigger_stmt;
    SyntaqliteCreateVirtualTableStmt create_virtual_table_stmt;
} SyntaqliteNode;

// Access node by ID
static inline SyntaqliteNode* syntaqlite_ast_node(SyntaqliteAst *ast, uint32_t id) {
    if (id == SYNTAQLITE_NULL_NODE) return NULL;
    return (SyntaqliteNode*)(ast->arena + ast->offsets[id]);
}

#define AST_NODE(ast, id) syntaqlite_ast_node(ast, id)

// ============ Node Size Table ============

// Returns the fixed size of a node type (0 for variable-size nodes like lists)
size_t syntaqlite_node_base_size(uint8_t tag);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_AST_NODES_H
