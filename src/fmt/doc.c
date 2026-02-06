// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model implementation for Wadler-Lindig pretty printer.

#include "src/fmt/doc.h"

#include <string.h>

void syntaqlite_doc_context_init(SyntaqliteDocContext *ctx) {
    syntaqlite_arena_init(&ctx->arena);
}

void syntaqlite_doc_context_cleanup(SyntaqliteDocContext *ctx) {
    syntaqlite_arena_free(&ctx->arena);
}

uint32_t doc_text(SyntaqliteDocContext *ctx, const char *text, uint32_t length) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_TEXT, sizeof(SyntaqliteDocText));
    SyntaqliteDocText *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->text;
    node->length = length;
    node->text = text;
    return id;
}

uint32_t doc_line(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_LINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_softline(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_SOFTLINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_hardline(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_HARDLINE, sizeof(SyntaqliteDocLeaf));
}

uint32_t doc_nest(SyntaqliteDocContext *ctx, int32_t indent, uint32_t child) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_NEST, sizeof(SyntaqliteDocNest));
    SyntaqliteDocNest *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->nest;
    node->indent = indent;
    node->child = child;
    return id;
}

uint32_t doc_group(SyntaqliteDocContext *ctx, uint32_t child) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_GROUP, sizeof(SyntaqliteDocGroup));
    SyntaqliteDocGroup *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->group;
    node->child = child;
    return id;
}

uint32_t doc_concat(SyntaqliteDocContext *ctx, uint32_t *children, uint32_t count) {
    // Filter out NULL_DOC children
    uint32_t filtered[count > 0 ? count : 1];
    uint32_t n = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (children[i] != SYNTAQLITE_NULL_DOC) filtered[n++] = children[i];
    }
    if (n == 0) return SYNTAQLITE_NULL_DOC;
    if (n == 1) return filtered[0];
    size_t size = offsetof(SyntaqliteDocConcat, children) + n * sizeof(uint32_t);
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_CONCAT, size);
    SyntaqliteDocConcat *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->concat;
    memcpy(node->children, filtered, n * sizeof(uint32_t));
    node->count = n;
    return id;
}

uint32_t doc_line_suffix(SyntaqliteDocContext *ctx, uint32_t child) {
    uint32_t id = syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_LINE_SUFFIX, sizeof(SyntaqliteDocLineSuffix));
    SyntaqliteDocLineSuffix *node = &((SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->line_suffix;
    node->child = child;
    return id;
}

uint32_t doc_break_parent(SyntaqliteDocContext *ctx) {
    return syntaqlite_arena_alloc(&ctx->arena, SYNTAQLITE_DOC_BREAK_PARENT, sizeof(SyntaqliteDocLeaf));
}

void syntaqlite_doc_debug_print(SyntaqliteDocContext *ctx, uint32_t doc_id,
                                 FILE *out, int depth) {
    if (doc_id == SYNTAQLITE_NULL_DOC) {
        fprintf(out, "%*sNULL\n", depth * 2, "");
        return;
    }
    SyntaqliteDoc *doc = DOC_NODE(ctx, doc_id);
    if (!doc) { fprintf(out, "%*s<invalid>\n", depth * 2, ""); return; }

    switch (doc->tag) {
        case SYNTAQLITE_DOC_TEXT:
            fprintf(out, "%*sTEXT \"%.*s\"\n", depth * 2, "",
                    (int)doc->text.length, doc->text.text);
            break;
        case SYNTAQLITE_DOC_LINE:
            fprintf(out, "%*sLINE\n", depth * 2, "");
            break;
        case SYNTAQLITE_DOC_SOFTLINE:
            fprintf(out, "%*sSOFTLINE\n", depth * 2, "");
            break;
        case SYNTAQLITE_DOC_HARDLINE:
            fprintf(out, "%*sHARDLINE\n", depth * 2, "");
            break;
        case SYNTAQLITE_DOC_BREAK_PARENT:
            fprintf(out, "%*sBREAK_PARENT\n", depth * 2, "");
            break;
        case SYNTAQLITE_DOC_NEST:
            fprintf(out, "%*sNEST(%d)\n", depth * 2, "", doc->nest.indent);
            syntaqlite_doc_debug_print(ctx, doc->nest.child, out, depth + 1);
            break;
        case SYNTAQLITE_DOC_GROUP:
            fprintf(out, "%*sGROUP\n", depth * 2, "");
            syntaqlite_doc_debug_print(ctx, doc->group.child, out, depth + 1);
            break;
        case SYNTAQLITE_DOC_LINE_SUFFIX:
            fprintf(out, "%*sLINE_SUFFIX\n", depth * 2, "");
            syntaqlite_doc_debug_print(ctx, doc->line_suffix.child, out, depth + 1);
            break;
        case SYNTAQLITE_DOC_CONCAT:
            fprintf(out, "%*sCONCAT(%u)\n", depth * 2, "", doc->concat.count);
            for (uint32_t i = 0; i < doc->concat.count; i++)
                syntaqlite_doc_debug_print(ctx, doc->concat.children[i], out, depth + 1);
            break;
        default:
            fprintf(out, "%*s<unknown tag=%d>\n", depth * 2, "", doc->tag);
            break;
    }
}
