// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#ifndef SYNTAQLITE_AST_NODES_H
#define SYNTAQLITE_AST_NODES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Null node ID (used for nullable fields)
#define SYNTAQLITE_NULL_NODE 0xFFFFFFFFu

// ============ Helper Types ============

// Source location span (offset + length into source text)
typedef struct SyntaqliteSourceSpan {
    uint32_t offset;
    uint16_t length;
} SyntaqliteSourceSpan;

// Create a source span from offset and length
static inline SyntaqliteSourceSpan syntaqlite_span(uint32_t offset, uint16_t length) {
    SyntaqliteSourceSpan span = {offset, length};
    return span;
}

// Empty source span (no location)
#define SYNTAQLITE_NO_SPAN ((SyntaqliteSourceSpan){0, 0})

// ============ Value Enums ============

typedef enum {
    SYNTAQLITE_LITERAL_TYPE_INTEGER = 0,
    SYNTAQLITE_LITERAL_TYPE_FLOAT = 1,
    SYNTAQLITE_LITERAL_TYPE_STRING = 2,
    SYNTAQLITE_LITERAL_TYPE_BLOB = 3,
    SYNTAQLITE_LITERAL_TYPE_NULL = 4,
} SyntaqliteLiteralType;

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
    uint8_t op;  // inline
    uint32_t left;  // index -> Expr
    uint32_t right;  // index -> Expr
} SyntaqliteBinaryExpr;

typedef struct SyntaqliteUnaryExpr {
    uint8_t tag;
    uint8_t op;  // inline
    uint32_t operand;  // index -> Expr
} SyntaqliteUnaryExpr;

typedef struct SyntaqliteLiteral {
    uint8_t tag;
    uint8_t literal_type;  // inline
    SyntaqliteSourceSpan source;  // inline
} SyntaqliteLiteral;

typedef struct SyntaqliteResultColumn {
    uint8_t tag;
    uint8_t flags;  // inline
    SyntaqliteSourceSpan alias;  // inline
    uint32_t expr;  // index -> Expr
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
    uint8_t flags;  // inline
    uint32_t columns;  // index -> ResultColumnList
} SyntaqliteSelectStmt;

// ============ Union for Ergonomic Access ============
// Note: This is for type-punning convenience, NOT for storage sizing

typedef union SyntaqliteNode {
    uint8_t tag;
    SyntaqliteBinaryExpr binary_expr;
    SyntaqliteUnaryExpr unary_expr;
    SyntaqliteLiteral literal;
    SyntaqliteResultColumn result_column;
    SyntaqliteResultColumnList result_column_list;
    SyntaqliteSelectStmt select_stmt;
} SyntaqliteNode;

// ============ Arena Storage ============

typedef struct SyntaqliteAst {
    uint8_t *arena;           // Packed nodes of varying sizes
    uint32_t arena_size;
    uint32_t arena_capacity;
    uint32_t *offsets;        // offsets[node_id] = byte position in arena
    uint32_t node_count;
    uint32_t node_capacity;
} SyntaqliteAst;

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
