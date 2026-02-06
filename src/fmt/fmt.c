// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from ast_codegen node definitions - DO NOT EDIT
// Regenerate with: python3 python/tools/extract_sqlite.py

#include "src/fmt/fmt.h"

#include "src/ast/ast_nodes.h"
#include "src/fmt/doc.h"
#include "src/fmt/doc_layout.h"

#include <stdlib.h>
#include <string.h>

// ============ Formatter Context ============

typedef struct {
    SyntaqliteDocContext docs;
    SyntaqliteArena *ast;
    const char *source;
    SyntaqliteTokenList *token_list;
    SyntaqliteFmtOptions *options;
} FmtCtx;

// ============ Helpers ============

static uint32_t kw(FmtCtx *ctx, const char *text) {
    return doc_text(&ctx->docs, text, (uint32_t)strlen(text));
}

static uint32_t span_text(FmtCtx *ctx, SyntaqliteSourceSpan span) {
    if (span.length == 0) return SYNTAQLITE_NULL_DOC;
    return doc_text(&ctx->docs, ctx->source + span.offset, span.length);
}

static uint32_t format_node(FmtCtx *ctx, uint32_t node_id);

// ============ Comma-Separated List ============

static uint32_t format_comma_list(FmtCtx *ctx, uint32_t *children, uint32_t count) {
    if (count == 0) return kw(ctx, "");
    uint32_t cat = doc_concat_empty(&ctx->docs);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t child_doc = format_node(ctx, children[i]);
        if (i > 0) {
            cat = doc_concat_append(&ctx->docs, cat, kw(ctx, ","));
            cat = doc_concat_append(&ctx->docs, cat, doc_line(&ctx->docs));
        }
        cat = doc_concat_append(&ctx->docs, cat, child_doc);
    }
    return cat;
}

// ============ Clause Helper ============

static uint32_t format_clause(FmtCtx *ctx, const char *keyword, uint32_t body_doc) {
    if (body_doc == SYNTAQLITE_NULL_DOC) return SYNTAQLITE_NULL_DOC;
    uint32_t inner = doc_concat_empty(&ctx->docs);
    inner = doc_concat_append(&ctx->docs, inner, doc_line(&ctx->docs));
    inner = doc_concat_append(&ctx->docs, inner, body_doc);
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, doc_line(&ctx->docs));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, keyword));
    result = doc_concat_append(&ctx->docs, result,
        doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner));
    return result;
}

// ============ Node Formatters ============

static uint32_t format_binary_expr(FmtCtx *ctx, SyntaqliteBinaryExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->left);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t kw_3 = kw(ctx, " ");
    if (kw_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_3);
    uint32_t ed_4 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_BINARY_OP_PLUS: ed_4 = kw(ctx, "+"); break;
        case SYNTAQLITE_BINARY_OP_MINUS: ed_4 = kw(ctx, "-"); break;
        case SYNTAQLITE_BINARY_OP_STAR: ed_4 = kw(ctx, "*"); break;
        case SYNTAQLITE_BINARY_OP_SLASH: ed_4 = kw(ctx, "/"); break;
        case SYNTAQLITE_BINARY_OP_REM: ed_4 = kw(ctx, "%"); break;
        case SYNTAQLITE_BINARY_OP_LT: ed_4 = kw(ctx, "<"); break;
        case SYNTAQLITE_BINARY_OP_GT: ed_4 = kw(ctx, ">"); break;
        case SYNTAQLITE_BINARY_OP_LE: ed_4 = kw(ctx, "<="); break;
        case SYNTAQLITE_BINARY_OP_GE: ed_4 = kw(ctx, ">="); break;
        case SYNTAQLITE_BINARY_OP_EQ: ed_4 = kw(ctx, "="); break;
        case SYNTAQLITE_BINARY_OP_NE: ed_4 = kw(ctx, "!="); break;
        case SYNTAQLITE_BINARY_OP_AND: ed_4 = kw(ctx, "AND"); break;
        case SYNTAQLITE_BINARY_OP_OR: ed_4 = kw(ctx, "OR"); break;
        case SYNTAQLITE_BINARY_OP_BITAND: ed_4 = kw(ctx, "&"); break;
        case SYNTAQLITE_BINARY_OP_BITOR: ed_4 = kw(ctx, "|"); break;
        case SYNTAQLITE_BINARY_OP_LSHIFT: ed_4 = kw(ctx, "<<"); break;
        case SYNTAQLITE_BINARY_OP_RSHIFT: ed_4 = kw(ctx, ">>"); break;
        case SYNTAQLITE_BINARY_OP_CONCAT: ed_4 = kw(ctx, "||"); break;
        case SYNTAQLITE_BINARY_OP_PTR: ed_4 = kw(ctx, "->"); break;
        default: break;
    }
    if (ed_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ed_4);
    uint32_t ln_5 = doc_line(&ctx->docs);
    if (ln_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ln_5);
    uint32_t ch_6 = format_node(ctx, node->right);
    if (ch_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_6);
    uint32_t grp_7 = doc_group(&ctx->docs, cat_1);
    return grp_7;
}

static uint32_t format_unary_expr(FmtCtx *ctx, SyntaqliteUnaryExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ed_2 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_UNARY_OP_MINUS: ed_2 = kw(ctx, "-"); break;
        case SYNTAQLITE_UNARY_OP_PLUS: ed_2 = kw(ctx, "+"); break;
        case SYNTAQLITE_UNARY_OP_BITNOT: ed_2 = kw(ctx, "~"); break;
        case SYNTAQLITE_UNARY_OP_NOT: ed_2 = kw(ctx, "NOT "); break;
        default: break;
    }
    if (ed_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ed_2);
    uint32_t ch_3 = format_node(ctx, node->operand);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    return cat_1;
}

static uint32_t format_literal(FmtCtx *ctx, SyntaqliteLiteral *node) {
    uint32_t sp_1 = span_text(ctx, node->source);
    return sp_1;
}

static uint32_t format_result_column(FmtCtx *ctx, SyntaqliteResultColumn *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->flags.star) {
        uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
        if (node->expr != SYNTAQLITE_NULL_NODE) {
            uint32_t cat_4 = doc_concat_empty(&ctx->docs);
            uint32_t ch_5 = format_node(ctx, node->expr);
            if (ch_5 != SYNTAQLITE_NULL_DOC)
                cat_4 = doc_concat_append(&ctx->docs, cat_4, ch_5);
            uint32_t kw_6 = kw(ctx, ".*");
            if (kw_6 != SYNTAQLITE_NULL_DOC)
                cat_4 = doc_concat_append(&ctx->docs, cat_4, kw_6);
            cond_3 = cat_4;
        } else {
            uint32_t kw_7 = kw(ctx, "*");
            cond_3 = kw_7;
        }
        cond_2 = cond_3;
    } else {
        uint32_t ch_8 = format_node(ctx, node->expr);
        cond_2 = ch_8;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t cond_9 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t cat_10 = doc_concat_empty(&ctx->docs);
        uint32_t kw_11 = kw(ctx, " AS ");
        if (kw_11 != SYNTAQLITE_NULL_DOC)
            cat_10 = doc_concat_append(&ctx->docs, cat_10, kw_11);
        uint32_t sp_12 = span_text(ctx, node->alias);
        if (sp_12 != SYNTAQLITE_NULL_DOC)
            cat_10 = doc_concat_append(&ctx->docs, cat_10, sp_12);
        cond_9 = cat_10;
    }
    if (cond_9 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_9);
    return cat_1;
}

static uint32_t format_select_stmt(FmtCtx *ctx, SyntaqliteSelectStmt *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) {
        uint32_t kw_3 = kw(ctx, "SELECT DISTINCT");
        cond_2 = kw_3;
    } else {
        uint32_t kw_4 = kw(ctx, "SELECT");
        cond_2 = kw_4;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_6 = doc_concat_empty(&ctx->docs);
        uint32_t ln_7 = doc_line(&ctx->docs);
        if (ln_7 != SYNTAQLITE_NULL_DOC)
            cat_6 = doc_concat_append(&ctx->docs, cat_6, ln_7);
        uint32_t ch_8 = format_node(ctx, node->columns);
        if (ch_8 != SYNTAQLITE_NULL_DOC)
            cat_6 = doc_concat_append(&ctx->docs, cat_6, ch_8);
        uint32_t nst_9 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_6);
        uint32_t grp_10 = doc_group(&ctx->docs, nst_9);
        cond_5 = grp_10;
    }
    if (cond_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_5);
    uint32_t cond_11 = SYNTAQLITE_NULL_DOC;
    if (node->from_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_12 = format_node(ctx, node->from_clause);
        uint32_t cl_13 = format_clause(ctx, "FROM", ch_12);
        cond_11 = cl_13;
    }
    if (cond_11 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_11);
    uint32_t cond_14 = SYNTAQLITE_NULL_DOC;
    if (node->where != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_15 = format_node(ctx, node->where);
        uint32_t cl_16 = format_clause(ctx, "WHERE", ch_15);
        cond_14 = cl_16;
    }
    if (cond_14 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_14);
    uint32_t cond_17 = SYNTAQLITE_NULL_DOC;
    if (node->groupby != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_18 = format_node(ctx, node->groupby);
        uint32_t cl_19 = format_clause(ctx, "GROUP BY", ch_18);
        cond_17 = cl_19;
    }
    if (cond_17 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_17);
    uint32_t cond_20 = SYNTAQLITE_NULL_DOC;
    if (node->having != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_21 = format_node(ctx, node->having);
        uint32_t cl_22 = format_clause(ctx, "HAVING", ch_21);
        cond_20 = cl_22;
    }
    if (cond_20 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_20);
    uint32_t cond_23 = SYNTAQLITE_NULL_DOC;
    if (node->orderby != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_24 = format_node(ctx, node->orderby);
        uint32_t cl_25 = format_clause(ctx, "ORDER BY", ch_24);
        cond_23 = cl_25;
    }
    if (cond_23 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_23);
    uint32_t cond_26 = SYNTAQLITE_NULL_DOC;
    if (node->limit_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_27 = format_node(ctx, node->limit_clause);
        uint32_t cl_28 = format_clause(ctx, "LIMIT", ch_27);
        cond_26 = cl_28;
    }
    if (cond_26 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_26);
    uint32_t cond_29 = SYNTAQLITE_NULL_DOC;
    if (node->window_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_30 = format_node(ctx, node->window_clause);
        uint32_t cl_31 = format_clause(ctx, "WINDOW", ch_30);
        cond_29 = cl_31;
    }
    if (cond_29 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_29);
    uint32_t grp_32 = doc_group(&ctx->docs, cat_1);
    return grp_32;
}

static uint32_t format_ordering_term(FmtCtx *ctx, SyntaqliteOrderingTerm *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->expr);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->sort_order == SYNTAQLITE_SORT_ORDER_DESC) {
        uint32_t kw_4 = kw(ctx, " DESC");
        cond_3 = kw_4;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->nulls_order == SYNTAQLITE_NULLS_ORDER_FIRST) {
        uint32_t kw_6 = kw(ctx, " NULLS FIRST");
        cond_5 = kw_6;
    }
    if (cond_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_5);
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->nulls_order == SYNTAQLITE_NULLS_ORDER_LAST) {
        uint32_t kw_8 = kw(ctx, " NULLS LAST");
        cond_7 = kw_8;
    }
    if (cond_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_7);
    return cat_1;
}

static uint32_t format_limit_clause(FmtCtx *ctx, SyntaqliteLimitClause *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->limit);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->offset != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_4 = doc_concat_empty(&ctx->docs);
        uint32_t kw_5 = kw(ctx, " OFFSET ");
        if (kw_5 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, kw_5);
        uint32_t ch_6 = format_node(ctx, node->offset);
        if (ch_6 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, ch_6);
        cond_3 = cat_4;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    return cat_1;
}

static uint32_t format_column_ref(FmtCtx *ctx, SyntaqliteColumnRef *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t cat_3 = doc_concat_empty(&ctx->docs);
        uint32_t sp_4 = span_text(ctx, node->schema);
        if (sp_4 != SYNTAQLITE_NULL_DOC)
            cat_3 = doc_concat_append(&ctx->docs, cat_3, sp_4);
        uint32_t kw_5 = kw(ctx, ".");
        if (kw_5 != SYNTAQLITE_NULL_DOC)
            cat_3 = doc_concat_append(&ctx->docs, cat_3, kw_5);
        cond_2 = cat_3;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->table.length > 0) {
        uint32_t cat_7 = doc_concat_empty(&ctx->docs);
        uint32_t sp_8 = span_text(ctx, node->table);
        if (sp_8 != SYNTAQLITE_NULL_DOC)
            cat_7 = doc_concat_append(&ctx->docs, cat_7, sp_8);
        uint32_t kw_9 = kw(ctx, ".");
        if (kw_9 != SYNTAQLITE_NULL_DOC)
            cat_7 = doc_concat_append(&ctx->docs, cat_7, kw_9);
        cond_6 = cat_7;
    }
    if (cond_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_6);
    uint32_t sp_10 = span_text(ctx, node->column);
    if (sp_10 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_10);
    return cat_1;
}

static uint32_t format_function_call(FmtCtx *ctx, SyntaqliteFunctionCall *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t sp_2 = span_text(ctx, node->func_name);
    if (sp_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_2);
    uint32_t kw_3 = kw(ctx, "(");
    if (kw_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_3);
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) {
        uint32_t kw_5 = kw(ctx, "DISTINCT ");
        cond_4 = kw_5;
    }
    if (cond_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_4);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->flags.star) {
        uint32_t kw_7 = kw(ctx, "*");
        cond_6 = kw_7;
    } else {
        uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
        if (node->args != SYNTAQLITE_NULL_NODE) {
            uint32_t cat_9 = doc_concat_empty(&ctx->docs);
            uint32_t cat_10 = doc_concat_empty(&ctx->docs);
            uint32_t sl_11 = doc_softline(&ctx->docs);
            if (sl_11 != SYNTAQLITE_NULL_DOC)
                cat_10 = doc_concat_append(&ctx->docs, cat_10, sl_11);
            uint32_t ch_12 = format_node(ctx, node->args);
            if (ch_12 != SYNTAQLITE_NULL_DOC)
                cat_10 = doc_concat_append(&ctx->docs, cat_10, ch_12);
            uint32_t nst_13 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_10);
            uint32_t grp_14 = doc_group(&ctx->docs, nst_13);
            if (grp_14 != SYNTAQLITE_NULL_DOC)
                cat_9 = doc_concat_append(&ctx->docs, cat_9, grp_14);
            uint32_t sl_15 = doc_softline(&ctx->docs);
            if (sl_15 != SYNTAQLITE_NULL_DOC)
                cat_9 = doc_concat_append(&ctx->docs, cat_9, sl_15);
            cond_8 = cat_9;
        }
        cond_6 = cond_8;
    }
    if (cond_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_6);
    uint32_t kw_16 = kw(ctx, ")");
    if (kw_16 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_16);
    uint32_t cond_17 = SYNTAQLITE_NULL_DOC;
    if (node->filter_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_18 = doc_concat_empty(&ctx->docs);
        uint32_t kw_19 = kw(ctx, " ");
        if (kw_19 != SYNTAQLITE_NULL_DOC)
            cat_18 = doc_concat_append(&ctx->docs, cat_18, kw_19);
        uint32_t ch_20 = format_node(ctx, node->filter_clause);
        if (ch_20 != SYNTAQLITE_NULL_DOC)
            cat_18 = doc_concat_append(&ctx->docs, cat_18, ch_20);
        cond_17 = cat_18;
    }
    if (cond_17 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_17);
    uint32_t cond_21 = SYNTAQLITE_NULL_DOC;
    if (node->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_22 = doc_concat_empty(&ctx->docs);
        uint32_t kw_23 = kw(ctx, " ");
        if (kw_23 != SYNTAQLITE_NULL_DOC)
            cat_22 = doc_concat_append(&ctx->docs, cat_22, kw_23);
        uint32_t ch_24 = format_node(ctx, node->over_clause);
        if (ch_24 != SYNTAQLITE_NULL_DOC)
            cat_22 = doc_concat_append(&ctx->docs, cat_22, ch_24);
        cond_21 = cat_22;
    }
    if (cond_21 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_21);
    return cat_1;
}

static uint32_t format_is_expr(FmtCtx *ctx, SyntaqliteIsExpr *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_IS_OP_ISNULL: {
            uint32_t cat_2 = doc_concat_empty(&ctx->docs);
            uint32_t ch_3 = format_node(ctx, node->left);
            if (ch_3 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, ch_3);
            uint32_t kw_4 = kw(ctx, " ISNULL");
            if (kw_4 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, kw_4);
            sw_1 = cat_2;
            break;
        }
        case SYNTAQLITE_IS_OP_NOTNULL: {
            uint32_t cat_5 = doc_concat_empty(&ctx->docs);
            uint32_t ch_6 = format_node(ctx, node->left);
            if (ch_6 != SYNTAQLITE_NULL_DOC)
                cat_5 = doc_concat_append(&ctx->docs, cat_5, ch_6);
            uint32_t kw_7 = kw(ctx, " NOTNULL");
            if (kw_7 != SYNTAQLITE_NULL_DOC)
                cat_5 = doc_concat_append(&ctx->docs, cat_5, kw_7);
            sw_1 = cat_5;
            break;
        }
        case SYNTAQLITE_IS_OP_IS: {
            uint32_t cat_8 = doc_concat_empty(&ctx->docs);
            uint32_t ch_9 = format_node(ctx, node->left);
            if (ch_9 != SYNTAQLITE_NULL_DOC)
                cat_8 = doc_concat_append(&ctx->docs, cat_8, ch_9);
            uint32_t kw_10 = kw(ctx, " IS ");
            if (kw_10 != SYNTAQLITE_NULL_DOC)
                cat_8 = doc_concat_append(&ctx->docs, cat_8, kw_10);
            uint32_t ch_11 = format_node(ctx, node->right);
            if (ch_11 != SYNTAQLITE_NULL_DOC)
                cat_8 = doc_concat_append(&ctx->docs, cat_8, ch_11);
            sw_1 = cat_8;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_NOT: {
            uint32_t cat_12 = doc_concat_empty(&ctx->docs);
            uint32_t ch_13 = format_node(ctx, node->left);
            if (ch_13 != SYNTAQLITE_NULL_DOC)
                cat_12 = doc_concat_append(&ctx->docs, cat_12, ch_13);
            uint32_t kw_14 = kw(ctx, " IS NOT ");
            if (kw_14 != SYNTAQLITE_NULL_DOC)
                cat_12 = doc_concat_append(&ctx->docs, cat_12, kw_14);
            uint32_t ch_15 = format_node(ctx, node->right);
            if (ch_15 != SYNTAQLITE_NULL_DOC)
                cat_12 = doc_concat_append(&ctx->docs, cat_12, ch_15);
            sw_1 = cat_12;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_NOT_DISTINCT: {
            uint32_t cat_16 = doc_concat_empty(&ctx->docs);
            uint32_t ch_17 = format_node(ctx, node->left);
            if (ch_17 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, ch_17);
            uint32_t kw_18 = kw(ctx, " IS NOT DISTINCT FROM ");
            if (kw_18 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, kw_18);
            uint32_t ch_19 = format_node(ctx, node->right);
            if (ch_19 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, ch_19);
            sw_1 = cat_16;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_DISTINCT: {
            uint32_t cat_20 = doc_concat_empty(&ctx->docs);
            uint32_t ch_21 = format_node(ctx, node->left);
            if (ch_21 != SYNTAQLITE_NULL_DOC)
                cat_20 = doc_concat_append(&ctx->docs, cat_20, ch_21);
            uint32_t kw_22 = kw(ctx, " IS DISTINCT FROM ");
            if (kw_22 != SYNTAQLITE_NULL_DOC)
                cat_20 = doc_concat_append(&ctx->docs, cat_20, kw_22);
            uint32_t ch_23 = format_node(ctx, node->right);
            if (ch_23 != SYNTAQLITE_NULL_DOC)
                cat_20 = doc_concat_append(&ctx->docs, cat_20, ch_23);
            sw_1 = cat_20;
            break;
        }
        default: break;
    }
    return sw_1;
}

static uint32_t format_between_expr(FmtCtx *ctx, SyntaqliteBetweenExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->operand);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->negated) {
        uint32_t kw_4 = kw(ctx, " NOT BETWEEN ");
        cond_3 = kw_4;
    } else {
        uint32_t kw_5 = kw(ctx, " BETWEEN ");
        cond_3 = kw_5;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t ch_6 = format_node(ctx, node->low);
    if (ch_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_6);
    uint32_t kw_7 = kw(ctx, " AND ");
    if (kw_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_7);
    uint32_t ch_8 = format_node(ctx, node->high);
    if (ch_8 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_8);
    return cat_1;
}

static uint32_t format_like_expr(FmtCtx *ctx, SyntaqliteLikeExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->operand);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->negated) {
        uint32_t kw_4 = kw(ctx, " NOT LIKE ");
        cond_3 = kw_4;
    } else {
        uint32_t kw_5 = kw(ctx, " LIKE ");
        cond_3 = kw_5;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t ch_6 = format_node(ctx, node->pattern);
    if (ch_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_6);
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->escape != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_8 = doc_concat_empty(&ctx->docs);
        uint32_t kw_9 = kw(ctx, " ESCAPE ");
        if (kw_9 != SYNTAQLITE_NULL_DOC)
            cat_8 = doc_concat_append(&ctx->docs, cat_8, kw_9);
        uint32_t ch_10 = format_node(ctx, node->escape);
        if (ch_10 != SYNTAQLITE_NULL_DOC)
            cat_8 = doc_concat_append(&ctx->docs, cat_8, ch_10);
        cond_7 = cat_8;
    }
    if (cond_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_7);
    return cat_1;
}

static uint32_t format_case_expr(FmtCtx *ctx, SyntaqliteCaseExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "CASE");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->operand != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_4 = doc_concat_empty(&ctx->docs);
        uint32_t kw_5 = kw(ctx, " ");
        if (kw_5 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, kw_5);
        uint32_t ch_6 = format_node(ctx, node->operand);
        if (ch_6 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, ch_6);
        cond_3 = cat_4;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->whens != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_8 = format_node(ctx, node->whens);
        cond_7 = ch_8;
    }
    if (cond_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_7);
    uint32_t cond_9 = SYNTAQLITE_NULL_DOC;
    if (node->else_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_10 = doc_concat_empty(&ctx->docs);
        uint32_t kw_11 = kw(ctx, " ELSE ");
        if (kw_11 != SYNTAQLITE_NULL_DOC)
            cat_10 = doc_concat_append(&ctx->docs, cat_10, kw_11);
        uint32_t ch_12 = format_node(ctx, node->else_expr);
        if (ch_12 != SYNTAQLITE_NULL_DOC)
            cat_10 = doc_concat_append(&ctx->docs, cat_10, ch_12);
        cond_9 = cat_10;
    }
    if (cond_9 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_9);
    uint32_t kw_13 = kw(ctx, " END");
    if (kw_13 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_13);
    return cat_1;
}

static uint32_t format_case_when(FmtCtx *ctx, SyntaqliteCaseWhen *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, " WHEN ");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t ch_3 = format_node(ctx, node->when_expr);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    uint32_t kw_4 = kw(ctx, " THEN ");
    if (kw_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_4);
    uint32_t ch_5 = format_node(ctx, node->then_expr);
    if (ch_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_5);
    return cat_1;
}

static uint32_t format_case_when_list(FmtCtx *ctx, SyntaqliteCaseWhenList *node) {
    uint32_t lst_1 = doc_concat_empty(&ctx->docs);
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        uint32_t item_2 = format_node(ctx, _child_id);
        lst_1 = doc_concat_append(&ctx->docs, lst_1, item_2);
    }
    return lst_1;
}

static uint32_t format_compound_select(FmtCtx *ctx, SyntaqliteCompoundSelect *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->left);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t hl_3 = doc_hardline(&ctx->docs);
    if (hl_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, hl_3);
    uint32_t ed_4 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_COMPOUND_OP_UNION: ed_4 = kw(ctx, "UNION"); break;
        case SYNTAQLITE_COMPOUND_OP_UNION_ALL: ed_4 = kw(ctx, "UNION ALL"); break;
        case SYNTAQLITE_COMPOUND_OP_INTERSECT: ed_4 = kw(ctx, "INTERSECT"); break;
        case SYNTAQLITE_COMPOUND_OP_EXCEPT: ed_4 = kw(ctx, "EXCEPT"); break;
        default: break;
    }
    if (ed_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ed_4);
    uint32_t hl_5 = doc_hardline(&ctx->docs);
    if (hl_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, hl_5);
    uint32_t ch_6 = format_node(ctx, node->right);
    if (ch_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_6);
    return cat_1;
}

static uint32_t format_subquery_expr(FmtCtx *ctx, SyntaqliteSubqueryExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "(");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t ch_3 = format_node(ctx, node->select);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    uint32_t kw_4 = kw(ctx, ")");
    if (kw_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_4);
    return cat_1;
}

static uint32_t format_exists_expr(FmtCtx *ctx, SyntaqliteExistsExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "EXISTS (");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t ch_3 = format_node(ctx, node->select);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    uint32_t kw_4 = kw(ctx, ")");
    if (kw_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_4);
    return cat_1;
}

static uint32_t format_in_expr(FmtCtx *ctx, SyntaqliteInExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->operand);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->negated) {
        uint32_t kw_4 = kw(ctx, " NOT IN ");
        cond_3 = kw_4;
    } else {
        uint32_t kw_5 = kw(ctx, " IN ");
        cond_3 = kw_5;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t kw_6 = kw(ctx, "(");
    if (kw_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_6);
    uint32_t ch_7 = format_node(ctx, node->source);
    if (ch_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_7);
    uint32_t kw_8 = kw(ctx, ")");
    if (kw_8 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_8);
    return cat_1;
}

static uint32_t format_variable(FmtCtx *ctx, SyntaqliteVariable *node) {
    uint32_t sp_1 = span_text(ctx, node->source);
    return sp_1;
}

static uint32_t format_collate_expr(FmtCtx *ctx, SyntaqliteCollateExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t ch_2 = format_node(ctx, node->expr);
    if (ch_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_2);
    uint32_t kw_3 = kw(ctx, " COLLATE ");
    if (kw_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_3);
    uint32_t sp_4 = span_text(ctx, node->collation);
    if (sp_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_4);
    return cat_1;
}

static uint32_t format_cast_expr(FmtCtx *ctx, SyntaqliteCastExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "CAST(");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t ch_3 = format_node(ctx, node->expr);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    uint32_t kw_4 = kw(ctx, " AS ");
    if (kw_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_4);
    uint32_t sp_5 = span_text(ctx, node->type_name);
    if (sp_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_5);
    uint32_t kw_6 = kw(ctx, ")");
    if (kw_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_6);
    return cat_1;
}

static uint32_t format_values_row_list(FmtCtx *ctx, SyntaqliteValuesRowList *node) {
    uint32_t lst_1 = doc_concat_empty(&ctx->docs);
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t cat_2 = doc_concat_empty(&ctx->docs);
            uint32_t kw_3 = kw(ctx, ",");
            if (kw_3 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, kw_3);
            uint32_t ln_4 = doc_line(&ctx->docs);
            if (ln_4 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, ln_4);
            lst_1 = doc_concat_append(&ctx->docs, lst_1, cat_2);
        }
        uint32_t cat_5 = doc_concat_empty(&ctx->docs);
        uint32_t kw_6 = kw(ctx, "(");
        if (kw_6 != SYNTAQLITE_NULL_DOC)
            cat_5 = doc_concat_append(&ctx->docs, cat_5, kw_6);
        uint32_t item_7 = format_node(ctx, _child_id);
        if (item_7 != SYNTAQLITE_NULL_DOC)
            cat_5 = doc_concat_append(&ctx->docs, cat_5, item_7);
        uint32_t kw_8 = kw(ctx, ")");
        if (kw_8 != SYNTAQLITE_NULL_DOC)
            cat_5 = doc_concat_append(&ctx->docs, cat_5, kw_8);
        lst_1 = doc_concat_append(&ctx->docs, lst_1, cat_5);
    }
    return lst_1;
}

static uint32_t format_values_clause(FmtCtx *ctx, SyntaqliteValuesClause *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "VALUES ");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t ch_3 = format_node(ctx, node->rows);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    return cat_1;
}

static uint32_t format_cte_definition(FmtCtx *ctx, SyntaqliteCteDefinition *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t sp_2 = span_text(ctx, node->cte_name);
    if (sp_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_4 = doc_concat_empty(&ctx->docs);
        uint32_t kw_5 = kw(ctx, "(");
        if (kw_5 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, kw_5);
        uint32_t ch_6 = format_node(ctx, node->columns);
        if (ch_6 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, ch_6);
        uint32_t kw_7 = kw(ctx, ")");
        if (kw_7 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, kw_7);
        cond_3 = cat_4;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t kw_8 = kw(ctx, " AS ");
    if (kw_8 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_8);
    uint32_t cond_9 = SYNTAQLITE_NULL_DOC;
    if (node->materialized == SYNTAQLITE_MATERIALIZED_MATERIALIZED) {
        uint32_t kw_10 = kw(ctx, "MATERIALIZED ");
        cond_9 = kw_10;
    }
    if (cond_9 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_9);
    uint32_t cond_11 = SYNTAQLITE_NULL_DOC;
    if (node->materialized == SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED) {
        uint32_t kw_12 = kw(ctx, "NOT MATERIALIZED ");
        cond_11 = kw_12;
    }
    if (cond_11 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_11);
    uint32_t kw_13 = kw(ctx, "(");
    if (kw_13 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_13);
    uint32_t ch_14 = format_node(ctx, node->select);
    if (ch_14 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_14);
    uint32_t kw_15 = kw(ctx, ")");
    if (kw_15 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_15);
    return cat_1;
}

static uint32_t format_with_clause(FmtCtx *ctx, SyntaqliteWithClause *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->recursive) {
        uint32_t kw_3 = kw(ctx, "WITH RECURSIVE ");
        cond_2 = kw_3;
    } else {
        uint32_t kw_4 = kw(ctx, "WITH ");
        cond_2 = kw_4;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->ctes != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_6 = format_node(ctx, node->ctes);
        cond_5 = ch_6;
    }
    if (cond_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_5);
    uint32_t hl_7 = doc_hardline(&ctx->docs);
    if (hl_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, hl_7);
    uint32_t ch_8 = format_node(ctx, node->select);
    if (ch_8 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_8);
    return cat_1;
}

static uint32_t format_aggregate_function_call(FmtCtx *ctx, SyntaqliteAggregateFunctionCall *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t sp_2 = span_text(ctx, node->func_name);
    if (sp_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_2);
    uint32_t kw_3 = kw(ctx, "(");
    if (kw_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_3);
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) {
        uint32_t kw_5 = kw(ctx, "DISTINCT ");
        cond_4 = kw_5;
    }
    if (cond_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_4);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->args != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_7 = format_node(ctx, node->args);
        uint32_t grp_8 = doc_group(&ctx->docs, ch_7);
        cond_6 = grp_8;
    }
    if (cond_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_6);
    uint32_t cond_9 = SYNTAQLITE_NULL_DOC;
    if (node->orderby != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_10 = doc_concat_empty(&ctx->docs);
        uint32_t kw_11 = kw(ctx, " ORDER BY ");
        if (kw_11 != SYNTAQLITE_NULL_DOC)
            cat_10 = doc_concat_append(&ctx->docs, cat_10, kw_11);
        uint32_t ch_12 = format_node(ctx, node->orderby);
        if (ch_12 != SYNTAQLITE_NULL_DOC)
            cat_10 = doc_concat_append(&ctx->docs, cat_10, ch_12);
        cond_9 = cat_10;
    }
    if (cond_9 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_9);
    uint32_t kw_13 = kw(ctx, ")");
    if (kw_13 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_13);
    uint32_t cond_14 = SYNTAQLITE_NULL_DOC;
    if (node->filter_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_15 = doc_concat_empty(&ctx->docs);
        uint32_t kw_16 = kw(ctx, " ");
        if (kw_16 != SYNTAQLITE_NULL_DOC)
            cat_15 = doc_concat_append(&ctx->docs, cat_15, kw_16);
        uint32_t ch_17 = format_node(ctx, node->filter_clause);
        if (ch_17 != SYNTAQLITE_NULL_DOC)
            cat_15 = doc_concat_append(&ctx->docs, cat_15, ch_17);
        cond_14 = cat_15;
    }
    if (cond_14 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_14);
    uint32_t cond_18 = SYNTAQLITE_NULL_DOC;
    if (node->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_19 = doc_concat_empty(&ctx->docs);
        uint32_t kw_20 = kw(ctx, " ");
        if (kw_20 != SYNTAQLITE_NULL_DOC)
            cat_19 = doc_concat_append(&ctx->docs, cat_19, kw_20);
        uint32_t ch_21 = format_node(ctx, node->over_clause);
        if (ch_21 != SYNTAQLITE_NULL_DOC)
            cat_19 = doc_concat_append(&ctx->docs, cat_19, ch_21);
        cond_18 = cat_19;
    }
    if (cond_18 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_18);
    return cat_1;
}

static uint32_t format_raise_expr(FmtCtx *ctx, SyntaqliteRaiseExpr *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "RAISE(");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t sw_3 = SYNTAQLITE_NULL_DOC;
    switch (node->raise_type) {
        case SYNTAQLITE_RAISE_TYPE_IGNORE: {
            uint32_t kw_4 = kw(ctx, "IGNORE");
            sw_3 = kw_4;
            break;
        }
        case SYNTAQLITE_RAISE_TYPE_ROLLBACK: {
            uint32_t kw_5 = kw(ctx, "ROLLBACK");
            sw_3 = kw_5;
            break;
        }
        case SYNTAQLITE_RAISE_TYPE_ABORT: {
            uint32_t kw_6 = kw(ctx, "ABORT");
            sw_3 = kw_6;
            break;
        }
        case SYNTAQLITE_RAISE_TYPE_FAIL: {
            uint32_t kw_7 = kw(ctx, "FAIL");
            sw_3 = kw_7;
            break;
        }
        default: break;
    }
    if (sw_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sw_3);
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->error_message != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_9 = doc_concat_empty(&ctx->docs);
        uint32_t kw_10 = kw(ctx, ", ");
        if (kw_10 != SYNTAQLITE_NULL_DOC)
            cat_9 = doc_concat_append(&ctx->docs, cat_9, kw_10);
        uint32_t ch_11 = format_node(ctx, node->error_message);
        if (ch_11 != SYNTAQLITE_NULL_DOC)
            cat_9 = doc_concat_append(&ctx->docs, cat_9, ch_11);
        cond_8 = cat_9;
    }
    if (cond_8 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_8);
    uint32_t kw_12 = kw(ctx, ")");
    if (kw_12 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_12);
    return cat_1;
}

static uint32_t format_table_ref(FmtCtx *ctx, SyntaqliteTableRef *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t cat_3 = doc_concat_empty(&ctx->docs);
        uint32_t sp_4 = span_text(ctx, node->schema);
        if (sp_4 != SYNTAQLITE_NULL_DOC)
            cat_3 = doc_concat_append(&ctx->docs, cat_3, sp_4);
        uint32_t kw_5 = kw(ctx, ".");
        if (kw_5 != SYNTAQLITE_NULL_DOC)
            cat_3 = doc_concat_append(&ctx->docs, cat_3, kw_5);
        cond_2 = cat_3;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t sp_6 = span_text(ctx, node->table_name);
    if (sp_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sp_6);
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t cat_8 = doc_concat_empty(&ctx->docs);
        uint32_t kw_9 = kw(ctx, " AS ");
        if (kw_9 != SYNTAQLITE_NULL_DOC)
            cat_8 = doc_concat_append(&ctx->docs, cat_8, kw_9);
        uint32_t sp_10 = span_text(ctx, node->alias);
        if (sp_10 != SYNTAQLITE_NULL_DOC)
            cat_8 = doc_concat_append(&ctx->docs, cat_8, sp_10);
        cond_7 = cat_8;
    }
    if (cond_7 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_7);
    return cat_1;
}

static uint32_t format_subquery_table_source(FmtCtx *ctx, SyntaqliteSubqueryTableSource *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "(");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t ch_3 = format_node(ctx, node->select);
    if (ch_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_3);
    uint32_t kw_4 = kw(ctx, ")");
    if (kw_4 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_4);
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t cat_6 = doc_concat_empty(&ctx->docs);
        uint32_t kw_7 = kw(ctx, " AS ");
        if (kw_7 != SYNTAQLITE_NULL_DOC)
            cat_6 = doc_concat_append(&ctx->docs, cat_6, kw_7);
        uint32_t sp_8 = span_text(ctx, node->alias);
        if (sp_8 != SYNTAQLITE_NULL_DOC)
            cat_6 = doc_concat_append(&ctx->docs, cat_6, sp_8);
        cond_5 = cat_6;
    }
    if (cond_5 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_5);
    return cat_1;
}

static uint32_t format_join_clause(FmtCtx *ctx, SyntaqliteJoinClause *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->join_type) {
        case SYNTAQLITE_JOIN_TYPE_COMMA: {
            uint32_t cat_2 = doc_concat_empty(&ctx->docs);
            uint32_t ch_3 = format_node(ctx, node->left);
            if (ch_3 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, ch_3);
            uint32_t kw_4 = kw(ctx, ", ");
            if (kw_4 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, kw_4);
            uint32_t ch_5 = format_node(ctx, node->right);
            if (ch_5 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, ch_5);
            uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
            if (node->on_expr != SYNTAQLITE_NULL_NODE) {
                uint32_t cat_7 = doc_concat_empty(&ctx->docs);
                uint32_t hl_8 = doc_hardline(&ctx->docs);
                if (hl_8 != SYNTAQLITE_NULL_DOC)
                    cat_7 = doc_concat_append(&ctx->docs, cat_7, hl_8);
                uint32_t kw_9 = kw(ctx, "ON ");
                if (kw_9 != SYNTAQLITE_NULL_DOC)
                    cat_7 = doc_concat_append(&ctx->docs, cat_7, kw_9);
                uint32_t ch_10 = format_node(ctx, node->on_expr);
                if (ch_10 != SYNTAQLITE_NULL_DOC)
                    cat_7 = doc_concat_append(&ctx->docs, cat_7, ch_10);
                cond_6 = cat_7;
            }
            if (cond_6 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, cond_6);
            uint32_t cond_11 = SYNTAQLITE_NULL_DOC;
            if (node->using_columns != SYNTAQLITE_NULL_NODE) {
                uint32_t cat_12 = doc_concat_empty(&ctx->docs);
                uint32_t kw_13 = kw(ctx, " USING (");
                if (kw_13 != SYNTAQLITE_NULL_DOC)
                    cat_12 = doc_concat_append(&ctx->docs, cat_12, kw_13);
                uint32_t ch_14 = format_node(ctx, node->using_columns);
                if (ch_14 != SYNTAQLITE_NULL_DOC)
                    cat_12 = doc_concat_append(&ctx->docs, cat_12, ch_14);
                uint32_t kw_15 = kw(ctx, ")");
                if (kw_15 != SYNTAQLITE_NULL_DOC)
                    cat_12 = doc_concat_append(&ctx->docs, cat_12, kw_15);
                cond_11 = cat_12;
            }
            if (cond_11 != SYNTAQLITE_NULL_DOC)
                cat_2 = doc_concat_append(&ctx->docs, cat_2, cond_11);
            sw_1 = cat_2;
            break;
        }
        default: {
            uint32_t cat_16 = doc_concat_empty(&ctx->docs);
            uint32_t ch_17 = format_node(ctx, node->left);
            if (ch_17 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, ch_17);
            uint32_t hl_18 = doc_hardline(&ctx->docs);
            if (hl_18 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, hl_18);
            uint32_t ed_19 = SYNTAQLITE_NULL_DOC;
            switch (node->join_type) {
                case SYNTAQLITE_JOIN_TYPE_INNER: ed_19 = kw(ctx, "JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_LEFT: ed_19 = kw(ctx, "LEFT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_RIGHT: ed_19 = kw(ctx, "RIGHT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_FULL: ed_19 = kw(ctx, "FULL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_CROSS: ed_19 = kw(ctx, "CROSS JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_INNER: ed_19 = kw(ctx, "NATURAL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT: ed_19 = kw(ctx, "NATURAL LEFT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT: ed_19 = kw(ctx, "NATURAL RIGHT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_FULL: ed_19 = kw(ctx, "NATURAL FULL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_COMMA: ed_19 = kw(ctx, ","); break;
                default: break;
            }
            if (ed_19 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, ed_19);
            uint32_t kw_20 = kw(ctx, " ");
            if (kw_20 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, kw_20);
            uint32_t ch_21 = format_node(ctx, node->right);
            if (ch_21 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, ch_21);
            uint32_t cond_22 = SYNTAQLITE_NULL_DOC;
            if (node->on_expr != SYNTAQLITE_NULL_NODE) {
                uint32_t cat_23 = doc_concat_empty(&ctx->docs);
                uint32_t hl_24 = doc_hardline(&ctx->docs);
                if (hl_24 != SYNTAQLITE_NULL_DOC)
                    cat_23 = doc_concat_append(&ctx->docs, cat_23, hl_24);
                uint32_t kw_25 = kw(ctx, "ON ");
                if (kw_25 != SYNTAQLITE_NULL_DOC)
                    cat_23 = doc_concat_append(&ctx->docs, cat_23, kw_25);
                uint32_t ch_26 = format_node(ctx, node->on_expr);
                if (ch_26 != SYNTAQLITE_NULL_DOC)
                    cat_23 = doc_concat_append(&ctx->docs, cat_23, ch_26);
                cond_22 = cat_23;
            }
            if (cond_22 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, cond_22);
            uint32_t cond_27 = SYNTAQLITE_NULL_DOC;
            if (node->using_columns != SYNTAQLITE_NULL_NODE) {
                uint32_t cat_28 = doc_concat_empty(&ctx->docs);
                uint32_t kw_29 = kw(ctx, " USING (");
                if (kw_29 != SYNTAQLITE_NULL_DOC)
                    cat_28 = doc_concat_append(&ctx->docs, cat_28, kw_29);
                uint32_t ch_30 = format_node(ctx, node->using_columns);
                if (ch_30 != SYNTAQLITE_NULL_DOC)
                    cat_28 = doc_concat_append(&ctx->docs, cat_28, ch_30);
                uint32_t kw_31 = kw(ctx, ")");
                if (kw_31 != SYNTAQLITE_NULL_DOC)
                    cat_28 = doc_concat_append(&ctx->docs, cat_28, kw_31);
                cond_27 = cat_28;
            }
            if (cond_27 != SYNTAQLITE_NULL_DOC)
                cat_16 = doc_concat_append(&ctx->docs, cat_16, cond_27);
            sw_1 = cat_16;
            break;
        }
    }
    return sw_1;
}

static uint32_t format_join_prefix(FmtCtx *ctx, SyntaqliteJoinPrefix *node) {
    uint32_t ch_1 = format_node(ctx, node->source);
    return ch_1;
}

static uint32_t format_delete_stmt(FmtCtx *ctx, SyntaqliteDeleteStmt *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "DELETE");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->table != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_4 = format_node(ctx, node->table);
        uint32_t cl_5 = format_clause(ctx, "FROM", ch_4);
        cond_3 = cl_5;
    }
    if (cond_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_3);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->where != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_7 = format_node(ctx, node->where);
        uint32_t cl_8 = format_clause(ctx, "WHERE", ch_7);
        cond_6 = cl_8;
    }
    if (cond_6 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_6);
    return cat_1;
}

static uint32_t format_set_clause(FmtCtx *ctx, SyntaqliteSetClause *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->column.length > 0) {
        uint32_t sp_3 = span_text(ctx, node->column);
        cond_2 = sp_3;
    } else {
        uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
        if (node->columns != SYNTAQLITE_NULL_NODE) {
            uint32_t cat_5 = doc_concat_empty(&ctx->docs);
            uint32_t kw_6 = kw(ctx, "(");
            if (kw_6 != SYNTAQLITE_NULL_DOC)
                cat_5 = doc_concat_append(&ctx->docs, cat_5, kw_6);
            uint32_t ch_7 = format_node(ctx, node->columns);
            if (ch_7 != SYNTAQLITE_NULL_DOC)
                cat_5 = doc_concat_append(&ctx->docs, cat_5, ch_7);
            uint32_t kw_8 = kw(ctx, ")");
            if (kw_8 != SYNTAQLITE_NULL_DOC)
                cat_5 = doc_concat_append(&ctx->docs, cat_5, kw_8);
            cond_4 = cat_5;
        }
        cond_2 = cond_4;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t kw_9 = kw(ctx, " = ");
    if (kw_9 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_9);
    uint32_t ch_10 = format_node(ctx, node->value);
    if (ch_10 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_10);
    return cat_1;
}

static uint32_t format_update_stmt(FmtCtx *ctx, SyntaqliteUpdateStmt *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t kw_2 = kw(ctx, "UPDATE");
    if (kw_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_2);
    uint32_t sw_3 = SYNTAQLITE_NULL_DOC;
    switch (node->conflict_action) {
        case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: {
            uint32_t kw_4 = kw(ctx, " OR ROLLBACK");
            sw_3 = kw_4;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_ABORT: {
            uint32_t kw_5 = kw(ctx, " OR ABORT");
            sw_3 = kw_5;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_FAIL: {
            uint32_t kw_6 = kw(ctx, " OR FAIL");
            sw_3 = kw_6;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_IGNORE: {
            uint32_t kw_7 = kw(ctx, " OR IGNORE");
            sw_3 = kw_7;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_REPLACE: {
            uint32_t kw_8 = kw(ctx, " OR REPLACE");
            sw_3 = kw_8;
            break;
        }
        default: break;
    }
    if (sw_3 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, sw_3);
    uint32_t kw_9 = kw(ctx, " ");
    if (kw_9 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_9);
    uint32_t ch_10 = format_node(ctx, node->table);
    if (ch_10 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_10);
    uint32_t cond_11 = SYNTAQLITE_NULL_DOC;
    if (node->setlist != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_12 = format_node(ctx, node->setlist);
        uint32_t cl_13 = format_clause(ctx, "SET", ch_12);
        cond_11 = cl_13;
    }
    if (cond_11 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_11);
    uint32_t cond_14 = SYNTAQLITE_NULL_DOC;
    if (node->from_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_15 = format_node(ctx, node->from_clause);
        uint32_t cl_16 = format_clause(ctx, "FROM", ch_15);
        cond_14 = cl_16;
    }
    if (cond_14 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_14);
    uint32_t cond_17 = SYNTAQLITE_NULL_DOC;
    if (node->where != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_18 = format_node(ctx, node->where);
        uint32_t cl_19 = format_clause(ctx, "WHERE", ch_18);
        cond_17 = cl_19;
    }
    if (cond_17 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_17);
    return cat_1;
}

static uint32_t format_insert_stmt(FmtCtx *ctx, SyntaqliteInsertStmt *node) {
    uint32_t cat_1 = doc_concat_empty(&ctx->docs);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->conflict_action == SYNTAQLITE_CONFLICT_ACTION_REPLACE) {
        uint32_t kw_3 = kw(ctx, "REPLACE");
        cond_2 = kw_3;
    } else {
        uint32_t cat_4 = doc_concat_empty(&ctx->docs);
        uint32_t kw_5 = kw(ctx, "INSERT");
        if (kw_5 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, kw_5);
        uint32_t sw_6 = SYNTAQLITE_NULL_DOC;
        switch (node->conflict_action) {
            case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: {
                uint32_t kw_7 = kw(ctx, " OR ROLLBACK");
                sw_6 = kw_7;
                break;
            }
            case SYNTAQLITE_CONFLICT_ACTION_ABORT: {
                uint32_t kw_8 = kw(ctx, " OR ABORT");
                sw_6 = kw_8;
                break;
            }
            case SYNTAQLITE_CONFLICT_ACTION_FAIL: {
                uint32_t kw_9 = kw(ctx, " OR FAIL");
                sw_6 = kw_9;
                break;
            }
            case SYNTAQLITE_CONFLICT_ACTION_IGNORE: {
                uint32_t kw_10 = kw(ctx, " OR IGNORE");
                sw_6 = kw_10;
                break;
            }
            default: break;
        }
        if (sw_6 != SYNTAQLITE_NULL_DOC)
            cat_4 = doc_concat_append(&ctx->docs, cat_4, sw_6);
        cond_2 = cat_4;
    }
    if (cond_2 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_2);
    uint32_t kw_11 = kw(ctx, " INTO ");
    if (kw_11 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, kw_11);
    uint32_t ch_12 = format_node(ctx, node->table);
    if (ch_12 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, ch_12);
    uint32_t cond_13 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_14 = doc_concat_empty(&ctx->docs);
        uint32_t kw_15 = kw(ctx, " (");
        if (kw_15 != SYNTAQLITE_NULL_DOC)
            cat_14 = doc_concat_append(&ctx->docs, cat_14, kw_15);
        uint32_t ch_16 = format_node(ctx, node->columns);
        if (ch_16 != SYNTAQLITE_NULL_DOC)
            cat_14 = doc_concat_append(&ctx->docs, cat_14, ch_16);
        uint32_t kw_17 = kw(ctx, ")");
        if (kw_17 != SYNTAQLITE_NULL_DOC)
            cat_14 = doc_concat_append(&ctx->docs, cat_14, kw_17);
        cond_13 = cat_14;
    }
    if (cond_13 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_13);
    uint32_t cond_18 = SYNTAQLITE_NULL_DOC;
    if (node->source != SYNTAQLITE_NULL_NODE) {
        uint32_t cat_19 = doc_concat_empty(&ctx->docs);
        uint32_t hl_20 = doc_hardline(&ctx->docs);
        if (hl_20 != SYNTAQLITE_NULL_DOC)
            cat_19 = doc_concat_append(&ctx->docs, cat_19, hl_20);
        uint32_t ch_21 = format_node(ctx, node->source);
        if (ch_21 != SYNTAQLITE_NULL_DOC)
            cat_19 = doc_concat_append(&ctx->docs, cat_19, ch_21);
        cond_18 = cat_19;
    }
    if (cond_18 != SYNTAQLITE_NULL_DOC)
        cat_1 = doc_concat_append(&ctx->docs, cat_1, cond_18);
    return cat_1;
}

// ============ Main Dispatcher ============

static uint32_t format_node(FmtCtx *ctx, uint32_t node_id) {
    if (node_id == SYNTAQLITE_NULL_NODE) return SYNTAQLITE_NULL_DOC;

    SyntaqliteNode *node = AST_NODE(ctx->ast, node_id);
    if (!node) return SYNTAQLITE_NULL_DOC;

    switch (node->tag) {
        case SYNTAQLITE_NODE_BINARY_EXPR:
            return format_binary_expr(ctx, &node->binary_expr);
        case SYNTAQLITE_NODE_UNARY_EXPR:
            return format_unary_expr(ctx, &node->unary_expr);
        case SYNTAQLITE_NODE_LITERAL:
            return format_literal(ctx, &node->literal);
        case SYNTAQLITE_NODE_EXPR_LIST:
            return format_comma_list(ctx, node->expr_list.children, node->expr_list.count);
        case SYNTAQLITE_NODE_RESULT_COLUMN:
            return format_result_column(ctx, &node->result_column);
        case SYNTAQLITE_NODE_RESULT_COLUMN_LIST:
            return format_comma_list(ctx, node->result_column_list.children, node->result_column_list.count);
        case SYNTAQLITE_NODE_SELECT_STMT:
            return format_select_stmt(ctx, &node->select_stmt);
        case SYNTAQLITE_NODE_ORDERING_TERM:
            return format_ordering_term(ctx, &node->ordering_term);
        case SYNTAQLITE_NODE_ORDER_BY_LIST:
            return format_comma_list(ctx, node->order_by_list.children, node->order_by_list.count);
        case SYNTAQLITE_NODE_LIMIT_CLAUSE:
            return format_limit_clause(ctx, &node->limit_clause);
        case SYNTAQLITE_NODE_COLUMN_REF:
            return format_column_ref(ctx, &node->column_ref);
        case SYNTAQLITE_NODE_FUNCTION_CALL:
            return format_function_call(ctx, &node->function_call);
        case SYNTAQLITE_NODE_IS_EXPR:
            return format_is_expr(ctx, &node->is_expr);
        case SYNTAQLITE_NODE_BETWEEN_EXPR:
            return format_between_expr(ctx, &node->between_expr);
        case SYNTAQLITE_NODE_LIKE_EXPR:
            return format_like_expr(ctx, &node->like_expr);
        case SYNTAQLITE_NODE_CASE_EXPR:
            return format_case_expr(ctx, &node->case_expr);
        case SYNTAQLITE_NODE_CASE_WHEN:
            return format_case_when(ctx, &node->case_when);
        case SYNTAQLITE_NODE_CASE_WHEN_LIST:
            return format_case_when_list(ctx, &node->case_when_list);
        case SYNTAQLITE_NODE_COMPOUND_SELECT:
            return format_compound_select(ctx, &node->compound_select);
        case SYNTAQLITE_NODE_SUBQUERY_EXPR:
            return format_subquery_expr(ctx, &node->subquery_expr);
        case SYNTAQLITE_NODE_EXISTS_EXPR:
            return format_exists_expr(ctx, &node->exists_expr);
        case SYNTAQLITE_NODE_IN_EXPR:
            return format_in_expr(ctx, &node->in_expr);
        case SYNTAQLITE_NODE_VARIABLE:
            return format_variable(ctx, &node->variable);
        case SYNTAQLITE_NODE_COLLATE_EXPR:
            return format_collate_expr(ctx, &node->collate_expr);
        case SYNTAQLITE_NODE_CAST_EXPR:
            return format_cast_expr(ctx, &node->cast_expr);
        case SYNTAQLITE_NODE_VALUES_ROW_LIST:
            return format_values_row_list(ctx, &node->values_row_list);
        case SYNTAQLITE_NODE_VALUES_CLAUSE:
            return format_values_clause(ctx, &node->values_clause);
        case SYNTAQLITE_NODE_CTE_DEFINITION:
            return format_cte_definition(ctx, &node->cte_definition);
        case SYNTAQLITE_NODE_CTE_LIST:
            return format_comma_list(ctx, node->cte_list.children, node->cte_list.count);
        case SYNTAQLITE_NODE_WITH_CLAUSE:
            return format_with_clause(ctx, &node->with_clause);
        case SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL:
            return format_aggregate_function_call(ctx, &node->aggregate_function_call);
        case SYNTAQLITE_NODE_RAISE_EXPR:
            return format_raise_expr(ctx, &node->raise_expr);
        case SYNTAQLITE_NODE_TABLE_REF:
            return format_table_ref(ctx, &node->table_ref);
        case SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE:
            return format_subquery_table_source(ctx, &node->subquery_table_source);
        case SYNTAQLITE_NODE_JOIN_CLAUSE:
            return format_join_clause(ctx, &node->join_clause);
        case SYNTAQLITE_NODE_JOIN_PREFIX:
            return format_join_prefix(ctx, &node->join_prefix);
        case SYNTAQLITE_NODE_DELETE_STMT:
            return format_delete_stmt(ctx, &node->delete_stmt);
        case SYNTAQLITE_NODE_SET_CLAUSE:
            return format_set_clause(ctx, &node->set_clause);
        case SYNTAQLITE_NODE_SET_CLAUSE_LIST:
            return format_comma_list(ctx, node->set_clause_list.children, node->set_clause_list.count);
        case SYNTAQLITE_NODE_UPDATE_STMT:
            return format_update_stmt(ctx, &node->update_stmt);
        case SYNTAQLITE_NODE_INSERT_STMT:
            return format_insert_stmt(ctx, &node->insert_stmt);
        case SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST:
            return format_comma_list(ctx, node->column_constraint_list.children, node->column_constraint_list.count);
        case SYNTAQLITE_NODE_COLUMN_DEF_LIST:
            return format_comma_list(ctx, node->column_def_list.children, node->column_def_list.count);
        case SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST:
            return format_comma_list(ctx, node->table_constraint_list.children, node->table_constraint_list.count);
        case SYNTAQLITE_NODE_WINDOW_DEF_LIST:
            return format_comma_list(ctx, node->window_def_list.children, node->window_def_list.count);
        case SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST:
            return format_comma_list(ctx, node->named_window_def_list.children, node->named_window_def_list.count);
        case SYNTAQLITE_NODE_TRIGGER_CMD_LIST:
            return format_comma_list(ctx, node->trigger_cmd_list.children, node->trigger_cmd_list.count);
        default:
            return kw(ctx, "/* UNSUPPORTED */");
    }
}

// ============ Public API ============

char *syntaqlite_format(SyntaqliteArena *ast, uint32_t root_id,
                        const char *source, SyntaqliteTokenList *token_list,
                        SyntaqliteFmtOptions *options) {
    SyntaqliteFmtOptions default_options = SYNTAQLITE_FMT_OPTIONS_DEFAULT;
    if (!options) options = &default_options;

    FmtCtx ctx;
    syntaqlite_doc_context_init(&ctx.docs);
    ctx.ast = ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = options;

    uint32_t root_doc = format_node(&ctx, root_id);
    if (root_doc == SYNTAQLITE_NULL_DOC) {
        syntaqlite_doc_context_cleanup(&ctx.docs);
        char *empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    char *result = syntaqlite_doc_layout(&ctx.docs, root_doc, options->target_width);
    syntaqlite_doc_context_cleanup(&ctx.docs);
    return result;
}
