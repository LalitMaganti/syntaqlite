// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Comment attachment: pre-classifies comment tokens (LEADING vs TRAILING)
// and maps each to its owning AST node based on source ranges.

#ifndef SYNQ_SRC_FMT_COMMENT_ATTACH_H
#define SYNQ_SRC_FMT_COMMENT_ATTACH_H

#include "src/ast/ast_base.h"
#include "src/ast/ast_nodes_gen.h"
#include "src/fmt/token_list.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Comment placement relative to adjacent real tokens.
typedef enum {
    SYNQ_COMMENT_LEADING = 0,   // Before the next real token
    SYNQ_COMMENT_TRAILING = 1,  // After the previous real token, same line
} SynqCommentKind;

// Comment attachment: maps each comment token to its owning AST node.
typedef struct {
    uint32_t *owner_node;  // owner_node[tok_idx] = AST node_id (or SYNQ_NULL_NODE)
    uint8_t *position;     // SynqCommentKind (LEADING or TRAILING)
    uint32_t count;        // = token_list->count
} SynqCommentAttachment;

// Build comment attachment using neighbor-based matching on AST node ranges.
// Returns NULL if no comments exist.
SynqCommentAttachment *synq_comment_attach(
    SynqAstContext *astCtx, uint32_t root_id,
    const char *source, SynqTokenList *token_list);

void synq_comment_attachment_free(SynqCommentAttachment *att);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_FMT_COMMENT_ATTACH_H
