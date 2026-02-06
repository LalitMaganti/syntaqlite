// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Formatter helpers: comment emission, comma lists, clause formatting,
// synq_format_node dispatcher, and public API entry points.

#include "src/fmt/fmt_helpers.h"

#include "src/fmt/doc_layout.h"
#include "src/fmt/fmt.h"

#include <stdlib.h>

// External definitions for inline functions (C99/C11 requirement).
extern inline uint32_t synq_kw(SynqFmtCtx *ctx, const char *text);
extern inline uint32_t synq_span_text(SynqFmtCtx *ctx, SynqSourceSpan span);
extern inline uint32_t synq_emit_single_comment(SynqFmtCtx *ctx, uint32_t tok_idx);

// ============ Comment Helpers ============

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

// ============ Comma-Separated List ============

uint32_t synq_format_comma_list(SynqFmtCtx *ctx, uint32_t *children, uint32_t count) {
    if (count == 0) return synq_kw(ctx, "");
    uint32_t buf[count * 3];
    uint32_t n = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (i > 0) {
            buf[n++] = synq_kw(ctx, ",");
            buf[n++] = synq_doc_line(&ctx->docs);
        }
        buf[n++] = synq_format_node(ctx, children[i]);
    }
    return synq_doc_group(&ctx->docs, synq_doc_concat(&ctx->docs, buf, n));
}

// ============ Clause Helper ============

uint32_t synq_format_clause(SynqFmtCtx *ctx, const char *keyword, uint32_t body_id) {
    if (body_id == SYNQ_NULL_NODE) return SYNQ_NULL_DOC;
    // Extract body's leading comments so they appear before the keyword.
    uint32_t leading = synq_emit_owned_comments(ctx, body_id, SYNQ_COMMENT_LEADING);
    uint32_t kw_doc = synq_kw(ctx, keyword);
    uint32_t body_doc = synq_dispatch_format(ctx, body_id);
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

// ============ Main Dispatcher ============

uint32_t synq_format_node(SynqFmtCtx *ctx, uint32_t node_id) {
    if (node_id == SYNQ_NULL_NODE) return SYNQ_NULL_DOC;

    uint32_t leading = synq_emit_owned_comments(ctx, node_id, SYNQ_COMMENT_LEADING);
    uint32_t body = synq_dispatch_format(ctx, node_id);
    uint32_t trailing = synq_emit_owned_comments(ctx, node_id, SYNQ_COMMENT_TRAILING);

    uint32_t parts[3] = { leading, body, trailing };
    return synq_doc_concat(&ctx->docs, parts, 3);
}

// ============ Public API ============

char *synq_format(SynqAstContext *astCtx, uint32_t root_id,
                        const char *source, SynqTokenList *token_list,
                        SynqFmtOptions *options) {
    SynqFmtOptions default_options = SYNQ_FMT_OPTIONS_DEFAULT;
    if (!options) options = &default_options;

    SynqFmtCtx ctx;
    synq_doc_context_init(&ctx.docs);
    ctx.ast = &astCtx->ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = options;
    ctx.comment_att = synq_comment_attach(astCtx, root_id, source, token_list);

    uint32_t root_doc = synq_format_node(&ctx, root_id);

    synq_comment_attachment_free(ctx.comment_att);

    if (root_doc == SYNQ_NULL_DOC) {
        synq_doc_context_cleanup(&ctx.docs);
        char *empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    char *result = synq_doc_layout(&ctx.docs, root_doc, options->target_width);
    synq_doc_context_cleanup(&ctx.docs);
    return result;
}

char *synq_format_debug_ir(SynqAstContext *astCtx, uint32_t root_id,
                                  const char *source, SynqTokenList *token_list,
                                  SynqFmtOptions *options) {
    SynqFmtOptions default_options = SYNQ_FMT_OPTIONS_DEFAULT;
    if (!options) options = &default_options;

    SynqFmtCtx ctx;
    synq_doc_context_init(&ctx.docs);
    ctx.ast = &astCtx->ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = options;
    ctx.comment_att = synq_comment_attach(astCtx, root_id, source, token_list);

    uint32_t root_doc = synq_format_node(&ctx, root_id);

    synq_comment_attachment_free(ctx.comment_att);

    // Print debug IR to a temporary file and read it back
    char *buf = NULL;
    size_t buf_size = 0;
    FILE *mem = open_memstream(&buf, &buf_size);
    if (mem) {
        synq_doc_debug_print(&ctx.docs, root_doc, mem, 0);
        fclose(mem);
    }

    synq_doc_context_cleanup(&ctx.docs);
    return buf;
}
