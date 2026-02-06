// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "src/fmt/comment_map.h"
#include "src/sqlite_tokens.h"

#include <stdlib.h>

SyntaqliteCommentMap *syntaqlite_comment_map_build(
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

    SyntaqliteCommentMap *map = malloc(sizeof(SyntaqliteCommentMap));
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

void syntaqlite_comment_map_free(SyntaqliteCommentMap *map) {
    if (!map) return;
    free(map->kinds);
    free(map);
}
