// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/formatter.h"

#include "src/fmt/fmt_helpers.h"

#include <stdlib.h>

// Internal parser accessors (defined in parser_api.c).
SynqAstContext *syntaqlite_parser_ast_context(SyntaqliteParser *p);
struct SynqTokenList *syntaqlite_parser_token_list(SyntaqliteParser *p);

char *syntaqlite_format(SyntaqliteParser *parser, uint32_t root_id,
                        const SyntaqliteFormatOptions *options) {
    SynqFmtOptions opts = SYNQ_FMT_OPTIONS_DEFAULT;
    if (options) {
        opts.target_width = options->target_width;
        opts.indent_width = options->indent_width;
    }

    SynqAstContext *astCtx = syntaqlite_parser_ast_context(parser);
    const char *source = syntaqlite_parser_source(parser);
    SynqTokenList *token_list = syntaqlite_parser_token_list(parser);

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

char *syntaqlite_format_debug_ir(SyntaqliteParser *parser, uint32_t root_id,
                                 const SyntaqliteFormatOptions *options) {
    SynqFmtOptions opts = SYNQ_FMT_OPTIONS_DEFAULT;
    if (options) {
        opts.target_width = options->target_width;
        opts.indent_width = options->indent_width;
    }

    SynqAstContext *astCtx = syntaqlite_parser_ast_context(parser);
    const char *source = syntaqlite_parser_source(parser);
    SynqTokenList *token_list = syntaqlite_parser_token_list(parser);

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
