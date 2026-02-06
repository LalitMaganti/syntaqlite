// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Formatter helpers: context struct, inline builders, and helper declarations.
// Used by both the hand-written helpers (fmt_helpers.c) and the generated
// node formatters (fmt.c).

#ifndef SYNQ_SRC_FMT_FMT_HELPERS_H
#define SYNQ_SRC_FMT_FMT_HELPERS_H

#include "src/ast/ast_base.h"
#include "src/fmt/comment_attach.h"
#include "src/fmt/doc.h"
#include "src/fmt/fmt_options.h"
#include "src/fmt/token_list.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============ Formatter Context ============

typedef struct {
    SynqDocContext docs;
    SynqArena *ast;
    const char *source;
    SynqTokenList *token_list;
    SynqFmtOptions *options;
    SynqCommentAttachment *comment_att;
} SynqFmtCtx;

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
uint32_t synq_dispatch_format(SynqFmtCtx *ctx, uint32_t node_id);
uint32_t synq_format_comma_list(SynqFmtCtx *ctx, uint32_t *children, uint32_t count);
uint32_t synq_format_clause(SynqFmtCtx *ctx, const char *keyword, uint32_t body_id);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_FMT_FMT_HELPERS_H
