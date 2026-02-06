// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Formatter internals: comment attachment, comment emission, clause formatting,
// and synq_format_node dispatcher.

#include "src/formatter/fmt_helpers.h"

#include "syntaqlite/sqlite_tokens_gen.h"

#include <stdlib.h>

// External definitions for inline functions (C99/C11 requirement).
extern inline uint32_t synq_kw(SynqFmtCtx *ctx, const char *text);
extern inline uint32_t synq_span_text(SynqFmtCtx *ctx, SynqSourceSpan span);
extern inline uint32_t synq_emit_single_comment(SynqFmtCtx *ctx, uint32_t tok_idx);

// ============ Comment Classification ============

typedef struct {
    uint8_t *kinds;   // SynqCommentKind per token position
    uint32_t count;
} CommentMap;

static CommentMap *comment_map_build(
    const char *source, SynqTokenList *token_list) {
    if (!token_list || token_list->count == 0) return NULL;

    int has_comments = 0;
    for (uint32_t i = 0; i < token_list->count; i++) {
        if (token_list->data[i].type == TK_COMMENT) {
            has_comments = 1;
            break;
        }
    }
    if (!has_comments) return NULL;

    CommentMap *map = malloc(sizeof(CommentMap));
    if (!map) return NULL;

    map->count = token_list->count;
    map->kinds = calloc(token_list->count, sizeof(uint8_t));
    if (!map->kinds) {
        free(map);
        return NULL;
    }

    uint32_t prev_real_end = 0;
    int seen_real = 0;

    for (uint32_t i = 0; i < token_list->count; i++) {
        SynqRawToken *tok = &token_list->data[i];

        if (tok->type == TK_COMMENT) {
            uint8_t kind = SYNQ_COMMENT_LEADING;
            if (seen_real) {
                int has_newline = 0;
                for (uint32_t j = prev_real_end; j < tok->offset; j++) {
                    if (source[j] == '\n') { has_newline = 1; break; }
                }
                if (!has_newline) kind = SYNQ_COMMENT_TRAILING;
            }
            map->kinds[i] = kind;
        } else if (tok->type != TK_SPACE) {
            prev_real_end = tok->offset + tok->length;
            seen_real = 1;
        }
    }

    return map;
}

static void comment_map_free(CommentMap *map) {
    if (!map) return;
    free(map->kinds);
    free(map);
}

// ============ Comment Attachment ============

// Find the node ending closest to (but <=) `offset`.
// Prefers innermost (smallest) when tied on end position.
static uint32_t find_preceding_node(SynqAstContext *astCtx,
                                     uint32_t offset) {
    uint32_t best = SYNQ_NULL_NODE;
    uint32_t best_end = 0;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SynqSourceRange r = astCtx->ranges.data[n];
        if (r.first == 0 && r.last == 0) continue;
        if (r.last <= offset && r.last >= best_end) {
            if (r.last > best_end ||
                (best != SYNQ_NULL_NODE &&
                 (r.last - r.first) < (astCtx->ranges.data[best].last -
                                        astCtx->ranges.data[best].first))) {
                best_end = r.last;
                best = n;
            }
        }
    }
    return best;
}

// Find the node starting closest to (but >= comment_end).
// Prefers innermost (smallest) when tied on start position.
static uint32_t find_following_node(SynqAstContext *astCtx,
                                     uint32_t comment_end) {
    uint32_t best = SYNQ_NULL_NODE;
    uint32_t best_start = UINT32_MAX;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SynqSourceRange r = astCtx->ranges.data[n];
        if (r.first == 0 && r.last == 0) continue;
        if (r.first >= comment_end && r.first <= best_start) {
            if (r.first < best_start ||
                (best != SYNQ_NULL_NODE &&
                 (r.last - r.first) < (astCtx->ranges.data[best].last -
                                        astCtx->ranges.data[best].first))) {
                best_start = r.first;
                best = n;
            }
        }
    }
    return best;
}

SynqCommentAttachment *synq_comment_attach(
    SynqAstContext *astCtx, uint32_t root_id,
    const char *source, SynqTokenList *token_list) {

    if (!token_list || token_list->count == 0) return NULL;

    // Check for comments
    int has_comments = 0;
    for (uint32_t i = 0; i < token_list->count; i++) {
        if (token_list->data[i].type == TK_COMMENT) {
            has_comments = 1;
            break;
        }
    }
    if (!has_comments) return NULL;

    CommentMap *map = comment_map_build(source, token_list);

    SynqCommentAttachment *att = malloc(sizeof(SynqCommentAttachment));
    att->count = token_list->count;
    att->owner_node = calloc(token_list->count, sizeof(uint32_t));
    att->position = calloc(token_list->count, sizeof(uint8_t));

    for (uint32_t i = 0; i < att->count; i++)
        att->owner_node[i] = SYNQ_NULL_NODE;

    for (uint32_t i = 0; i < token_list->count; i++) {
        if (token_list->data[i].type != TK_COMMENT) continue;

        uint32_t c_offset = token_list->data[i].offset;
        uint32_t c_end = c_offset + token_list->data[i].length;
        uint8_t kind = map ? map->kinds[i] : SYNQ_COMMENT_LEADING;
        att->position[i] = kind;

        uint32_t owner;

        if (kind == SYNQ_COMMENT_TRAILING) {
            owner = find_preceding_node(astCtx, c_offset);
            // End-of-statement: nothing follows -> attach to root
            if (owner != SYNQ_NULL_NODE) {
                uint32_t following = find_following_node(astCtx, c_end);
                if (following == SYNQ_NULL_NODE)
                    owner = root_id;
            }
        } else {
            owner = find_following_node(astCtx, c_end);
            // Start-of-statement: nothing precedes -> attach to root
            if (owner != SYNQ_NULL_NODE) {
                uint32_t preceding = find_preceding_node(astCtx, c_offset);
                if (preceding == SYNQ_NULL_NODE)
                    owner = root_id;
            }
        }

        att->owner_node[i] = (owner != SYNQ_NULL_NODE) ? owner : root_id;
    }

    comment_map_free(map);
    return att;
}

void synq_comment_attachment_free(SynqCommentAttachment *att) {
    if (!att) return;
    free(att->owner_node);
    free(att->position);
    free(att);
}

// ============ Comment Emission ============

uint32_t synq_emit_owned_comments(SynqFmtCtx *ctx, uint32_t node_id, uint8_t kind) {
    if (!ctx->comment_att) return SYNQ_NULL_DOC;
    uint32_t parts[64];
    uint32_t n = 0;
    for (uint32_t i = 0; i < ctx->comment_att->count && n < 62; i++) {
        if (ctx->comment_att->owner_node[i] != node_id) continue;
        if (ctx->comment_att->position[i] != kind) continue;

        int is_line = (ctx->source[ctx->token_list->data[i].offset] == '-');
        if (kind == SYNQ_COMMENT_LEADING) {
            parts[n++] = synq_emit_single_comment(ctx, i);
            if (n < 62) parts[n++] = is_line ? synq_doc_hardline(&ctx->docs)
                : synq_doc_text(&ctx->docs, " ", 1);
        } else {
            uint32_t sp_parts[2];
            sp_parts[0] = synq_doc_text(&ctx->docs, " ", 1);
            sp_parts[1] = synq_emit_single_comment(ctx, i);
            uint32_t inner = synq_doc_concat(&ctx->docs, sp_parts, 2);
            if (is_line) {
                // Line comments consume the rest of the line.
                parts[n++] = synq_doc_line_suffix(&ctx->docs, inner);
                if (n < 62) parts[n++] = synq_doc_break_parent(&ctx->docs);
            } else {
                // Block comments are inline.
                parts[n++] = inner;
            }
        }
    }
    if (n == 0) return SYNQ_NULL_DOC;
    return synq_doc_concat(&ctx->docs, parts, n);
}

// ============ Clause Helper ============

uint32_t synq_format_clause(SynqFmtCtx *ctx, const char *keyword, uint32_t body_id) {
    if (body_id == SYNQ_NULL_NODE) return SYNQ_NULL_DOC;
    // Extract body's leading comments so they appear before the keyword.
    uint32_t leading = synq_emit_owned_comments(ctx, body_id, SYNQ_COMMENT_LEADING);
    uint32_t kw_doc = synq_kw(ctx, keyword);
    uint32_t body_doc = synq_fmt_interpret(ctx, body_id);
    uint32_t trailing = synq_emit_owned_comments(ctx, body_id, SYNQ_COMMENT_TRAILING);
    if (body_doc == SYNQ_NULL_DOC) return SYNQ_NULL_DOC;
    uint32_t body_parts[] = { body_doc, trailing };
    uint32_t body_trail = synq_doc_concat(&ctx->docs, body_parts, 2);
    uint32_t inner_items[] = { synq_doc_line(&ctx->docs), body_trail };
    uint32_t inner = synq_doc_concat(&ctx->docs, inner_items, 2);
    uint32_t items[] = {
        synq_doc_line(&ctx->docs),
        leading,
        kw_doc,
        synq_doc_group(&ctx->docs, synq_doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner)),
    };
    return synq_doc_concat(&ctx->docs, items, 4);
}

// ============ Main Entry ============

uint32_t synq_format_node(SynqFmtCtx *ctx, uint32_t node_id) {
    if (node_id == SYNQ_NULL_NODE) return SYNQ_NULL_DOC;

    uint32_t leading = synq_emit_owned_comments(ctx, node_id, SYNQ_COMMENT_LEADING);
    uint32_t body = synq_fmt_interpret(ctx, node_id);
    uint32_t trailing = synq_emit_owned_comments(ctx, node_id, SYNQ_COMMENT_TRAILING);

    uint32_t parts[3] = { leading, body, trailing };
    return synq_doc_concat(&ctx->docs, parts, 3);
}

