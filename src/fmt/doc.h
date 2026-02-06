// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model for Wadler-Lindig pretty printer.

#ifndef SYNQ_SRC_FMT_DOC_H
#define SYNQ_SRC_FMT_DOC_H

#include "src/base/arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Null document ID (used for nullable fields)
#define SYNQ_NULL_DOC 0xFFFFFFFFu

// Document node tags
typedef enum {
    SYNQ_DOC_TEXT,
    SYNQ_DOC_LINE,       // space when flat, newline+indent when broken
    SYNQ_DOC_SOFTLINE,   // empty when flat, newline+indent when broken
    SYNQ_DOC_HARDLINE,   // always newline+indent
    SYNQ_DOC_NEST,       // increase indent for child
    SYNQ_DOC_GROUP,      // try flat, break if doesn't fit
    SYNQ_DOC_CONCAT,     // sequence of children
    SYNQ_DOC_LINE_SUFFIX,// buffered until next line break (for trailing comments)
    SYNQ_DOC_BREAK_PARENT,// forces containing group into break mode
} SynqDocTag;

// ============ Node Structs ============

typedef struct SynqDocText {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t length;
    const char *text;  // points into source or static string, not owned
} SynqDocText;

typedef struct SynqDocLeaf {
    uint8_t tag;  // LINE, SOFTLINE, or HARDLINE
} SynqDocLeaf;

typedef struct SynqDocNest {
    uint8_t tag;
    uint8_t _pad[3];
    int32_t indent;
    uint32_t child;
} SynqDocNest;

typedef struct SynqDocGroup {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t child;
} SynqDocGroup;

typedef struct SynqDocConcat {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t count;
    uint32_t children[];  // flexible array of doc IDs
} SynqDocConcat;

typedef struct SynqDocLineSuffix {
    uint8_t tag;
    uint8_t _pad[3];
    uint32_t child;
} SynqDocLineSuffix;

// ============ Node Union ============

typedef union SynqDoc {
    uint8_t tag;
    SynqDocText text;
    SynqDocLeaf leaf;
    SynqDocNest nest;
    SynqDocGroup group;
    SynqDocConcat concat;
    SynqDocLineSuffix line_suffix;
} SynqDoc;

// ============ Doc Context ============

typedef struct SynqDocContext {
    SynqArena arena;
} SynqDocContext;

void synq_doc_context_init(SynqDocContext *ctx);
void synq_doc_context_cleanup(SynqDocContext *ctx);

// Access node by ID
inline SynqDoc* synq_doc_node(SynqDocContext *ctx, uint32_t id) {
    if (id == SYNQ_NULL_DOC) return NULL;
    return (SynqDoc*)(ctx->arena.data + ctx->arena.offsets[id]);
}
#define DOC_NODE(ctx, id) synq_doc_node(ctx, id)

// Builder functions - return doc ID
uint32_t synq_doc_text(SynqDocContext *ctx, const char *text, uint32_t length);
uint32_t synq_doc_line(SynqDocContext *ctx);
uint32_t synq_doc_softline(SynqDocContext *ctx);
uint32_t synq_doc_hardline(SynqDocContext *ctx);
uint32_t synq_doc_nest(SynqDocContext *ctx, int32_t indent, uint32_t child);
uint32_t synq_doc_group(SynqDocContext *ctx, uint32_t child);

// Concat: create from array of child IDs
uint32_t synq_doc_concat(SynqDocContext *ctx, uint32_t *children, uint32_t count);

// Line suffix: content buffered until the next line break (for trailing comments)
uint32_t synq_doc_line_suffix(SynqDocContext *ctx, uint32_t child);

// Break parent: forces the containing group into break mode (no visible output)
uint32_t synq_doc_break_parent(SynqDocContext *ctx);

// Debug: print doc tree in a human-readable format
void synq_doc_debug_print(SynqDocContext *ctx, uint32_t doc_id,
                                 FILE *out, int depth);

#ifdef __cplusplus
}
#endif

#endif  // SYNQ_SRC_FMT_DOC_H
