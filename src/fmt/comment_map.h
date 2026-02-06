// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Comment map: pre-classifies comment tokens before formatting.
//
// Each TK_COMMENT token is classified as LEADING (before the next real token)
// or TRAILING (after the previous real token, same line). The formatter uses
// this to emit comments with correct spacing without on-the-fly classification.

#ifndef SYNTAQLITE_SRC_FMT_COMMENT_MAP_H
#define SYNTAQLITE_SRC_FMT_COMMENT_MAP_H

#include "src/token_list.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Comment placement relative to adjacent real tokens.
typedef enum {
    SYNTAQLITE_COMMENT_LEADING = 0,   // Before the next real token
    SYNTAQLITE_COMMENT_TRAILING = 1,  // After the previous real token, same line
} SyntaqliteCommentKind;

// Pre-classified comment data indexed by token list position.
// kinds[i] is valid only when token_list->data[i].type == TK_COMMENT.
typedef struct {
    uint8_t *kinds;    // SyntaqliteCommentKind per token position
    uint32_t count;    // Equal to token_list->count
} SyntaqliteCommentMap;

// Build a comment map by pre-classifying each TK_COMMENT token.
// Returns NULL if token_list is NULL, empty, or has no comments.
// Caller must free with syntaqlite_comment_map_free().
SyntaqliteCommentMap *syntaqlite_comment_map_build(
    const char *source, SyntaqliteTokenList *token_list);

// Free a comment map.
void syntaqlite_comment_map_free(SyntaqliteCommentMap *map);

// Find the index of the first token with offset >= the given source offset.
// Returns token_list->count if no such token exists.
static inline uint32_t syntaqlite_token_find(
    SyntaqliteTokenList *token_list, uint32_t offset) {
    uint32_t lo = 0, hi = token_list->count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (token_list->data[mid].offset < offset)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

#ifdef __cplusplus
}
#endif

#endif // SYNTAQLITE_SRC_FMT_COMMENT_MAP_H
