// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Comment attachment implementation: pre-classifies comment tokens and
// assigns each to its owning AST node based on source ranges and
// neighboring-node proximity.

#include "src/fmt/comment_attach.h"

#include "src/sqlite_tokens.h"

#include <stdlib.h>

// ---------------------------------------------------------------------------
// Comment classification (LEADING vs TRAILING)
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t *kinds;    // SyntaqliteCommentKind per token position
    uint32_t count;    // Equal to token_list->count
} CommentMap;

// Build a comment map by pre-classifying each TK_COMMENT token.
// Returns NULL if token_list is NULL, empty, or has no comments.
static CommentMap *comment_map_build(
    const char *source, SyntaqliteTokenList *token_list) {
    if (!token_list || token_list->count == 0) return NULL;

    // Check if there are any comments.
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

    // Classify each comment as LEADING or TRAILING.
    // TRAILING: no newline between previous real token's end and comment start.
    uint32_t prev_real_end = 0;
    int seen_real = 0;

    for (uint32_t i = 0; i < token_list->count; i++) {
        SyntaqliteRawToken *tok = &token_list->data[i];

        if (tok->type == TK_COMMENT) {
            uint8_t kind = SYNTAQLITE_COMMENT_LEADING;
            if (seen_real) {
                int has_newline = 0;
                for (uint32_t j = prev_real_end; j < tok->offset; j++) {
                    if (source[j] == '\n') { has_newline = 1; break; }
                }
                if (!has_newline) kind = SYNTAQLITE_COMMENT_TRAILING;
            }
            map->kinds[i] = kind;
        } else if (tok->type != TK_SPACE) {
            // Real token
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
// Node-to-comment ownership
// ---------------------------------------------------------------------------

// Find the innermost node whose range contains `offset`.
// Returns SYNTAQLITE_NULL_NODE if none found.
static uint32_t find_enclosing(SyntaqliteAstContext *astCtx,
                                uint32_t offset) {
    uint32_t best = SYNTAQLITE_NULL_NODE;
    uint32_t best_size = UINT32_MAX;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SyntaqliteSourceRange r = astCtx->ranges.data[n];
        if (r.first == 0 && r.last == 0) continue;
        if (offset >= r.first && offset < r.last) {
            uint32_t size = r.last - r.first;
            if (size < best_size) {
                best_size = size;
                best = n;
            }
        }
    }
    return best;
}

// Find the node whose range.last is closest to (but <=) `offset`.
// This is the "preceding" node — the one that ends just before the comment.
static uint32_t find_preceding_node(SyntaqliteAstContext *astCtx,
                                     uint32_t offset) {
    uint32_t best = SYNTAQLITE_NULL_NODE;
    uint32_t best_end = 0;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SyntaqliteSourceRange r = astCtx->ranges.data[n];
        if (r.first == 0 && r.last == 0) continue;
        if (r.last <= offset && r.last >= best_end) {
            // Prefer the smallest (innermost) node when tied on end position
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

// Find the node whose range.first is closest to (but >= comment end).
// This is the "following" node — the one that starts just after the comment.
static uint32_t find_following_node(SyntaqliteAstContext *astCtx,
                                     uint32_t comment_end) {
    uint32_t best = SYNTAQLITE_NULL_NODE;
    uint32_t best_start = UINT32_MAX;
    for (uint32_t n = 0; n < astCtx->ranges.count; n++) {
        SyntaqliteSourceRange r = astCtx->ranges.data[n];
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

SyntaqliteCommentAttachment *syntaqlite_comment_attach(
    SyntaqliteAstContext *astCtx, uint32_t root_id,
    const char *source, SyntaqliteTokenList *token_list) {

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

    // Build comment map for LEADING/TRAILING classification
    CommentMap *map = comment_map_build(source, token_list);

    SyntaqliteCommentAttachment *att = malloc(sizeof(SyntaqliteCommentAttachment));
    att->count = token_list->count;
    att->owner_node = calloc(token_list->count, sizeof(uint32_t));
    att->position = calloc(token_list->count, sizeof(uint8_t));

    // Initialize all owners to NULL_NODE
    for (uint32_t i = 0; i < att->count; i++)
        att->owner_node[i] = SYNTAQLITE_NULL_NODE;

    for (uint32_t i = 0; i < token_list->count; i++) {
        if (token_list->data[i].type != TK_COMMENT) continue;

        uint32_t c_offset = token_list->data[i].offset;
        uint32_t c_end = c_offset + token_list->data[i].length;
        uint8_t kind = map ? map->kinds[i] : SYNTAQLITE_COMMENT_LEADING;
        att->position[i] = kind;

        // 1. Try innermost enclosing node
        uint32_t owner = find_enclosing(astCtx, c_offset);
        if (owner != SYNTAQLITE_NULL_NODE) {
            att->owner_node[i] = owner;
            continue;
        }

        // 2. Comment is between nodes — find neighbor based on kind
        if (kind == SYNTAQLITE_COMMENT_TRAILING) {
            // Attach to the preceding node (the one that just ended)
            owner = find_preceding_node(astCtx, c_offset);
        } else {
            // Attach to the following node (the one about to start)
            owner = find_following_node(astCtx, c_end);
        }

        att->owner_node[i] = (owner != SYNTAQLITE_NULL_NODE) ? owner : root_id;
    }

    comment_map_free(map);
    return att;
}

void syntaqlite_comment_attachment_free(SyntaqliteCommentAttachment *att) {
    if (!att) return;
    free(att->owner_node);
    free(att->position);
    free(att);
}
