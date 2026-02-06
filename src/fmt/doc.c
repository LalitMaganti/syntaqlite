// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model and layout engine for Wadler-Lindig pretty printer.

#include "src/fmt/doc.h"
#include "src/base/vec.h"

#include <stdlib.h>
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

// ============ Flat Width Measurement ============

// Returns the width if the doc were rendered flat.
// Returns UINT32_MAX if it contains a hardline or line_suffix (won't fit flat).
static uint32_t measure_flat(SynqDocContext *ctx, uint32_t doc_id) {
    if (doc_id == SYNQ_NULL_DOC) return 0;

    SynqDoc *doc = DOC_NODE(ctx, doc_id);
    if (!doc) return 0;

    switch (doc->tag) {
        case SYNQ_DOC_TEXT:
            return doc->text.length;

        case SYNQ_DOC_LINE:
            return 1;  // space when flat

        case SYNQ_DOC_SOFTLINE:
            return 0;  // empty when flat

        case SYNQ_DOC_HARDLINE:
            return UINT32_MAX;  // won't fit flat

        case SYNQ_DOC_LINE_SUFFIX:
            return measure_flat(ctx, doc->line_suffix.child);

        case SYNQ_DOC_BREAK_PARENT:
            return UINT32_MAX;

        case SYNQ_DOC_NEST: {
            return measure_flat(ctx, doc->nest.child);
        }

        case SYNQ_DOC_GROUP:
            return measure_flat(ctx, doc->group.child);

        case SYNQ_DOC_CONCAT: {
            uint32_t total = 0;
            for (uint32_t i = 0; i < doc->concat.count; i++) {
                uint32_t w = measure_flat(ctx, doc->concat.children[i]);
                if (w == UINT32_MAX) return UINT32_MAX;
                total += w;
                if (total >= UINT32_MAX) return UINT32_MAX;
            }
            return total;
        }

        default:
            return 0;
    }
}

// ============ Layout Stack ============

typedef enum {
    LAYOUT_MODE_FLAT,
    LAYOUT_MODE_BREAK,
} LayoutMode;

typedef struct {
    int32_t indent;
    LayoutMode mode;
    uint32_t doc_id;
} LayoutEntry;

// buf_append_indent is a macro so it works with the anonymous struct type
#define buf_append_indent(buf, indent_count) \
    do { \
        for (int32_t _i = 0; _i < (indent_count); _i++) { \
            synq_vec_push((buf), ' '); \
        } \
    } while (0)

// ============ Layout Engine ============

char *synq_doc_layout(SynqDocContext *ctx, uint32_t root_id, uint32_t target_width) {
    if (root_id == SYNQ_NULL_DOC) {
        char *empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    SYNQ_VEC(char) buf;
    synq_vec_init(&buf);

    SYNQ_VEC(LayoutEntry) stack;
    synq_vec_init(&stack);

    // Pending line suffixes (trailing comments buffered until next line break)
    SYNQ_VEC(uint32_t) pending_suffixes;
    synq_vec_init(&pending_suffixes);

    // Current column position (for fitting decisions)
    uint32_t column = 0;

    // Push root in break mode
    LayoutEntry root_entry = {0, LAYOUT_MODE_BREAK, root_id};
    synq_vec_push(&stack, root_entry);

    while (stack.count > 0) {
        // Pop from stack
        LayoutEntry entry = synq_vec_pop(&stack);
        int32_t indent = entry.indent;
        LayoutMode mode = entry.mode;
        uint32_t doc_id = entry.doc_id;

        if (doc_id == SYNQ_NULL_DOC) continue;

        SynqDoc *doc = DOC_NODE(ctx, doc_id);
        if (!doc) continue;

        switch (doc->tag) {
            case SYNQ_DOC_TEXT: {
                synq_vec_push_n(&buf, doc->text.text, doc->text.length);
                column += doc->text.length;
                break;
            }

            case SYNQ_DOC_LINE: {
                if (mode == LAYOUT_MODE_FLAT) {
                    synq_vec_push(&buf, ' ');
                    column += 1;
                } else if (pending_suffixes.count > 0) {
                    // Re-push this line break, then push suffixes ahead of it
                    LayoutEntry re = {indent, mode, doc_id};
                    synq_vec_push(&stack, re);
                    for (uint32_t s = 0; s < pending_suffixes.count; s++) {
                        LayoutEntry se = {indent, LAYOUT_MODE_FLAT, pending_suffixes.data[s]};
                        synq_vec_push(&stack, se);
                    }
                    pending_suffixes.count = 0;
                } else {
                    synq_vec_push(&buf, '\n');
                    buf_append_indent(&buf, indent);
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNQ_DOC_SOFTLINE: {
                if (mode == LAYOUT_MODE_FLAT) {
                    // empty when flat
                } else if (pending_suffixes.count > 0) {
                    LayoutEntry re = {indent, mode, doc_id};
                    synq_vec_push(&stack, re);
                    for (uint32_t s = 0; s < pending_suffixes.count; s++) {
                        LayoutEntry se = {indent, LAYOUT_MODE_FLAT, pending_suffixes.data[s]};
                        synq_vec_push(&stack, se);
                    }
                    pending_suffixes.count = 0;
                } else {
                    synq_vec_push(&buf, '\n');
                    buf_append_indent(&buf, indent);
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNQ_DOC_HARDLINE: {
                if (pending_suffixes.count > 0) {
                    LayoutEntry re = {indent, mode, doc_id};
                    synq_vec_push(&stack, re);
                    for (uint32_t s = 0; s < pending_suffixes.count; s++) {
                        LayoutEntry se = {indent, LAYOUT_MODE_FLAT, pending_suffixes.data[s]};
                        synq_vec_push(&stack, se);
                    }
                    pending_suffixes.count = 0;
                } else {
                    synq_vec_push(&buf, '\n');
                    buf_append_indent(&buf, indent);
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNQ_DOC_LINE_SUFFIX: {
                // Buffer the child content until the next line break
                synq_vec_push(&pending_suffixes, doc->line_suffix.child);
                break;
            }

            case SYNQ_DOC_BREAK_PARENT: {
                // No output; forces group to break via measure_flat returning UINT32_MAX
                break;
            }

            case SYNQ_DOC_NEST: {
                LayoutEntry e = {indent + doc->nest.indent, mode, doc->nest.child};
                synq_vec_push(&stack, e);
                break;
            }

            case SYNQ_DOC_GROUP: {
                if (mode == LAYOUT_MODE_FLAT) {
                    LayoutEntry e = {indent, LAYOUT_MODE_FLAT, doc->group.child};
                    synq_vec_push(&stack, e);
                } else {
                    uint32_t flat_width = measure_flat(ctx, doc->group.child);
                    LayoutMode child_mode = (flat_width != UINT32_MAX && column + flat_width <= target_width)
                        ? LAYOUT_MODE_FLAT : LAYOUT_MODE_BREAK;
                    LayoutEntry e = {indent, child_mode, doc->group.child};
                    synq_vec_push(&stack, e);
                }
                break;
            }

            case SYNQ_DOC_CONCAT: {
                // Push children in reverse order so first child is processed first
                for (uint32_t i = doc->concat.count; i > 0; i--) {
                    LayoutEntry e = {indent, mode, doc->concat.children[i - 1]};
                    synq_vec_push(&stack, e);
                }
                break;
            }

            default:
                break;
        }
    }

    // Flush any remaining pending suffixes (e.g. trailing comment at end of input)
    if (pending_suffixes.count > 0) {
        for (uint32_t s = pending_suffixes.count; s > 0; s--) {
            LayoutEntry se = {0, LAYOUT_MODE_FLAT, pending_suffixes.data[s - 1]};
            synq_vec_push(&stack, se);
        }
        pending_suffixes.count = 0;
        // Continue the main loop to render them
        while (stack.count > 0) {
            LayoutEntry entry = synq_vec_pop(&stack);
            if (entry.doc_id == SYNQ_NULL_DOC) continue;
            SynqDoc *doc = DOC_NODE(ctx, entry.doc_id);
            if (!doc) continue;
            switch (doc->tag) {
                case SYNQ_DOC_TEXT:
                    synq_vec_push_n(&buf, doc->text.text, doc->text.length);
                    break;
                case SYNQ_DOC_CONCAT:
                    for (uint32_t i = doc->concat.count; i > 0; i--) {
                        LayoutEntry e = {0, LAYOUT_MODE_FLAT, doc->concat.children[i - 1]};
                        synq_vec_push(&stack, e);
                    }
                    break;
                default: break;
            }
        }
    }

    // Null-terminate
    synq_vec_push(&buf, '\0');
    buf.count--;  // don't count null terminator in size

    synq_vec_free(&pending_suffixes);
    synq_vec_free(&stack);
    return buf.data;
}
