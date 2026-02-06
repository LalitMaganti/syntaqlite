// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model implementation for Wadler-Lindig pretty printer.

#include "src/fmt/doc.h"

#include <string.h>


// External definitions for inline functions (C99/C11).
extern inline SynqDoc* synq_doc_node(SynqDocContext *ctx, uint32_t id);

void synq_doc_context_init(SynqDocContext *ctx) {
    synq_arena_init(&ctx->arena);
}

void synq_doc_context_cleanup(SynqDocContext *ctx) {
    synq_arena_free(&ctx->arena);
}

uint32_t synq_doc_text(SynqDocContext *ctx, const char *text, uint32_t length) {
    uint32_t id = synq_arena_alloc(&ctx->arena, SYNQ_DOC_TEXT, sizeof(SynqDocText));
    SynqDocText *node = &((SynqDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->text;
    node->length = length;
    node->text = text;
    return id;
}

uint32_t synq_doc_line(SynqDocContext *ctx) {
    return synq_arena_alloc(&ctx->arena, SYNQ_DOC_LINE, sizeof(SynqDocLeaf));
}

uint32_t synq_doc_softline(SynqDocContext *ctx) {
    return synq_arena_alloc(&ctx->arena, SYNQ_DOC_SOFTLINE, sizeof(SynqDocLeaf));
}

uint32_t synq_doc_hardline(SynqDocContext *ctx) {
    return synq_arena_alloc(&ctx->arena, SYNQ_DOC_HARDLINE, sizeof(SynqDocLeaf));
}

uint32_t synq_doc_nest(SynqDocContext *ctx, int32_t indent, uint32_t child) {
    uint32_t id = synq_arena_alloc(&ctx->arena, SYNQ_DOC_NEST, sizeof(SynqDocNest));
    SynqDocNest *node = &((SynqDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->nest;
    node->indent = indent;
    node->child = child;
    return id;
}

uint32_t synq_doc_group(SynqDocContext *ctx, uint32_t child) {
    uint32_t id = synq_arena_alloc(&ctx->arena, SYNQ_DOC_GROUP, sizeof(SynqDocGroup));
    SynqDocGroup *node = &((SynqDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->group;
    node->child = child;
    return id;
}

uint32_t synq_doc_concat(SynqDocContext *ctx, uint32_t *children, uint32_t count) {
    // Filter out NULL_DOC children
    uint32_t filtered[count > 0 ? count : 1];
    uint32_t n = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (children[i] != SYNQ_NULL_DOC) filtered[n++] = children[i];
    }
    if (n == 0) return SYNQ_NULL_DOC;
    if (n == 1) return filtered[0];
    size_t size = offsetof(SynqDocConcat, children) + n * sizeof(uint32_t);
    uint32_t id = synq_arena_alloc(&ctx->arena, SYNQ_DOC_CONCAT, size);
    SynqDocConcat *node = &((SynqDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->concat;
    memcpy(node->children, filtered, n * sizeof(uint32_t));
    node->count = n;
    return id;
}

uint32_t synq_doc_line_suffix(SynqDocContext *ctx, uint32_t child) {
    uint32_t id = synq_arena_alloc(&ctx->arena, SYNQ_DOC_LINE_SUFFIX, sizeof(SynqDocLineSuffix));
    SynqDocLineSuffix *node = &((SynqDoc*)(ctx->arena.data + ctx->arena.offsets[id]))->line_suffix;
    node->child = child;
    return id;
}

uint32_t synq_doc_break_parent(SynqDocContext *ctx) {
    return synq_arena_alloc(&ctx->arena, SYNQ_DOC_BREAK_PARENT, sizeof(SynqDocLeaf));
}

void synq_doc_debug_print(SynqDocContext *ctx, uint32_t doc_id,
                                 FILE *out, int depth) {
    if (doc_id == SYNQ_NULL_DOC) {
        fprintf(out, "%*sNULL\n", depth * 2, "");
        return;
    }
    SynqDoc *doc = DOC_NODE(ctx, doc_id);
    if (!doc) { fprintf(out, "%*s<invalid>\n", depth * 2, ""); return; }

    switch (doc->tag) {
        case SYNQ_DOC_TEXT:
            fprintf(out, "%*sTEXT \"%.*s\"\n", depth * 2, "",
                    (int)doc->text.length, doc->text.text);
            break;
        case SYNQ_DOC_LINE:
            fprintf(out, "%*sLINE\n", depth * 2, "");
            break;
        case SYNQ_DOC_SOFTLINE:
            fprintf(out, "%*sSOFTLINE\n", depth * 2, "");
            break;
        case SYNQ_DOC_HARDLINE:
            fprintf(out, "%*sHARDLINE\n", depth * 2, "");
            break;
        case SYNQ_DOC_BREAK_PARENT:
            fprintf(out, "%*sBREAK_PARENT\n", depth * 2, "");
            break;
        case SYNQ_DOC_NEST:
            fprintf(out, "%*sNEST(%d)\n", depth * 2, "", doc->nest.indent);
            synq_doc_debug_print(ctx, doc->nest.child, out, depth + 1);
            break;
        case SYNQ_DOC_GROUP:
            fprintf(out, "%*sGROUP\n", depth * 2, "");
            synq_doc_debug_print(ctx, doc->group.child, out, depth + 1);
            break;
        case SYNQ_DOC_LINE_SUFFIX:
            fprintf(out, "%*sLINE_SUFFIX\n", depth * 2, "");
            synq_doc_debug_print(ctx, doc->line_suffix.child, out, depth + 1);
            break;
        case SYNQ_DOC_CONCAT:
            fprintf(out, "%*sCONCAT(%u)\n", depth * 2, "", doc->concat.count);
            for (uint32_t i = 0; i < doc->concat.count; i++)
                synq_doc_debug_print(ctx, doc->concat.children[i], out, depth + 1);
            break;
        default:
            fprintf(out, "%*s<unknown tag=%d>\n", depth * 2, "", doc->tag);
            break;
    }
}
