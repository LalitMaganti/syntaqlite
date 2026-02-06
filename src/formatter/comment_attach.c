// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Comment attachment: classifies each comment as LEADING or TRAILING,
// then assigns it to an owning AST node.
//
// Strategy — pure neighbor-based, no range enclosure:
//   1. Classify each comment as LEADING (newline before it) or TRAILING
//      (same line as previous real token).
//   2. TRAILING comments attach to the nearest preceding AST node.
//      If nothing follows the comment, it's at the end of the statement
//      and attaches to the root instead.
//   3. LEADING comments attach to the nearest following AST node.
//      If nothing precedes the comment, it's before the statement
//      and attaches to the root instead.

#include "src/formatter/comment_attach.h"

#include "syntaqlite/sqlite_tokens_gen.h"

#include <stdlib.h>

// ---------------------------------------------------------------------------
// Comment classification (LEADING vs TRAILING)
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t *kinds;   // SynqCommentKind per token position
    uint32_t count;
} CommentMap;

static CommentMap *comment_map_build(
    const char *source, SynqTokenList *token_list) {
    if (!token_list || token_list->count == 0) return NULL;

    int has_comments = 0;
    for (uint32_t i = 0; i < token_list->count; i++) {
        if (token_list->data[i].type == SYNTAQLITE_TOKEN_COMMENT) {
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

        if (tok->type == SYNTAQLITE_TOKEN_COMMENT) {
            uint8_t kind = SYNQ_COMMENT_LEADING;
            if (seen_real) {
                int has_newline = 0;
                for (uint32_t j = prev_real_end; j < tok->offset; j++) {
                    if (source[j] == '\n') { has_newline = 1; break; }
                }
                if (!has_newline) kind = SYNQ_COMMENT_TRAILING;
            }
            map->kinds[i] = kind;
        } else if (tok->type != SYNTAQLITE_TOKEN_SPACE) {
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

// ---------------------------------------------------------------------------
// Neighbor search
// ---------------------------------------------------------------------------

// Find the node ending closest to (but <=) `offset`.
// Prefers innermost (smallest) when tied on end position.
static uint32_t find_preceding_node(SynqAstContext *astCtx,
                                     uint32_t offset) {
    uint32_t best = SYNTAQLITE_NULL_NODE;
    uint32_t best_end = 0;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SynqSourceRange r = astCtx->ranges.data[n];
        if (r.first == 0 && r.last == 0) continue;
        if (r.last <= offset && r.last >= best_end) {
            if (r.last > best_end ||
                (best != SYNTAQLITE_NULL_NODE &&
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
    uint32_t best = SYNTAQLITE_NULL_NODE;
    uint32_t best_start = UINT32_MAX;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SynqSourceRange r = astCtx->ranges.data[n];
        if (r.first == 0 && r.last == 0) continue;
        if (r.first >= comment_end && r.first <= best_start) {
            if (r.first < best_start ||
                (best != SYNTAQLITE_NULL_NODE &&
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
        if (token_list->data[i].type == SYNTAQLITE_TOKEN_COMMENT) {
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
        att->owner_node[i] = SYNTAQLITE_NULL_NODE;

    for (uint32_t i = 0; i < token_list->count; i++) {
        if (token_list->data[i].type != SYNTAQLITE_TOKEN_COMMENT) continue;

        uint32_t c_offset = token_list->data[i].offset;
        uint32_t c_end = c_offset + token_list->data[i].length;
        uint8_t kind = map ? map->kinds[i] : SYNQ_COMMENT_LEADING;
        att->position[i] = kind;

        uint32_t owner;

        if (kind == SYNQ_COMMENT_TRAILING) {
            owner = find_preceding_node(astCtx, c_offset);
            // End-of-statement: nothing follows → attach to root
            if (owner != SYNTAQLITE_NULL_NODE) {
                uint32_t following = find_following_node(astCtx, c_end);
                if (following == SYNTAQLITE_NULL_NODE)
                    owner = root_id;
            }
        } else {
            owner = find_following_node(astCtx, c_end);
            // Start-of-statement: nothing precedes → attach to root
            if (owner != SYNTAQLITE_NULL_NODE) {
                uint32_t preceding = find_preceding_node(astCtx, c_offset);
                if (preceding == SYNTAQLITE_NULL_NODE)
                    owner = root_id;
            }
        }

        att->owner_node[i] = (owner != SYNTAQLITE_NULL_NODE) ? owner : root_id;
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
