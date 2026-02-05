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

static const char* const syntaqlite_literal_type_names[] = {
    "INTEGER",
    "FLOAT",
    "STRING",
    "BLOB",
    "NULL",
    "CURRENT",
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
    uint8_t flags;
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
    uint8_t flags;
    uint32_t columns;
    uint32_t where;
    uint32_t groupby;
    uint32_t having;
    uint32_t orderby;
    uint32_t limit_clause;
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
    uint8_t flags;
    uint32_t args;
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
