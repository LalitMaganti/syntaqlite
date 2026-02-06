// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from ast_codegen node definitions - DO NOT EDIT
// Regenerate with: python3 python/tools/extract_sqlite.py

#include "src/fmt/fmt.h"

#include "src/ast/ast_base.h"
#include "src/ast/ast_nodes.h"
#include "src/fmt/comment_attach.h"
#include "src/fmt/doc.h"
#include "src/fmt/doc_layout.h"
#include "src/sqlite_tokens.h"

#include <stdlib.h>
#include <string.h>

// ============ Formatter Context ============

typedef struct {
    SyntaqliteDocContext docs;
    SyntaqliteArena *ast;
    const char *source;
    SyntaqliteTokenList *token_list;
    SyntaqliteFmtOptions *options;
    SyntaqliteCommentAttachment *comment_att;
} FmtCtx;

// ============ Helpers ============

static uint32_t kw(FmtCtx *ctx, const char *text) {
    return doc_text(&ctx->docs, text, (uint32_t)strlen(text));
}

static uint32_t span_text(FmtCtx *ctx, SyntaqliteSourceSpan span) {
    if (span.length == 0) return SYNTAQLITE_NULL_DOC;
    return doc_text(&ctx->docs, ctx->source + span.offset, span.length);
}

// ============ Comment Helpers ============

static uint32_t emit_single_comment(FmtCtx *ctx, uint32_t tok_idx) {
    SyntaqliteRawToken *tok = &ctx->token_list->data[tok_idx];
    return doc_text(&ctx->docs, ctx->source + tok->offset, tok->length);
}

static uint32_t emit_owned_comments(FmtCtx *ctx, uint32_t node_id, uint8_t kind) {
    if (!ctx->comment_att) return SYNTAQLITE_NULL_DOC;
    uint32_t parts[64];
    uint32_t n = 0;
    for (uint32_t i = 0; i < ctx->comment_att->count && n < 62; i++) {
        if (ctx->comment_att->owner_node[i] != node_id) continue;
        if (ctx->comment_att->position[i] != kind) continue;

        int is_line = (ctx->source[ctx->token_list->data[i].offset] == '-');
        if (kind == SYNTAQLITE_COMMENT_LEADING) {
            parts[n++] = emit_single_comment(ctx, i);
            if (n < 62) parts[n++] = is_line ? doc_hardline(&ctx->docs)
                : doc_text(&ctx->docs, " ", 1);
        } else {
            uint32_t sp_parts[2];
            sp_parts[0] = doc_text(&ctx->docs, " ", 1);
            sp_parts[1] = emit_single_comment(ctx, i);
            uint32_t inner = doc_concat(&ctx->docs, sp_parts, 2);
            if (is_line) {
                // Line comments consume the rest of the line.
                parts[n++] = doc_line_suffix(&ctx->docs, inner);
                if (n < 62) parts[n++] = doc_break_parent(&ctx->docs);
            } else {
                // Block comments are inline.
                parts[n++] = inner;
            }
        }
    }
    if (n == 0) return SYNTAQLITE_NULL_DOC;
    return doc_concat(&ctx->docs, parts, n);
}

static uint32_t format_node(FmtCtx *ctx, uint32_t node_id);
static uint32_t dispatch_format(FmtCtx *ctx, uint32_t node_id);

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
    return doc_group(&ctx->docs, doc_concat(&ctx->docs, buf, n));
}

// ============ Clause Helper ============

static uint32_t format_clause(FmtCtx *ctx, const char *keyword, uint32_t body_id) {
    if (body_id == SYNTAQLITE_NULL_NODE) return SYNTAQLITE_NULL_DOC;
    // Extract body's leading comments so they appear before the keyword.
    uint32_t leading = emit_owned_comments(ctx, body_id, SYNTAQLITE_COMMENT_LEADING);
    uint32_t kw_doc = kw(ctx, keyword);
    uint32_t body_doc = dispatch_format(ctx, body_id);
    uint32_t trailing = emit_owned_comments(ctx, body_id, SYNTAQLITE_COMMENT_TRAILING);
    if (body_doc == SYNTAQLITE_NULL_DOC) return SYNTAQLITE_NULL_DOC;
    uint32_t body_parts[] = { body_doc, trailing };
    uint32_t body_trail = doc_concat(&ctx->docs, body_parts, 2);
    uint32_t inner_items[] = { doc_line(&ctx->docs), body_trail };
    uint32_t inner = doc_concat(&ctx->docs, inner_items, 2);
    uint32_t items[] = {
        doc_line(&ctx->docs),
        leading,
        kw_doc,
        doc_group(&ctx->docs, doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner)),
    };
    return doc_concat(&ctx->docs, items, 4);
}

// ============ Node Formatters ============

static uint32_t format_binary_expr(FmtCtx *ctx, SyntaqliteBinaryExpr *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_BINARY_OP_AND: {
            uint32_t ch_2 = format_node(ctx, node->left);
            uint32_t ln_3 = doc_line(&ctx->docs);
            uint32_t kw_4 = kw(ctx, "AND ");
            uint32_t ch_5 = format_node(ctx, node->right);
            uint32_t cat_6_items[] = { ch_2, ln_3, kw_4, ch_5 };
            uint32_t cat_6 = doc_concat(&ctx->docs, cat_6_items, 4);
            sw_1 = cat_6;
            break;
        }
        case SYNTAQLITE_BINARY_OP_OR: {
            uint32_t ch_7 = format_node(ctx, node->left);
            uint32_t ln_8 = doc_line(&ctx->docs);
            uint32_t kw_9 = kw(ctx, "OR ");
            uint32_t ch_10 = format_node(ctx, node->right);
            uint32_t cat_11_items[] = { ch_7, ln_8, kw_9, ch_10 };
            uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 4);
            sw_1 = cat_11;
            break;
        }
        default: {
            uint32_t ch_12 = format_node(ctx, node->left);
            uint32_t ln_13 = doc_line(&ctx->docs);
            uint32_t ed_14 = SYNTAQLITE_NULL_DOC;
            switch (node->op) {
                case SYNTAQLITE_BINARY_OP_PLUS: ed_14 = kw(ctx, "+"); break;
                case SYNTAQLITE_BINARY_OP_MINUS: ed_14 = kw(ctx, "-"); break;
                case SYNTAQLITE_BINARY_OP_STAR: ed_14 = kw(ctx, "*"); break;
                case SYNTAQLITE_BINARY_OP_SLASH: ed_14 = kw(ctx, "/"); break;
                case SYNTAQLITE_BINARY_OP_REM: ed_14 = kw(ctx, "%"); break;
                case SYNTAQLITE_BINARY_OP_LT: ed_14 = kw(ctx, "<"); break;
                case SYNTAQLITE_BINARY_OP_GT: ed_14 = kw(ctx, ">"); break;
                case SYNTAQLITE_BINARY_OP_LE: ed_14 = kw(ctx, "<="); break;
                case SYNTAQLITE_BINARY_OP_GE: ed_14 = kw(ctx, ">="); break;
                case SYNTAQLITE_BINARY_OP_EQ: ed_14 = kw(ctx, "="); break;
                case SYNTAQLITE_BINARY_OP_NE: ed_14 = kw(ctx, "!="); break;
                case SYNTAQLITE_BINARY_OP_AND: ed_14 = kw(ctx, "AND"); break;
                case SYNTAQLITE_BINARY_OP_OR: ed_14 = kw(ctx, "OR"); break;
                case SYNTAQLITE_BINARY_OP_BITAND: ed_14 = kw(ctx, "&"); break;
                case SYNTAQLITE_BINARY_OP_BITOR: ed_14 = kw(ctx, "|"); break;
                case SYNTAQLITE_BINARY_OP_LSHIFT: ed_14 = kw(ctx, "<<"); break;
                case SYNTAQLITE_BINARY_OP_RSHIFT: ed_14 = kw(ctx, ">>"); break;
                case SYNTAQLITE_BINARY_OP_CONCAT: ed_14 = kw(ctx, "||"); break;
                case SYNTAQLITE_BINARY_OP_PTR: ed_14 = kw(ctx, "->"); break;
                default: break;
            }
            uint32_t kw_15 = kw(ctx, " ");
            uint32_t ch_16 = format_node(ctx, node->right);
            uint32_t cat_17_items[] = { ch_12, ln_13, ed_14, kw_15, ch_16 };
            uint32_t cat_17 = doc_concat(&ctx->docs, cat_17_items, 5);
            uint32_t grp_18 = doc_group(&ctx->docs, cat_17);
            sw_1 = grp_18;
            break;
        }
    }
    return sw_1;
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
    uint32_t cat_3_items[] = { ed_1, ch_2 };
    return doc_concat(&ctx->docs, cat_3_items, 2);
}

static uint32_t format_literal(FmtCtx *ctx, SyntaqliteLiteral *node) {
    return span_text(ctx, node->source);
}

static uint32_t format_result_column(FmtCtx *ctx, SyntaqliteResultColumn *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->flags.star) {
        uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
        if (node->expr != SYNTAQLITE_NULL_NODE) {
            uint32_t ch_3 = format_node(ctx, node->expr);
            uint32_t kw_4 = kw(ctx, ".*");
            uint32_t cat_5_items[] = { ch_3, kw_4 };
            uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
            cond_2 = cat_5;
        } else {
            cond_2 = kw(ctx, "*");
        }
        cond_1 = cond_2;
    } else {
        cond_1 = format_node(ctx, node->expr);
    }
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t kw_9 = kw(ctx, " AS ");
        uint32_t sp_10 = span_text(ctx, node->alias);
        uint32_t cat_11_items[] = { kw_9, sp_10 };
        uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 2);
        cond_8 = cat_11;
    }
    uint32_t cat_12_items[] = { cond_1, cond_8 };
    return doc_concat(&ctx->docs, cat_12_items, 2);
}

static uint32_t format_select_stmt(FmtCtx *ctx, SyntaqliteSelectStmt *node) {
    uint32_t cond_1 = node->flags.distinct ? kw(ctx, "SELECT DISTINCT") : kw(ctx, "SELECT");
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t ln_5 = doc_line(&ctx->docs);
        uint32_t ch_6 = format_node(ctx, node->columns);
        uint32_t cat_7_items[] = { ln_5, ch_6 };
        uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 2);
        uint32_t nst_8 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_7);
        uint32_t grp_9 = doc_group(&ctx->docs, nst_8);
        cond_4 = grp_9;
    }
    uint32_t cl_10 = format_clause(ctx, "FROM", node->from_clause);
    uint32_t cl_11 = format_clause(ctx, "WHERE", node->where);
    uint32_t cl_12 = format_clause(ctx, "GROUP BY", node->groupby);
    uint32_t cl_13 = format_clause(ctx, "HAVING", node->having);
    uint32_t cl_14 = format_clause(ctx, "ORDER BY", node->orderby);
    uint32_t cl_15 = format_clause(ctx, "LIMIT", node->limit_clause);
    uint32_t cl_16 = format_clause(ctx, "WINDOW", node->window_clause);
    uint32_t cat_17_items[] = { cond_1, cond_4, cl_10, cl_11, cl_12, cl_13, cl_14, cl_15, cl_16 };
    uint32_t cat_17 = doc_concat(&ctx->docs, cat_17_items, 9);
    return doc_group(&ctx->docs, cat_17);
}

static uint32_t format_ordering_term(FmtCtx *ctx, SyntaqliteOrderingTerm *node) {
    uint32_t ch_1 = format_node(ctx, node->expr);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->sort_order == SYNTAQLITE_SORT_ORDER_DESC) cond_2 = kw(ctx, " DESC");
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->nulls_order == SYNTAQLITE_NULLS_ORDER_FIRST) cond_4 = kw(ctx, " NULLS FIRST");
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->nulls_order == SYNTAQLITE_NULLS_ORDER_LAST) cond_6 = kw(ctx, " NULLS LAST");
    uint32_t cat_8_items[] = { ch_1, cond_2, cond_4, cond_6 };
    return doc_concat(&ctx->docs, cat_8_items, 4);
}

static uint32_t format_limit_clause(FmtCtx *ctx, SyntaqliteLimitClause *node) {
    uint32_t ch_1 = format_node(ctx, node->limit);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->offset != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_3 = kw(ctx, " OFFSET ");
        uint32_t ch_4 = format_node(ctx, node->offset);
        uint32_t cat_5_items[] = { kw_3, ch_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
        cond_2 = cat_5;
    }
    uint32_t cat_6_items[] = { ch_1, cond_2 };
    return doc_concat(&ctx->docs, cat_6_items, 2);
}

static uint32_t format_column_ref(FmtCtx *ctx, SyntaqliteColumnRef *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_2 = span_text(ctx, node->schema);
        uint32_t kw_3 = kw(ctx, ".");
        uint32_t cat_4_items[] = { sp_2, kw_3 };
        uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
        cond_1 = cat_4;
    }
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->table.length > 0) {
        uint32_t sp_6 = span_text(ctx, node->table);
        uint32_t kw_7 = kw(ctx, ".");
        uint32_t cat_8_items[] = { sp_6, kw_7 };
        uint32_t cat_8 = doc_concat(&ctx->docs, cat_8_items, 2);
        cond_5 = cat_8;
    }
    uint32_t sp_9 = span_text(ctx, node->column);
    uint32_t cat_10_items[] = { cond_1, cond_5, sp_9 };
    return doc_concat(&ctx->docs, cat_10_items, 3);
}

static uint32_t format_function_call(FmtCtx *ctx, SyntaqliteFunctionCall *node) {
    uint32_t sp_1 = span_text(ctx, node->func_name);
    uint32_t kw_2 = kw(ctx, "(");
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) cond_3 = kw(ctx, "DISTINCT ");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->flags.star) {
        cond_5 = kw(ctx, "*");
    } else {
        uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
        if (node->args != SYNTAQLITE_NULL_NODE) {
            uint32_t sl_8 = doc_softline(&ctx->docs);
            uint32_t ch_9 = format_node(ctx, node->args);
            uint32_t cat_10_items[] = { sl_8, ch_9 };
            uint32_t cat_10 = doc_concat(&ctx->docs, cat_10_items, 2);
            uint32_t nst_11 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_10);
            cond_7 = nst_11;
        }
        cond_5 = cond_7;
    }
    uint32_t sl_12 = doc_softline(&ctx->docs);
    uint32_t kw_13 = kw(ctx, ")");
    uint32_t cat_14_items[] = { kw_2, cond_3, cond_5, sl_12, kw_13 };
    uint32_t cat_14 = doc_concat(&ctx->docs, cat_14_items, 5);
    uint32_t grp_15 = doc_group(&ctx->docs, cat_14);
    uint32_t cond_16 = SYNTAQLITE_NULL_DOC;
    if (node->filter_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_17 = kw(ctx, " FILTER (WHERE ");
        uint32_t ch_18 = format_node(ctx, node->filter_clause);
        uint32_t kw_19 = kw(ctx, ")");
        uint32_t cat_20_items[] = { kw_17, ch_18, kw_19 };
        uint32_t cat_20 = doc_concat(&ctx->docs, cat_20_items, 3);
        cond_16 = cat_20;
    }
    uint32_t cond_21 = SYNTAQLITE_NULL_DOC;
    if (node->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_22 = kw(ctx, " OVER ");
        uint32_t ch_23 = format_node(ctx, node->over_clause);
        uint32_t cat_24_items[] = { kw_22, ch_23 };
        uint32_t cat_24 = doc_concat(&ctx->docs, cat_24_items, 2);
        cond_21 = cat_24;
    }
    uint32_t cat_25_items[] = { sp_1, grp_15, cond_16, cond_21 };
    return doc_concat(&ctx->docs, cat_25_items, 4);
}

static uint32_t format_is_expr(FmtCtx *ctx, SyntaqliteIsExpr *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_IS_OP_ISNULL: {
            uint32_t ch_2 = format_node(ctx, node->left);
            uint32_t kw_3 = kw(ctx, " ISNULL");
            uint32_t cat_4_items[] = { ch_2, kw_3 };
            uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
            sw_1 = cat_4;
            break;
        }
        case SYNTAQLITE_IS_OP_NOTNULL: {
            uint32_t ch_5 = format_node(ctx, node->left);
            uint32_t kw_6 = kw(ctx, " NOTNULL");
            uint32_t cat_7_items[] = { ch_5, kw_6 };
            uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 2);
            sw_1 = cat_7;
            break;
        }
        case SYNTAQLITE_IS_OP_IS: {
            uint32_t ch_8 = format_node(ctx, node->left);
            uint32_t kw_9 = kw(ctx, " IS ");
            uint32_t ch_10 = format_node(ctx, node->right);
            uint32_t cat_11_items[] = { ch_8, kw_9, ch_10 };
            uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 3);
            sw_1 = cat_11;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_NOT: {
            uint32_t ch_12 = format_node(ctx, node->left);
            uint32_t kw_13 = kw(ctx, " IS NOT ");
            uint32_t ch_14 = format_node(ctx, node->right);
            uint32_t cat_15_items[] = { ch_12, kw_13, ch_14 };
            uint32_t cat_15 = doc_concat(&ctx->docs, cat_15_items, 3);
            sw_1 = cat_15;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_NOT_DISTINCT: {
            uint32_t ch_16 = format_node(ctx, node->left);
            uint32_t kw_17 = kw(ctx, " IS NOT DISTINCT FROM ");
            uint32_t ch_18 = format_node(ctx, node->right);
            uint32_t cat_19_items[] = { ch_16, kw_17, ch_18 };
            uint32_t cat_19 = doc_concat(&ctx->docs, cat_19_items, 3);
            sw_1 = cat_19;
            break;
        }
        case SYNTAQLITE_IS_OP_IS_DISTINCT: {
            uint32_t ch_20 = format_node(ctx, node->left);
            uint32_t kw_21 = kw(ctx, " IS DISTINCT FROM ");
            uint32_t ch_22 = format_node(ctx, node->right);
            uint32_t cat_23_items[] = { ch_20, kw_21, ch_22 };
            uint32_t cat_23 = doc_concat(&ctx->docs, cat_23_items, 3);
            sw_1 = cat_23;
            break;
        }
        default: break;
    }
    return sw_1;
}

static uint32_t format_between_expr(FmtCtx *ctx, SyntaqliteBetweenExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->operand);
    uint32_t cond_2 = node->negated ? kw(ctx, " NOT BETWEEN ") : kw(ctx, " BETWEEN ");
    uint32_t ch_5 = format_node(ctx, node->low);
    uint32_t kw_6 = kw(ctx, " AND ");
    uint32_t ch_7 = format_node(ctx, node->high);
    uint32_t cat_8_items[] = { ch_1, cond_2, ch_5, kw_6, ch_7 };
    return doc_concat(&ctx->docs, cat_8_items, 5);
}

static uint32_t format_like_expr(FmtCtx *ctx, SyntaqliteLikeExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->operand);
    uint32_t cond_2 = node->negated ? kw(ctx, " NOT LIKE ") : kw(ctx, " LIKE ");
    uint32_t ch_5 = format_node(ctx, node->pattern);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->escape != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_7 = kw(ctx, " ESCAPE ");
        uint32_t ch_8 = format_node(ctx, node->escape);
        uint32_t cat_9_items[] = { kw_7, ch_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
        cond_6 = cat_9;
    }
    uint32_t cat_10_items[] = { ch_1, cond_2, ch_5, cond_6 };
    return doc_concat(&ctx->docs, cat_10_items, 4);
}

static uint32_t format_case_expr(FmtCtx *ctx, SyntaqliteCaseExpr *node) {
    uint32_t kw_1 = kw(ctx, "CASE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->operand != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_3 = kw(ctx, " ");
        uint32_t ch_4 = format_node(ctx, node->operand);
        uint32_t cat_5_items[] = { kw_3, ch_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
        cond_2 = cat_5;
    }
    uint32_t ch_6 = format_node(ctx, node->whens);
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->else_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_8 = kw(ctx, " ELSE ");
        uint32_t ch_9 = format_node(ctx, node->else_expr);
        uint32_t cat_10_items[] = { kw_8, ch_9 };
        uint32_t cat_10 = doc_concat(&ctx->docs, cat_10_items, 2);
        cond_7 = cat_10;
    }
    uint32_t kw_11 = kw(ctx, " END");
    uint32_t cat_12_items[] = { kw_1, cond_2, ch_6, cond_7, kw_11 };
    return doc_concat(&ctx->docs, cat_12_items, 5);
}

static uint32_t format_case_when(FmtCtx *ctx, SyntaqliteCaseWhen *node) {
    uint32_t kw_1 = kw(ctx, " WHEN ");
    uint32_t ch_2 = format_node(ctx, node->when_expr);
    uint32_t kw_3 = kw(ctx, " THEN ");
    uint32_t ch_4 = format_node(ctx, node->then_expr);
    uint32_t cat_5_items[] = { kw_1, ch_2, kw_3, ch_4 };
    return doc_concat(&ctx->docs, cat_5_items, 4);
}

static uint32_t format_case_when_list(FmtCtx *ctx, SyntaqliteCaseWhenList *node) {
    uint32_t _buf_1[node->count > 0 ? node->count : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        uint32_t item_4 = format_node(ctx, _child_id);
        _buf_1[_n_2++] = item_4;
    }
    return doc_concat(&ctx->docs, _buf_1, _n_2);
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
    uint32_t cat_6_items[] = { ch_1, hl_2, ed_3, hl_4, ch_5 };
    return doc_concat(&ctx->docs, cat_6_items, 5);
}

static uint32_t format_subquery_expr(FmtCtx *ctx, SyntaqliteSubqueryExpr *node) {
    uint32_t kw_1 = kw(ctx, "(");
    uint32_t ch_2 = format_node(ctx, node->select);
    uint32_t kw_3 = kw(ctx, ")");
    uint32_t cat_4_items[] = { kw_1, ch_2, kw_3 };
    return doc_concat(&ctx->docs, cat_4_items, 3);
}

static uint32_t format_exists_expr(FmtCtx *ctx, SyntaqliteExistsExpr *node) {
    uint32_t kw_1 = kw(ctx, "EXISTS (");
    uint32_t ch_2 = format_node(ctx, node->select);
    uint32_t kw_3 = kw(ctx, ")");
    uint32_t cat_4_items[] = { kw_1, ch_2, kw_3 };
    return doc_concat(&ctx->docs, cat_4_items, 3);
}

static uint32_t format_in_expr(FmtCtx *ctx, SyntaqliteInExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->operand);
    uint32_t cond_2 = node->negated ? kw(ctx, " NOT IN ") : kw(ctx, " IN ");
    uint32_t kw_5 = kw(ctx, "(");
    uint32_t ch_6 = format_node(ctx, node->source);
    uint32_t kw_7 = kw(ctx, ")");
    uint32_t cat_8_items[] = { ch_1, cond_2, kw_5, ch_6, kw_7 };
    return doc_concat(&ctx->docs, cat_8_items, 5);
}

static uint32_t format_variable(FmtCtx *ctx, SyntaqliteVariable *node) {
    return span_text(ctx, node->source);
}

static uint32_t format_collate_expr(FmtCtx *ctx, SyntaqliteCollateExpr *node) {
    uint32_t ch_1 = format_node(ctx, node->expr);
    uint32_t kw_2 = kw(ctx, " COLLATE ");
    uint32_t sp_3 = span_text(ctx, node->collation);
    uint32_t cat_4_items[] = { ch_1, kw_2, sp_3 };
    return doc_concat(&ctx->docs, cat_4_items, 3);
}

static uint32_t format_cast_expr(FmtCtx *ctx, SyntaqliteCastExpr *node) {
    uint32_t kw_1 = kw(ctx, "CAST(");
    uint32_t ch_2 = format_node(ctx, node->expr);
    uint32_t kw_3 = kw(ctx, " AS ");
    uint32_t sp_4 = span_text(ctx, node->type_name);
    uint32_t kw_5 = kw(ctx, ")");
    uint32_t cat_6_items[] = { kw_1, ch_2, kw_3, sp_4, kw_5 };
    return doc_concat(&ctx->docs, cat_6_items, 5);
}

static uint32_t format_values_row_list(FmtCtx *ctx, SyntaqliteValuesRowList *node) {
    uint32_t _buf_1[node->count * 2 > 0 ? node->count * 2 : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t kw_4 = kw(ctx, ",");
            uint32_t ln_5 = doc_line(&ctx->docs);
            uint32_t cat_6_items[] = { kw_4, ln_5 };
            uint32_t cat_6 = doc_concat(&ctx->docs, cat_6_items, 2);
            _buf_1[_n_2++] = cat_6;
        }
        uint32_t kw_7 = kw(ctx, "(");
        uint32_t item_8 = format_node(ctx, _child_id);
        uint32_t kw_9 = kw(ctx, ")");
        uint32_t cat_10_items[] = { kw_7, item_8, kw_9 };
        uint32_t cat_10 = doc_concat(&ctx->docs, cat_10_items, 3);
        _buf_1[_n_2++] = cat_10;
    }
    return doc_concat(&ctx->docs, _buf_1, _n_2);
}

static uint32_t format_values_clause(FmtCtx *ctx, SyntaqliteValuesClause *node) {
    uint32_t kw_1 = kw(ctx, "VALUES");
    uint32_t hl_2 = doc_hardline(&ctx->docs);
    uint32_t ch_3 = format_node(ctx, node->rows);
    uint32_t cat_4_items[] = { hl_2, ch_3 };
    uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
    uint32_t nst_5 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_4);
    uint32_t cat_6_items[] = { kw_1, nst_5 };
    return doc_concat(&ctx->docs, cat_6_items, 2);
}

static uint32_t format_cte_definition(FmtCtx *ctx, SyntaqliteCteDefinition *node) {
    uint32_t sp_1 = span_text(ctx, node->cte_name);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_3 = kw(ctx, "(");
        uint32_t ch_4 = format_node(ctx, node->columns);
        uint32_t kw_5 = kw(ctx, ")");
        uint32_t cat_6_items[] = { kw_3, ch_4, kw_5 };
        uint32_t cat_6 = doc_concat(&ctx->docs, cat_6_items, 3);
        cond_2 = cat_6;
    }
    uint32_t kw_7 = kw(ctx, " AS ");
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->materialized == SYNTAQLITE_MATERIALIZED_MATERIALIZED) cond_8 = kw(ctx, "MATERIALIZED ");
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->materialized == SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED) cond_10 = kw(ctx, "NOT MATERIALIZED ");
    uint32_t kw_12 = kw(ctx, "(");
    uint32_t ch_13 = format_node(ctx, node->select);
    uint32_t kw_14 = kw(ctx, ")");
    uint32_t cat_15_items[] = { sp_1, cond_2, kw_7, cond_8, cond_10, kw_12, ch_13, kw_14 };
    return doc_concat(&ctx->docs, cat_15_items, 8);
}

static uint32_t format_with_clause(FmtCtx *ctx, SyntaqliteWithClause *node) {
    uint32_t cond_1 = node->recursive ? kw(ctx, "WITH RECURSIVE ") : kw(ctx, "WITH ");
    uint32_t ch_4 = format_node(ctx, node->ctes);
    uint32_t hl_5 = doc_hardline(&ctx->docs);
    uint32_t ch_6 = format_node(ctx, node->select);
    uint32_t cat_7_items[] = { cond_1, ch_4, hl_5, ch_6 };
    return doc_concat(&ctx->docs, cat_7_items, 4);
}

static uint32_t format_aggregate_function_call(FmtCtx *ctx, SyntaqliteAggregateFunctionCall *node) {
    uint32_t sp_1 = span_text(ctx, node->func_name);
    uint32_t kw_2 = kw(ctx, "(");
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->flags.distinct) cond_3 = kw(ctx, "DISTINCT ");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->args != SYNTAQLITE_NULL_NODE) {
        uint32_t sl_6 = doc_softline(&ctx->docs);
        uint32_t ch_7 = format_node(ctx, node->args);
        uint32_t cat_8_items[] = { sl_6, ch_7 };
        uint32_t cat_8 = doc_concat(&ctx->docs, cat_8_items, 2);
        uint32_t nst_9 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_8);
        cond_5 = nst_9;
    }
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->orderby != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_11 = kw(ctx, " ORDER BY ");
        uint32_t ch_12 = format_node(ctx, node->orderby);
        uint32_t cat_13_items[] = { kw_11, ch_12 };
        uint32_t cat_13 = doc_concat(&ctx->docs, cat_13_items, 2);
        cond_10 = cat_13;
    }
    uint32_t sl_14 = doc_softline(&ctx->docs);
    uint32_t kw_15 = kw(ctx, ")");
    uint32_t cat_16_items[] = { kw_2, cond_3, cond_5, cond_10, sl_14, kw_15 };
    uint32_t cat_16 = doc_concat(&ctx->docs, cat_16_items, 6);
    uint32_t grp_17 = doc_group(&ctx->docs, cat_16);
    uint32_t cond_18 = SYNTAQLITE_NULL_DOC;
    if (node->filter_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_19 = kw(ctx, " FILTER (WHERE ");
        uint32_t ch_20 = format_node(ctx, node->filter_clause);
        uint32_t kw_21 = kw(ctx, ")");
        uint32_t cat_22_items[] = { kw_19, ch_20, kw_21 };
        uint32_t cat_22 = doc_concat(&ctx->docs, cat_22_items, 3);
        cond_18 = cat_22;
    }
    uint32_t cond_23 = SYNTAQLITE_NULL_DOC;
    if (node->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_24 = kw(ctx, " OVER ");
        uint32_t ch_25 = format_node(ctx, node->over_clause);
        uint32_t cat_26_items[] = { kw_24, ch_25 };
        uint32_t cat_26 = doc_concat(&ctx->docs, cat_26_items, 2);
        cond_23 = cat_26;
    }
    uint32_t cat_27_items[] = { sp_1, grp_17, cond_18, cond_23 };
    return doc_concat(&ctx->docs, cat_27_items, 4);
}

static uint32_t format_raise_expr(FmtCtx *ctx, SyntaqliteRaiseExpr *node) {
    uint32_t kw_1 = kw(ctx, "RAISE(");
    uint32_t sw_2 = SYNTAQLITE_NULL_DOC;
    switch (node->raise_type) {
        case SYNTAQLITE_RAISE_TYPE_IGNORE: sw_2 = kw(ctx, "IGNORE"); break;
        case SYNTAQLITE_RAISE_TYPE_ROLLBACK: sw_2 = kw(ctx, "ROLLBACK"); break;
        case SYNTAQLITE_RAISE_TYPE_ABORT: sw_2 = kw(ctx, "ABORT"); break;
        case SYNTAQLITE_RAISE_TYPE_FAIL: sw_2 = kw(ctx, "FAIL"); break;
        default: break;
    }
    uint32_t cond_7 = SYNTAQLITE_NULL_DOC;
    if (node->error_message != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_8 = kw(ctx, ", ");
        uint32_t ch_9 = format_node(ctx, node->error_message);
        uint32_t cat_10_items[] = { kw_8, ch_9 };
        uint32_t cat_10 = doc_concat(&ctx->docs, cat_10_items, 2);
        cond_7 = cat_10;
    }
    uint32_t kw_11 = kw(ctx, ")");
    uint32_t cat_12_items[] = { kw_1, sw_2, cond_7, kw_11 };
    return doc_concat(&ctx->docs, cat_12_items, 4);
}

static uint32_t format_table_ref(FmtCtx *ctx, SyntaqliteTableRef *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_2 = span_text(ctx, node->schema);
        uint32_t kw_3 = kw(ctx, ".");
        uint32_t cat_4_items[] = { sp_2, kw_3 };
        uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
        cond_1 = cat_4;
    }
    uint32_t sp_5 = span_text(ctx, node->table_name);
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t kw_7 = kw(ctx, " AS ");
        uint32_t sp_8 = span_text(ctx, node->alias);
        uint32_t cat_9_items[] = { kw_7, sp_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
        cond_6 = cat_9;
    }
    uint32_t cat_10_items[] = { cond_1, sp_5, cond_6 };
    return doc_concat(&ctx->docs, cat_10_items, 3);
}

static uint32_t format_subquery_table_source(FmtCtx *ctx, SyntaqliteSubqueryTableSource *node) {
    uint32_t kw_1 = kw(ctx, "(");
    uint32_t ch_2 = format_node(ctx, node->select);
    uint32_t kw_3 = kw(ctx, ")");
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->alias.length > 0) {
        uint32_t kw_5 = kw(ctx, " AS ");
        uint32_t sp_6 = span_text(ctx, node->alias);
        uint32_t cat_7_items[] = { kw_5, sp_6 };
        uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 2);
        cond_4 = cat_7;
    }
    uint32_t cat_8_items[] = { kw_1, ch_2, kw_3, cond_4 };
    return doc_concat(&ctx->docs, cat_8_items, 4);
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
                uint32_t cat_9_items[] = { hl_6, kw_7, ch_8 };
                uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 3);
                cond_5 = cat_9;
            }
            uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
            if (node->using_columns != SYNTAQLITE_NULL_NODE) {
                uint32_t kw_11 = kw(ctx, " USING (");
                uint32_t ch_12 = format_node(ctx, node->using_columns);
                uint32_t kw_13 = kw(ctx, ")");
                uint32_t cat_14_items[] = { kw_11, ch_12, kw_13 };
                uint32_t cat_14 = doc_concat(&ctx->docs, cat_14_items, 3);
                cond_10 = cat_14;
            }
            uint32_t cat_15_items[] = { ch_2, kw_3, ch_4, cond_5, cond_10 };
            uint32_t cat_15 = doc_concat(&ctx->docs, cat_15_items, 5);
            sw_1 = cat_15;
            break;
        }
        default: {
            uint32_t ch_16 = format_node(ctx, node->left);
            uint32_t hl_17 = doc_hardline(&ctx->docs);
            uint32_t ed_18 = SYNTAQLITE_NULL_DOC;
            switch (node->join_type) {
                case SYNTAQLITE_JOIN_TYPE_INNER: ed_18 = kw(ctx, "JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_LEFT: ed_18 = kw(ctx, "LEFT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_RIGHT: ed_18 = kw(ctx, "RIGHT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_FULL: ed_18 = kw(ctx, "FULL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_CROSS: ed_18 = kw(ctx, "CROSS JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_INNER: ed_18 = kw(ctx, "NATURAL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT: ed_18 = kw(ctx, "NATURAL LEFT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT: ed_18 = kw(ctx, "NATURAL RIGHT JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_NATURAL_FULL: ed_18 = kw(ctx, "NATURAL FULL JOIN"); break;
                case SYNTAQLITE_JOIN_TYPE_COMMA: ed_18 = kw(ctx, ","); break;
                default: break;
            }
            uint32_t kw_19 = kw(ctx, " ");
            uint32_t ch_20 = format_node(ctx, node->right);
            uint32_t cond_21 = SYNTAQLITE_NULL_DOC;
            if (node->on_expr != SYNTAQLITE_NULL_NODE) {
                uint32_t hl_22 = doc_hardline(&ctx->docs);
                uint32_t kw_23 = kw(ctx, "ON ");
                uint32_t ch_24 = format_node(ctx, node->on_expr);
                uint32_t cat_25_items[] = { hl_22, kw_23, ch_24 };
                uint32_t cat_25 = doc_concat(&ctx->docs, cat_25_items, 3);
                cond_21 = cat_25;
            }
            uint32_t cond_26 = SYNTAQLITE_NULL_DOC;
            if (node->using_columns != SYNTAQLITE_NULL_NODE) {
                uint32_t kw_27 = kw(ctx, " USING (");
                uint32_t ch_28 = format_node(ctx, node->using_columns);
                uint32_t kw_29 = kw(ctx, ")");
                uint32_t cat_30_items[] = { kw_27, ch_28, kw_29 };
                uint32_t cat_30 = doc_concat(&ctx->docs, cat_30_items, 3);
                cond_26 = cat_30;
            }
            uint32_t cat_31_items[] = { ch_16, hl_17, ed_18, kw_19, ch_20, cond_21, cond_26 };
            uint32_t cat_31 = doc_concat(&ctx->docs, cat_31_items, 7);
            sw_1 = cat_31;
            break;
        }
    }
    return sw_1;
}

static uint32_t format_join_prefix(FmtCtx *ctx, SyntaqliteJoinPrefix *node) {
    return format_node(ctx, node->source);
}

static uint32_t format_delete_stmt(FmtCtx *ctx, SyntaqliteDeleteStmt *node) {
    uint32_t kw_1 = kw(ctx, "DELETE");
    uint32_t cl_2 = format_clause(ctx, "FROM", node->table);
    uint32_t cl_3 = format_clause(ctx, "WHERE", node->where);
    uint32_t cat_4_items[] = { kw_1, cl_2, cl_3 };
    uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 3);
    return doc_group(&ctx->docs, cat_4);
}

static uint32_t format_set_clause(FmtCtx *ctx, SyntaqliteSetClause *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->column.length > 0) {
        cond_1 = span_text(ctx, node->column);
    } else {
        uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
        if (node->columns != SYNTAQLITE_NULL_NODE) {
            uint32_t kw_4 = kw(ctx, "(");
            uint32_t ch_5 = format_node(ctx, node->columns);
            uint32_t kw_6 = kw(ctx, ")");
            uint32_t cat_7_items[] = { kw_4, ch_5, kw_6 };
            uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 3);
            cond_3 = cat_7;
        }
        cond_1 = cond_3;
    }
    uint32_t kw_8 = kw(ctx, " = ");
    uint32_t ch_9 = format_node(ctx, node->value);
    uint32_t cat_10_items[] = { cond_1, kw_8, ch_9 };
    return doc_concat(&ctx->docs, cat_10_items, 3);
}

static uint32_t format_update_stmt(FmtCtx *ctx, SyntaqliteUpdateStmt *node) {
    uint32_t kw_1 = kw(ctx, "UPDATE");
    uint32_t sw_2 = SYNTAQLITE_NULL_DOC;
    switch (node->conflict_action) {
        case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_2 = kw(ctx, " OR ROLLBACK"); break;
        case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_2 = kw(ctx, " OR ABORT"); break;
        case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_2 = kw(ctx, " OR FAIL"); break;
        case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_2 = kw(ctx, " OR IGNORE"); break;
        case SYNTAQLITE_CONFLICT_ACTION_REPLACE: sw_2 = kw(ctx, " OR REPLACE"); break;
        default: break;
    }
    uint32_t kw_8 = kw(ctx, " ");
    uint32_t ch_9 = format_node(ctx, node->table);
    uint32_t cl_10 = format_clause(ctx, "SET", node->setlist);
    uint32_t cl_11 = format_clause(ctx, "FROM", node->from_clause);
    uint32_t cl_12 = format_clause(ctx, "WHERE", node->where);
    uint32_t cat_13_items[] = { kw_1, sw_2, kw_8, ch_9, cl_10, cl_11, cl_12 };
    uint32_t cat_13 = doc_concat(&ctx->docs, cat_13_items, 7);
    return doc_group(&ctx->docs, cat_13);
}

static uint32_t format_insert_stmt(FmtCtx *ctx, SyntaqliteInsertStmt *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->conflict_action == SYNTAQLITE_CONFLICT_ACTION_REPLACE) {
        cond_1 = kw(ctx, "REPLACE");
    } else {
        uint32_t kw_3 = kw(ctx, "INSERT");
        uint32_t sw_4 = SYNTAQLITE_NULL_DOC;
        switch (node->conflict_action) {
            case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_4 = kw(ctx, " OR ROLLBACK"); break;
            case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_4 = kw(ctx, " OR ABORT"); break;
            case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_4 = kw(ctx, " OR FAIL"); break;
            case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_4 = kw(ctx, " OR IGNORE"); break;
            default: break;
        }
        uint32_t cat_9_items[] = { kw_3, sw_4 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
        cond_1 = cat_9;
    }
    uint32_t kw_10 = kw(ctx, " INTO ");
    uint32_t ch_11 = format_node(ctx, node->table);
    uint32_t cond_12 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_13 = kw(ctx, " (");
        uint32_t ch_14 = format_node(ctx, node->columns);
        uint32_t kw_15 = kw(ctx, ")");
        uint32_t cat_16_items[] = { kw_13, ch_14, kw_15 };
        uint32_t cat_16 = doc_concat(&ctx->docs, cat_16_items, 3);
        cond_12 = cat_16;
    }
    uint32_t cond_17 = SYNTAQLITE_NULL_DOC;
    if (node->source != SYNTAQLITE_NULL_NODE) {
        uint32_t hl_18 = doc_hardline(&ctx->docs);
        uint32_t ch_19 = format_node(ctx, node->source);
        uint32_t cat_20_items[] = { hl_18, ch_19 };
        uint32_t cat_20 = doc_concat(&ctx->docs, cat_20_items, 2);
        cond_17 = cat_20;
    }
    uint32_t cat_21_items[] = { cond_1, kw_10, ch_11, cond_12, cond_17 };
    return doc_concat(&ctx->docs, cat_21_items, 5);
}

static uint32_t format_qualified_name(FmtCtx *ctx, SyntaqliteQualifiedName *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_2 = span_text(ctx, node->schema);
        uint32_t kw_3 = kw(ctx, ".");
        uint32_t cat_4_items[] = { sp_2, kw_3 };
        uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
        cond_1 = cat_4;
    }
    uint32_t sp_5 = span_text(ctx, node->object_name);
    uint32_t cat_6_items[] = { cond_1, sp_5 };
    return doc_concat(&ctx->docs, cat_6_items, 2);
}

static uint32_t format_drop_stmt(FmtCtx *ctx, SyntaqliteDropStmt *node) {
    uint32_t kw_1 = kw(ctx, "DROP ");
    uint32_t ed_2 = SYNTAQLITE_NULL_DOC;
    switch (node->object_type) {
        case SYNTAQLITE_DROP_OBJECT_TYPE_TABLE: ed_2 = kw(ctx, "TABLE"); break;
        case SYNTAQLITE_DROP_OBJECT_TYPE_INDEX: ed_2 = kw(ctx, "INDEX"); break;
        case SYNTAQLITE_DROP_OBJECT_TYPE_VIEW: ed_2 = kw(ctx, "VIEW"); break;
        case SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER: ed_2 = kw(ctx, "TRIGGER"); break;
        default: break;
    }
    uint32_t cond_3 = SYNTAQLITE_NULL_DOC;
    if (node->if_exists) cond_3 = kw(ctx, " IF EXISTS");
    uint32_t kw_5 = kw(ctx, " ");
    uint32_t ch_6 = format_node(ctx, node->target);
    uint32_t cat_7_items[] = { kw_1, ed_2, cond_3, kw_5, ch_6 };
    return doc_concat(&ctx->docs, cat_7_items, 5);
}

static uint32_t format_alter_table_stmt(FmtCtx *ctx, SyntaqliteAlterTableStmt *node) {
    uint32_t kw_1 = kw(ctx, "ALTER TABLE ");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->target != SYNTAQLITE_NULL_NODE) {
        uint32_t ch_3 = format_node(ctx, node->target);
        uint32_t kw_4 = kw(ctx, " ");
        uint32_t cat_5_items[] = { ch_3, kw_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
        cond_2 = cat_5;
    }
    uint32_t sw_6 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_ALTER_OP_RENAME_TABLE: {
            uint32_t kw_7 = kw(ctx, "RENAME TO ");
            uint32_t sp_8 = span_text(ctx, node->new_name);
            uint32_t cat_9_items[] = { kw_7, sp_8 };
            uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
            sw_6 = cat_9;
            break;
        }
        case SYNTAQLITE_ALTER_OP_RENAME_COLUMN: {
            uint32_t kw_10 = kw(ctx, "RENAME COLUMN ");
            uint32_t sp_11 = span_text(ctx, node->old_name);
            uint32_t kw_12 = kw(ctx, " TO ");
            uint32_t sp_13 = span_text(ctx, node->new_name);
            uint32_t cat_14_items[] = { kw_10, sp_11, kw_12, sp_13 };
            uint32_t cat_14 = doc_concat(&ctx->docs, cat_14_items, 4);
            sw_6 = cat_14;
            break;
        }
        case SYNTAQLITE_ALTER_OP_DROP_COLUMN: {
            uint32_t kw_15 = kw(ctx, "DROP COLUMN ");
            uint32_t sp_16 = span_text(ctx, node->old_name);
            uint32_t cat_17_items[] = { kw_15, sp_16 };
            uint32_t cat_17 = doc_concat(&ctx->docs, cat_17_items, 2);
            sw_6 = cat_17;
            break;
        }
        case SYNTAQLITE_ALTER_OP_ADD_COLUMN: {
            uint32_t kw_18 = kw(ctx, "ADD COLUMN ");
            uint32_t sp_19 = span_text(ctx, node->old_name);
            uint32_t cat_20_items[] = { kw_18, sp_19 };
            uint32_t cat_20 = doc_concat(&ctx->docs, cat_20_items, 2);
            sw_6 = cat_20;
            break;
        }
        default: break;
    }
    uint32_t cat_21_items[] = { kw_1, cond_2, sw_6 };
    return doc_concat(&ctx->docs, cat_21_items, 3);
}

static uint32_t format_transaction_stmt(FmtCtx *ctx, SyntaqliteTransactionStmt *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_TRANSACTION_OP_BEGIN: {
            uint32_t kw_2 = kw(ctx, "BEGIN");
            uint32_t ed_3 = SYNTAQLITE_NULL_DOC;
            switch (node->trans_type) {
                case SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE: ed_3 = kw(ctx, " IMMEDIATE"); break;
                case SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE: ed_3 = kw(ctx, " EXCLUSIVE"); break;
                default: break;
            }
            uint32_t cat_4_items[] = { kw_2, ed_3 };
            uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
            sw_1 = cat_4;
            break;
        }
        case SYNTAQLITE_TRANSACTION_OP_COMMIT: sw_1 = kw(ctx, "COMMIT"); break;
        case SYNTAQLITE_TRANSACTION_OP_ROLLBACK: sw_1 = kw(ctx, "ROLLBACK"); break;
        default: break;
    }
    return sw_1;
}

static uint32_t format_savepoint_stmt(FmtCtx *ctx, SyntaqliteSavepointStmt *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->op) {
        case SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT: {
            uint32_t kw_2 = kw(ctx, "SAVEPOINT ");
            uint32_t sp_3 = span_text(ctx, node->savepoint_name);
            uint32_t cat_4_items[] = { kw_2, sp_3 };
            uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
            sw_1 = cat_4;
            break;
        }
        case SYNTAQLITE_SAVEPOINT_OP_RELEASE: {
            uint32_t kw_5 = kw(ctx, "RELEASE SAVEPOINT ");
            uint32_t sp_6 = span_text(ctx, node->savepoint_name);
            uint32_t cat_7_items[] = { kw_5, sp_6 };
            uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 2);
            sw_1 = cat_7;
            break;
        }
        case SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO: {
            uint32_t kw_8 = kw(ctx, "ROLLBACK TO SAVEPOINT ");
            uint32_t sp_9 = span_text(ctx, node->savepoint_name);
            uint32_t cat_10_items[] = { kw_8, sp_9 };
            uint32_t cat_10 = doc_concat(&ctx->docs, cat_10_items, 2);
            sw_1 = cat_10;
            break;
        }
        default: break;
    }
    return sw_1;
}

static uint32_t format_pragma_stmt(FmtCtx *ctx, SyntaqlitePragmaStmt *node) {
    uint32_t kw_1 = kw(ctx, "PRAGMA ");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_3 = span_text(ctx, node->schema);
        uint32_t kw_4 = kw(ctx, ".");
        uint32_t cat_5_items[] = { sp_3, kw_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
        cond_2 = cat_5;
    }
    uint32_t sp_6 = span_text(ctx, node->pragma_name);
    uint32_t sw_7 = SYNTAQLITE_NULL_DOC;
    switch (node->pragma_form) {
        case SYNTAQLITE_PRAGMA_FORM_EQ: {
            uint32_t kw_8 = kw(ctx, " = ");
            uint32_t sp_9 = span_text(ctx, node->value);
            uint32_t cat_10_items[] = { kw_8, sp_9 };
            uint32_t cat_10 = doc_concat(&ctx->docs, cat_10_items, 2);
            sw_7 = cat_10;
            break;
        }
        case SYNTAQLITE_PRAGMA_FORM_CALL: {
            uint32_t kw_11 = kw(ctx, "(");
            uint32_t sp_12 = span_text(ctx, node->value);
            uint32_t kw_13 = kw(ctx, ")");
            uint32_t cat_14_items[] = { kw_11, sp_12, kw_13 };
            uint32_t cat_14 = doc_concat(&ctx->docs, cat_14_items, 3);
            sw_7 = cat_14;
            break;
        }
        default: break;
    }
    uint32_t cat_15_items[] = { kw_1, cond_2, sp_6, sw_7 };
    return doc_concat(&ctx->docs, cat_15_items, 4);
}

static uint32_t format_analyze_stmt(FmtCtx *ctx, SyntaqliteAnalyzeStmt *node) {
    uint32_t cond_1 = node->kind == SYNTAQLITE_ANALYZE_KIND_REINDEX ? kw(ctx, "REINDEX") : kw(ctx, "ANALYZE");
    uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t kw_5 = kw(ctx, " ");
        uint32_t sp_6 = span_text(ctx, node->schema);
        uint32_t kw_7 = kw(ctx, ".");
        uint32_t sp_8 = span_text(ctx, node->target_name);
        uint32_t cat_9_items[] = { kw_5, sp_6, kw_7, sp_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 4);
        cond_4 = cat_9;
    } else {
        uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
        if (node->target_name.length > 0) {
            uint32_t kw_11 = kw(ctx, " ");
            uint32_t sp_12 = span_text(ctx, node->target_name);
            uint32_t cat_13_items[] = { kw_11, sp_12 };
            uint32_t cat_13 = doc_concat(&ctx->docs, cat_13_items, 2);
            cond_10 = cat_13;
        }
        cond_4 = cond_10;
    }
    uint32_t cat_14_items[] = { cond_1, cond_4 };
    return doc_concat(&ctx->docs, cat_14_items, 2);
}

static uint32_t format_attach_stmt(FmtCtx *ctx, SyntaqliteAttachStmt *node) {
    uint32_t kw_1 = kw(ctx, "ATTACH ");
    uint32_t ch_2 = format_node(ctx, node->filename);
    uint32_t kw_3 = kw(ctx, " AS ");
    uint32_t ch_4 = format_node(ctx, node->db_name);
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->key != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_6 = kw(ctx, " KEY ");
        uint32_t ch_7 = format_node(ctx, node->key);
        uint32_t cat_8_items[] = { kw_6, ch_7 };
        uint32_t cat_8 = doc_concat(&ctx->docs, cat_8_items, 2);
        cond_5 = cat_8;
    }
    uint32_t cat_9_items[] = { kw_1, ch_2, kw_3, ch_4, cond_5 };
    return doc_concat(&ctx->docs, cat_9_items, 5);
}

static uint32_t format_detach_stmt(FmtCtx *ctx, SyntaqliteDetachStmt *node) {
    uint32_t kw_1 = kw(ctx, "DETACH ");
    uint32_t ch_2 = format_node(ctx, node->db_name);
    uint32_t cat_3_items[] = { kw_1, ch_2 };
    return doc_concat(&ctx->docs, cat_3_items, 2);
}

static uint32_t format_vacuum_stmt(FmtCtx *ctx, SyntaqliteVacuumStmt *node) {
    uint32_t kw_1 = kw(ctx, "VACUUM");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t kw_3 = kw(ctx, " ");
        uint32_t sp_4 = span_text(ctx, node->schema);
        uint32_t cat_5_items[] = { kw_3, sp_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
        cond_2 = cat_5;
    }
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->into_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_7 = kw(ctx, " INTO ");
        uint32_t ch_8 = format_node(ctx, node->into_expr);
        uint32_t cat_9_items[] = { kw_7, ch_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
        cond_6 = cat_9;
    }
    uint32_t cat_10_items[] = { kw_1, cond_2, cond_6 };
    return doc_concat(&ctx->docs, cat_10_items, 3);
}

static uint32_t format_explain_stmt(FmtCtx *ctx, SyntaqliteExplainStmt *node) {
    uint32_t cond_1 = node->explain_mode == SYNTAQLITE_EXPLAIN_MODE_QUERY_PLAN ? kw(ctx, "EXPLAIN QUERY PLAN") : kw(ctx, "EXPLAIN");
    uint32_t hl_4 = doc_hardline(&ctx->docs);
    uint32_t ch_5 = format_node(ctx, node->stmt);
    uint32_t cat_6_items[] = { cond_1, hl_4, ch_5 };
    return doc_concat(&ctx->docs, cat_6_items, 3);
}

static uint32_t format_create_index_stmt(FmtCtx *ctx, SyntaqliteCreateIndexStmt *node) {
    uint32_t kw_1 = kw(ctx, "CREATE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->is_unique) cond_2 = kw(ctx, " UNIQUE");
    uint32_t kw_4 = kw(ctx, " INDEX");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->if_not_exists) cond_5 = kw(ctx, " IF NOT EXISTS");
    uint32_t kw_7 = kw(ctx, " ");
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_9 = span_text(ctx, node->schema);
        uint32_t kw_10 = kw(ctx, ".");
        uint32_t cat_11_items[] = { sp_9, kw_10 };
        uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 2);
        cond_8 = cat_11;
    }
    uint32_t sp_12 = span_text(ctx, node->index_name);
    uint32_t kw_13 = kw(ctx, " ON ");
    uint32_t sp_14 = span_text(ctx, node->table_name);
    uint32_t kw_15 = kw(ctx, " (");
    uint32_t ch_16 = format_node(ctx, node->columns);
    uint32_t kw_17 = kw(ctx, ")");
    uint32_t cl_18 = format_clause(ctx, "WHERE", node->where);
    uint32_t cat_19_items[] = { kw_1, cond_2, kw_4, cond_5, kw_7, cond_8, sp_12, kw_13, sp_14, kw_15, ch_16, kw_17, cl_18 };
    uint32_t cat_19 = doc_concat(&ctx->docs, cat_19_items, 13);
    return doc_group(&ctx->docs, cat_19);
}

static uint32_t format_create_view_stmt(FmtCtx *ctx, SyntaqliteCreateViewStmt *node) {
    uint32_t kw_1 = kw(ctx, "CREATE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->is_temp) cond_2 = kw(ctx, " TEMP");
    uint32_t kw_4 = kw(ctx, " VIEW");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->if_not_exists) cond_5 = kw(ctx, " IF NOT EXISTS");
    uint32_t kw_7 = kw(ctx, " ");
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_9 = span_text(ctx, node->schema);
        uint32_t kw_10 = kw(ctx, ".");
        uint32_t cat_11_items[] = { sp_9, kw_10 };
        uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 2);
        cond_8 = cat_11;
    }
    uint32_t sp_12 = span_text(ctx, node->view_name);
    uint32_t cond_13 = SYNTAQLITE_NULL_DOC;
    if (node->column_names != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_14 = kw(ctx, "(");
        uint32_t ch_15 = format_node(ctx, node->column_names);
        uint32_t kw_16 = kw(ctx, ")");
        uint32_t cat_17_items[] = { kw_14, ch_15, kw_16 };
        uint32_t cat_17 = doc_concat(&ctx->docs, cat_17_items, 3);
        uint32_t grp_18 = doc_group(&ctx->docs, cat_17);
        cond_13 = grp_18;
    }
    uint32_t kw_19 = kw(ctx, " AS");
    uint32_t hl_20 = doc_hardline(&ctx->docs);
    uint32_t ch_21 = format_node(ctx, node->select);
    uint32_t cat_22_items[] = { kw_1, cond_2, kw_4, cond_5, kw_7, cond_8, sp_12, cond_13, kw_19, hl_20, ch_21 };
    return doc_concat(&ctx->docs, cat_22_items, 11);
}

static uint32_t format_foreign_key_clause(FmtCtx *ctx, SyntaqliteForeignKeyClause *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->ref_table.length > 0) {
        uint32_t kw_2 = kw(ctx, "REFERENCES ");
        uint32_t sp_3 = span_text(ctx, node->ref_table);
        uint32_t cat_4_items[] = { kw_2, sp_3 };
        uint32_t cat_4 = doc_concat(&ctx->docs, cat_4_items, 2);
        cond_1 = cat_4;
    }
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->ref_columns != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_6 = kw(ctx, "(");
        uint32_t ch_7 = format_node(ctx, node->ref_columns);
        uint32_t kw_8 = kw(ctx, ")");
        uint32_t cat_9_items[] = { kw_6, ch_7, kw_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 3);
        cond_5 = cat_9;
    }
    uint32_t sw_10 = SYNTAQLITE_NULL_DOC;
    switch (node->on_delete) {
        case SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL: sw_10 = kw(ctx, " ON DELETE SET NULL"); break;
        case SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT: sw_10 = kw(ctx, " ON DELETE SET DEFAULT"); break;
        case SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE: sw_10 = kw(ctx, " ON DELETE CASCADE"); break;
        case SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT: sw_10 = kw(ctx, " ON DELETE RESTRICT"); break;
        default: break;
    }
    uint32_t sw_15 = SYNTAQLITE_NULL_DOC;
    switch (node->on_update) {
        case SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL: sw_15 = kw(ctx, " ON UPDATE SET NULL"); break;
        case SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT: sw_15 = kw(ctx, " ON UPDATE SET DEFAULT"); break;
        case SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE: sw_15 = kw(ctx, " ON UPDATE CASCADE"); break;
        case SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT: sw_15 = kw(ctx, " ON UPDATE RESTRICT"); break;
        default: break;
    }
    uint32_t cond_20 = SYNTAQLITE_NULL_DOC;
    if (node->is_deferred) cond_20 = kw(ctx, "DEFERRABLE INITIALLY DEFERRED");
    uint32_t cat_22_items[] = { cond_1, cond_5, sw_10, sw_15, cond_20 };
    return doc_concat(&ctx->docs, cat_22_items, 5);
}

static uint32_t format_column_constraint(FmtCtx *ctx, SyntaqliteColumnConstraint *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->constraint_name.length > 0) {
        uint32_t kw_2 = kw(ctx, "CONSTRAINT ");
        uint32_t sp_3 = span_text(ctx, node->constraint_name);
        uint32_t kw_4 = kw(ctx, " ");
        uint32_t cat_5_items[] = { kw_2, sp_3, kw_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 3);
        cond_1 = cat_5;
    }
    uint32_t sw_6 = SYNTAQLITE_NULL_DOC;
    switch (node->kind) {
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY: {
            uint32_t kw_7 = kw(ctx, "PRIMARY KEY");
            uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
            if (node->sort_order == SYNTAQLITE_SORT_ORDER_DESC) cond_8 = kw(ctx, " DESC");
            uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
            if (node->is_autoincrement) cond_10 = kw(ctx, " AUTOINCREMENT");
            uint32_t sw_12 = SYNTAQLITE_NULL_DOC;
            switch (node->onconf) {
                case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_12 = kw(ctx, " ON CONFLICT ROLLBACK"); break;
                case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_12 = kw(ctx, " ON CONFLICT ABORT"); break;
                case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_12 = kw(ctx, " ON CONFLICT FAIL"); break;
                case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_12 = kw(ctx, " ON CONFLICT IGNORE"); break;
                case SYNTAQLITE_CONFLICT_ACTION_REPLACE: sw_12 = kw(ctx, " ON CONFLICT REPLACE"); break;
                default: break;
            }
            uint32_t cat_18_items[] = { kw_7, cond_8, cond_10, sw_12 };
            uint32_t cat_18 = doc_concat(&ctx->docs, cat_18_items, 4);
            sw_6 = cat_18;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL: {
            uint32_t kw_19 = kw(ctx, "NOT NULL");
            uint32_t sw_20 = SYNTAQLITE_NULL_DOC;
            switch (node->onconf) {
                case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_20 = kw(ctx, " ON CONFLICT ROLLBACK"); break;
                case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_20 = kw(ctx, " ON CONFLICT ABORT"); break;
                case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_20 = kw(ctx, " ON CONFLICT FAIL"); break;
                case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_20 = kw(ctx, " ON CONFLICT IGNORE"); break;
                case SYNTAQLITE_CONFLICT_ACTION_REPLACE: sw_20 = kw(ctx, " ON CONFLICT REPLACE"); break;
                default: break;
            }
            uint32_t cat_26_items[] = { kw_19, sw_20 };
            uint32_t cat_26 = doc_concat(&ctx->docs, cat_26_items, 2);
            sw_6 = cat_26;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE: {
            uint32_t kw_27 = kw(ctx, "UNIQUE");
            uint32_t sw_28 = SYNTAQLITE_NULL_DOC;
            switch (node->onconf) {
                case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_28 = kw(ctx, " ON CONFLICT ROLLBACK"); break;
                case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_28 = kw(ctx, " ON CONFLICT ABORT"); break;
                case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_28 = kw(ctx, " ON CONFLICT FAIL"); break;
                case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_28 = kw(ctx, " ON CONFLICT IGNORE"); break;
                case SYNTAQLITE_CONFLICT_ACTION_REPLACE: sw_28 = kw(ctx, " ON CONFLICT REPLACE"); break;
                default: break;
            }
            uint32_t cat_34_items[] = { kw_27, sw_28 };
            uint32_t cat_34 = doc_concat(&ctx->docs, cat_34_items, 2);
            sw_6 = cat_34;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK: {
            uint32_t kw_35 = kw(ctx, "CHECK(");
            uint32_t ch_36 = format_node(ctx, node->check_expr);
            uint32_t kw_37 = kw(ctx, ")");
            uint32_t cat_38_items[] = { kw_35, ch_36, kw_37 };
            uint32_t cat_38 = doc_concat(&ctx->docs, cat_38_items, 3);
            sw_6 = cat_38;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT: {
            uint32_t kw_39 = kw(ctx, "DEFAULT ");
            uint32_t ch_40 = format_node(ctx, node->default_expr);
            uint32_t cat_41_items[] = { kw_39, ch_40 };
            uint32_t cat_41 = doc_concat(&ctx->docs, cat_41_items, 2);
            sw_6 = cat_41;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE: {
            uint32_t kw_42 = kw(ctx, "COLLATE ");
            uint32_t sp_43 = span_text(ctx, node->collation_name);
            uint32_t cat_44_items[] = { kw_42, sp_43 };
            uint32_t cat_44 = doc_concat(&ctx->docs, cat_44_items, 2);
            sw_6 = cat_44;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES: sw_6 = format_node(ctx, node->fk_clause); break;
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED: {
            uint32_t kw_46 = kw(ctx, "AS (");
            uint32_t ch_47 = format_node(ctx, node->generated_expr);
            uint32_t kw_48 = kw(ctx, ")");
            uint32_t cond_49 = SYNTAQLITE_NULL_DOC;
            if (node->generated_storage == SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED) cond_49 = kw(ctx, " STORED");
            uint32_t cat_51_items[] = { kw_46, ch_47, kw_48, cond_49 };
            uint32_t cat_51 = doc_concat(&ctx->docs, cat_51_items, 4);
            sw_6 = cat_51;
            break;
        }
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL: sw_6 = kw(ctx, "NULL"); break;
        default: break;
    }
    uint32_t cat_53_items[] = { cond_1, sw_6 };
    return doc_concat(&ctx->docs, cat_53_items, 2);
}

static uint32_t format_column_constraint_list(FmtCtx *ctx, SyntaqliteColumnConstraintList *node) {
    uint32_t _buf_1[node->count * 2 > 0 ? node->count * 2 : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t kw_4 = kw(ctx, " ");
            _buf_1[_n_2++] = kw_4;
        }
        uint32_t item_5 = format_node(ctx, _child_id);
        _buf_1[_n_2++] = item_5;
    }
    return doc_concat(&ctx->docs, _buf_1, _n_2);
}

static uint32_t format_column_def(FmtCtx *ctx, SyntaqliteColumnDef *node) {
    uint32_t sp_1 = span_text(ctx, node->column_name);
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->type_name.length > 0) {
        uint32_t kw_3 = kw(ctx, " ");
        uint32_t sp_4 = span_text(ctx, node->type_name);
        uint32_t cat_5_items[] = { kw_3, sp_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
        cond_2 = cat_5;
    }
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->constraints != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_7 = kw(ctx, " ");
        uint32_t ch_8 = format_node(ctx, node->constraints);
        uint32_t cat_9_items[] = { kw_7, ch_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
        cond_6 = cat_9;
    }
    uint32_t cat_10_items[] = { sp_1, cond_2, cond_6 };
    return doc_concat(&ctx->docs, cat_10_items, 3);
}

static uint32_t format_column_def_list(FmtCtx *ctx, SyntaqliteColumnDefList *node) {
    uint32_t _buf_1[node->count * 2 > 0 ? node->count * 2 : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t kw_4 = kw(ctx, ",");
            uint32_t ln_5 = doc_line(&ctx->docs);
            uint32_t cat_6_items[] = { kw_4, ln_5 };
            uint32_t cat_6 = doc_concat(&ctx->docs, cat_6_items, 2);
            _buf_1[_n_2++] = cat_6;
        }
        uint32_t item_7 = format_node(ctx, _child_id);
        _buf_1[_n_2++] = item_7;
    }
    return doc_concat(&ctx->docs, _buf_1, _n_2);
}

static uint32_t format_table_constraint(FmtCtx *ctx, SyntaqliteTableConstraint *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->constraint_name.length > 0) {
        uint32_t kw_2 = kw(ctx, "CONSTRAINT ");
        uint32_t sp_3 = span_text(ctx, node->constraint_name);
        uint32_t kw_4 = kw(ctx, " ");
        uint32_t cat_5_items[] = { kw_2, sp_3, kw_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 3);
        cond_1 = cat_5;
    }
    uint32_t sw_6 = SYNTAQLITE_NULL_DOC;
    switch (node->kind) {
        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY: {
            uint32_t kw_7 = kw(ctx, "PRIMARY KEY(");
            uint32_t ch_8 = format_node(ctx, node->columns);
            uint32_t kw_9 = kw(ctx, ")");
            uint32_t sw_10 = SYNTAQLITE_NULL_DOC;
            switch (node->onconf) {
                case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_10 = kw(ctx, " ON CONFLICT ROLLBACK"); break;
                case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_10 = kw(ctx, " ON CONFLICT ABORT"); break;
                case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_10 = kw(ctx, " ON CONFLICT FAIL"); break;
                case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_10 = kw(ctx, " ON CONFLICT IGNORE"); break;
                case SYNTAQLITE_CONFLICT_ACTION_REPLACE: sw_10 = kw(ctx, " ON CONFLICT REPLACE"); break;
                default: break;
            }
            uint32_t cat_16_items[] = { kw_7, ch_8, kw_9, sw_10 };
            uint32_t cat_16 = doc_concat(&ctx->docs, cat_16_items, 4);
            sw_6 = cat_16;
            break;
        }
        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE: {
            uint32_t kw_17 = kw(ctx, "UNIQUE(");
            uint32_t ch_18 = format_node(ctx, node->columns);
            uint32_t kw_19 = kw(ctx, ")");
            uint32_t sw_20 = SYNTAQLITE_NULL_DOC;
            switch (node->onconf) {
                case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: sw_20 = kw(ctx, " ON CONFLICT ROLLBACK"); break;
                case SYNTAQLITE_CONFLICT_ACTION_ABORT: sw_20 = kw(ctx, " ON CONFLICT ABORT"); break;
                case SYNTAQLITE_CONFLICT_ACTION_FAIL: sw_20 = kw(ctx, " ON CONFLICT FAIL"); break;
                case SYNTAQLITE_CONFLICT_ACTION_IGNORE: sw_20 = kw(ctx, " ON CONFLICT IGNORE"); break;
                case SYNTAQLITE_CONFLICT_ACTION_REPLACE: sw_20 = kw(ctx, " ON CONFLICT REPLACE"); break;
                default: break;
            }
            uint32_t cat_26_items[] = { kw_17, ch_18, kw_19, sw_20 };
            uint32_t cat_26 = doc_concat(&ctx->docs, cat_26_items, 4);
            sw_6 = cat_26;
            break;
        }
        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK: {
            uint32_t kw_27 = kw(ctx, "CHECK(");
            uint32_t ch_28 = format_node(ctx, node->check_expr);
            uint32_t kw_29 = kw(ctx, ")");
            uint32_t cat_30_items[] = { kw_27, ch_28, kw_29 };
            uint32_t cat_30 = doc_concat(&ctx->docs, cat_30_items, 3);
            sw_6 = cat_30;
            break;
        }
        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY: {
            uint32_t kw_31 = kw(ctx, "FOREIGN KEY(");
            uint32_t ch_32 = format_node(ctx, node->columns);
            uint32_t kw_33 = kw(ctx, ") ");
            uint32_t ch_34 = format_node(ctx, node->fk_clause);
            uint32_t cat_35_items[] = { kw_31, ch_32, kw_33, ch_34 };
            uint32_t cat_35 = doc_concat(&ctx->docs, cat_35_items, 4);
            sw_6 = cat_35;
            break;
        }
        default: break;
    }
    uint32_t cat_36_items[] = { cond_1, sw_6 };
    return doc_concat(&ctx->docs, cat_36_items, 2);
}

static uint32_t format_table_constraint_list(FmtCtx *ctx, SyntaqliteTableConstraintList *node) {
    uint32_t _buf_1[node->count * 2 > 0 ? node->count * 2 : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t kw_4 = kw(ctx, ",");
            uint32_t ln_5 = doc_line(&ctx->docs);
            uint32_t cat_6_items[] = { kw_4, ln_5 };
            uint32_t cat_6 = doc_concat(&ctx->docs, cat_6_items, 2);
            _buf_1[_n_2++] = cat_6;
        }
        uint32_t item_7 = format_node(ctx, _child_id);
        _buf_1[_n_2++] = item_7;
    }
    return doc_concat(&ctx->docs, _buf_1, _n_2);
}

static uint32_t format_create_table_stmt(FmtCtx *ctx, SyntaqliteCreateTableStmt *node) {
    uint32_t kw_1 = kw(ctx, "CREATE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->is_temp) cond_2 = kw(ctx, " TEMP");
    uint32_t kw_4 = kw(ctx, " TABLE");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->if_not_exists) cond_5 = kw(ctx, " IF NOT EXISTS");
    uint32_t kw_7 = kw(ctx, " ");
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_9 = span_text(ctx, node->schema);
        uint32_t kw_10 = kw(ctx, ".");
        uint32_t cat_11_items[] = { sp_9, kw_10 };
        uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 2);
        cond_8 = cat_11;
    }
    uint32_t sp_12 = span_text(ctx, node->table_name);
    uint32_t cond_13 = SYNTAQLITE_NULL_DOC;
    if (node->columns != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_14 = kw(ctx, "(");
        uint32_t sl_15 = doc_softline(&ctx->docs);
        uint32_t ch_16 = format_node(ctx, node->columns);
        uint32_t cond_17 = SYNTAQLITE_NULL_DOC;
        if (node->table_constraints != SYNTAQLITE_NULL_NODE) {
            uint32_t kw_18 = kw(ctx, ",");
            uint32_t ln_19 = doc_line(&ctx->docs);
            uint32_t ch_20 = format_node(ctx, node->table_constraints);
            uint32_t cat_21_items[] = { kw_18, ln_19, ch_20 };
            uint32_t cat_21 = doc_concat(&ctx->docs, cat_21_items, 3);
            cond_17 = cat_21;
        }
        uint32_t cat_22_items[] = { sl_15, ch_16, cond_17 };
        uint32_t cat_22 = doc_concat(&ctx->docs, cat_22_items, 3);
        uint32_t nst_23 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_22);
        uint32_t grp_24 = doc_group(&ctx->docs, nst_23);
        uint32_t sl_25 = doc_softline(&ctx->docs);
        uint32_t kw_26 = kw(ctx, ")");
        uint32_t cat_27_items[] = { kw_14, grp_24, sl_25, kw_26 };
        uint32_t cat_27 = doc_concat(&ctx->docs, cat_27_items, 4);
        cond_13 = cat_27;
    }
    uint32_t cond_28 = SYNTAQLITE_NULL_DOC;
    if (node->as_select != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_29 = kw(ctx, " AS");
        uint32_t hl_30 = doc_hardline(&ctx->docs);
        uint32_t ch_31 = format_node(ctx, node->as_select);
        uint32_t cat_32_items[] = { kw_29, hl_30, ch_31 };
        uint32_t cat_32 = doc_concat(&ctx->docs, cat_32_items, 3);
        cond_28 = cat_32;
    }
    uint32_t cond_33 = SYNTAQLITE_NULL_DOC;
    if (node->flags.without_rowid) cond_33 = kw(ctx, " WITHOUT ROWID");
    uint32_t cond_35 = SYNTAQLITE_NULL_DOC;
    if (node->flags.strict) cond_35 = kw(ctx, " STRICT");
    uint32_t cat_37_items[] = { kw_1, cond_2, kw_4, cond_5, kw_7, cond_8, sp_12, cond_13, cond_28, cond_33, cond_35 };
    uint32_t cat_37 = doc_concat(&ctx->docs, cat_37_items, 11);
    return doc_group(&ctx->docs, cat_37);
}

static uint32_t format_frame_bound(FmtCtx *ctx, SyntaqliteFrameBound *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->bound_type) {
        case SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING: sw_1 = kw(ctx, "UNBOUNDED PRECEDING"); break;
        case SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING: {
            uint32_t ch_3 = format_node(ctx, node->expr);
            uint32_t kw_4 = kw(ctx, " PRECEDING");
            uint32_t cat_5_items[] = { ch_3, kw_4 };
            uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 2);
            sw_1 = cat_5;
            break;
        }
        case SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW: sw_1 = kw(ctx, "CURRENT ROW"); break;
        case SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING: {
            uint32_t ch_7 = format_node(ctx, node->expr);
            uint32_t kw_8 = kw(ctx, " FOLLOWING");
            uint32_t cat_9_items[] = { ch_7, kw_8 };
            uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
            sw_1 = cat_9;
            break;
        }
        case SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING: sw_1 = kw(ctx, "UNBOUNDED FOLLOWING"); break;
        default: break;
    }
    return sw_1;
}

static uint32_t format_frame_spec(FmtCtx *ctx, SyntaqliteFrameSpec *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->frame_type) {
        case SYNTAQLITE_FRAME_TYPE_RANGE: sw_1 = kw(ctx, "RANGE"); break;
        case SYNTAQLITE_FRAME_TYPE_ROWS: sw_1 = kw(ctx, "ROWS"); break;
        case SYNTAQLITE_FRAME_TYPE_GROUPS: sw_1 = kw(ctx, "GROUPS"); break;
        default: break;
    }
    uint32_t kw_5 = kw(ctx, " BETWEEN ");
    uint32_t ch_6 = format_node(ctx, node->start_bound);
    uint32_t kw_7 = kw(ctx, " AND ");
    uint32_t ch_8 = format_node(ctx, node->end_bound);
    uint32_t sw_9 = SYNTAQLITE_NULL_DOC;
    switch (node->exclude) {
        case SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS: sw_9 = kw(ctx, " EXCLUDE NO OTHERS"); break;
        case SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW: sw_9 = kw(ctx, " EXCLUDE CURRENT ROW"); break;
        case SYNTAQLITE_FRAME_EXCLUDE_GROUP: sw_9 = kw(ctx, " EXCLUDE GROUP"); break;
        case SYNTAQLITE_FRAME_EXCLUDE_TIES: sw_9 = kw(ctx, " EXCLUDE TIES"); break;
        default: break;
    }
    uint32_t cat_14_items[] = { sw_1, kw_5, ch_6, kw_7, ch_8, sw_9 };
    return doc_concat(&ctx->docs, cat_14_items, 6);
}

static uint32_t format_window_def(FmtCtx *ctx, SyntaqliteWindowDef *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->base_window_name.length > 0) {
        cond_1 = span_text(ctx, node->base_window_name);
    } else {
        uint32_t kw_3 = kw(ctx, "(");
        uint32_t cond_4 = SYNTAQLITE_NULL_DOC;
        if (node->partition_by != SYNTAQLITE_NULL_NODE) {
            uint32_t kw_5 = kw(ctx, "PARTITION BY ");
            uint32_t ch_6 = format_node(ctx, node->partition_by);
            uint32_t cat_7_items[] = { kw_5, ch_6 };
            uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 2);
            cond_4 = cat_7;
        }
        uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
        if (node->orderby != SYNTAQLITE_NULL_NODE) {
            uint32_t cond_9 = SYNTAQLITE_NULL_DOC;
            if (node->partition_by != SYNTAQLITE_NULL_NODE) cond_9 = kw(ctx, " ");
            uint32_t kw_11 = kw(ctx, "ORDER BY ");
            uint32_t ch_12 = format_node(ctx, node->orderby);
            uint32_t cat_13_items[] = { cond_9, kw_11, ch_12 };
            uint32_t cat_13 = doc_concat(&ctx->docs, cat_13_items, 3);
            cond_8 = cat_13;
        }
        uint32_t cond_14 = SYNTAQLITE_NULL_DOC;
        if (node->frame != SYNTAQLITE_NULL_NODE) {
            uint32_t cond_15 = SYNTAQLITE_NULL_DOC;
            if (node->partition_by != SYNTAQLITE_NULL_NODE) {
                cond_15 = kw(ctx, " ");
            } else {
                uint32_t cond_17 = SYNTAQLITE_NULL_DOC;
                if (node->orderby != SYNTAQLITE_NULL_NODE) cond_17 = kw(ctx, " ");
                cond_15 = cond_17;
            }
            uint32_t ch_19 = format_node(ctx, node->frame);
            uint32_t cat_20_items[] = { cond_15, ch_19 };
            uint32_t cat_20 = doc_concat(&ctx->docs, cat_20_items, 2);
            cond_14 = cat_20;
        }
        uint32_t kw_21 = kw(ctx, ")");
        uint32_t cat_22_items[] = { kw_3, cond_4, cond_8, cond_14, kw_21 };
        uint32_t cat_22 = doc_concat(&ctx->docs, cat_22_items, 5);
        cond_1 = cat_22;
    }
    return cond_1;
}

static uint32_t format_named_window_def(FmtCtx *ctx, SyntaqliteNamedWindowDef *node) {
    uint32_t sp_1 = span_text(ctx, node->window_name);
    uint32_t kw_2 = kw(ctx, " AS ");
    uint32_t ch_3 = format_node(ctx, node->window_def);
    uint32_t cat_4_items[] = { sp_1, kw_2, ch_3 };
    return doc_concat(&ctx->docs, cat_4_items, 3);
}

static uint32_t format_filter_over(FmtCtx *ctx, SyntaqliteFilterOver *node) {
    uint32_t cond_1 = SYNTAQLITE_NULL_DOC;
    if (node->filter_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_2 = kw(ctx, "FILTER (WHERE ");
        uint32_t ch_3 = format_node(ctx, node->filter_expr);
        uint32_t kw_4 = kw(ctx, ")");
        uint32_t cat_5_items[] = { kw_2, ch_3, kw_4 };
        uint32_t cat_5 = doc_concat(&ctx->docs, cat_5_items, 3);
        cond_1 = cat_5;
    }
    uint32_t cond_6 = SYNTAQLITE_NULL_DOC;
    if (node->over_def != SYNTAQLITE_NULL_NODE) {
        uint32_t kw_7 = kw(ctx, " OVER ");
        uint32_t ch_8 = format_node(ctx, node->over_def);
        uint32_t cat_9_items[] = { kw_7, ch_8 };
        uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
        cond_6 = cat_9;
    }
    uint32_t cond_10 = SYNTAQLITE_NULL_DOC;
    if (node->over_name.length > 0) {
        uint32_t kw_11 = kw(ctx, " OVER ");
        uint32_t sp_12 = span_text(ctx, node->over_name);
        uint32_t cat_13_items[] = { kw_11, sp_12 };
        uint32_t cat_13 = doc_concat(&ctx->docs, cat_13_items, 2);
        cond_10 = cat_13;
    }
    uint32_t cat_14_items[] = { cond_1, cond_6, cond_10 };
    return doc_concat(&ctx->docs, cat_14_items, 3);
}

static uint32_t format_trigger_event(FmtCtx *ctx, SyntaqliteTriggerEvent *node) {
    uint32_t sw_1 = SYNTAQLITE_NULL_DOC;
    switch (node->event_type) {
        case SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE: sw_1 = kw(ctx, "DELETE"); break;
        case SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT: sw_1 = kw(ctx, "INSERT"); break;
        case SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE: {
            uint32_t kw_4 = kw(ctx, "UPDATE");
            uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
            if (node->columns != SYNTAQLITE_NULL_NODE) {
                uint32_t kw_6 = kw(ctx, " OF ");
                uint32_t ch_7 = format_node(ctx, node->columns);
                uint32_t cat_8_items[] = { kw_6, ch_7 };
                uint32_t cat_8 = doc_concat(&ctx->docs, cat_8_items, 2);
                cond_5 = cat_8;
            }
            uint32_t cat_9_items[] = { kw_4, cond_5 };
            uint32_t cat_9 = doc_concat(&ctx->docs, cat_9_items, 2);
            sw_1 = cat_9;
            break;
        }
        default: break;
    }
    return sw_1;
}

static uint32_t format_trigger_cmd_list(FmtCtx *ctx, SyntaqliteTriggerCmdList *node) {
    uint32_t _buf_1[node->count * 2 > 0 ? node->count * 2 : 1];
    uint32_t _n_2 = 0;
    for (uint32_t _i = 0; _i < node->count; _i++) {
        uint32_t _child_id = node->children[_i];
        if (_i > 0) {
            uint32_t hl_4 = doc_hardline(&ctx->docs);
            _buf_1[_n_2++] = hl_4;
        }
        uint32_t item_5 = format_node(ctx, _child_id);
        uint32_t kw_6 = kw(ctx, ";");
        uint32_t cat_7_items[] = { item_5, kw_6 };
        uint32_t cat_7 = doc_concat(&ctx->docs, cat_7_items, 2);
        _buf_1[_n_2++] = cat_7;
    }
    return doc_concat(&ctx->docs, _buf_1, _n_2);
}

static uint32_t format_create_trigger_stmt(FmtCtx *ctx, SyntaqliteCreateTriggerStmt *node) {
    uint32_t kw_1 = kw(ctx, "CREATE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->is_temp) cond_2 = kw(ctx, " TEMP");
    uint32_t kw_4 = kw(ctx, " TRIGGER");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->if_not_exists) cond_5 = kw(ctx, " IF NOT EXISTS");
    uint32_t kw_7 = kw(ctx, " ");
    uint32_t cond_8 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_9 = span_text(ctx, node->schema);
        uint32_t kw_10 = kw(ctx, ".");
        uint32_t cat_11_items[] = { sp_9, kw_10 };
        uint32_t cat_11 = doc_concat(&ctx->docs, cat_11_items, 2);
        cond_8 = cat_11;
    }
    uint32_t sp_12 = span_text(ctx, node->trigger_name);
    uint32_t kw_13 = kw(ctx, " ");
    uint32_t sw_14 = SYNTAQLITE_NULL_DOC;
    switch (node->timing) {
        case SYNTAQLITE_TRIGGER_TIMING_BEFORE: sw_14 = kw(ctx, "BEFORE"); break;
        case SYNTAQLITE_TRIGGER_TIMING_AFTER: sw_14 = kw(ctx, "AFTER"); break;
        case SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF: sw_14 = kw(ctx, "INSTEAD OF"); break;
        default: break;
    }
    uint32_t kw_18 = kw(ctx, " ");
    uint32_t ch_19 = format_node(ctx, node->event);
    uint32_t kw_20 = kw(ctx, " ON ");
    uint32_t ch_21 = format_node(ctx, node->table);
    uint32_t cond_22 = SYNTAQLITE_NULL_DOC;
    if (node->when_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t hl_23 = doc_hardline(&ctx->docs);
        uint32_t kw_24 = kw(ctx, "WHEN ");
        uint32_t ch_25 = format_node(ctx, node->when_expr);
        uint32_t cat_26_items[] = { hl_23, kw_24, ch_25 };
        uint32_t cat_26 = doc_concat(&ctx->docs, cat_26_items, 3);
        cond_22 = cat_26;
    }
    uint32_t hl_27 = doc_hardline(&ctx->docs);
    uint32_t kw_28 = kw(ctx, "BEGIN");
    uint32_t cond_29 = SYNTAQLITE_NULL_DOC;
    if (node->body != SYNTAQLITE_NULL_NODE) {
        uint32_t hl_30 = doc_hardline(&ctx->docs);
        uint32_t ch_31 = format_node(ctx, node->body);
        uint32_t cat_32_items[] = { hl_30, ch_31 };
        uint32_t cat_32 = doc_concat(&ctx->docs, cat_32_items, 2);
        uint32_t nst_33 = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, cat_32);
        cond_29 = nst_33;
    }
    uint32_t hl_34 = doc_hardline(&ctx->docs);
    uint32_t kw_35 = kw(ctx, "END");
    uint32_t cat_36_items[] = { kw_1, cond_2, kw_4, cond_5, kw_7, cond_8, sp_12, kw_13, sw_14, kw_18, ch_19, kw_20, ch_21, cond_22, hl_27, kw_28, cond_29, hl_34, kw_35 };
    return doc_concat(&ctx->docs, cat_36_items, 19);
}

static uint32_t format_create_virtual_table_stmt(FmtCtx *ctx, SyntaqliteCreateVirtualTableStmt *node) {
    uint32_t kw_1 = kw(ctx, "CREATE VIRTUAL TABLE");
    uint32_t cond_2 = SYNTAQLITE_NULL_DOC;
    if (node->if_not_exists) cond_2 = kw(ctx, " IF NOT EXISTS");
    uint32_t kw_4 = kw(ctx, " ");
    uint32_t cond_5 = SYNTAQLITE_NULL_DOC;
    if (node->schema.length > 0) {
        uint32_t sp_6 = span_text(ctx, node->schema);
        uint32_t kw_7 = kw(ctx, ".");
        uint32_t cat_8_items[] = { sp_6, kw_7 };
        uint32_t cat_8 = doc_concat(&ctx->docs, cat_8_items, 2);
        cond_5 = cat_8;
    }
    uint32_t sp_9 = span_text(ctx, node->table_name);
    uint32_t kw_10 = kw(ctx, " USING ");
    uint32_t sp_11 = span_text(ctx, node->module_name);
    uint32_t cond_12 = SYNTAQLITE_NULL_DOC;
    if (node->module_args.length > 0) {
        uint32_t kw_13 = kw(ctx, "(");
        uint32_t sp_14 = span_text(ctx, node->module_args);
        uint32_t kw_15 = kw(ctx, ")");
        uint32_t cat_16_items[] = { kw_13, sp_14, kw_15 };
        uint32_t cat_16 = doc_concat(&ctx->docs, cat_16_items, 3);
        cond_12 = cat_16;
    }
    uint32_t cat_17_items[] = { kw_1, cond_2, kw_4, cond_5, sp_9, kw_10, sp_11, cond_12 };
    return doc_concat(&ctx->docs, cat_17_items, 8);
}

// ============ Dispatch ============

static uint32_t dispatch_format(FmtCtx *ctx, uint32_t node_id) {
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
        case SYNTAQLITE_NODE_QUALIFIED_NAME:
            return format_qualified_name(ctx, &node->qualified_name);
        case SYNTAQLITE_NODE_DROP_STMT:
            return format_drop_stmt(ctx, &node->drop_stmt);
        case SYNTAQLITE_NODE_ALTER_TABLE_STMT:
            return format_alter_table_stmt(ctx, &node->alter_table_stmt);
        case SYNTAQLITE_NODE_TRANSACTION_STMT:
            return format_transaction_stmt(ctx, &node->transaction_stmt);
        case SYNTAQLITE_NODE_SAVEPOINT_STMT:
            return format_savepoint_stmt(ctx, &node->savepoint_stmt);
        case SYNTAQLITE_NODE_PRAGMA_STMT:
            return format_pragma_stmt(ctx, &node->pragma_stmt);
        case SYNTAQLITE_NODE_ANALYZE_STMT:
            return format_analyze_stmt(ctx, &node->analyze_stmt);
        case SYNTAQLITE_NODE_ATTACH_STMT:
            return format_attach_stmt(ctx, &node->attach_stmt);
        case SYNTAQLITE_NODE_DETACH_STMT:
            return format_detach_stmt(ctx, &node->detach_stmt);
        case SYNTAQLITE_NODE_VACUUM_STMT:
            return format_vacuum_stmt(ctx, &node->vacuum_stmt);
        case SYNTAQLITE_NODE_EXPLAIN_STMT:
            return format_explain_stmt(ctx, &node->explain_stmt);
        case SYNTAQLITE_NODE_CREATE_INDEX_STMT:
            return format_create_index_stmt(ctx, &node->create_index_stmt);
        case SYNTAQLITE_NODE_CREATE_VIEW_STMT:
            return format_create_view_stmt(ctx, &node->create_view_stmt);
        case SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE:
            return format_foreign_key_clause(ctx, &node->foreign_key_clause);
        case SYNTAQLITE_NODE_COLUMN_CONSTRAINT:
            return format_column_constraint(ctx, &node->column_constraint);
        case SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST:
            return format_column_constraint_list(ctx, &node->column_constraint_list);
        case SYNTAQLITE_NODE_COLUMN_DEF:
            return format_column_def(ctx, &node->column_def);
        case SYNTAQLITE_NODE_COLUMN_DEF_LIST:
            return format_column_def_list(ctx, &node->column_def_list);
        case SYNTAQLITE_NODE_TABLE_CONSTRAINT:
            return format_table_constraint(ctx, &node->table_constraint);
        case SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST:
            return format_table_constraint_list(ctx, &node->table_constraint_list);
        case SYNTAQLITE_NODE_CREATE_TABLE_STMT:
            return format_create_table_stmt(ctx, &node->create_table_stmt);
        case SYNTAQLITE_NODE_FRAME_BOUND:
            return format_frame_bound(ctx, &node->frame_bound);
        case SYNTAQLITE_NODE_FRAME_SPEC:
            return format_frame_spec(ctx, &node->frame_spec);
        case SYNTAQLITE_NODE_WINDOW_DEF:
            return format_window_def(ctx, &node->window_def);
        case SYNTAQLITE_NODE_WINDOW_DEF_LIST:
            return format_comma_list(ctx, node->window_def_list.children, node->window_def_list.count);
        case SYNTAQLITE_NODE_NAMED_WINDOW_DEF:
            return format_named_window_def(ctx, &node->named_window_def);
        case SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST:
            return format_comma_list(ctx, node->named_window_def_list.children, node->named_window_def_list.count);
        case SYNTAQLITE_NODE_FILTER_OVER:
            return format_filter_over(ctx, &node->filter_over);
        case SYNTAQLITE_NODE_TRIGGER_EVENT:
            return format_trigger_event(ctx, &node->trigger_event);
        case SYNTAQLITE_NODE_TRIGGER_CMD_LIST:
            return format_trigger_cmd_list(ctx, &node->trigger_cmd_list);
        case SYNTAQLITE_NODE_CREATE_TRIGGER_STMT:
            return format_create_trigger_stmt(ctx, &node->create_trigger_stmt);
        case SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT:
            return format_create_virtual_table_stmt(ctx, &node->create_virtual_table_stmt);
        default:
            return kw(ctx, "/* UNSUPPORTED */");
    }
}

// ============ Main Dispatcher ============

static uint32_t format_node(FmtCtx *ctx, uint32_t node_id) {
    if (node_id == SYNTAQLITE_NULL_NODE) return SYNTAQLITE_NULL_DOC;

    uint32_t leading = emit_owned_comments(ctx, node_id, SYNTAQLITE_COMMENT_LEADING);
    uint32_t body = dispatch_format(ctx, node_id);
    uint32_t trailing = emit_owned_comments(ctx, node_id, SYNTAQLITE_COMMENT_TRAILING);

    uint32_t parts[3] = { leading, body, trailing };
    return doc_concat(&ctx->docs, parts, 3);
}

// ============ Public API ============

char *syntaqlite_format(SyntaqliteAstContext *astCtx, uint32_t root_id,
                        const char *source, SyntaqliteTokenList *token_list,
                        SyntaqliteFmtOptions *options) {
    SyntaqliteFmtOptions default_options = SYNTAQLITE_FMT_OPTIONS_DEFAULT;
    if (!options) options = &default_options;

    FmtCtx ctx;
    syntaqlite_doc_context_init(&ctx.docs);
    ctx.ast = &astCtx->ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = options;
    ctx.comment_att = syntaqlite_comment_attach(astCtx, root_id, source, token_list);

    uint32_t root_doc = format_node(&ctx, root_id);

    syntaqlite_comment_attachment_free(ctx.comment_att);

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

char *syntaqlite_format_debug_ir(SyntaqliteAstContext *astCtx, uint32_t root_id,
                                  const char *source, SyntaqliteTokenList *token_list,
                                  SyntaqliteFmtOptions *options) {
    SyntaqliteFmtOptions default_options = SYNTAQLITE_FMT_OPTIONS_DEFAULT;
    if (!options) options = &default_options;

    FmtCtx ctx;
    syntaqlite_doc_context_init(&ctx.docs);
    ctx.ast = &astCtx->ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = options;
    ctx.comment_att = syntaqlite_comment_attach(astCtx, root_id, source, token_list);

    uint32_t root_doc = format_node(&ctx, root_id);

    syntaqlite_comment_attachment_free(ctx.comment_att);

    // Print debug IR to a temporary file and read it back
    char *buf = NULL;
    size_t buf_size = 0;
    FILE *mem = open_memstream(&buf, &buf_size);
    if (mem) {
        syntaqlite_doc_debug_print(&ctx.docs, root_doc, mem, 0);
        fclose(mem);
    }

    syntaqlite_doc_context_cleanup(&ctx.docs);
    return buf;
}
