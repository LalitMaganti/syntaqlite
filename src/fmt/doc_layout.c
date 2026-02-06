// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Wadler-Lindig layout engine implementation.

#include "src/fmt/doc_layout.h"

#include <stdlib.h>
#include <string.h>

// ============ Output Buffer ============

typedef struct {
    char *data;
    uint32_t size;
    uint32_t capacity;
} OutputBuf;

static int buf_init(OutputBuf *buf) {
    buf->capacity = 256;
    buf->data = (char*)malloc(buf->capacity);
    if (!buf->data) return 0;
    buf->size = 0;
    return 1;
}

static int buf_append(OutputBuf *buf, const char *str, uint32_t len) {
    if (buf->size + len + 1 > buf->capacity) {
        uint32_t new_cap = buf->capacity * 2 + len + 256;
        char *new_data = (char*)realloc(buf->data, new_cap);
        if (!new_data) return 0;
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->size, str, len);
    buf->size += len;
    return 1;
}

static int buf_append_char(OutputBuf *buf, char c) {
    return buf_append(buf, &c, 1);
}

static int buf_append_indent(OutputBuf *buf, int32_t indent) {
    for (int32_t i = 0; i < indent; i++) {
        if (!buf_append_char(buf, ' ')) return 0;
    }
    return 1;
}

// ============ Flat Width Measurement ============

// Returns the width if the doc were rendered flat.
// Returns UINT32_MAX if it contains a hardline (won't fit flat).
static uint32_t measure_flat(SyntaqliteDocArena *arena, uint32_t doc_id) {
    if (doc_id == SYNTAQLITE_NULL_DOC) return 0;

    SyntaqliteDoc *doc = DOC_NODE(arena, doc_id);
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
            return measure_flat(arena, doc->nest.child);
        }

        case SYNTAQLITE_DOC_GROUP: {
            return measure_flat(arena, doc->group.child);
        }

        case SYNTAQLITE_DOC_CONCAT: {
            uint32_t total = 0;
            for (uint32_t i = 0; i < doc->concat.count; i++) {
                uint32_t w = measure_flat(arena, doc->concat.children[i]);
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

typedef struct {
    LayoutEntry *data;
    uint32_t size;
    uint32_t capacity;
} LayoutStack;

static int stack_init(LayoutStack *stack) {
    stack->capacity = 64;
    stack->data = (LayoutEntry*)malloc(stack->capacity * sizeof(LayoutEntry));
    if (!stack->data) return 0;
    stack->size = 0;
    return 1;
}

static void stack_free(LayoutStack *stack) {
    free(stack->data);
    stack->data = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

static int stack_push(LayoutStack *stack, int32_t indent, LayoutMode mode, uint32_t doc_id) {
    if (doc_id == SYNTAQLITE_NULL_DOC) return 1;  // skip nulls
    if (stack->size >= stack->capacity) {
        uint32_t new_cap = stack->capacity * 2;
        LayoutEntry *new_data = (LayoutEntry*)realloc(stack->data, new_cap * sizeof(LayoutEntry));
        if (!new_data) return 0;
        stack->data = new_data;
        stack->capacity = new_cap;
    }
    stack->data[stack->size].indent = indent;
    stack->data[stack->size].mode = mode;
    stack->data[stack->size].doc_id = doc_id;
    stack->size++;
    return 1;
}

// ============ Layout Engine ============

char *syntaqlite_doc_layout(SyntaqliteDocArena *arena, uint32_t root_id, uint32_t target_width) {
    if (root_id == SYNTAQLITE_NULL_DOC) {
        char *empty = (char*)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    OutputBuf buf;
    if (!buf_init(&buf)) return NULL;

    LayoutStack stack;
    if (!stack_init(&stack)) {
        free(buf.data);
        return NULL;
    }

    // Current column position (for fitting decisions)
    uint32_t column = 0;

    // Push root in break mode
    if (!stack_push(&stack, 0, LAYOUT_MODE_BREAK, root_id)) {
        free(buf.data);
        stack_free(&stack);
        return NULL;
    }

    while (stack.size > 0) {
        // Pop from stack
        stack.size--;
        int32_t indent = stack.data[stack.size].indent;
        LayoutMode mode = stack.data[stack.size].mode;
        uint32_t doc_id = stack.data[stack.size].doc_id;

        if (doc_id == SYNTAQLITE_NULL_DOC) continue;

        SyntaqliteDoc *doc = DOC_NODE(arena, doc_id);
        if (!doc) continue;

        switch (doc->tag) {
            case SYNTAQLITE_DOC_TEXT: {
                if (!buf_append(&buf, doc->text.text, doc->text.length)) goto fail;
                column += doc->text.length;
                break;
            }

            case SYNTAQLITE_DOC_LINE: {
                if (mode == LAYOUT_MODE_FLAT) {
                    if (!buf_append_char(&buf, ' ')) goto fail;
                    column += 1;
                } else {
                    if (!buf_append_char(&buf, '\n')) goto fail;
                    if (!buf_append_indent(&buf, indent)) goto fail;
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNTAQLITE_DOC_SOFTLINE: {
                if (mode == LAYOUT_MODE_FLAT) {
                    // empty when flat
                } else {
                    if (!buf_append_char(&buf, '\n')) goto fail;
                    if (!buf_append_indent(&buf, indent)) goto fail;
                    column = (uint32_t)indent;
                }
                break;
            }

            case SYNTAQLITE_DOC_HARDLINE: {
                if (!buf_append_char(&buf, '\n')) goto fail;
                if (!buf_append_indent(&buf, indent)) goto fail;
                column = (uint32_t)indent;
                break;
            }

            case SYNTAQLITE_DOC_NEST: {
                if (!stack_push(&stack, indent + doc->nest.indent, mode, doc->nest.child)) goto fail;
                break;
            }

            case SYNTAQLITE_DOC_GROUP: {
                if (mode == LAYOUT_MODE_FLAT) {
                    // Already flat, stay flat
                    if (!stack_push(&stack, indent, LAYOUT_MODE_FLAT, doc->group.child)) goto fail;
                } else {
                    // Try flat: measure width
                    uint32_t flat_width = measure_flat(arena, doc->group.child);
                    if (flat_width != UINT32_MAX && column + flat_width <= target_width) {
                        if (!stack_push(&stack, indent, LAYOUT_MODE_FLAT, doc->group.child)) goto fail;
                    } else {
                        if (!stack_push(&stack, indent, LAYOUT_MODE_BREAK, doc->group.child)) goto fail;
                    }
                }
                break;
            }

            case SYNTAQLITE_DOC_CONCAT: {
                // Push children in reverse order so first child is processed first
                for (uint32_t i = doc->concat.count; i > 0; i--) {
                    if (!stack_push(&stack, indent, mode, doc->concat.children[i - 1])) goto fail;
                }
                break;
            }

            default:
                break;
        }
    }

    // Null-terminate
    if (!buf_append_char(&buf, '\0')) goto fail;
    buf.size--;  // don't count null terminator in size

    stack_free(&stack);
    return buf.data;

fail:
    free(buf.data);
    stack_free(&stack);
    return NULL;
}
