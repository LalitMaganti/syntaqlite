// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Formatter context, comment attachment, and helper functions.

#ifndef SYNQ_SRC_FMT_FMT_HELPERS_H
#define SYNQ_SRC_FMT_FMT_HELPERS_H

#include "src/ast/ast_base.h"
#include "src/fmt/doc.h"
#include "src/fmt/fmt_ops.h"
#include "src/fmt/fmt_options.h"
#include "src/fmt/token_list.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============ Comment Attachment ============

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

// ============ Formatter Context ============

struct SynqFmtCtx {
    SynqDocContext docs;
    SynqArena *ast;
    const char *source;
    SynqTokenList *token_list;
    SynqFmtOptions *options;
    SynqCommentAttachment *comment_att;
};

// ============ Inline Helpers ============

inline uint32_t synq_kw(SynqFmtCtx *ctx, const char *text) {
    return synq_doc_text(&ctx->docs, text, (uint32_t)strlen(text));
}

inline uint32_t synq_span_text(SynqFmtCtx *ctx, SynqSourceSpan span) {
    if (span.length == 0) return SYNQ_NULL_DOC;
    return synq_doc_text(&ctx->docs, ctx->source + span.offset, span.length);
}

inline uint32_t synq_emit_single_comment(SynqFmtCtx *ctx, uint32_t tok_idx) {
    SynqRawToken *tok = &ctx->token_list->data[tok_idx];
    return synq_doc_text(&ctx->docs, ctx->source + tok->offset, tok->length);
}

// ============ Non-Inline Helpers ============

uint32_t synq_emit_owned_comments(SynqFmtCtx *ctx, uint32_t node_id, uint8_t kind);
uint32_t synq_format_node(SynqFmtCtx *ctx, uint32_t node_id);
uint32_t synq_format_clause(SynqFmtCtx *ctx, const char *keyword, uint32_t body_id);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_FMT_FMT_HELPERS_H
