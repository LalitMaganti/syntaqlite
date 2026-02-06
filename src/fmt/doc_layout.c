// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Wadler-Lindig layout engine implementation.

#include "src/fmt/doc_layout.h"
#include "src/vec.h"

#include <stdlib.h>
#include <string.h>

// ============ Flat Width Measurement ============

// Returns the width if the doc were rendered flat.
// Returns UINT32_MAX if it contains a hardline (won't fit flat).
static uint32_t measure_flat(SyntaqliteDocContext *ctx, uint32_t doc_id) {
    if (doc_id == SYNTAQLITE_NULL_DOC) return 0;

    SyntaqliteDoc *doc = DOC_NODE(ctx, doc_id);
    if (!doc) return 0;

    switch (doc->tag) {
        case SYNTAQLITE_DOC_TEXT:
            return doc->text.length;

        case SYNTAQLITE_DOC_LINE:
            return 1;  // space when flat

        case SYNTAQLITE_DOC_SOFTLINE:
            return 0;  // empty when flat

        case SYNTAQLITE_DOC_HARDLINE:
            return UINT32_MAX;  // won't fit flat

        case SYNTAQLITE_DOC_NEST: {
            return measure_flat(ctx, doc->nest.child);
        }

        case SYNTAQLITE_DOC_GROUP: {
            return measure_flat(ctx, doc->group.child);
        }

        case SYNTAQLITE_DOC_CONCAT: {
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
            syntaqlite_vec_push((buf), ' '); \
        } \
    } while (0)

// ============ Layout Engine ============

char *syntaqlite_doc_layout(SyntaqliteDocContext *ctx, uint32_t root_id, uint32_t target_width) {
    if (root_id == SYNTAQLITE_NULL_DOC) {
        char *empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    SYNTAQLITE_VEC(char) buf;
    syntaqlite_vec_init(&buf);

    SYNTAQLITE_VEC(LayoutEntry) stack;
    syntaqlite_vec_init(&stack);

    // Current column position (for fitting decisions)
    uint32_t column = 0;

    // Push root in break mode
    LayoutEntry root_entry = {0, LAYOUT_MODE_BREAK, root_id};
    syntaqlite_vec_push(&stack, root_entry);

    while (stack.count > 0) {
        // Pop from stack
        LayoutEntry entry = syntaqlite_vec_pop(&stack);
        int32_t indent = entry.indent;
        LayoutMode mode = entry.mode;
        uint32_t doc_id = entry.doc_id;

        if (doc_id == SYNTAQLITE_NULL_DOC) continue;

        SyntaqliteDoc *doc = DOC_NODE(ctx, doc_id);
        if (!doc) continue;

        switch (doc->tag) {
            case SYNTAQLITE_DOC_TEXT: {
                syntaqlite_vec_push_n(&buf, doc->text.text, doc->text.length);
                column += doc->text.length;
                break;
            }

            case SYNTAQLITE_DOC_LINE: {
                if (mode == LAYOUT_MODE_FLAT) {
                    syntaqlite_vec_push(&buf, ' ');
                    column += 1;
                } else {
                    syntaqlite_vec_push(&buf, '\n');
                    buf_append_indent(&buf, indent);
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNTAQLITE_DOC_SOFTLINE: {
                if (mode == LAYOUT_MODE_FLAT) {
                    // empty when flat
                } else {
                    syntaqlite_vec_push(&buf, '\n');
                    buf_append_indent(&buf, indent);
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNTAQLITE_DOC_HARDLINE: {
                syntaqlite_vec_push(&buf, '\n');
                buf_append_indent(&buf, indent);
                column = (uint32_t)indent;
                break;
            }

            case SYNTAQLITE_DOC_NEST: {
                LayoutEntry e = {indent + doc->nest.indent, mode, doc->nest.child};
                syntaqlite_vec_push(&stack, e);
                break;
            }

            case SYNTAQLITE_DOC_GROUP: {
                if (mode == LAYOUT_MODE_FLAT) {
                    LayoutEntry e = {indent, LAYOUT_MODE_FLAT, doc->group.child};
                    syntaqlite_vec_push(&stack, e);
                } else {
                    uint32_t flat_width = measure_flat(ctx, doc->group.child);
                    LayoutMode child_mode = (flat_width != UINT32_MAX && column + flat_width <= target_width)
                        ? LAYOUT_MODE_FLAT : LAYOUT_MODE_BREAK;
                    LayoutEntry e = {indent, child_mode, doc->group.child};
                    syntaqlite_vec_push(&stack, e);
                }
                break;
            }

            case SYNTAQLITE_DOC_CONCAT: {
                // Push children in reverse order so first child is processed first
                for (uint32_t i = doc->concat.count; i > 0; i--) {
                    LayoutEntry e = {indent, mode, doc->concat.children[i - 1]};
                    syntaqlite_vec_push(&stack, e);
                }
                break;
            }

            default:
                break;
        }
    }

    // Null-terminate
    syntaqlite_vec_push(&buf, '\0');
    buf.count--;  // don't count null terminator in size

    syntaqlite_vec_free(&stack);
    return buf.data;
}
