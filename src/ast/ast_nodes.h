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
} SyntaqliteBinaryOp;

static const char* const syntaqlite_literal_type_names[] = {
    "INTEGER",
    "FLOAT",
    "STRING",
    "BLOB",
    "NULL",
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
};

// ============ Node Tags ============

typedef enum {
    SYNTAQLITE_NODE_NULL = 0,
    SYNTAQLITE_NODE_BINARY_EXPR,
    SYNTAQLITE_NODE_UNARY_EXPR,
    SYNTAQLITE_NODE_LITERAL,
    SYNTAQLITE_NODE_RESULT_COLUMN,
    SYNTAQLITE_NODE_RESULT_COLUMN_LIST,
    SYNTAQLITE_NODE_SELECT_STMT,
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
    uint8_t op;
    uint32_t operand;
} SyntaqliteUnaryExpr;

typedef struct SyntaqliteLiteral {
    uint8_t tag;
    SyntaqliteLiteralType literal_type;
    SyntaqliteSourceSpan source;
} SyntaqliteLiteral;

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
} SyntaqliteSelectStmt;

// ============ Node Union ============

typedef union SyntaqliteNode {
    uint8_t tag;
    SyntaqliteBinaryExpr binary_expr;
    SyntaqliteUnaryExpr unary_expr;
    SyntaqliteLiteral literal;
    SyntaqliteResultColumn result_column;
    SyntaqliteResultColumnList result_column_list;
    SyntaqliteSelectStmt select_stmt;
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
