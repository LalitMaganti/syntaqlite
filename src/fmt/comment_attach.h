// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Comment attachment: pre-classifies comment tokens (LEADING vs TRAILING)
// and maps each to its owning AST node based on source ranges.

#ifndef SYNTAQLITE_SRC_FMT_COMMENT_ATTACH_H
#define SYNTAQLITE_SRC_FMT_COMMENT_ATTACH_H

#include "src/ast/ast_base.h"
#include "src/ast/ast_nodes.h"
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

// Comment attachment: maps each comment token to its owning AST node.
typedef struct {
    uint32_t *owner_node;  // owner_node[tok_idx] = AST node_id (or SYNTAQLITE_NULL_NODE)
    uint8_t *position;     // SyntaqliteCommentKind (LEADING or TRAILING)
    uint32_t count;        // = token_list->count
} SyntaqliteCommentAttachment;

// Build comment attachment using neighbor-based matching on AST node ranges.
// Returns NULL if no comments exist.
SyntaqliteCommentAttachment *syntaqlite_comment_attach(
    SyntaqliteAstContext *astCtx, uint32_t root_id,
    const char *source, SyntaqliteTokenList *token_list);

void syntaqlite_comment_attachment_free(SyntaqliteCommentAttachment *att);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_FMT_COMMENT_ATTACH_H
