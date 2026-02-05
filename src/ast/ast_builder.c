// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#include "src/ast/ast_builder.h"

#include <stdlib.h>
#include <string.h>

// ============ Node Size Table ============

static const size_t node_base_sizes[] = {
    [SYNTAQLITE_NODE_NULL] = 0,
    [SYNTAQLITE_NODE_BINARY_EXPR] = sizeof(SyntaqliteBinaryExpr),
    [SYNTAQLITE_NODE_UNARY_EXPR] = sizeof(SyntaqliteUnaryExpr),
    [SYNTAQLITE_NODE_LITERAL] = sizeof(SyntaqliteLiteral),
    [SYNTAQLITE_NODE_RESULT_COLUMN] = sizeof(SyntaqliteResultColumn),
    [SYNTAQLITE_NODE_RESULT_COLUMN_LIST] = sizeof(SyntaqliteResultColumnList),
    [SYNTAQLITE_NODE_SELECT_STMT] = sizeof(SyntaqliteSelectStmt),
};

size_t syntaqlite_node_base_size(uint8_t tag) {
    if (tag >= SYNTAQLITE_NODE_COUNT) return 0;
    return node_base_sizes[tag];
}

// ============ Initialize/Free ============

void syntaqlite_ast_init(SyntaqliteAst *ast) {
    ast->arena = NULL;
    ast->arena_size = 0;
    ast->arena_capacity = 0;
    ast->offsets = NULL;
    ast->node_count = 0;
    ast->node_capacity = 0;
}

void syntaqlite_ast_free(SyntaqliteAst *ast) {
    free(ast->arena);
    free(ast->offsets);
    ast->arena = NULL;
    ast->offsets = NULL;
    ast->arena_size = 0;
    ast->arena_capacity = 0;
    ast->node_count = 0;
    ast->node_capacity = 0;
}

// ============ Generic Allocator ============

uint32_t ast_alloc(SyntaqliteAstContext *ctx, uint8_t tag, size_t size) {
    SyntaqliteAst *ast = ctx->ast;

    // Grow arena if needed
    if (ast->arena_size + size > ast->arena_capacity) {
        size_t new_capacity = ast->arena_capacity * 2 + size + 1024;
        uint8_t *new_arena = (uint8_t*)realloc(ast->arena, new_capacity);
        if (!new_arena) {
            ctx->error_code = 1;
            ctx->error_msg = "Out of memory (arena)";
            return SYNTAQLITE_NULL_NODE;
        }
        ast->arena = new_arena;
        ast->arena_capacity = (uint32_t)new_capacity;
    }

    // Grow offset table if needed
    if (ast->node_count >= ast->node_capacity) {
        size_t new_capacity = ast->node_capacity * 2 + 64;
        uint32_t *new_offsets = (uint32_t*)realloc(ast->offsets, new_capacity * sizeof(uint32_t));
        if (!new_offsets) {
            ctx->error_code = 1;
            ctx->error_msg = "Out of memory (offsets)";
            return SYNTAQLITE_NULL_NODE;
        }
        ast->offsets = new_offsets;
        ast->node_capacity = (uint32_t)new_capacity;
    }

    uint32_t node_id = ast->node_count++;
    ast->offsets[node_id] = ast->arena_size;
    ast->arena[ast->arena_size] = tag;  // Set tag byte
    ast->arena_size += (uint32_t)size;

    return node_id;
}

// ============ Builder Functions ============

uint32_t ast_binary_expr(SyntaqliteAstContext *ctx, uint8_t op, uint32_t left, uint32_t right) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_BINARY_EXPR, sizeof(SyntaqliteBinaryExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteBinaryExpr *node = (SyntaqliteBinaryExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->left = left;
    node->right = right;
    return id;
}

uint32_t ast_unary_expr(SyntaqliteAstContext *ctx, uint8_t op, uint32_t operand) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_UNARY_EXPR, sizeof(SyntaqliteUnaryExpr));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteUnaryExpr *node = (SyntaqliteUnaryExpr*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->op = op;
    node->operand = operand;
    return id;
}

uint32_t ast_literal(
    SyntaqliteAstContext *ctx,
    uint8_t literal_type,
    uint32_t source_offset,
    uint16_t source_length
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_LITERAL, sizeof(SyntaqliteLiteral));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteLiteral *node = (SyntaqliteLiteral*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->literal_type = literal_type;
    node->source_offset = source_offset;
    node->source_length = source_length;
    return id;
}

uint32_t ast_result_column(
    SyntaqliteAstContext *ctx,
    uint8_t flags,
    uint32_t alias_offset,
    uint16_t alias_length,
    uint32_t expr
) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN, sizeof(SyntaqliteResultColumn));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteResultColumn *node = (SyntaqliteResultColumn*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->flags = flags;
    node->alias_offset = alias_offset;
    node->alias_length = alias_length;
    node->expr = expr;
    return id;
}

uint32_t ast_result_column_list_empty(SyntaqliteAstContext *ctx) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, sizeof(SyntaqliteResultColumnList));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteResultColumnList *node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 0;
    return id;
}

uint32_t ast_result_column_list(SyntaqliteAstContext *ctx, uint32_t first_child) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, sizeof(SyntaqliteResultColumnList) + sizeof(uint32_t));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteResultColumnList *node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->count = 1;
    node->children[0] = first_child;
    return id;
}

uint32_t ast_result_column_list_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {
    if (list_id == SYNTAQLITE_NULL_NODE) {
        return ast_result_column_list(ctx, child);
    }

    // Get current list
    SyntaqliteResultColumnList *old_node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);
    uint32_t old_count = old_node->count;

    // Allocate new list with space for one more child
    size_t new_size = sizeof(SyntaqliteResultColumnList) + (old_count + 1) * sizeof(uint32_t);
    uint32_t new_id = ast_alloc(ctx, SYNTAQLITE_NODE_RESULT_COLUMN_LIST, new_size);
    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;

    // Re-fetch old node pointer (may have moved due to realloc)
    old_node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[list_id]);

    SyntaqliteResultColumnList *new_node = (SyntaqliteResultColumnList*)
        (ctx->ast->arena + ctx->ast->offsets[new_id]);

    // Copy old children and add new one
    new_node->count = old_count + 1;
    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));
    new_node->children[old_count] = child;

    return new_id;
}

uint32_t ast_select_stmt(SyntaqliteAstContext *ctx, uint8_t flags, uint32_t columns) {
    uint32_t id = ast_alloc(ctx, SYNTAQLITE_NODE_SELECT_STMT, sizeof(SyntaqliteSelectStmt));
    if (id == SYNTAQLITE_NULL_NODE) return id;

    SyntaqliteSelectStmt *node = (SyntaqliteSelectStmt*)
        (ctx->ast->arena + ctx->ast->offsets[id]);
    node->flags = flags;
    node->columns = columns;
    return id;
}

