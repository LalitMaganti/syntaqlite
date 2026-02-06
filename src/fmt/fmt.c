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
    uint32_t buf[count * 3];
    uint32_t n = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (i > 0) {
            buf[n++] = kw(ctx, ",");
            buf[n++] = doc_line(&ctx->docs);
        }
        buf[n++] = format_node(ctx, children[i]);
    }
    return doc_concat(&ctx->docs, buf, n);
}

// ============ Clause Helper ============

static uint32_t format_clause(FmtCtx *ctx, const char *keyword, uint32_t body_doc) {
    if (body_doc == SYNTAQLITE_NULL_DOC) return SYNTAQLITE_NULL_DOC;
    uint32_t inner_items[] = { doc_line(&ctx->docs), body_doc };
    uint32_t inner = doc_concat(&ctx->docs, inner_items, 2);
    uint32_t items[] = {
        doc_line(&ctx->docs),
        kw(ctx, keyword),
        doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner),
    };
    return doc_concat(&ctx->docs, items, 3);
}

// ============ Node Formatters ============

static uint32_t format_binary_expr(FmtCtx *ctx, SyntaqliteBinaryExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->left);
    uint32_t kw_2 = kw(ctx, " ");
    uint32_t ed_3 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_BINARY_OP_PLUS: ed_3 = kw(ctx, "+"); break;
        case SYNTAQLITE_BINARY_OP_MINUS: ed_3 = kw(ctx, "-"); break;
        case SYNTAQLITE_BINARY_OP_STAR: ed_3 = kw(ctx, "*"); break;
        case SYNTAQLITE_BINARY_OP_SLASH: ed_3 = kw(ctx, "/"); break;
        case SYNTAQLITE_BINARY_OP_REM: ed_3 = kw(ctx, "%"); break;
        case SYNTAQLITE_BINARY_OP_LT: ed_3 = kw(ctx, "<"); break;
        case SYNTAQLITE_BINARY_OP_GT: ed_3 = kw(ctx, ">"); break;
        case SYNTAQLITE_BINARY_OP_LE: ed_3 = kw(ctx, "<="); break;
        case SYNTAQLITE_BINARY_OP_GE: ed_3 = kw(ctx, ">="); break;
        case SYNTAQLITE_BINARY_OP_EQ: ed_3 = kw(ctx, "="); break;
        case SYNTAQLITE_BINARY_OP_NE: ed_3 = kw(ctx, "!="); break;
        case SYNTAQLITE_BINARY_OP_AND: ed_3 = kw(ctx, "AND"); break;
        case SYNTAQLITE_BINARY_OP_OR: ed_3 = kw(ctx, "OR"); break;
        case SYNTAQLITE_BINARY_OP_BITAND: ed_3 = kw(ctx, "&"); break;
        case SYNTAQLITE_BINARY_OP_BITOR: ed_3 = kw(ctx, "|"); break;
        case SYNTAQLITE_BINARY_OP_LSHIFT: ed_3 = kw(ctx, "<<"); break;
        case SYNTAQLITE_BINARY_OP_RSHIFT: ed_3 = kw(ctx, ">>"); break;
        case SYNTAQLITE_BINARY_OP_CONCAT: ed_3 = kw(ctx, "||"); break;
        case SYNTAQLITE_BINARY_OP_PTR: ed_3 = kw(ctx, "->"); break;
        default: break;
    }
    uint32_t ln_4 = doc_line(&ctx->docs);
    uint32_t ch_5 = format_node(ctx, node->right);
    uint32_t _buf_6[5];
    uint32_t _n_7 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ch_1;
    if (kw_2 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_2;
    if (ed_3 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ed_3;
    if (ln_4 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ln_4;
    if (ch_5 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ch_5;
    uint32_t cat_8 = doc_concat(&ctx->docs, _buf_6, _n_7);
    uint32_t grp_9 = doc_group(&ctx->docs, cat_8);
    return grp_9;
}

static uint32_t format_unary_expr(FmtCtx *ctx, SyntaqliteUnaryExpr *node) {
    uint32_t ed_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_UNARY_OP_MINUS: ed_1 = kw(ctx, "-"); break;
        case SYNTAQLITE_UNARY_OP_PLUS: ed_1 = kw(ctx, "+"); break;
        case SYNTAQLITE_UNARY_OP_BITNOT: ed_1 = kw(ctx, "~"); break;
        case SYNTAQLITE_UNARY_OP_NOT: ed_1 = kw(ctx, "NOT "); break;
        default: break;
    }
    uint32_t ch_2 = format_node(ctx, node->operand);
    uint32_t _buf_3[2];
    uint32_t _n_4 = 0;
    if (ed_1 != SYNTAQLITE_NULL_DOC) _buf_3[_n_4++] = ed_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_3[_n_4++] = ch_2;
    uint32_t cat_5 = doc_concat(&ctx->docs, _buf_3, _n_4);
    return cat_5;
}

static uint32_t format_literal(FmtCtx *ctx, SyntaqliteLiteral *node) {
    uint32_t sp_1 = span_text(ctx, node->source);
    return sp_1;
}

static uint32_t format_result_column(FmtCtx *ctx, SyntaqliteResultColumn *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->flags.star) {
        uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
        if (node->expr != SYNTAQLITE_NULL_NODE) {
            uint32_t ch_3 = format_node(ctx, node->expr);
            uint32_t kw_4 = kw(ctx, ".*");
            uint32_t _buf_5[2];
            uint32_t _n_6 = 0;
            if (ch_3 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = ch_3;
            if (kw_4 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = kw_4;
            uint32_t cat_7 = doc_concat(&ctx->docs, _buf_5, _n_6);
            cond_2 = cat_7;
        } else {
            uint32_t kw_8 = kw(ctx, "*");
            cond_2 = kw_8;
        }
        cond_1 = cond_2;
    } else {
        uint32_t ch_9 = format_node(ctx, node->expr);
        cond_1 = ch_9;
    }
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t kw_11 = kw(ctx, " AS ");
        uint32_t sp_12 = span_text(ctx, node->alias);
        uint32_t _buf_13[2];
        uint32_t _n_14 = 0;
        if (kw_11 != SYNTAQLITE_NULL_DOC) _buf_13[_n_14++] = kw_11;
        if (sp_12 != SYNTAQLITE_NULL_DOC) _buf_13[_n_14++] = sp_12;
        uint32_t cat_15 = doc_concat(&ctx->docs, _buf_13, _n_14);
        cond_10 = cat_15;
    }
    uint32_t _buf_16[2];
    uint32_t _n_17 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = cond_1;
    if (cond_10 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = cond_10;
    uint32_t cat_18 = doc_concat(&ctx->docs, _buf_16, _n_17);
    return cat_18;
}

static uint32_t format_select_stmt(FmtCtx *ctx, SyntaqliteSelectStmt *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) {
        uint32_t kw_2 = kw(ctx, "SELECT DISTINCT");
        cond_1 = kw_2;
    } else {
        uint32_t kw_3 = kw(ctx, "SELECT");
        cond_1 = kw_3;
    }
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t ln_5 = doc_line(&ctx->docs);
        uint32_t ch_6 = format_node(ctx, node->columns);
        uint32_t _buf_7[2];
        uint32_t _n_8 = 0;
        if (ln_5 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = ln_5;
        if (ch_6 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = ch_6;
        uint32_t cat_9 = doc_concat(&ctx->docs, _buf_7, _n_8);
        uint32_t nst_10 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_9);
        uint32_t grp_11 = doc_group(&ctx->docs, nst_10);
        cond_4 = grp_11;
    }
    uint32_t cond_12 = SYNTAQLITE_NULL_DOC;
    if (node->from_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_13 = format_node(ctx, node->from_clause);
        uint32_t cl_14 = format_clause(ctx, "FROM", ch_13);
        cond_12 = cl_14;
    }
    uint32_t cond_15 = SYNTAQLITE_NULL_DOC;
    if (node->where != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_16 = format_node(ctx, node->where);
        uint32_t cl_17 = format_clause(ctx, "WHERE", ch_16);
        cond_15 = cl_17;
    }
    uint32_t cond_18 = SYNTAQLITE_NULL_DOC;
    if (node->groupby != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_19 = format_node(ctx, node->groupby);
        uint32_t cl_20 = format_clause(ctx, "GROUP BY", ch_19);
        cond_18 = cl_20;
    }
    uint32_t cond_21 = SYNTAQLITE_NULL_DOC;
    if (node->having != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_22 = format_node(ctx, node->having);
        uint32_t cl_23 = format_clause(ctx, "HAVING", ch_22);
        cond_21 = cl_23;
    }
    uint32_t cond_24 = SYNTAQLITE_NULL_DOC;
    if (node->orderby != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_25 = format_node(ctx, node->orderby);
        uint32_t cl_26 = format_clause(ctx, "ORDER BY", ch_25);
        cond_24 = cl_26;
    }
    uint32_t cond_27 = SYNTAQLITE_NULL_DOC;
    if (node->limit_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_28 = format_node(ctx, node->limit_clause);
        uint32_t cl_29 = format_clause(ctx, "LIMIT", ch_28);
        cond_27 = cl_29;
    }
    uint32_t cond_30 = SYNTAQLITE_NULL_DOC;
    if (node->window_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_31 = format_node(ctx, node->window_clause);
        uint32_t cl_32 = format_clause(ctx, "WINDOW", ch_31);
        cond_30 = cl_32;
    }
    uint32_t _buf_33[9];
    uint32_t _n_34 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_1;
    if (cond_4 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_4;
    if (cond_12 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_12;
    if (cond_15 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_15;
    if (cond_18 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_18;
    if (cond_21 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_21;
    if (cond_24 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_24;
    if (cond_27 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_27;
    if (cond_30 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = cond_30;
    uint32_t cat_35 = doc_concat(&ctx->docs, _buf_33, _n_34);
    uint32_t grp_36 = doc_group(&ctx->docs, cat_35);
    return grp_36;
}

static uint32_t format_ordering_term(FmtCtx *ctx, SyntaqliteOrderingTerm *node) {
    uint32_t ch_1 = format_node(ctx, node->expr);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->sort_order == SYNTAQLITE_SORT_ORDER_DESC) {
        uint32_t kw_3 = kw(ctx, " DESC");
        cond_2 = kw_3;
    }
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->nulls_order == SYNTAQLITE_NULLS_ORDER_FIRST) {
        uint32_t kw_5 = kw(ctx, " NULLS FIRST");
        cond_4 = kw_5;
    }
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->nulls_order == SYNTAQLITE_NULLS_ORDER_LAST) {
        uint32_t kw_7 = kw(ctx, " NULLS LAST");
        cond_6 = kw_7;
    }
    uint32_t _buf_8[4];
    uint32_t _n_9 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_2;
    if (cond_4 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_4;
    if (cond_6 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_6;
    uint32_t cat_10 = doc_concat(&ctx->docs, _buf_8, _n_9);
    return cat_10;
}

static uint32_t format_limit_clause(FmtCtx *ctx, SyntaqliteLimitClause *node) {
    uint32_t ch_1 = format_node(ctx, node->limit);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->offset != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_3 = kw(ctx, " OFFSET ");
        uint32_t ch_4 = format_node(ctx, node->offset);
        uint32_t _buf_5[2];
        uint32_t _n_6 = 0;
        if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = kw_3;
        if (ch_4 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = ch_4;
        uint32_t cat_7 = doc_concat(&ctx->docs, _buf_5, _n_6);
        cond_2 = cat_7;
    }
    uint32_t _buf_8[2];
    uint32_t _n_9 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_2;
    uint32_t cat_10 = doc_concat(&ctx->docs, _buf_8, _n_9);
    return cat_10;
}

static uint32_t format_column_ref(FmtCtx *ctx, SyntaqliteColumnRef *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_2 = span_text(ctx, node->schema);
        uint32_t kw_3 = kw(ctx, ".");
        uint32_t _buf_4[2];
        uint32_t _n_5 = 0;
        if (sp_2 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = sp_2;
        if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_3;
        uint32_t cat_6 = doc_concat(&ctx->docs, _buf_4, _n_5);
        cond_1 = cat_6;
    }
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->table.length > 0) {
        uint32_t sp_8 = span_text(ctx, node->table);
        uint32_t kw_9 = kw(ctx, ".");
        uint32_t _buf_10[2];
        uint32_t _n_11 = 0;
        if (sp_8 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = sp_8;
        if (kw_9 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = kw_9;
        uint32_t cat_12 = doc_concat(&ctx->docs, _buf_10, _n_11);
        cond_7 = cat_12;
    }
    uint32_t sp_13 = span_text(ctx, node->column);
    uint32_t _buf_14[3];
    uint32_t _n_15 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = cond_1;
    if (cond_7 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = cond_7;
    if (sp_13 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = sp_13;
    uint32_t cat_16 = doc_concat(&ctx->docs, _buf_14, _n_15);
    return cat_16;
}

static uint32_t format_function_call(FmtCtx *ctx, SyntaqliteFunctionCall *node) {
    uint32_t sp_1 = span_text(ctx, node->func_name);
    uint32_t kw_2 = kw(ctx, "(");
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) {
        uint32_t kw_4 = kw(ctx, "DISTINCT ");
        cond_3 = kw_4;
    }
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->flags.star) {
        uint32_t kw_6 = kw(ctx, "*");
        cond_5 = kw_6;
    } else {
        uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
        if (node->args != SYNTAQLITE_NULL_NODE) {
            uint32_t sl_8 = doc_softline(&ctx->docs);
            uint32_t ch_9 = format_node(ctx, node->args);
            uint32_t _buf_10[2];
            uint32_t _n_11 = 0;
            if (sl_8 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = sl_8;
            if (ch_9 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = ch_9;
            uint32_t cat_12 = doc_concat(&ctx->docs, _buf_10, _n_11);
            uint32_t nst_13 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_12);
            uint32_t grp_14 = doc_group(&ctx->docs, nst_13);
            uint32_t sl_15 = doc_softline(&ctx->docs);
            uint32_t _buf_16[2];
            uint32_t _n_17 = 0;
            if (grp_14 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = grp_14;
            if (sl_15 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = sl_15;
            uint32_t cat_18 = doc_concat(&ctx->docs, _buf_16, _n_17);
            cond_7 = cat_18;
        }
        cond_5 = cond_7;
    }
    uint32_t kw_19 = kw(ctx, ")");
    uint32_t cond_20 = SYNTAQLITE_NULL_DOC;
    if (node->filter_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_21 = kw(ctx, " ");
        uint32_t ch_22 = format_node(ctx, node->filter_clause);
        uint32_t _buf_23[2];
        uint32_t _n_24 = 0;
        if (kw_21 != SYNTAQLITE_NULL_DOC) _buf_23[_n_24++] = kw_21;
        if (ch_22 != SYNTAQLITE_NULL_DOC) _buf_23[_n_24++] = ch_22;
        uint32_t cat_25 = doc_concat(&ctx->docs, _buf_23, _n_24);
        cond_20 = cat_25;
    }
    uint32_t cond_26 = SYNTAQLITE_NULL_DOC;
    if (node->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_27 = kw(ctx, " ");
        uint32_t ch_28 = format_node(ctx, node->over_clause);
        uint32_t _buf_29[2];
        uint32_t _n_30 = 0;
        if (kw_27 != SYNTAQLITE_NULL_DOC) _buf_29[_n_30++] = kw_27;
        if (ch_28 != SYNTAQLITE_NULL_DOC) _buf_29[_n_30++] = ch_28;
        uint32_t cat_31 = doc_concat(&ctx->docs, _buf_29, _n_30);
        cond_26 = cat_31;
    }
    uint32_t _buf_32[7];
    uint32_t _n_33 = 0;
    if (sp_1 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = sp_1;
    if (kw_2 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = kw_2;
    if (cond_3 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = cond_3;
    if (cond_5 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = cond_5;
    if (kw_19 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = kw_19;
    if (cond_20 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = cond_20;
    if (cond_26 != SYNTAQLITE_NULL_DOC) _buf_32[_n_33++] = cond_26;
    uint32_t cat_34 = doc_concat(&ctx->docs, _buf_32, _n_33);
    return cat_34;
}

static uint32_t format_is_expr(FmtCtx *ctx, SyntaqliteIsExpr *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_IS_OP_ISNULL: {
            uint32_t ch_2 = format_node(ctx, node->left);
            uint32_t kw_3 = kw(ctx, " ISNULL");
            uint32_t _buf_4[2];
            uint32_t _n_5 = 0;
            if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = ch_2;
            if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_3;
            uint32_t cat_6 = doc_concat(&ctx->docs, _buf_4, _n_5);
            sw_1 = cat_6;
            break;
        }
        case SYNTAQLITE_IS_OP_NOTNULL: {
            uint32_t ch_7 = format_node(ctx, node->left);
            uint32_t kw_8 = kw(ctx, " NOTNULL");
            uint32_t _buf_9[2];
            uint32_t _n_10 = 0;
            if (ch_7 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = ch_7;
            if (kw_8 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = kw_8;
            uint32_t cat_11 = doc_concat(&ctx->docs, _buf_9, _n_10);
            sw_1 = cat_11;
            break;
        }
        case SYNTAQLITE_IS_OP_IS: {
            uint32_t ch_12 = format_node(ctx, node->left);
            uint32_t kw_13 = kw(ctx, " IS ");
            uint32_t ch_14 = format_node(ctx, node->right);
            uint32_t _buf_15[3];
            uint32_t _n_16 = 0;
            if (ch_12 != SYNTAQLITE_NULL_DOC) _buf_15[_n_16++] = ch_12;
            if (kw_13 != SYNTAQLITE_NULL_DOC) _buf_15[_n_16++] = kw_13;
            if (ch_14 != SYNTAQLITE_NULL_DOC) _buf_15[_n_16++] = ch_14;
            uint32_t cat_17 = doc_concat(&ctx->docs, _buf_15, _n_16);
            sw_1 = cat_17;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_NOT: {
            uint32_t ch_18 = format_node(ctx, node->left);
            uint32_t kw_19 = kw(ctx, " IS NOT ");
            uint32_t ch_20 = format_node(ctx, node->right);
            uint32_t _buf_21[3];
            uint32_t _n_22 = 0;
            if (ch_18 != SYNTAQLITE_NULL_DOC) _buf_21[_n_22++] = ch_18;
            if (kw_19 != SYNTAQLITE_NULL_DOC) _buf_21[_n_22++] = kw_19;
            if (ch_20 != SYNTAQLITE_NULL_DOC) _buf_21[_n_22++] = ch_20;
            uint32_t cat_23 = doc_concat(&ctx->docs, _buf_21, _n_22);
            sw_1 = cat_23;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_NOT_DISTINCT: {
            uint32_t ch_24 = format_node(ctx, node->left);
            uint32_t kw_25 = kw(ctx, " IS NOT DISTINCT FROM ");
            uint32_t ch_26 = format_node(ctx, node->right);
            uint32_t _buf_27[3];
            uint32_t _n_28 = 0;
            if (ch_24 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = ch_24;
            if (kw_25 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = kw_25;
            if (ch_26 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = ch_26;
            uint32_t cat_29 = doc_concat(&ctx->docs, _buf_27, _n_28);
            sw_1 = cat_29;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_DISTINCT: {
            uint32_t ch_30 = format_node(ctx, node->left);
            uint32_t kw_31 = kw(ctx, " IS DISTINCT FROM ");
            uint32_t ch_32 = format_node(ctx, node->right);
            uint32_t _buf_33[3];
            uint32_t _n_34 = 0;
            if (ch_30 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = ch_30;
            if (kw_31 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = kw_31;
            if (ch_32 != SYNTAQLITE_NULL_DOC) _buf_33[_n_34++] = ch_32;
            uint32_t cat_35 = doc_concat(&ctx->docs, _buf_33, _n_34);
            sw_1 = cat_35;
            break;
        }
        default: break;
    }
    return sw_1;
}

static uint32_t format_between_expr(FmtCtx *ctx, SyntaqliteBetweenExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->operand);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->negated) {
        uint32_t kw_3 = kw(ctx, " NOT BETWEEN ");
        cond_2 = kw_3;
    } else {
        uint32_t kw_4 = kw(ctx, " BETWEEN ");
        cond_2 = kw_4;
    }
    uint32_t ch_5 = format_node(ctx, node->low);
    uint32_t kw_6 = kw(ctx, " AND ");
    uint32_t ch_7 = format_node(ctx, node->high);
    uint32_t _buf_8[5];
    uint32_t _n_9 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_2;
    if (ch_5 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_5;
    if (kw_6 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = kw_6;
    if (ch_7 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_7;
    uint32_t cat_10 = doc_concat(&ctx->docs, _buf_8, _n_9);
    return cat_10;
}

static uint32_t format_like_expr(FmtCtx *ctx, SyntaqliteLikeExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->operand);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->negated) {
        uint32_t kw_3 = kw(ctx, " NOT LIKE ");
        cond_2 = kw_3;
    } else {
        uint32_t kw_4 = kw(ctx, " LIKE ");
        cond_2 = kw_4;
    }
    uint32_t ch_5 = format_node(ctx, node->pattern);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->escape != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_7 = kw(ctx, " ESCAPE ");
        uint32_t ch_8 = format_node(ctx, node->escape);
        uint32_t _buf_9[2];
        uint32_t _n_10 = 0;
        if (kw_7 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = kw_7;
        if (ch_8 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = ch_8;
        uint32_t cat_11 = doc_concat(&ctx->docs, _buf_9, _n_10);
        cond_6 = cat_11;
    }
    uint32_t _buf_12[4];
    uint32_t _n_13 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = ch_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = cond_2;
    if (ch_5 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = ch_5;
    if (cond_6 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = cond_6;
    uint32_t cat_14 = doc_concat(&ctx->docs, _buf_12, _n_13);
    return cat_14;
}

static uint32_t format_case_expr(FmtCtx *ctx, SyntaqliteCaseExpr *node) {
    uint32_t kw_1 = kw(ctx, "CASE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->operand != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_3 = kw(ctx, " ");
        uint32_t ch_4 = format_node(ctx, node->operand);
        uint32_t _buf_5[2];
        uint32_t _n_6 = 0;
        if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = kw_3;
        if (ch_4 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = ch_4;
        uint32_t cat_7 = doc_concat(&ctx->docs, _buf_5, _n_6);
        cond_2 = cat_7;
    }
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->whens != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_9 = format_node(ctx, node->whens);
        cond_8 = ch_9;
    }
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->else_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_11 = kw(ctx, " ELSE ");
        uint32_t ch_12 = format_node(ctx, node->else_expr);
        uint32_t _buf_13[2];
        uint32_t _n_14 = 0;
        if (kw_11 != SYNTAQLITE_NULL_DOC) _buf_13[_n_14++] = kw_11;
        if (ch_12 != SYNTAQLITE_NULL_DOC) _buf_13[_n_14++] = ch_12;
        uint32_t cat_15 = doc_concat(&ctx->docs, _buf_13, _n_14);
        cond_10 = cat_15;
    }
    uint32_t kw_16 = kw(ctx, " END");
    uint32_t _buf_17[5];
    uint32_t _n_18 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = kw_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = cond_2;
    if (cond_8 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = cond_8;
    if (cond_10 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = cond_10;
    if (kw_16 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = kw_16;
    uint32_t cat_19 = doc_concat(&ctx->docs, _buf_17, _n_18);
    return cat_19;
}

static uint32_t format_case_when(FmtCtx *ctx, SyntaqliteCaseWhen *node) {
    uint32_t kw_1 = kw(ctx, " WHEN ");
    uint32_t ch_2 = format_node(ctx, node->when_expr);
    uint32_t kw_3 = kw(ctx, " THEN ");
    uint32_t ch_4 = format_node(ctx, node->then_expr);
    uint32_t _buf_5[4];
    uint32_t _n_6 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = kw_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = ch_2;
    if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = kw_3;
    if (ch_4 != SYNTAQLITE_NULL_DOC) _buf_5[_n_6++] = ch_4;
    uint32_t cat_7 = doc_concat(&ctx->docs, _buf_5, _n_6);
    return cat_7;
}

static uint32_t format_case_when_list(FmtCtx *ctx, SyntaqliteCaseWhenList *node) {
    uint32_t _buf_1[node->count > 0 ? node->count : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        uint32_t item_4 = format_node(ctx, _child_id);
        _buf_1[_n_2++] = item_4;
    }
    uint32_t lst_3 = doc_concat(&ctx->docs, _buf_1, _n_2);
    return lst_3;
}

static uint32_t format_compound_select(FmtCtx *ctx, SyntaqliteCompoundSelect *node) {
    uint32_t ch_1 = format_node(ctx, node->left);
    uint32_t hl_2 = doc_hardline(&ctx->docs);
    uint32_t ed_3 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_COMPOUND_OP_UNION: ed_3 = kw(ctx, "UNION"); break;
        case SYNTAQLITE_COMPOUND_OP_UNION_ALL: ed_3 = kw(ctx, "UNION ALL"); break;
        case SYNTAQLITE_COMPOUND_OP_INTERSECT: ed_3 = kw(ctx, "INTERSECT"); break;
        case SYNTAQLITE_COMPOUND_OP_EXCEPT: ed_3 = kw(ctx, "EXCEPT"); break;
        default: break;
    }
    uint32_t hl_4 = doc_hardline(&ctx->docs);
    uint32_t ch_5 = format_node(ctx, node->right);
    uint32_t _buf_6[5];
    uint32_t _n_7 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ch_1;
    if (hl_2 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = hl_2;
    if (ed_3 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ed_3;
    if (hl_4 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = hl_4;
    if (ch_5 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ch_5;
    uint32_t cat_8 = doc_concat(&ctx->docs, _buf_6, _n_7);
    return cat_8;
}

static uint32_t format_subquery_expr(FmtCtx *ctx, SyntaqliteSubqueryExpr *node) {
    uint32_t kw_1 = kw(ctx, "(");
    uint32_t ch_2 = format_node(ctx, node->select);
    uint32_t kw_3 = kw(ctx, ")");
    uint32_t _buf_4[3];
    uint32_t _n_5 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = ch_2;
    if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_3;
    uint32_t cat_6 = doc_concat(&ctx->docs, _buf_4, _n_5);
    return cat_6;
}

static uint32_t format_exists_expr(FmtCtx *ctx, SyntaqliteExistsExpr *node) {
    uint32_t kw_1 = kw(ctx, "EXISTS (");
    uint32_t ch_2 = format_node(ctx, node->select);
    uint32_t kw_3 = kw(ctx, ")");
    uint32_t _buf_4[3];
    uint32_t _n_5 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = ch_2;
    if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_3;
    uint32_t cat_6 = doc_concat(&ctx->docs, _buf_4, _n_5);
    return cat_6;
}

static uint32_t format_in_expr(FmtCtx *ctx, SyntaqliteInExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->operand);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->negated) {
        uint32_t kw_3 = kw(ctx, " NOT IN ");
        cond_2 = kw_3;
    } else {
        uint32_t kw_4 = kw(ctx, " IN ");
        cond_2 = kw_4;
    }
    uint32_t kw_5 = kw(ctx, "(");
    uint32_t ch_6 = format_node(ctx, node->source);
    uint32_t kw_7 = kw(ctx, ")");
    uint32_t _buf_8[5];
    uint32_t _n_9 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_2;
    if (kw_5 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = kw_5;
    if (ch_6 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_6;
    if (kw_7 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = kw_7;
    uint32_t cat_10 = doc_concat(&ctx->docs, _buf_8, _n_9);
    return cat_10;
}

static uint32_t format_variable(FmtCtx *ctx, SyntaqliteVariable *node) {
    uint32_t sp_1 = span_text(ctx, node->source);
    return sp_1;
}

static uint32_t format_collate_expr(FmtCtx *ctx, SyntaqliteCollateExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->expr);
    uint32_t kw_2 = kw(ctx, " COLLATE ");
    uint32_t sp_3 = span_text(ctx, node->collation);
    uint32_t _buf_4[3];
    uint32_t _n_5 = 0;
    if (ch_1 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = ch_1;
    if (kw_2 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_2;
    if (sp_3 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = sp_3;
    uint32_t cat_6 = doc_concat(&ctx->docs, _buf_4, _n_5);
    return cat_6;
}

static uint32_t format_cast_expr(FmtCtx *ctx, SyntaqliteCastExpr *node) {
    uint32_t kw_1 = kw(ctx, "CAST(");
    uint32_t ch_2 = format_node(ctx, node->expr);
    uint32_t kw_3 = kw(ctx, " AS ");
    uint32_t sp_4 = span_text(ctx, node->type_name);
    uint32_t kw_5 = kw(ctx, ")");
    uint32_t _buf_6[5];
    uint32_t _n_7 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ch_2;
    if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_3;
    if (sp_4 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = sp_4;
    if (kw_5 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_5;
    uint32_t cat_8 = doc_concat(&ctx->docs, _buf_6, _n_7);
    return cat_8;
}

static uint32_t format_values_row_list(FmtCtx *ctx, SyntaqliteValuesRowList *node) {
    uint32_t _buf_1[node->count * 2 > 0 ? node->count * 2 : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t kw_4 = kw(ctx, ",");
            uint32_t ln_5 = doc_line(&ctx->docs);
            uint32_t _buf_6[2];
            uint32_t _n_7 = 0;
            if (kw_4 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_4;
            if (ln_5 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ln_5;
            uint32_t cat_8 = doc_concat(&ctx->docs, _buf_6, _n_7);
            _buf_1[_n_2++] = cat_8;
        }
        uint32_t kw_9 = kw(ctx, "(");
        uint32_t item_10 = format_node(ctx, _child_id);
        uint32_t kw_11 = kw(ctx, ")");
        uint32_t _buf_12[3];
        uint32_t _n_13 = 0;
        if (kw_9 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = kw_9;
        if (item_10 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = item_10;
        if (kw_11 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = kw_11;
        uint32_t cat_14 = doc_concat(&ctx->docs, _buf_12, _n_13);
        _buf_1[_n_2++] = cat_14;
    }
    uint32_t lst_3 = doc_concat(&ctx->docs, _buf_1, _n_2);
    return lst_3;
}

static uint32_t format_values_clause(FmtCtx *ctx, SyntaqliteValuesClause *node) {
    uint32_t kw_1 = kw(ctx, "VALUES ");
    uint32_t ch_2 = format_node(ctx, node->rows);
    uint32_t _buf_3[2];
    uint32_t _n_4 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_3[_n_4++] = kw_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_3[_n_4++] = ch_2;
    uint32_t cat_5 = doc_concat(&ctx->docs, _buf_3, _n_4);
    return cat_5;
}

static uint32_t format_cte_definition(FmtCtx *ctx, SyntaqliteCteDefinition *node) {
    uint32_t sp_1 = span_text(ctx, node->cte_name);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_3 = kw(ctx, "(");
        uint32_t ch_4 = format_node(ctx, node->columns);
        uint32_t kw_5 = kw(ctx, ")");
        uint32_t _buf_6[3];
        uint32_t _n_7 = 0;
        if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_3;
        if (ch_4 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = ch_4;
        if (kw_5 != SYNTAQLITE_NULL_DOC) _buf_6[_n_7++] = kw_5;
        uint32_t cat_8 = doc_concat(&ctx->docs, _buf_6, _n_7);
        cond_2 = cat_8;
    }
    uint32_t kw_9 = kw(ctx, " AS ");
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->materialized == SYNTAQLITE_MATERIALIZED_MATERIALIZED) {
        uint32_t kw_11 = kw(ctx, "MATERIALIZED ");
        cond_10 = kw_11;
    }
    uint32_t cond_12 = SYNTAQLITE_NULL_DOC;
    if (node->materialized == SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED) {
        uint32_t kw_13 = kw(ctx, "NOT MATERIALIZED ");
        cond_12 = kw_13;
    }
    uint32_t kw_14 = kw(ctx, "(");
    uint32_t ch_15 = format_node(ctx, node->select);
    uint32_t kw_16 = kw(ctx, ")");
    uint32_t _buf_17[8];
    uint32_t _n_18 = 0;
    if (sp_1 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = sp_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = cond_2;
    if (kw_9 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = kw_9;
    if (cond_10 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = cond_10;
    if (cond_12 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = cond_12;
    if (kw_14 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = kw_14;
    if (ch_15 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = ch_15;
    if (kw_16 != SYNTAQLITE_NULL_DOC) _buf_17[_n_18++] = kw_16;
    uint32_t cat_19 = doc_concat(&ctx->docs, _buf_17, _n_18);
    return cat_19;
}

static uint32_t format_with_clause(FmtCtx *ctx, SyntaqliteWithClause *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->recursive) {
        uint32_t kw_2 = kw(ctx, "WITH RECURSIVE ");
        cond_1 = kw_2;
    } else {
        uint32_t kw_3 = kw(ctx, "WITH ");
        cond_1 = kw_3;
    }
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->ctes != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_5 = format_node(ctx, node->ctes);
        cond_4 = ch_5;
    }
    uint32_t hl_6 = doc_hardline(&ctx->docs);
    uint32_t ch_7 = format_node(ctx, node->select);
    uint32_t _buf_8[4];
    uint32_t _n_9 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_1;
    if (cond_4 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_4;
    if (hl_6 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = hl_6;
    if (ch_7 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = ch_7;
    uint32_t cat_10 = doc_concat(&ctx->docs, _buf_8, _n_9);
    return cat_10;
}

static uint32_t format_aggregate_function_call(FmtCtx *ctx, SyntaqliteAggregateFunctionCall *node) {
    uint32_t sp_1 = span_text(ctx, node->func_name);
    uint32_t kw_2 = kw(ctx, "(");
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) {
        uint32_t kw_4 = kw(ctx, "DISTINCT ");
        cond_3 = kw_4;
    }
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->args != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_6 = format_node(ctx, node->args);
        uint32_t grp_7 = doc_group(&ctx->docs, ch_6);
        cond_5 = grp_7;
    }
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->orderby != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_9 = kw(ctx, " ORDER BY ");
        uint32_t ch_10 = format_node(ctx, node->orderby);
        uint32_t _buf_11[2];
        uint32_t _n_12 = 0;
        if (kw_9 != SYNTAQLITE_NULL_DOC) _buf_11[_n_12++] = kw_9;
        if (ch_10 != SYNTAQLITE_NULL_DOC) _buf_11[_n_12++] = ch_10;
        uint32_t cat_13 = doc_concat(&ctx->docs, _buf_11, _n_12);
        cond_8 = cat_13;
    }
    uint32_t kw_14 = kw(ctx, ")");
    uint32_t cond_15 = SYNTAQLITE_NULL_DOC;
    if (node->filter_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_16 = kw(ctx, " ");
        uint32_t ch_17 = format_node(ctx, node->filter_clause);
        uint32_t _buf_18[2];
        uint32_t _n_19 = 0;
        if (kw_16 != SYNTAQLITE_NULL_DOC) _buf_18[_n_19++] = kw_16;
        if (ch_17 != SYNTAQLITE_NULL_DOC) _buf_18[_n_19++] = ch_17;
        uint32_t cat_20 = doc_concat(&ctx->docs, _buf_18, _n_19);
        cond_15 = cat_20;
    }
    uint32_t cond_21 = SYNTAQLITE_NULL_DOC;
    if (node->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_22 = kw(ctx, " ");
        uint32_t ch_23 = format_node(ctx, node->over_clause);
        uint32_t _buf_24[2];
        uint32_t _n_25 = 0;
        if (kw_22 != SYNTAQLITE_NULL_DOC) _buf_24[_n_25++] = kw_22;
        if (ch_23 != SYNTAQLITE_NULL_DOC) _buf_24[_n_25++] = ch_23;
        uint32_t cat_26 = doc_concat(&ctx->docs, _buf_24, _n_25);
        cond_21 = cat_26;
    }
    uint32_t _buf_27[8];
    uint32_t _n_28 = 0;
    if (sp_1 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = sp_1;
    if (kw_2 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = kw_2;
    if (cond_3 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_3;
    if (cond_5 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_5;
    if (cond_8 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_8;
    if (kw_14 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = kw_14;
    if (cond_15 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_15;
    if (cond_21 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_21;
    uint32_t cat_29 = doc_concat(&ctx->docs, _buf_27, _n_28);
    return cat_29;
}

static uint32_t format_raise_expr(FmtCtx *ctx, SyntaqliteRaiseExpr *node) {
    uint32_t kw_1 = kw(ctx, "RAISE(");
    uint32_t sw_2 = SYNTAQLITE_NULL_DOC;
    switch (node->raise_type) {
        case SYNTAQLITE_RAISE_TYPE_IGNORE: {
            uint32_t kw_3 = kw(ctx, "IGNORE");
            sw_2 = kw_3;
            break;
        }
        case SYNTAQLITE_RAISE_TYPE_ROLLBACK: {
            uint32_t kw_4 = kw(ctx, "ROLLBACK");
            sw_2 = kw_4;
            break;
        }
        case SYNTAQLITE_RAISE_TYPE_ABORT: {
            uint32_t kw_5 = kw(ctx, "ABORT");
            sw_2 = kw_5;
            break;
        }
        case SYNTAQLITE_RAISE_TYPE_FAIL: {
            uint32_t kw_6 = kw(ctx, "FAIL");
            sw_2 = kw_6;
            break;
        }
        default: break;
    }
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->error_message != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_8 = kw(ctx, ", ");
        uint32_t ch_9 = format_node(ctx, node->error_message);
        uint32_t _buf_10[2];
        uint32_t _n_11 = 0;
        if (kw_8 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = kw_8;
        if (ch_9 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = ch_9;
        uint32_t cat_12 = doc_concat(&ctx->docs, _buf_10, _n_11);
        cond_7 = cat_12;
    }
    uint32_t kw_13 = kw(ctx, ")");
    uint32_t _buf_14[4];
    uint32_t _n_15 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = kw_1;
    if (sw_2 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = sw_2;
    if (cond_7 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = cond_7;
    if (kw_13 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = kw_13;
    uint32_t cat_16 = doc_concat(&ctx->docs, _buf_14, _n_15);
    return cat_16;
}

static uint32_t format_table_ref(FmtCtx *ctx, SyntaqliteTableRef *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_2 = span_text(ctx, node->schema);
        uint32_t kw_3 = kw(ctx, ".");
        uint32_t _buf_4[2];
        uint32_t _n_5 = 0;
        if (sp_2 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = sp_2;
        if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_4[_n_5++] = kw_3;
        uint32_t cat_6 = doc_concat(&ctx->docs, _buf_4, _n_5);
        cond_1 = cat_6;
    }
    uint32_t sp_7 = span_text(ctx, node->table_name);
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t kw_9 = kw(ctx, " AS ");
        uint32_t sp_10 = span_text(ctx, node->alias);
        uint32_t _buf_11[2];
        uint32_t _n_12 = 0;
        if (kw_9 != SYNTAQLITE_NULL_DOC) _buf_11[_n_12++] = kw_9;
        if (sp_10 != SYNTAQLITE_NULL_DOC) _buf_11[_n_12++] = sp_10;
        uint32_t cat_13 = doc_concat(&ctx->docs, _buf_11, _n_12);
        cond_8 = cat_13;
    }
    uint32_t _buf_14[3];
    uint32_t _n_15 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = cond_1;
    if (sp_7 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = sp_7;
    if (cond_8 != SYNTAQLITE_NULL_DOC) _buf_14[_n_15++] = cond_8;
    uint32_t cat_16 = doc_concat(&ctx->docs, _buf_14, _n_15);
    return cat_16;
}

static uint32_t format_subquery_table_source(FmtCtx *ctx, SyntaqliteSubqueryTableSource *node) {
    uint32_t kw_1 = kw(ctx, "(");
    uint32_t ch_2 = format_node(ctx, node->select);
    uint32_t kw_3 = kw(ctx, ")");
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t kw_5 = kw(ctx, " AS ");
        uint32_t sp_6 = span_text(ctx, node->alias);
        uint32_t _buf_7[2];
        uint32_t _n_8 = 0;
        if (kw_5 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = kw_5;
        if (sp_6 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = sp_6;
        uint32_t cat_9 = doc_concat(&ctx->docs, _buf_7, _n_8);
        cond_4 = cat_9;
    }
    uint32_t _buf_10[4];
    uint32_t _n_11 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = kw_1;
    if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = ch_2;
    if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = kw_3;
    if (cond_4 != SYNTAQLITE_NULL_DOC) _buf_10[_n_11++] = cond_4;
    uint32_t cat_12 = doc_concat(&ctx->docs, _buf_10, _n_11);
    return cat_12;
}

static uint32_t format_join_clause(FmtCtx *ctx, SyntaqliteJoinClause *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->join_type) {
        case SYNTAQLITE_JOIN_TYPE_COMMA: {
            uint32_t ch_2 = format_node(ctx, node->left);
            uint32_t kw_3 = kw(ctx, ", ");
            uint32_t ch_4 = format_node(ctx, node->right);
            uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
            if (node->on_expr != SYNTAQLITE_NULL_NODE) {
                uint32_t hl_6 = doc_hardline(&ctx->docs);
                uint32_t kw_7 = kw(ctx, "ON ");
                uint32_t ch_8 = format_node(ctx, node->on_expr);
                uint32_t _buf_9[3];
                uint32_t _n_10 = 0;
                if (hl_6 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = hl_6;
                if (kw_7 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = kw_7;
                if (ch_8 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = ch_8;
                uint32_t cat_11 = doc_concat(&ctx->docs, _buf_9, _n_10);
                cond_5 = cat_11;
            }
            uint32_t cond_12 = SYNTAQLITE_NULL_DOC;
            if (node->using_columns != SYNTAQLITE_NULL_NODE) {
                uint32_t kw_13 = kw(ctx, " USING (");
                uint32_t ch_14 = format_node(ctx, node->using_columns);
                uint32_t kw_15 = kw(ctx, ")");
                uint32_t _buf_16[3];
                uint32_t _n_17 = 0;
                if (kw_13 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = kw_13;
                if (ch_14 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = ch_14;
                if (kw_15 != SYNTAQLITE_NULL_DOC) _buf_16[_n_17++] = kw_15;
                uint32_t cat_18 = doc_concat(&ctx->docs, _buf_16, _n_17);
                cond_12 = cat_18;
            }
            uint32_t _buf_19[5];
            uint32_t _n_20 = 0;
            if (ch_2 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = ch_2;
            if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = kw_3;
            if (ch_4 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = ch_4;
            if (cond_5 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = cond_5;
            if (cond_12 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = cond_12;
            uint32_t cat_21 = doc_concat(&ctx->docs, _buf_19, _n_20);
            sw_1 = cat_21;
            break;
        }
        default: {
            uint32_t ch_22 = format_node(ctx, node->left);
            uint32_t hl_23 = doc_hardline(&ctx->docs);
            uint32_t ed_24 = SYNTAQLITE_NULL_DOC;
            switch (node->join_type) {
                case SYNTAQLITE_JOIN_TYPE_INNER: ed_24 = kw(ctx, "JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_LEFT: ed_24 = kw(ctx, "LEFT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_RIGHT: ed_24 = kw(ctx, "RIGHT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_FULL: ed_24 = kw(ctx, "FULL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_CROSS: ed_24 = kw(ctx, "CROSS JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_INNER: ed_24 = kw(ctx, "NATURAL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT: ed_24 = kw(ctx, "NATURAL LEFT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT: ed_24 = kw(ctx, "NATURAL RIGHT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_FULL: ed_24 = kw(ctx, "NATURAL FULL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_COMMA: ed_24 = kw(ctx, ","); break;
                default: break;
            }
            uint32_t kw_25 = kw(ctx, " ");
            uint32_t ch_26 = format_node(ctx, node->right);
            uint32_t cond_27 = SYNTAQLITE_NULL_DOC;
            if (node->on_expr != SYNTAQLITE_NULL_NODE) {
                uint32_t hl_28 = doc_hardline(&ctx->docs);
                uint32_t kw_29 = kw(ctx, "ON ");
                uint32_t ch_30 = format_node(ctx, node->on_expr);
                uint32_t _buf_31[3];
                uint32_t _n_32 = 0;
                if (hl_28 != SYNTAQLITE_NULL_DOC) _buf_31[_n_32++] = hl_28;
                if (kw_29 != SYNTAQLITE_NULL_DOC) _buf_31[_n_32++] = kw_29;
                if (ch_30 != SYNTAQLITE_NULL_DOC) _buf_31[_n_32++] = ch_30;
                uint32_t cat_33 = doc_concat(&ctx->docs, _buf_31, _n_32);
                cond_27 = cat_33;
            }
            uint32_t cond_34 = SYNTAQLITE_NULL_DOC;
            if (node->using_columns != SYNTAQLITE_NULL_NODE) {
                uint32_t kw_35 = kw(ctx, " USING (");
                uint32_t ch_36 = format_node(ctx, node->using_columns);
                uint32_t kw_37 = kw(ctx, ")");
                uint32_t _buf_38[3];
                uint32_t _n_39 = 0;
                if (kw_35 != SYNTAQLITE_NULL_DOC) _buf_38[_n_39++] = kw_35;
                if (ch_36 != SYNTAQLITE_NULL_DOC) _buf_38[_n_39++] = ch_36;
                if (kw_37 != SYNTAQLITE_NULL_DOC) _buf_38[_n_39++] = kw_37;
                uint32_t cat_40 = doc_concat(&ctx->docs, _buf_38, _n_39);
                cond_34 = cat_40;
            }
            uint32_t _buf_41[7];
            uint32_t _n_42 = 0;
            if (ch_22 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = ch_22;
            if (hl_23 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = hl_23;
            if (ed_24 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = ed_24;
            if (kw_25 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = kw_25;
            if (ch_26 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = ch_26;
            if (cond_27 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = cond_27;
            if (cond_34 != SYNTAQLITE_NULL_DOC) _buf_41[_n_42++] = cond_34;
            uint32_t cat_43 = doc_concat(&ctx->docs, _buf_41, _n_42);
            sw_1 = cat_43;
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
    uint32_t kw_1 = kw(ctx, "DELETE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->table != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_3 = format_node(ctx, node->table);
        uint32_t cl_4 = format_clause(ctx, "FROM", ch_3);
        cond_2 = cl_4;
    }
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->where != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_6 = format_node(ctx, node->where);
        uint32_t cl_7 = format_clause(ctx, "WHERE", ch_6);
        cond_5 = cl_7;
    }
    uint32_t _buf_8[3];
    uint32_t _n_9 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = kw_1;
    if (cond_2 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_2;
    if (cond_5 != SYNTAQLITE_NULL_DOC) _buf_8[_n_9++] = cond_5;
    uint32_t cat_10 = doc_concat(&ctx->docs, _buf_8, _n_9);
    return cat_10;
}

static uint32_t format_set_clause(FmtCtx *ctx, SyntaqliteSetClause *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->column.length > 0) {
        uint32_t sp_2 = span_text(ctx, node->column);
        cond_1 = sp_2;
    } else {
        uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
        if (node->columns != SYNTAQLITE_NULL_NODE) {
            uint32_t kw_4 = kw(ctx, "(");
            uint32_t ch_5 = format_node(ctx, node->columns);
            uint32_t kw_6 = kw(ctx, ")");
            uint32_t _buf_7[3];
            uint32_t _n_8 = 0;
            if (kw_4 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = kw_4;
            if (ch_5 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = ch_5;
            if (kw_6 != SYNTAQLITE_NULL_DOC) _buf_7[_n_8++] = kw_6;
            uint32_t cat_9 = doc_concat(&ctx->docs, _buf_7, _n_8);
            cond_3 = cat_9;
        }
        cond_1 = cond_3;
    }
    uint32_t kw_10 = kw(ctx, " = ");
    uint32_t ch_11 = format_node(ctx, node->value);
    uint32_t _buf_12[3];
    uint32_t _n_13 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = cond_1;
    if (kw_10 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = kw_10;
    if (ch_11 != SYNTAQLITE_NULL_DOC) _buf_12[_n_13++] = ch_11;
    uint32_t cat_14 = doc_concat(&ctx->docs, _buf_12, _n_13);
    return cat_14;
}

static uint32_t format_update_stmt(FmtCtx *ctx, SyntaqliteUpdateStmt *node) {
    uint32_t kw_1 = kw(ctx, "UPDATE");
    uint32_t sw_2 = SYNTAQLITE_NULL_DOC;
    switch (node->conflict_action) {
        case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: {
            uint32_t kw_3 = kw(ctx, " OR ROLLBACK");
            sw_2 = kw_3;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_ABORT: {
            uint32_t kw_4 = kw(ctx, " OR ABORT");
            sw_2 = kw_4;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_FAIL: {
            uint32_t kw_5 = kw(ctx, " OR FAIL");
            sw_2 = kw_5;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_IGNORE: {
            uint32_t kw_6 = kw(ctx, " OR IGNORE");
            sw_2 = kw_6;
            break;
        }
        case SYNTAQLITE_CONFLICT_ACTION_REPLACE: {
            uint32_t kw_7 = kw(ctx, " OR REPLACE");
            sw_2 = kw_7;
            break;
        }
        default: break;
    }
    uint32_t kw_8 = kw(ctx, " ");
    uint32_t ch_9 = format_node(ctx, node->table);
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->setlist != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_11 = format_node(ctx, node->setlist);
        uint32_t cl_12 = format_clause(ctx, "SET", ch_11);
        cond_10 = cl_12;
    }
    uint32_t cond_13 = SYNTAQLITE_NULL_DOC;
    if (node->from_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_14 = format_node(ctx, node->from_clause);
        uint32_t cl_15 = format_clause(ctx, "FROM", ch_14);
        cond_13 = cl_15;
    }
    uint32_t cond_16 = SYNTAQLITE_NULL_DOC;
    if (node->where != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_17 = format_node(ctx, node->where);
        uint32_t cl_18 = format_clause(ctx, "WHERE", ch_17);
        cond_16 = cl_18;
    }
    uint32_t _buf_19[7];
    uint32_t _n_20 = 0;
    if (kw_1 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = kw_1;
    if (sw_2 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = sw_2;
    if (kw_8 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = kw_8;
    if (ch_9 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = ch_9;
    if (cond_10 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = cond_10;
    if (cond_13 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = cond_13;
    if (cond_16 != SYNTAQLITE_NULL_DOC) _buf_19[_n_20++] = cond_16;
    uint32_t cat_21 = doc_concat(&ctx->docs, _buf_19, _n_20);
    return cat_21;
}

static uint32_t format_insert_stmt(FmtCtx *ctx, SyntaqliteInsertStmt *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->conflict_action == SYNTAQLITE_CONFLICT_ACTION_REPLACE) {
        uint32_t kw_2 = kw(ctx, "REPLACE");
        cond_1 = kw_2;
    } else {
        uint32_t kw_3 = kw(ctx, "INSERT");
        uint32_t sw_4 = SYNTAQLITE_NULL_DOC;
        switch (node->conflict_action) {
            case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: {
                uint32_t kw_5 = kw(ctx, " OR ROLLBACK");
                sw_4 = kw_5;
                break;
            }
            case SYNTAQLITE_CONFLICT_ACTION_ABORT: {
                uint32_t kw_6 = kw(ctx, " OR ABORT");
                sw_4 = kw_6;
                break;
            }
            case SYNTAQLITE_CONFLICT_ACTION_FAIL: {
                uint32_t kw_7 = kw(ctx, " OR FAIL");
                sw_4 = kw_7;
                break;
            }
            case SYNTAQLITE_CONFLICT_ACTION_IGNORE: {
                uint32_t kw_8 = kw(ctx, " OR IGNORE");
                sw_4 = kw_8;
                break;
            }
            default: break;
        }
        uint32_t _buf_9[2];
        uint32_t _n_10 = 0;
        if (kw_3 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = kw_3;
        if (sw_4 != SYNTAQLITE_NULL_DOC) _buf_9[_n_10++] = sw_4;
        uint32_t cat_11 = doc_concat(&ctx->docs, _buf_9, _n_10);
        cond_1 = cat_11;
    }
    uint32_t kw_12 = kw(ctx, " INTO ");
    uint32_t ch_13 = format_node(ctx, node->table);
    uint32_t cond_14 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_15 = kw(ctx, " (");
        uint32_t ch_16 = format_node(ctx, node->columns);
        uint32_t kw_17 = kw(ctx, ")");
        uint32_t _buf_18[3];
        uint32_t _n_19 = 0;
        if (kw_15 != SYNTAQLITE_NULL_DOC) _buf_18[_n_19++] = kw_15;
        if (ch_16 != SYNTAQLITE_NULL_DOC) _buf_18[_n_19++] = ch_16;
        if (kw_17 != SYNTAQLITE_NULL_DOC) _buf_18[_n_19++] = kw_17;
        uint32_t cat_20 = doc_concat(&ctx->docs, _buf_18, _n_19);
        cond_14 = cat_20;
    }
    uint32_t cond_21 = SYNTAQLITE_NULL_DOC;
    if (node->source != SYNTAQLITE_NULL_NODE) {
        uint32_t hl_22 = doc_hardline(&ctx->docs);
        uint32_t ch_23 = format_node(ctx, node->source);
        uint32_t _buf_24[2];
        uint32_t _n_25 = 0;
        if (hl_22 != SYNTAQLITE_NULL_DOC) _buf_24[_n_25++] = hl_22;
        if (ch_23 != SYNTAQLITE_NULL_DOC) _buf_24[_n_25++] = ch_23;
        uint32_t cat_26 = doc_concat(&ctx->docs, _buf_24, _n_25);
        cond_21 = cat_26;
    }
    uint32_t _buf_27[5];
    uint32_t _n_28 = 0;
    if (cond_1 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_1;
    if (kw_12 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = kw_12;
    if (ch_13 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = ch_13;
    if (cond_14 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_14;
    if (cond_21 != SYNTAQLITE_NULL_DOC) _buf_27[_n_28++] = cond_21;
    uint32_t cat_29 = doc_concat(&ctx->docs, _buf_27, _n_28);
    return cat_29;
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
