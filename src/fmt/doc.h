// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model for Wadler-Lindig pretty printer.

#ifndef SYNTAQLITE_SRC_FMT_DOC_H
#define SYNTAQLITE_SRC_FMT_DOC_H

#include "src/arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Null document ID (used for nullable fields)
#define SYNTAQLITE_NULL_DOC 0xFFFFFFFFu

// Document node tags
typedef enum {
    SYNTAQLITE_DOC_TEXT,
    SYNTAQLITE_DOC_LINE,       // space when flat, newline+indent when broken
    SYNTAQLITE_DOC_SOFTLINE,   // empty when flat, newline+indent when broken
    SYNTAQLITE_DOC_HARDLINE,   // always newline+indent
    SYNTAQLITE_DOC_NEST,       // increase indent for child
    SYNTAQLITE_DOC_GROUP,      // try flat, break if doesn't fit
    SYNTAQLITE_DOC_CONCAT,     // sequence of children
    SYNTAQLITE_DOC_LINE_SUFFIX,// buffered until next line break (for trailing comments)
    SYNTAQLITE_DOC_BREAK_PARENT,// forces containing group into break mode
} SyntaqliteDocTag;

// ============ Node Structs ============

typedef struct SyntaqliteDocText {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t length;
    const char *text;  // points into source or static string, not owned
} SyntaqliteDocText;

typedef struct SyntaqliteDocLeaf {
    uint8_t tag;  // LINE, SOFTLINE, or HARDLINE
} SyntaqliteDocLeaf;

typedef struct SyntaqliteDocNest {
    uint8_t tag;
    uint8_t _pad[3];
    int32_t indent;
    uint32_t child;
} SyntaqliteDocNest;

typedef struct SyntaqliteDocGroup {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t child;
} SyntaqliteDocGroup;

typedef struct SyntaqliteDocConcat {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of doc IDs
} SyntaqliteDocConcat;

typedef struct SyntaqliteDocLineSuffix {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t child;
} SyntaqliteDocLineSuffix;

// ============ Node Union ============

typedef union SyntaqliteDoc {
    uint8_t tag;
    SyntaqliteDocText text;
    SyntaqliteDocLeaf leaf;
    SyntaqliteDocNest nest;
    SyntaqliteDocGroup group;
    SyntaqliteDocConcat concat;
    SyntaqliteDocLineSuffix line_suffix;
} SyntaqliteDoc;

// ============ Doc Context ============

typedef struct SyntaqliteDocContext {
    SyntaqliteArena arena;
} SyntaqliteDocContext;

void syntaqlite_doc_context_init(SyntaqliteDocContext *ctx);
void syntaqlite_doc_context_cleanup(SyntaqliteDocContext *ctx);

// Access node by ID
static inline SyntaqliteDoc* syntaqlite_doc_node(SyntaqliteDocContext *ctx, uint32_t id) {
    if (id == SYNTAQLITE_NULL_DOC) return NULL;
    return (SyntaqliteDoc*)(ctx->arena.data + ctx->arena.offsets[id]);
}
#define DOC_NODE(ctx, id) syntaqlite_doc_node(ctx, id)

// Builder functions - return doc ID
uint32_t doc_text(SyntaqliteDocContext *ctx, const char *text, uint32_t length);
uint32_t doc_line(SyntaqliteDocContext *ctx);
uint32_t doc_softline(SyntaqliteDocContext *ctx);
uint32_t doc_hardline(SyntaqliteDocContext *ctx);
uint32_t doc_nest(SyntaqliteDocContext *ctx, int32_t indent, uint32_t child);
uint32_t doc_group(SyntaqliteDocContext *ctx, uint32_t child);

// Concat: create from array of child IDs
uint32_t doc_concat(SyntaqliteDocContext *ctx, uint32_t *children, uint32_t count);

// Line suffix: content buffered until the next line break (for trailing comments)
uint32_t doc_line_suffix(SyntaqliteDocContext *ctx, uint32_t child);

// Break parent: forces the containing group into break mode (no visible output)
uint32_t doc_break_parent(SyntaqliteDocContext *ctx);

// Debug: print doc tree in a human-readable format
void syntaqlite_doc_debug_print(SyntaqliteDocContext *ctx, uint32_t doc_id,
                                 FILE *out, int depth);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_FMT_DOC_H
