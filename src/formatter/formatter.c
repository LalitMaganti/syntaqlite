// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/formatter.h"

#include "src/formatter/fmt_helpers.h"
#include "src/parser/parser.h"

#include <stdlib.h>
#include <string.h>

// ============ High-level API ============

char *syntaqlite_format(const char *sql, uint32_t len,
                        const SyntaqliteFormatOptions *options) {
    SyntaqliteParser *parser = syntaqlite_parser_create(
        (SyntaqliteParserConfig){.collect_tokens = 1});
    if (!parser) return NULL;
    syntaqlite_parser_reset(parser, sql, len);

    // Accumulate formatted output from all statements.
    char *out = NULL;
    size_t out_len = 0;

    SyntaqliteParseResult result;
    while ((result = syntaqlite_parser_next(parser)).root != SYNTAQLITE_NULL_NODE) {
        if (result.error) {
            free(out);
            syntaqlite_parser_destroy(parser);
            return NULL;
        }

        char *stmt = syntaqlite_format_stmt(parser, result.root, options);
        if (!stmt) continue;

        size_t stmt_len = strlen(stmt);
        char *new_out = (char *)realloc(out, out_len + stmt_len + 1);
        if (!new_out) {
            free(stmt);
            free(out);
            syntaqlite_parser_destroy(parser);
            return NULL;
        }
        out = new_out;
        memcpy(out + out_len, stmt, stmt_len);
        out_len += stmt_len;
        out[out_len] = '\0';
        free(stmt);
    }

    if (result.error) {
        free(out);
        syntaqlite_parser_destroy(parser);
        return NULL;
    }

    syntaqlite_parser_destroy(parser);

    if (!out) {
        out = (char *)malloc(1);
        if (out) out[0] = '\0';
    }
    return out;
}

// ============ Low-level API ============

char *syntaqlite_format_stmt(SyntaqliteParser *parser, uint32_t root_id,
                             const SyntaqliteFormatOptions *options) {
    SynqFmtOptions opts = SYNQ_FMT_OPTIONS_DEFAULT;
    if (options) {
        opts.target_width = options->target_width;
        opts.indent_width = options->indent_width;
    }

    SynqAstContext *astCtx = &parser->astCtx;
    const char *source = parser->source;
    SynqTokenList *token_list = parser->token_list;

    SynqFmtCtx ctx;
    synq_doc_context_init(&ctx.docs);
    ctx.ast = &astCtx->ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = &opts;
    ctx.comment_att = synq_comment_attach(astCtx, root_id, source, token_list);

    uint32_t root_doc = synq_format_node(&ctx, root_id);

    synq_comment_attachment_free(ctx.comment_att);

    if (root_doc == SYNQ_NULL_DOC) {
        synq_doc_context_cleanup(&ctx.docs);
        char *empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    char *result = synq_doc_layout(&ctx.docs, root_doc, opts.target_width);
    synq_doc_context_cleanup(&ctx.docs);
    return result;
}

char *syntaqlite_format_stmt_debug_ir(SyntaqliteParser *parser, uint32_t root_id,
                                      const SyntaqliteFormatOptions *options) {
    SynqFmtOptions opts = SYNQ_FMT_OPTIONS_DEFAULT;
    if (options) {
        opts.target_width = options->target_width;
        opts.indent_width = options->indent_width;
    }

    SynqAstContext *astCtx = &parser->astCtx;
    const char *source = parser->source;
    SynqTokenList *token_list = parser->token_list;

    SynqFmtCtx ctx;
    synq_doc_context_init(&ctx.docs);
    ctx.ast = &astCtx->ast;
    ctx.source = source;
    ctx.token_list = token_list;
    ctx.options = &opts;
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
