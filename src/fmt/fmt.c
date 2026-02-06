// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQL formatter implementation: walks AST and builds document tree,
// then renders via the layout engine.

#include "src/fmt/fmt.h"

#include "src/ast/ast_nodes.h"
#include "src/fmt/doc.h"
#include "src/fmt/doc_layout.h"

#include <stdarg.h>
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

// Emit a static keyword string (not owned by source).
static uint32_t kw(FmtCtx *ctx, const char *text) {
    return doc_text(&ctx->docs, text, (uint32_t)strlen(text));
}

// Emit text from a source span.
static uint32_t span_text(FmtCtx *ctx, SyntaqliteSourceSpan span) {
    if (span.length == 0) return SYNTAQLITE_NULL_DOC;
    return doc_text(&ctx->docs, ctx->source + span.offset, span.length);
}

// Build a concat from a variable number of doc IDs (terminated by SYNTAQLITE_NULL_DOC).
// Skips any SYNTAQLITE_NULL_DOC entries in the middle.
static uint32_t concat_docs(FmtCtx *ctx, uint32_t first, ...) {
    uint32_t cat = doc_concat_empty(&ctx->docs);
    if (cat == SYNTAQLITE_NULL_DOC) return cat;

    if (first != SYNTAQLITE_NULL_DOC) {
        cat = doc_concat_append(&ctx->docs, cat, first);
    }

    va_list args;
    va_start(args, first);
    for (;;) {
        uint32_t d = va_arg(args, uint32_t);
        if (d == SYNTAQLITE_NULL_DOC) break;
        cat = doc_concat_append(&ctx->docs, cat, d);
        if (cat == SYNTAQLITE_NULL_DOC) break;
    }
    va_end(args);
    return cat;
}

// Forward declaration
static uint32_t format_node(FmtCtx *ctx, uint32_t node_id);

// ============ Binary Operator Strings ============

static const char *binary_op_str(SyntaqliteBinaryOp op) {
    switch (op) {
        case SYNTAQLITE_BINARY_OP_PLUS: return "+";
        case SYNTAQLITE_BINARY_OP_MINUS: return "-";
        case SYNTAQLITE_BINARY_OP_STAR: return "*";
        case SYNTAQLITE_BINARY_OP_SLASH: return "/";
        case SYNTAQLITE_BINARY_OP_REM: return "%";
        case SYNTAQLITE_BINARY_OP_LT: return "<";
        case SYNTAQLITE_BINARY_OP_GT: return ">";
        case SYNTAQLITE_BINARY_OP_LE: return "<=";
        case SYNTAQLITE_BINARY_OP_GE: return ">=";
        case SYNTAQLITE_BINARY_OP_EQ: return "=";
        case SYNTAQLITE_BINARY_OP_NE: return "!=";
        case SYNTAQLITE_BINARY_OP_AND: return "AND";
        case SYNTAQLITE_BINARY_OP_OR: return "OR";
        case SYNTAQLITE_BINARY_OP_BITAND: return "&";
        case SYNTAQLITE_BINARY_OP_BITOR: return "|";
        case SYNTAQLITE_BINARY_OP_LSHIFT: return "<<";
        case SYNTAQLITE_BINARY_OP_RSHIFT: return ">>";
        case SYNTAQLITE_BINARY_OP_CONCAT: return "||";
        case SYNTAQLITE_BINARY_OP_PTR: return "->";
        default: return "??";
    }
}

static const char *unary_op_str(SyntaqliteUnaryOp op) {
    switch (op) {
        case SYNTAQLITE_UNARY_OP_MINUS: return "-";
        case SYNTAQLITE_UNARY_OP_PLUS: return "+";
        case SYNTAQLITE_UNARY_OP_BITNOT: return "~";
        case SYNTAQLITE_UNARY_OP_NOT: return "NOT ";
        default: return "??";
    }
}

static const char *is_op_str(SyntaqliteIsOp op) {
    switch (op) {
        case SYNTAQLITE_IS_OP_IS: return "IS";
        case SYNTAQLITE_IS_OP_IS_NOT: return "IS NOT";
        case SYNTAQLITE_IS_OP_ISNULL: return "ISNULL";
        case SYNTAQLITE_IS_OP_NOTNULL: return "NOTNULL";
        case SYNTAQLITE_IS_OP_IS_NOT_DISTINCT: return "IS NOT DISTINCT FROM";
        case SYNTAQLITE_IS_OP_IS_DISTINCT: return "IS DISTINCT FROM";
        default: return "??";
    }
}

static int is_op_postfix(SyntaqliteIsOp op) {
    return op == SYNTAQLITE_IS_OP_ISNULL || op == SYNTAQLITE_IS_OP_NOTNULL;
}

static const char *compound_op_str(SyntaqliteCompoundOp op) {
    switch (op) {
        case SYNTAQLITE_COMPOUND_OP_UNION: return "UNION";
        case SYNTAQLITE_COMPOUND_OP_UNION_ALL: return "UNION ALL";
        case SYNTAQLITE_COMPOUND_OP_INTERSECT: return "INTERSECT";
        case SYNTAQLITE_COMPOUND_OP_EXCEPT: return "EXCEPT";
        default: return "??";
    }
}

static const char *join_type_str(SyntaqliteJoinType jt) {
    switch (jt) {
        case SYNTAQLITE_JOIN_TYPE_COMMA: return ",";
        case SYNTAQLITE_JOIN_TYPE_INNER: return "JOIN";
        case SYNTAQLITE_JOIN_TYPE_LEFT: return "LEFT JOIN";
        case SYNTAQLITE_JOIN_TYPE_RIGHT: return "RIGHT JOIN";
        case SYNTAQLITE_JOIN_TYPE_FULL: return "FULL JOIN";
        case SYNTAQLITE_JOIN_TYPE_CROSS: return "CROSS JOIN";
        case SYNTAQLITE_JOIN_TYPE_NATURAL_INNER: return "NATURAL JOIN";
        case SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT: return "NATURAL LEFT JOIN";
        case SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT: return "NATURAL RIGHT JOIN";
        case SYNTAQLITE_JOIN_TYPE_NATURAL_FULL: return "NATURAL FULL JOIN";
        default: return "JOIN";
    }
}

static const char *conflict_action_str(SyntaqliteConflictAction ca) {
    switch (ca) {
        case SYNTAQLITE_CONFLICT_ACTION_ROLLBACK: return "OR ROLLBACK";
        case SYNTAQLITE_CONFLICT_ACTION_ABORT: return "OR ABORT";
        case SYNTAQLITE_CONFLICT_ACTION_FAIL: return "OR FAIL";
        case SYNTAQLITE_CONFLICT_ACTION_IGNORE: return "OR IGNORE";
        case SYNTAQLITE_CONFLICT_ACTION_REPLACE: return "OR REPLACE";
        default: return NULL;
    }
}

static const char *sort_order_str(SyntaqliteSortOrder so) {
    switch (so) {
        case SYNTAQLITE_SORT_ORDER_ASC: return "ASC";
        case SYNTAQLITE_SORT_ORDER_DESC: return "DESC";
        default: return "ASC";
    }
}

static const char *frame_type_str(SyntaqliteFrameType ft) {
    switch (ft) {
        case SYNTAQLITE_FRAME_TYPE_RANGE: return "RANGE";
        case SYNTAQLITE_FRAME_TYPE_ROWS: return "ROWS";
        case SYNTAQLITE_FRAME_TYPE_GROUPS: return "GROUPS";
        default: return "";
    }
}

static const char *frame_exclude_str(SyntaqliteFrameExclude fe) {
    switch (fe) {
        case SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS: return "EXCLUDE NO OTHERS";
        case SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW: return "EXCLUDE CURRENT ROW";
        case SYNTAQLITE_FRAME_EXCLUDE_GROUP: return "EXCLUDE GROUP";
        case SYNTAQLITE_FRAME_EXCLUDE_TIES: return "EXCLUDE TIES";
        default: return NULL;
    }
}

// ============ Comma-Separated List ============

// Format a list node's children as comma-separated with line breaks.
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

// Emit: hardline + keyword + nest(indent, line + body)
// Used for FROM, WHERE, GROUP BY, etc.
static uint32_t format_clause(FmtCtx *ctx, const char *keyword, uint32_t body_doc) {
    if (body_doc == SYNTAQLITE_NULL_DOC) return SYNTAQLITE_NULL_DOC;
    uint32_t inner = concat_docs(ctx,
        doc_line(&ctx->docs),
        body_doc,
        SYNTAQLITE_NULL_DOC);
    return concat_docs(ctx,
        doc_hardline(&ctx->docs),
        kw(ctx, keyword),
        doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner),
        SYNTAQLITE_NULL_DOC);
}

// ============ Node Formatters ============

static uint32_t format_select(FmtCtx *ctx, SyntaqliteSelectStmt *sel) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    // SELECT [DISTINCT]
    if (sel->flags.distinct) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "SELECT DISTINCT"));
    } else {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "SELECT"));
    }

    // Columns
    if (sel->columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, sel->columns);
        if (cols && cols->tag == SYNTAQLITE_NODE_RESULT_COLUMN_LIST) {
            uint32_t col_list = format_comma_list(ctx,
                cols->result_column_list.children, cols->result_column_list.count);
            uint32_t col_group = doc_group(&ctx->docs,
                doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width,
                    concat_docs(ctx, doc_line(&ctx->docs), col_list, SYNTAQLITE_NULL_DOC)));
            result = doc_concat_append(&ctx->docs, result, col_group);
        }
    }

    // FROM
    if (sel->from_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t from_body = format_node(ctx, sel->from_clause);
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "FROM", from_body));
    }

    // WHERE
    if (sel->where != SYNTAQLITE_NULL_NODE) {
        uint32_t where_body = format_node(ctx, sel->where);
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "WHERE", where_body));
    }

    // GROUP BY
    if (sel->groupby != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *gb = AST_NODE(ctx->ast, sel->groupby);
        uint32_t gb_body;
        if (gb && gb->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            gb_body = format_comma_list(ctx, gb->expr_list.children, gb->expr_list.count);
        } else {
            gb_body = format_node(ctx, sel->groupby);
        }
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "GROUP BY", gb_body));
    }

    // HAVING
    if (sel->having != SYNTAQLITE_NULL_NODE) {
        uint32_t having_body = format_node(ctx, sel->having);
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "HAVING", having_body));
    }

    // ORDER BY
    if (sel->orderby != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *ob = AST_NODE(ctx->ast, sel->orderby);
        uint32_t ob_body;
        if (ob && ob->tag == SYNTAQLITE_NODE_ORDER_BY_LIST) {
            ob_body = format_comma_list(ctx, ob->order_by_list.children, ob->order_by_list.count);
        } else {
            ob_body = format_node(ctx, sel->orderby);
        }
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "ORDER BY", ob_body));
    }

    // LIMIT
    if (sel->limit_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t limit_body = format_node(ctx, sel->limit_clause);
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "LIMIT", limit_body));
    }

    // WINDOW
    if (sel->window_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t window_body = format_node(ctx, sel->window_clause);
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "WINDOW", window_body));
    }

    return result;
}

static uint32_t format_binary_expr(FmtCtx *ctx, SyntaqliteBinaryExpr *expr) {
    uint32_t left = format_node(ctx, expr->left);
    uint32_t right = format_node(ctx, expr->right);
    const char *op = binary_op_str(expr->op);

    return doc_group(&ctx->docs, concat_docs(ctx,
        left,
        kw(ctx, " "),
        kw(ctx, op),
        doc_line(&ctx->docs),
        right,
        SYNTAQLITE_NULL_DOC));
}

static uint32_t format_unary_expr(FmtCtx *ctx, SyntaqliteUnaryExpr *expr) {
    uint32_t operand = format_node(ctx, expr->operand);
    const char *op = unary_op_str(expr->op);
    return concat_docs(ctx, kw(ctx, op), operand, SYNTAQLITE_NULL_DOC);
}

static uint32_t format_literal(FmtCtx *ctx, SyntaqliteLiteral *lit) {
    return span_text(ctx, lit->source);
}

static uint32_t format_column_ref(FmtCtx *ctx, SyntaqliteColumnRef *ref) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    if (ref->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ref->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    if (ref->table.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ref->table));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, ref->column));
    return result;
}

static uint32_t format_function_call(FmtCtx *ctx, SyntaqliteFunctionCall *func) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, func->func_name));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));

    if (func->flags.distinct) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "DISTINCT "));
    }

    if (func->flags.star) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "*"));
    } else if (func->args != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *args = AST_NODE(ctx->ast, func->args);
        if (args && args->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            uint32_t args_doc = format_comma_list(ctx,
                args->expr_list.children, args->expr_list.count);
            uint32_t args_group = doc_group(&ctx->docs,
                doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width,
                    concat_docs(ctx, doc_softline(&ctx->docs), args_doc, SYNTAQLITE_NULL_DOC)));
            result = doc_concat_append(&ctx->docs, result, args_group);
            result = doc_concat_append(&ctx->docs, result, doc_softline(&ctx->docs));
        }
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));

    // FILTER / OVER clause
    if (func->filter_clause != SYNTAQLITE_NULL_NODE || func->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t fo = format_node(ctx,
            func->filter_clause != SYNTAQLITE_NULL_NODE ? func->filter_clause : func->over_clause);
        if (fo != SYNTAQLITE_NULL_DOC) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
            result = doc_concat_append(&ctx->docs, result, fo);
        }
    }

    return result;
}

static uint32_t format_aggregate_function_call(FmtCtx *ctx, SyntaqliteAggregateFunctionCall *func) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, func->func_name));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));

    if (func->flags.distinct) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "DISTINCT "));
    }

    if (func->args != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *args = AST_NODE(ctx->ast, func->args);
        if (args && args->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            uint32_t args_doc = format_comma_list(ctx,
                args->expr_list.children, args->expr_list.count);
            result = doc_concat_append(&ctx->docs, result, doc_group(&ctx->docs, args_doc));
        }
    }

    // ORDER BY inside aggregate
    if (func->orderby != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " ORDER BY "));
        SyntaqliteNode *ob = AST_NODE(ctx->ast, func->orderby);
        if (ob && ob->tag == SYNTAQLITE_NODE_ORDER_BY_LIST) {
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, ob->order_by_list.children, ob->order_by_list.count));
        } else {
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, func->orderby));
        }
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));

    // FILTER / OVER
    if (func->filter_clause != SYNTAQLITE_NULL_NODE || func->over_clause != SYNTAQLITE_NULL_NODE) {
        uint32_t fo = format_node(ctx,
            func->filter_clause != SYNTAQLITE_NULL_NODE ? func->filter_clause : func->over_clause);
        if (fo != SYNTAQLITE_NULL_DOC) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
            result = doc_concat_append(&ctx->docs, result, fo);
        }
    }

    return result;
}

static uint32_t format_result_column(FmtCtx *ctx, SyntaqliteResultColumn *rc) {
    if (rc->flags.star && rc->expr == SYNTAQLITE_NULL_NODE) {
        return kw(ctx, "*");
    }

    uint32_t result = doc_concat_empty(&ctx->docs);

    if (rc->flags.star && rc->expr != SYNTAQLITE_NULL_NODE) {
        // table.*
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, rc->expr));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, ".*"));
    } else {
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, rc->expr));
    }

    if (rc->alias.length > 0) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " AS "));
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, rc->alias));
    }

    return result;
}

static uint32_t format_ordering_term(FmtCtx *ctx, SyntaqliteOrderingTerm *ot) {
    uint32_t result = format_node(ctx, ot->expr);

    if (ot->sort_order == SYNTAQLITE_SORT_ORDER_DESC) {
        result = concat_docs(ctx, result, kw(ctx, " "), kw(ctx, sort_order_str(ot->sort_order)), SYNTAQLITE_NULL_DOC);
    }

    if (ot->nulls_order == SYNTAQLITE_NULLS_ORDER_FIRST) {
        result = concat_docs(ctx, result, kw(ctx, " NULLS FIRST"), SYNTAQLITE_NULL_DOC);
    } else if (ot->nulls_order == SYNTAQLITE_NULLS_ORDER_LAST) {
        result = concat_docs(ctx, result, kw(ctx, " NULLS LAST"), SYNTAQLITE_NULL_DOC);
    }

    return result;
}

static uint32_t format_limit_clause(FmtCtx *ctx, SyntaqliteLimitClause *lim) {
    uint32_t result = format_node(ctx, lim->limit);
    if (lim->offset != SYNTAQLITE_NULL_NODE) {
        result = concat_docs(ctx, result, kw(ctx, " OFFSET "),
            format_node(ctx, lim->offset), SYNTAQLITE_NULL_DOC);
    }
    return result;
}

static uint32_t format_is_expr(FmtCtx *ctx, SyntaqliteIsExpr *expr) {
    if (is_op_postfix(expr->op)) {
        return concat_docs(ctx,
            format_node(ctx, expr->left),
            kw(ctx, " "),
            kw(ctx, is_op_str(expr->op)),
            SYNTAQLITE_NULL_DOC);
    }
    return concat_docs(ctx,
        format_node(ctx, expr->left),
        kw(ctx, " "),
        kw(ctx, is_op_str(expr->op)),
        kw(ctx, " "),
        format_node(ctx, expr->right),
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_between_expr(FmtCtx *ctx, SyntaqliteBetweenExpr *expr) {
    uint32_t result = format_node(ctx, expr->operand);
    if (expr->negated) {
        result = concat_docs(ctx, result, kw(ctx, " NOT BETWEEN "), SYNTAQLITE_NULL_DOC);
    } else {
        result = concat_docs(ctx, result, kw(ctx, " BETWEEN "), SYNTAQLITE_NULL_DOC);
    }
    result = concat_docs(ctx, result,
        format_node(ctx, expr->low),
        kw(ctx, " AND "),
        format_node(ctx, expr->high),
        SYNTAQLITE_NULL_DOC);
    return result;
}

static uint32_t format_like_expr(FmtCtx *ctx, SyntaqliteLikeExpr *expr) {
    uint32_t result = format_node(ctx, expr->operand);
    if (expr->negated) {
        result = concat_docs(ctx, result, kw(ctx, " NOT LIKE "), SYNTAQLITE_NULL_DOC);
    } else {
        result = concat_docs(ctx, result, kw(ctx, " LIKE "), SYNTAQLITE_NULL_DOC);
    }
    result = concat_docs(ctx, result, format_node(ctx, expr->pattern), SYNTAQLITE_NULL_DOC);
    if (expr->escape != SYNTAQLITE_NULL_NODE) {
        result = concat_docs(ctx, result, kw(ctx, " ESCAPE "),
            format_node(ctx, expr->escape), SYNTAQLITE_NULL_DOC);
    }
    return result;
}

static uint32_t format_case_expr(FmtCtx *ctx, SyntaqliteCaseExpr *expr) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "CASE"));

    if (expr->operand != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, expr->operand));
    }

    // WHEN clauses
    if (expr->whens != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *whens = AST_NODE(ctx->ast, expr->whens);
        if (whens && whens->tag == SYNTAQLITE_NODE_CASE_WHEN_LIST) {
            for (uint32_t i = 0; i < whens->case_when_list.count; i++) {
                SyntaqliteNode *w = AST_NODE(ctx->ast, whens->case_when_list.children[i]);
                if (w && w->tag == SYNTAQLITE_NODE_CASE_WHEN) {
                    result = doc_concat_append(&ctx->docs, result, kw(ctx, " WHEN "));
                    result = doc_concat_append(&ctx->docs, result,
                        format_node(ctx, w->case_when.when_expr));
                    result = doc_concat_append(&ctx->docs, result, kw(ctx, " THEN "));
                    result = doc_concat_append(&ctx->docs, result,
                        format_node(ctx, w->case_when.then_expr));
                }
            }
        }
    }

    if (expr->else_expr != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " ELSE "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, expr->else_expr));
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, " END"));
    return result;
}

static uint32_t format_in_expr(FmtCtx *ctx, SyntaqliteInExpr *expr) {
    uint32_t result = format_node(ctx, expr->operand);
    if (expr->negated) {
        result = concat_docs(ctx, result, kw(ctx, " NOT IN "), SYNTAQLITE_NULL_DOC);
    } else {
        result = concat_docs(ctx, result, kw(ctx, " IN "), SYNTAQLITE_NULL_DOC);
    }

    SyntaqliteNode *source = AST_NODE(ctx->ast, expr->source);
    if (source) {
        if (source->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            uint32_t inner = format_comma_list(ctx,
                source->expr_list.children, source->expr_list.count);
            result = concat_docs(ctx, result, kw(ctx, "("),
                doc_group(&ctx->docs, inner), kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
        } else {
            result = concat_docs(ctx, result, kw(ctx, "("),
                format_node(ctx, expr->source), kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
        }
    }

    return result;
}

static uint32_t format_subquery_expr(FmtCtx *ctx, SyntaqliteSubqueryExpr *expr) {
    return concat_docs(ctx,
        kw(ctx, "("),
        format_node(ctx, expr->select),
        kw(ctx, ")"),
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_exists_expr(FmtCtx *ctx, SyntaqliteExistsExpr *expr) {
    return concat_docs(ctx,
        kw(ctx, "EXISTS ("),
        format_node(ctx, expr->select),
        kw(ctx, ")"),
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_cast_expr(FmtCtx *ctx, SyntaqliteCastExpr *expr) {
    return concat_docs(ctx,
        kw(ctx, "CAST("),
        format_node(ctx, expr->expr),
        kw(ctx, " AS "),
        span_text(ctx, expr->type_name),
        kw(ctx, ")"),
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_collate_expr(FmtCtx *ctx, SyntaqliteCollateExpr *expr) {
    return concat_docs(ctx,
        format_node(ctx, expr->expr),
        kw(ctx, " COLLATE "),
        span_text(ctx, expr->collation),
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_table_ref(FmtCtx *ctx, SyntaqliteTableRef *ref) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    if (ref->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ref->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, ref->table_name));
    if (ref->alias.length > 0) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " AS "));
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ref->alias));
    }
    return result;
}

static uint32_t format_subquery_table_source(FmtCtx *ctx, SyntaqliteSubqueryTableSource *src) {
    uint32_t result = concat_docs(ctx,
        kw(ctx, "("),
        format_node(ctx, src->select),
        kw(ctx, ")"),
        SYNTAQLITE_NULL_DOC);
    if (src->alias.length > 0) {
        result = concat_docs(ctx, result, kw(ctx, " AS "),
            span_text(ctx, src->alias), SYNTAQLITE_NULL_DOC);
    }
    return result;
}

static uint32_t format_join_clause(FmtCtx *ctx, SyntaqliteJoinClause *join) {
    uint32_t left = format_node(ctx, join->left);
    uint32_t right = format_node(ctx, join->right);

    uint32_t result;
    if (join->join_type == SYNTAQLITE_JOIN_TYPE_COMMA) {
        result = concat_docs(ctx, left, kw(ctx, ", "), right, SYNTAQLITE_NULL_DOC);
    } else {
        result = concat_docs(ctx, left,
            doc_hardline(&ctx->docs),
            kw(ctx, join_type_str(join->join_type)),
            kw(ctx, " "),
            right,
            SYNTAQLITE_NULL_DOC);
    }

    if (join->on_expr != SYNTAQLITE_NULL_NODE) {
        uint32_t on_body = format_node(ctx, join->on_expr);
        uint32_t on_inner = concat_docs(ctx, doc_line(&ctx->docs), on_body, SYNTAQLITE_NULL_DOC);
        result = concat_docs(ctx, result,
            doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width,
                concat_docs(ctx, doc_hardline(&ctx->docs), kw(ctx, "ON"), on_inner, SYNTAQLITE_NULL_DOC)),
            SYNTAQLITE_NULL_DOC);
    }

    if (join->using_columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, join->using_columns);
        uint32_t using_inner;
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            using_inner = format_comma_list(ctx,
                cols->expr_list.children, cols->expr_list.count);
        } else {
            using_inner = format_node(ctx, join->using_columns);
        }
        result = concat_docs(ctx, result,
            kw(ctx, " USING ("), using_inner, kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
    }

    return result;
}

static uint32_t format_compound_select(FmtCtx *ctx, SyntaqliteCompoundSelect *cs) {
    uint32_t left = format_node(ctx, cs->left);
    uint32_t right = format_node(ctx, cs->right);
    return concat_docs(ctx,
        left,
        doc_hardline(&ctx->docs),
        kw(ctx, compound_op_str(cs->op)),
        doc_hardline(&ctx->docs),
        right,
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_with_clause(FmtCtx *ctx, SyntaqliteWithClause *wc) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    if (wc->recursive) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "WITH RECURSIVE "));
    } else {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "WITH "));
    }

    // CTE list
    if (wc->ctes != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *ctes = AST_NODE(ctx->ast, wc->ctes);
        if (ctes && ctes->tag == SYNTAQLITE_NODE_CTE_LIST) {
            uint32_t cte_docs = format_comma_list(ctx,
                ctes->cte_list.children, ctes->cte_list.count);
            result = doc_concat_append(&ctx->docs, result, cte_docs);
        }
    }

    result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, wc->select));
    return result;
}

static uint32_t format_cte_definition(FmtCtx *ctx, SyntaqliteCteDefinition *cte) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, cte->cte_name));

    // Column names
    if (cte->columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, cte->columns);
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
        }
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, " AS "));

    if (cte->materialized == SYNTAQLITE_MATERIALIZED_MATERIALIZED) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "MATERIALIZED "));
    } else if (cte->materialized == SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "NOT MATERIALIZED "));
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, cte->select));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));

    return result;
}

static uint32_t format_values_clause(FmtCtx *ctx, SyntaqliteValuesClause *vc) {
    uint32_t result = kw(ctx, "VALUES ");
    if (vc->rows != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *rows = AST_NODE(ctx->ast, vc->rows);
        if (rows && rows->tag == SYNTAQLITE_NODE_VALUES_ROW_LIST) {
            for (uint32_t i = 0; i < rows->values_row_list.count; i++) {
                if (i > 0) {
                    result = concat_docs(ctx, result, kw(ctx, ","), doc_line(&ctx->docs), SYNTAQLITE_NULL_DOC);
                }
                SyntaqliteNode *row = AST_NODE(ctx->ast, rows->values_row_list.children[i]);
                if (row && row->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                    result = concat_docs(ctx, result, kw(ctx, "("),
                        format_comma_list(ctx, row->expr_list.children, row->expr_list.count),
                        kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
                }
            }
        }
    }
    return result;
}

static uint32_t format_delete_stmt(FmtCtx *ctx, SyntaqliteDeleteStmt *del) {
    uint32_t result = kw(ctx, "DELETE");

    if (del->table != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "FROM", format_node(ctx, del->table)));
    }
    if (del->where != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "WHERE", format_node(ctx, del->where)));
    }
    return result;
}

static uint32_t format_update_stmt(FmtCtx *ctx, SyntaqliteUpdateStmt *upd) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    result = doc_concat_append(&ctx->docs, result, kw(ctx, "UPDATE"));

    const char *ca = conflict_action_str(upd->conflict_action);
    if (ca) {
        result = concat_docs(ctx, result, kw(ctx, " "), kw(ctx, ca), SYNTAQLITE_NULL_DOC);
    }

    result = concat_docs(ctx, result, kw(ctx, " "),
        format_node(ctx, upd->table), SYNTAQLITE_NULL_DOC);

    // SET
    if (upd->setlist != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *sl = AST_NODE(ctx->ast, upd->setlist);
        uint32_t set_body;
        if (sl && sl->tag == SYNTAQLITE_NODE_SET_CLAUSE_LIST) {
            set_body = format_comma_list(ctx,
                sl->set_clause_list.children, sl->set_clause_list.count);
        } else {
            set_body = format_node(ctx, upd->setlist);
        }
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "SET", set_body));
    }

    if (upd->from_clause != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "FROM", format_node(ctx, upd->from_clause)));
    }
    if (upd->where != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "WHERE", format_node(ctx, upd->where)));
    }
    return result;
}

static uint32_t format_set_clause(FmtCtx *ctx, SyntaqliteSetClause *sc) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    if (sc->column.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, sc->column));
    } else if (sc->columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, sc->columns);
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
        }
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, " = "));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, sc->value));
    return result;
}

static uint32_t format_insert_stmt(FmtCtx *ctx, SyntaqliteInsertStmt *ins) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    const char *ca = conflict_action_str(ins->conflict_action);
    if (ca && ins->conflict_action == SYNTAQLITE_CONFLICT_ACTION_REPLACE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "REPLACE"));
    } else {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "INSERT"));
        if (ca) {
            result = concat_docs(ctx, result, kw(ctx, " "), kw(ctx, ca), SYNTAQLITE_NULL_DOC);
        }
    }

    result = concat_docs(ctx, result, kw(ctx, " INTO "),
        format_node(ctx, ins->table), SYNTAQLITE_NULL_DOC);

    // Column list
    if (ins->columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, ins->columns);
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " ("));
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
        }
    }

    // Source (VALUES or SELECT)
    if (ins->source != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, ins->source));
    }

    return result;
}

static uint32_t format_qualified_name(FmtCtx *ctx, SyntaqliteQualifiedName *qn) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    if (qn->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, qn->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, qn->object_name));
    return result;
}

static uint32_t format_drop_stmt(FmtCtx *ctx, SyntaqliteDropStmt *drop) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "DROP "));

    switch (drop->object_type) {
        case SYNTAQLITE_DROP_OBJECT_TYPE_TABLE: result = doc_concat_append(&ctx->docs, result, kw(ctx, "TABLE")); break;
        case SYNTAQLITE_DROP_OBJECT_TYPE_INDEX: result = doc_concat_append(&ctx->docs, result, kw(ctx, "INDEX")); break;
        case SYNTAQLITE_DROP_OBJECT_TYPE_VIEW: result = doc_concat_append(&ctx->docs, result, kw(ctx, "VIEW")); break;
        case SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER: result = doc_concat_append(&ctx->docs, result, kw(ctx, "TRIGGER")); break;
    }

    if (drop->if_exists) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " IF EXISTS"));
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, drop->target));
    return result;
}

static uint32_t format_create_table_stmt(FmtCtx *ctx, SyntaqliteCreateTableStmt *ct) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "CREATE "));

    if (ct->is_temp) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "TEMP "));
    }
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "TABLE "));
    if (ct->if_not_exists) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "IF NOT EXISTS "));
    }

    if (ct->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ct->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, ct->table_name));

    // AS SELECT
    if (ct->as_select != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " AS "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, ct->as_select));
        return result;
    }

    // Columns
    if (ct->columns != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " ("));

        SyntaqliteNode *cols = AST_NODE(ctx->ast, ct->columns);
        uint32_t col_list = doc_concat_empty(&ctx->docs);
        if (cols && cols->tag == SYNTAQLITE_NODE_COLUMN_DEF_LIST) {
            col_list = format_comma_list(ctx,
                cols->column_def_list.children, cols->column_def_list.count);
        }

        // Table constraints
        if (ct->table_constraints != SYNTAQLITE_NULL_NODE) {
            SyntaqliteNode *tcs = AST_NODE(ctx->ast, ct->table_constraints);
            if (tcs && tcs->tag == SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST) {
                for (uint32_t i = 0; i < tcs->table_constraint_list.count; i++) {
                    col_list = doc_concat_append(&ctx->docs, col_list, kw(ctx, ","));
                    col_list = doc_concat_append(&ctx->docs, col_list, doc_line(&ctx->docs));
                    col_list = doc_concat_append(&ctx->docs, col_list,
                        format_node(ctx, tcs->table_constraint_list.children[i]));
                }
            }
        }

        uint32_t body = doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width,
            concat_docs(ctx, doc_hardline(&ctx->docs), col_list, SYNTAQLITE_NULL_DOC));
        result = doc_concat_append(&ctx->docs, result, body);
        result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
    }

    // WITHOUT ROWID / STRICT
    if (ct->flags.without_rowid) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " WITHOUT ROWID"));
    }
    if (ct->flags.strict) {
        if (ct->flags.without_rowid) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ","));
        }
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " STRICT"));
    }

    return result;
}

static uint32_t format_column_def(FmtCtx *ctx, SyntaqliteColumnDef *cd) {
    uint32_t result = span_text(ctx, cd->column_name);

    if (cd->type_name.length > 0) {
        result = concat_docs(ctx, result, kw(ctx, " "),
            span_text(ctx, cd->type_name), SYNTAQLITE_NULL_DOC);
    }

    if (cd->constraints != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *ccs = AST_NODE(ctx->ast, cd->constraints);
        if (ccs && ccs->tag == SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST) {
            for (uint32_t i = 0; i < ccs->column_constraint_list.count; i++) {
                result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
                result = doc_concat_append(&ctx->docs, result,
                    format_node(ctx, ccs->column_constraint_list.children[i]));
            }
        }
    }

    return result;
}

static uint32_t format_column_constraint(FmtCtx *ctx, SyntaqliteColumnConstraint *cc) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    if (cc->constraint_name.length > 0) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "CONSTRAINT "));
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, cc->constraint_name));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
    }

    switch (cc->kind) {
        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "PRIMARY KEY"));
            if (cc->sort_order == SYNTAQLITE_SORT_ORDER_DESC) {
                result = doc_concat_append(&ctx->docs, result, kw(ctx, " DESC"));
            }
            if (cc->onconf != SYNTAQLITE_CONFLICT_ACTION_DEFAULT) {
                result = concat_docs(ctx, result, kw(ctx, " ON CONFLICT "),
                    kw(ctx, syntaqlite_conflict_action_names[cc->onconf]), SYNTAQLITE_NULL_DOC);
            }
            if (cc->is_autoincrement) {
                result = doc_concat_append(&ctx->docs, result, kw(ctx, " AUTOINCREMENT"));
            }
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "NOT NULL"));
            if (cc->onconf != SYNTAQLITE_CONFLICT_ACTION_DEFAULT) {
                result = concat_docs(ctx, result, kw(ctx, " ON CONFLICT "),
                    kw(ctx, syntaqlite_conflict_action_names[cc->onconf]), SYNTAQLITE_NULL_DOC);
            }
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "UNIQUE"));
            if (cc->onconf != SYNTAQLITE_CONFLICT_ACTION_DEFAULT) {
                result = concat_docs(ctx, result, kw(ctx, " ON CONFLICT "),
                    kw(ctx, syntaqlite_conflict_action_names[cc->onconf]), SYNTAQLITE_NULL_DOC);
            }
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "CHECK ("));
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, cc->check_expr));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "DEFAULT "));
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, cc->default_expr));
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "REFERENCES "));
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, cc->fk_clause));
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "COLLATE "));
            result = doc_concat_append(&ctx->docs, result, span_text(ctx, cc->collation_name));
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "GENERATED ALWAYS AS ("));
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, cc->generated_expr));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
            if (cc->generated_storage == SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED) {
                result = doc_concat_append(&ctx->docs, result, kw(ctx, " STORED"));
            } else {
                result = doc_concat_append(&ctx->docs, result, kw(ctx, " VIRTUAL"));
            }
            break;

        case SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "NULL"));
            break;
    }

    return result;
}

static uint32_t format_table_constraint(FmtCtx *ctx, SyntaqliteTableConstraint *tc) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    if (tc->constraint_name.length > 0) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "CONSTRAINT "));
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, tc->constraint_name));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
    }

    switch (tc->kind) {
        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "PRIMARY KEY ("));
            if (tc->columns != SYNTAQLITE_NULL_NODE) {
                SyntaqliteNode *cols = AST_NODE(ctx->ast, tc->columns);
                if (cols && cols->tag == SYNTAQLITE_NODE_ORDER_BY_LIST) {
                    result = doc_concat_append(&ctx->docs, result,
                        format_comma_list(ctx, cols->order_by_list.children, cols->order_by_list.count));
                }
            }
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
            if (tc->onconf != SYNTAQLITE_CONFLICT_ACTION_DEFAULT) {
                result = concat_docs(ctx, result, kw(ctx, " ON CONFLICT "),
                    kw(ctx, syntaqlite_conflict_action_names[tc->onconf]), SYNTAQLITE_NULL_DOC);
            }
            break;

        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "UNIQUE ("));
            if (tc->columns != SYNTAQLITE_NULL_NODE) {
                SyntaqliteNode *cols = AST_NODE(ctx->ast, tc->columns);
                if (cols && cols->tag == SYNTAQLITE_NODE_ORDER_BY_LIST) {
                    result = doc_concat_append(&ctx->docs, result,
                        format_comma_list(ctx, cols->order_by_list.children, cols->order_by_list.count));
                }
            }
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
            if (tc->onconf != SYNTAQLITE_CONFLICT_ACTION_DEFAULT) {
                result = concat_docs(ctx, result, kw(ctx, " ON CONFLICT "),
                    kw(ctx, syntaqlite_conflict_action_names[tc->onconf]), SYNTAQLITE_NULL_DOC);
            }
            break;

        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "CHECK ("));
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, tc->check_expr));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
            break;

        case SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "FOREIGN KEY ("));
            if (tc->columns != SYNTAQLITE_NULL_NODE) {
                SyntaqliteNode *cols = AST_NODE(ctx->ast, tc->columns);
                if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                    result = doc_concat_append(&ctx->docs, result,
                        format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count));
                }
            }
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ") REFERENCES "));
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, tc->fk_clause));
            break;
    }

    return result;
}

static uint32_t format_foreign_key_clause(FmtCtx *ctx, SyntaqliteForeignKeyClause *fk) {
    uint32_t result = span_text(ctx, fk->ref_table);

    if (fk->ref_columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, fk->ref_columns);
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = concat_docs(ctx, result, kw(ctx, "("),
                format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count),
                kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
        }
    }

    if (fk->on_delete != SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION) {
        result = concat_docs(ctx, result, kw(ctx, " ON DELETE "),
            kw(ctx, syntaqlite_foreign_key_action_names[fk->on_delete]), SYNTAQLITE_NULL_DOC);
    }
    if (fk->on_update != SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION) {
        result = concat_docs(ctx, result, kw(ctx, " ON UPDATE "),
            kw(ctx, syntaqlite_foreign_key_action_names[fk->on_update]), SYNTAQLITE_NULL_DOC);
    }
    if (fk->is_deferred) {
        result = concat_docs(ctx, result, kw(ctx, " DEFERRABLE INITIALLY DEFERRED"), SYNTAQLITE_NULL_DOC);
    }

    return result;
}

static uint32_t format_create_index_stmt(FmtCtx *ctx, SyntaqliteCreateIndexStmt *ci) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "CREATE "));
    if (ci->is_unique) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "UNIQUE "));
    }
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "INDEX "));
    if (ci->if_not_exists) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "IF NOT EXISTS "));
    }
    if (ci->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ci->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, ci->index_name));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, " ON "));
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, ci->table_name));

    // Columns
    if (ci->columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, ci->columns);
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " ("));
        if (cols && cols->tag == SYNTAQLITE_NODE_ORDER_BY_LIST) {
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, cols->order_by_list.children, cols->order_by_list.count));
        }
        result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
    }

    if (ci->where != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result,
            format_clause(ctx, "WHERE", format_node(ctx, ci->where)));
    }

    return result;
}

static uint32_t format_create_view_stmt(FmtCtx *ctx, SyntaqliteCreateViewStmt *cv) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "CREATE "));
    if (cv->is_temp) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "TEMP "));
    }
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "VIEW "));
    if (cv->if_not_exists) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "IF NOT EXISTS "));
    }
    if (cv->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, cv->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, cv->view_name));

    if (cv->column_names != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, cv->column_names);
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count));
            result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
        }
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, " AS"));
    result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, cv->select));

    return result;
}

static uint32_t format_transaction_stmt(FmtCtx *ctx, SyntaqliteTransactionStmt *ts) {
    switch (ts->op) {
        case SYNTAQLITE_TRANSACTION_OP_BEGIN:
            if (ts->trans_type == SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE) {
                return kw(ctx, "BEGIN IMMEDIATE");
            } else if (ts->trans_type == SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE) {
                return kw(ctx, "BEGIN EXCLUSIVE");
            }
            return kw(ctx, "BEGIN");
        case SYNTAQLITE_TRANSACTION_OP_COMMIT:
            return kw(ctx, "COMMIT");
        case SYNTAQLITE_TRANSACTION_OP_ROLLBACK:
            return kw(ctx, "ROLLBACK");
        default:
            return kw(ctx, "BEGIN");
    }
}

static uint32_t format_explain_stmt(FmtCtx *ctx, SyntaqliteExplainStmt *es) {
    uint32_t result;
    if (es->explain_mode == SYNTAQLITE_EXPLAIN_MODE_QUERY_PLAN) {
        result = kw(ctx, "EXPLAIN QUERY PLAN");
    } else {
        result = kw(ctx, "EXPLAIN");
    }
    return concat_docs(ctx, result, doc_hardline(&ctx->docs),
        format_node(ctx, es->stmt), SYNTAQLITE_NULL_DOC);
}

static uint32_t format_raise_expr(FmtCtx *ctx, SyntaqliteRaiseExpr *expr) {
    uint32_t result = kw(ctx, "RAISE(");
    switch (expr->raise_type) {
        case SYNTAQLITE_RAISE_TYPE_IGNORE: result = concat_docs(ctx, result, kw(ctx, "IGNORE"), SYNTAQLITE_NULL_DOC); break;
        case SYNTAQLITE_RAISE_TYPE_ROLLBACK: result = concat_docs(ctx, result, kw(ctx, "ROLLBACK"), SYNTAQLITE_NULL_DOC); break;
        case SYNTAQLITE_RAISE_TYPE_ABORT: result = concat_docs(ctx, result, kw(ctx, "ABORT"), SYNTAQLITE_NULL_DOC); break;
        case SYNTAQLITE_RAISE_TYPE_FAIL: result = concat_docs(ctx, result, kw(ctx, "FAIL"), SYNTAQLITE_NULL_DOC); break;
    }
    if (expr->error_message != SYNTAQLITE_NULL_NODE) {
        result = concat_docs(ctx, result, kw(ctx, ", "),
            format_node(ctx, expr->error_message), SYNTAQLITE_NULL_DOC);
    }
    result = concat_docs(ctx, result, kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
    return result;
}

static uint32_t format_frame_bound(FmtCtx *ctx, SyntaqliteFrameBound *fb) {
    switch (fb->bound_type) {
        case SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING: return kw(ctx, "UNBOUNDED PRECEDING");
        case SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW: return kw(ctx, "CURRENT ROW");
        case SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING: return kw(ctx, "UNBOUNDED FOLLOWING");
        case SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING:
            return concat_docs(ctx, format_node(ctx, fb->expr), kw(ctx, " PRECEDING"), SYNTAQLITE_NULL_DOC);
        case SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING:
            return concat_docs(ctx, format_node(ctx, fb->expr), kw(ctx, " FOLLOWING"), SYNTAQLITE_NULL_DOC);
        default: return kw(ctx, "CURRENT ROW");
    }
}

static uint32_t format_frame_spec(FmtCtx *ctx, SyntaqliteFrameSpec *fs) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    if (fs->frame_type != SYNTAQLITE_FRAME_TYPE_NONE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, frame_type_str(fs->frame_type)));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
    }

    if (fs->end_bound != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "BETWEEN "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, fs->start_bound));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " AND "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, fs->end_bound));
    } else {
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, fs->start_bound));
    }

    const char *excl = frame_exclude_str(fs->exclude);
    if (excl) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, excl));
    }

    return result;
}

static uint32_t format_window_def(FmtCtx *ctx, SyntaqliteWindowDef *wd) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));

    if (wd->base_window_name.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, wd->base_window_name));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
    }

    if (wd->partition_by != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "PARTITION BY "));
        SyntaqliteNode *pb = AST_NODE(ctx->ast, wd->partition_by);
        if (pb && pb->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, pb->expr_list.children, pb->expr_list.count));
        } else {
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, wd->partition_by));
        }
        if (wd->orderby != SYNTAQLITE_NULL_NODE || wd->frame != SYNTAQLITE_NULL_NODE) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
        }
    }

    if (wd->orderby != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "ORDER BY "));
        SyntaqliteNode *ob = AST_NODE(ctx->ast, wd->orderby);
        if (ob && ob->tag == SYNTAQLITE_NODE_ORDER_BY_LIST) {
            result = doc_concat_append(&ctx->docs, result,
                format_comma_list(ctx, ob->order_by_list.children, ob->order_by_list.count));
        } else {
            result = doc_concat_append(&ctx->docs, result, format_node(ctx, wd->orderby));
        }
        if (wd->frame != SYNTAQLITE_NULL_NODE) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
        }
    }

    if (wd->frame != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, wd->frame));
    }

    result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
    return result;
}

static uint32_t format_filter_over(FmtCtx *ctx, SyntaqliteFilterOver *fo) {
    uint32_t result = doc_concat_empty(&ctx->docs);

    if (fo->filter_expr != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "FILTER (WHERE "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, fo->filter_expr));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
    }

    if (fo->over_def != SYNTAQLITE_NULL_NODE) {
        if (fo->filter_expr != SYNTAQLITE_NULL_NODE) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
        }
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "OVER "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, fo->over_def));
    } else if (fo->over_name.length > 0) {
        if (fo->filter_expr != SYNTAQLITE_NULL_NODE) {
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
        }
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "OVER "));
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, fo->over_name));
    }

    return result;
}

static uint32_t format_alter_table_stmt(FmtCtx *ctx, SyntaqliteAlterTableStmt *alt) {
    uint32_t result = concat_docs(ctx,
        kw(ctx, "ALTER TABLE "),
        format_node(ctx, alt->target),
        SYNTAQLITE_NULL_DOC);

    switch (alt->op) {
        case SYNTAQLITE_ALTER_OP_RENAME_TABLE:
            result = concat_docs(ctx, result, kw(ctx, " RENAME TO "),
                span_text(ctx, alt->new_name), SYNTAQLITE_NULL_DOC);
            break;
        case SYNTAQLITE_ALTER_OP_RENAME_COLUMN:
            result = concat_docs(ctx, result, kw(ctx, " RENAME COLUMN "),
                span_text(ctx, alt->old_name), kw(ctx, " TO "),
                span_text(ctx, alt->new_name), SYNTAQLITE_NULL_DOC);
            break;
        case SYNTAQLITE_ALTER_OP_DROP_COLUMN:
            result = concat_docs(ctx, result, kw(ctx, " DROP COLUMN "),
                span_text(ctx, alt->old_name), SYNTAQLITE_NULL_DOC);
            break;
        case SYNTAQLITE_ALTER_OP_ADD_COLUMN:
            result = concat_docs(ctx, result, kw(ctx, " ADD COLUMN "),
                span_text(ctx, alt->new_name), SYNTAQLITE_NULL_DOC);
            break;
    }

    return result;
}

static uint32_t format_pragma_stmt(FmtCtx *ctx, SyntaqlitePragmaStmt *ps) {
    uint32_t result = kw(ctx, "PRAGMA ");
    if (ps->schema.length > 0) {
        result = concat_docs(ctx, result, span_text(ctx, ps->schema), kw(ctx, "."), SYNTAQLITE_NULL_DOC);
    }
    result = concat_docs(ctx, result, span_text(ctx, ps->pragma_name), SYNTAQLITE_NULL_DOC);

    if (ps->pragma_form == SYNTAQLITE_PRAGMA_FORM_EQ) {
        result = concat_docs(ctx, result, kw(ctx, " = "),
            span_text(ctx, ps->value), SYNTAQLITE_NULL_DOC);
    } else if (ps->pragma_form == SYNTAQLITE_PRAGMA_FORM_CALL) {
        result = concat_docs(ctx, result, kw(ctx, "("),
            span_text(ctx, ps->value), kw(ctx, ")"), SYNTAQLITE_NULL_DOC);
    }

    return result;
}

static uint32_t format_savepoint_stmt(FmtCtx *ctx, SyntaqliteSavepointStmt *ss) {
    switch (ss->op) {
        case SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT:
            return concat_docs(ctx, kw(ctx, "SAVEPOINT "),
                span_text(ctx, ss->savepoint_name), SYNTAQLITE_NULL_DOC);
        case SYNTAQLITE_SAVEPOINT_OP_RELEASE:
            return concat_docs(ctx, kw(ctx, "RELEASE SAVEPOINT "),
                span_text(ctx, ss->savepoint_name), SYNTAQLITE_NULL_DOC);
        case SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO:
            return concat_docs(ctx, kw(ctx, "ROLLBACK TO SAVEPOINT "),
                span_text(ctx, ss->savepoint_name), SYNTAQLITE_NULL_DOC);
        default:
            return kw(ctx, "SAVEPOINT");
    }
}

static uint32_t format_analyze_stmt(FmtCtx *ctx, SyntaqliteAnalyzeStmt *as) {
    uint32_t result;
    if (as->kind == SYNTAQLITE_ANALYZE_KIND_REINDEX) {
        result = kw(ctx, "REINDEX");
    } else {
        result = kw(ctx, "ANALYZE");
    }

    if (as->schema.length > 0) {
        result = concat_docs(ctx, result, kw(ctx, " "),
            span_text(ctx, as->schema), SYNTAQLITE_NULL_DOC);
        if (as->target_name.length > 0) {
            result = concat_docs(ctx, result, kw(ctx, "."),
                span_text(ctx, as->target_name), SYNTAQLITE_NULL_DOC);
        }
    } else if (as->target_name.length > 0) {
        result = concat_docs(ctx, result, kw(ctx, " "),
            span_text(ctx, as->target_name), SYNTAQLITE_NULL_DOC);
    }

    return result;
}

static uint32_t format_attach_stmt(FmtCtx *ctx, SyntaqliteAttachStmt *as) {
    uint32_t result = concat_docs(ctx,
        kw(ctx, "ATTACH DATABASE "),
        format_node(ctx, as->filename),
        kw(ctx, " AS "),
        format_node(ctx, as->db_name),
        SYNTAQLITE_NULL_DOC);

    if (as->key != SYNTAQLITE_NULL_NODE) {
        result = concat_docs(ctx, result, kw(ctx, " KEY "),
            format_node(ctx, as->key), SYNTAQLITE_NULL_DOC);
    }

    return result;
}

static uint32_t format_detach_stmt(FmtCtx *ctx, SyntaqliteDetachStmt *ds) {
    return concat_docs(ctx, kw(ctx, "DETACH DATABASE "),
        format_node(ctx, ds->db_name), SYNTAQLITE_NULL_DOC);
}

static uint32_t format_vacuum_stmt(FmtCtx *ctx, SyntaqliteVacuumStmt *vs) {
    uint32_t result = kw(ctx, "VACUUM");
    if (vs->schema.length > 0) {
        result = concat_docs(ctx, result, kw(ctx, " "),
            span_text(ctx, vs->schema), SYNTAQLITE_NULL_DOC);
    }
    if (vs->into_expr != SYNTAQLITE_NULL_NODE) {
        result = concat_docs(ctx, result, kw(ctx, " INTO "),
            format_node(ctx, vs->into_expr), SYNTAQLITE_NULL_DOC);
    }
    return result;
}

static uint32_t format_create_trigger_stmt(FmtCtx *ctx, SyntaqliteCreateTriggerStmt *ct) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "CREATE "));
    if (ct->is_temp) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "TEMP "));
    }
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "TRIGGER "));
    if (ct->if_not_exists) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "IF NOT EXISTS "));
    }
    if (ct->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, ct->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, ct->trigger_name));

    // Timing
    switch (ct->timing) {
        case SYNTAQLITE_TRIGGER_TIMING_BEFORE:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " BEFORE"));
            break;
        case SYNTAQLITE_TRIGGER_TIMING_AFTER:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " AFTER"));
            break;
        case SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF:
            result = doc_concat_append(&ctx->docs, result, kw(ctx, " INSTEAD OF"));
            break;
    }

    // Event
    result = doc_concat_append(&ctx->docs, result, kw(ctx, " "));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, ct->event));

    // ON table
    result = doc_concat_append(&ctx->docs, result, kw(ctx, " ON "));
    result = doc_concat_append(&ctx->docs, result, format_node(ctx, ct->table));

    // WHEN
    if (ct->when_expr != SYNTAQLITE_NULL_NODE) {
        result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "WHEN "));
        result = doc_concat_append(&ctx->docs, result, format_node(ctx, ct->when_expr));
    }

    // BEGIN ... END
    result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "BEGIN"));

    if (ct->body != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *body = AST_NODE(ctx->ast, ct->body);
        if (body && body->tag == SYNTAQLITE_NODE_TRIGGER_CMD_LIST) {
            for (uint32_t i = 0; i < body->trigger_cmd_list.count; i++) {
                uint32_t stmt_doc = format_node(ctx, body->trigger_cmd_list.children[i]);
                uint32_t stmt_line = concat_docs(ctx, doc_hardline(&ctx->docs),
                    stmt_doc, kw(ctx, ";"), SYNTAQLITE_NULL_DOC);
                result = doc_concat_append(&ctx->docs, result,
                    doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, stmt_line));
            }
        }
    }

    result = doc_concat_append(&ctx->docs, result, doc_hardline(&ctx->docs));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "END"));

    return result;
}

static uint32_t format_trigger_event(FmtCtx *ctx, SyntaqliteTriggerEvent *te) {
    uint32_t result;
    switch (te->event_type) {
        case SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE: result = kw(ctx, "DELETE"); break;
        case SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT: result = kw(ctx, "INSERT"); break;
        case SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE: result = kw(ctx, "UPDATE"); break;
        default: result = kw(ctx, "UPDATE"); break;
    }

    if (te->columns != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *cols = AST_NODE(ctx->ast, te->columns);
        if (cols && cols->tag == SYNTAQLITE_NODE_EXPR_LIST) {
            result = concat_docs(ctx, result, kw(ctx, " OF "),
                format_comma_list(ctx, cols->expr_list.children, cols->expr_list.count),
                SYNTAQLITE_NULL_DOC);
        }
    }

    return result;
}

static uint32_t format_named_window_def(FmtCtx *ctx, SyntaqliteNamedWindowDef *nwd) {
    return concat_docs(ctx,
        span_text(ctx, nwd->window_name),
        kw(ctx, " AS "),
        format_node(ctx, nwd->window_def),
        SYNTAQLITE_NULL_DOC);
}

static uint32_t format_create_virtual_table_stmt(FmtCtx *ctx, SyntaqliteCreateVirtualTableStmt *cvt) {
    uint32_t result = doc_concat_empty(&ctx->docs);
    result = doc_concat_append(&ctx->docs, result, kw(ctx, "CREATE VIRTUAL TABLE "));
    if (cvt->if_not_exists) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "IF NOT EXISTS "));
    }
    if (cvt->schema.length > 0) {
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, cvt->schema));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "."));
    }
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, cvt->table_name));
    result = doc_concat_append(&ctx->docs, result, kw(ctx, " USING "));
    result = doc_concat_append(&ctx->docs, result, span_text(ctx, cvt->module_name));
    if (cvt->module_args.length > 0) {
        result = doc_concat_append(&ctx->docs, result, kw(ctx, "("));
        result = doc_concat_append(&ctx->docs, result, span_text(ctx, cvt->module_args));
        result = doc_concat_append(&ctx->docs, result, kw(ctx, ")"));
    }
    return result;
}

// ============ Main Dispatcher ============

static uint32_t format_node(FmtCtx *ctx, uint32_t node_id) {
    if (node_id == SYNTAQLITE_NULL_NODE) return SYNTAQLITE_NULL_DOC;

    SyntaqliteNode *node = AST_NODE(ctx->ast, node_id);
    if (!node) return SYNTAQLITE_NULL_DOC;

    switch (node->tag) {
        case SYNTAQLITE_NODE_SELECT_STMT:
            return format_select(ctx, &node->select_stmt);
        case SYNTAQLITE_NODE_BINARY_EXPR:
            return format_binary_expr(ctx, &node->binary_expr);
        case SYNTAQLITE_NODE_UNARY_EXPR:
            return format_unary_expr(ctx, &node->unary_expr);
        case SYNTAQLITE_NODE_LITERAL:
            return format_literal(ctx, &node->literal);
        case SYNTAQLITE_NODE_COLUMN_REF:
            return format_column_ref(ctx, &node->column_ref);
        case SYNTAQLITE_NODE_FUNCTION_CALL:
            return format_function_call(ctx, &node->function_call);
        case SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL:
            return format_aggregate_function_call(ctx, &node->aggregate_function_call);
        case SYNTAQLITE_NODE_RESULT_COLUMN:
            return format_result_column(ctx, &node->result_column);
        case SYNTAQLITE_NODE_ORDERING_TERM:
            return format_ordering_term(ctx, &node->ordering_term);
        case SYNTAQLITE_NODE_LIMIT_CLAUSE:
            return format_limit_clause(ctx, &node->limit_clause);
        case SYNTAQLITE_NODE_IS_EXPR:
            return format_is_expr(ctx, &node->is_expr);
        case SYNTAQLITE_NODE_BETWEEN_EXPR:
            return format_between_expr(ctx, &node->between_expr);
        case SYNTAQLITE_NODE_LIKE_EXPR:
            return format_like_expr(ctx, &node->like_expr);
        case SYNTAQLITE_NODE_CASE_EXPR:
            return format_case_expr(ctx, &node->case_expr);
        case SYNTAQLITE_NODE_IN_EXPR:
            return format_in_expr(ctx, &node->in_expr);
        case SYNTAQLITE_NODE_SUBQUERY_EXPR:
            return format_subquery_expr(ctx, &node->subquery_expr);
        case SYNTAQLITE_NODE_EXISTS_EXPR:
            return format_exists_expr(ctx, &node->exists_expr);
        case SYNTAQLITE_NODE_CAST_EXPR:
            return format_cast_expr(ctx, &node->cast_expr);
        case SYNTAQLITE_NODE_COLLATE_EXPR:
            return format_collate_expr(ctx, &node->collate_expr);
        case SYNTAQLITE_NODE_VARIABLE:
            return span_text(ctx, node->variable.source);
        case SYNTAQLITE_NODE_TABLE_REF:
            return format_table_ref(ctx, &node->table_ref);
        case SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE:
            return format_subquery_table_source(ctx, &node->subquery_table_source);
        case SYNTAQLITE_NODE_JOIN_CLAUSE:
            return format_join_clause(ctx, &node->join_clause);
        case SYNTAQLITE_NODE_COMPOUND_SELECT:
            return format_compound_select(ctx, &node->compound_select);
        case SYNTAQLITE_NODE_WITH_CLAUSE:
            return format_with_clause(ctx, &node->with_clause);
        case SYNTAQLITE_NODE_CTE_DEFINITION:
            return format_cte_definition(ctx, &node->cte_definition);
        case SYNTAQLITE_NODE_VALUES_CLAUSE:
            return format_values_clause(ctx, &node->values_clause);
        case SYNTAQLITE_NODE_DELETE_STMT:
            return format_delete_stmt(ctx, &node->delete_stmt);
        case SYNTAQLITE_NODE_UPDATE_STMT:
            return format_update_stmt(ctx, &node->update_stmt);
        case SYNTAQLITE_NODE_SET_CLAUSE:
            return format_set_clause(ctx, &node->set_clause);
        case SYNTAQLITE_NODE_INSERT_STMT:
            return format_insert_stmt(ctx, &node->insert_stmt);
        case SYNTAQLITE_NODE_QUALIFIED_NAME:
            return format_qualified_name(ctx, &node->qualified_name);
        case SYNTAQLITE_NODE_DROP_STMT:
            return format_drop_stmt(ctx, &node->drop_stmt);
        case SYNTAQLITE_NODE_CREATE_TABLE_STMT:
            return format_create_table_stmt(ctx, &node->create_table_stmt);
        case SYNTAQLITE_NODE_COLUMN_DEF:
            return format_column_def(ctx, &node->column_def);
        case SYNTAQLITE_NODE_COLUMN_CONSTRAINT:
            return format_column_constraint(ctx, &node->column_constraint);
        case SYNTAQLITE_NODE_TABLE_CONSTRAINT:
            return format_table_constraint(ctx, &node->table_constraint);
        case SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE:
            return format_foreign_key_clause(ctx, &node->foreign_key_clause);
        case SYNTAQLITE_NODE_CREATE_INDEX_STMT:
            return format_create_index_stmt(ctx, &node->create_index_stmt);
        case SYNTAQLITE_NODE_CREATE_VIEW_STMT:
            return format_create_view_stmt(ctx, &node->create_view_stmt);
        case SYNTAQLITE_NODE_TRANSACTION_STMT:
            return format_transaction_stmt(ctx, &node->transaction_stmt);
        case SYNTAQLITE_NODE_EXPLAIN_STMT:
            return format_explain_stmt(ctx, &node->explain_stmt);
        case SYNTAQLITE_NODE_RAISE_EXPR:
            return format_raise_expr(ctx, &node->raise_expr);
        case SYNTAQLITE_NODE_FRAME_BOUND:
            return format_frame_bound(ctx, &node->frame_bound);
        case SYNTAQLITE_NODE_FRAME_SPEC:
            return format_frame_spec(ctx, &node->frame_spec);
        case SYNTAQLITE_NODE_WINDOW_DEF:
            return format_window_def(ctx, &node->window_def);
        case SYNTAQLITE_NODE_FILTER_OVER:
            return format_filter_over(ctx, &node->filter_over);
        case SYNTAQLITE_NODE_ALTER_TABLE_STMT:
            return format_alter_table_stmt(ctx, &node->alter_table_stmt);
        case SYNTAQLITE_NODE_PRAGMA_STMT:
            return format_pragma_stmt(ctx, &node->pragma_stmt);
        case SYNTAQLITE_NODE_SAVEPOINT_STMT:
            return format_savepoint_stmt(ctx, &node->savepoint_stmt);
        case SYNTAQLITE_NODE_ANALYZE_STMT:
            return format_analyze_stmt(ctx, &node->analyze_stmt);
        case SYNTAQLITE_NODE_ATTACH_STMT:
            return format_attach_stmt(ctx, &node->attach_stmt);
        case SYNTAQLITE_NODE_DETACH_STMT:
            return format_detach_stmt(ctx, &node->detach_stmt);
        case SYNTAQLITE_NODE_VACUUM_STMT:
            return format_vacuum_stmt(ctx, &node->vacuum_stmt);
        case SYNTAQLITE_NODE_CREATE_TRIGGER_STMT:
            return format_create_trigger_stmt(ctx, &node->create_trigger_stmt);
        case SYNTAQLITE_NODE_TRIGGER_EVENT:
            return format_trigger_event(ctx, &node->trigger_event);
        case SYNTAQLITE_NODE_NAMED_WINDOW_DEF:
            return format_named_window_def(ctx, &node->named_window_def);
        case SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT:
            return format_create_virtual_table_stmt(ctx, &node->create_virtual_table_stmt);

        // List nodes: format children as comma-separated
        case SYNTAQLITE_NODE_EXPR_LIST:
            return format_comma_list(ctx, node->expr_list.children, node->expr_list.count);
        case SYNTAQLITE_NODE_RESULT_COLUMN_LIST:
            return format_comma_list(ctx, node->result_column_list.children, node->result_column_list.count);
        case SYNTAQLITE_NODE_ORDER_BY_LIST:
            return format_comma_list(ctx, node->order_by_list.children, node->order_by_list.count);
        case SYNTAQLITE_NODE_SET_CLAUSE_LIST:
            return format_comma_list(ctx, node->set_clause_list.children, node->set_clause_list.count);
        case SYNTAQLITE_NODE_CTE_LIST:
            return format_comma_list(ctx, node->cte_list.children, node->cte_list.count);
        case SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST:
            return format_comma_list(ctx, node->named_window_def_list.children, node->named_window_def_list.count);

        // Join prefix: format source
        case SYNTAQLITE_NODE_JOIN_PREFIX:
            return format_node(ctx, node->join_prefix.source);

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
