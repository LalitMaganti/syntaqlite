// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Document model for Wadler-Lindig pretty printer.

#ifndef SYNTAQLITE_SRC_FMT_DOC_H
#define SYNTAQLITE_SRC_FMT_DOC_H

#include <stddef.h>
#include <stdint.h>

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

// ============ Node Union ============

typedef union SyntaqliteDoc {
    uint8_t tag;
    SyntaqliteDocText text;
    SyntaqliteDocLeaf leaf;
    SyntaqliteDocNest nest;
    SyntaqliteDocGroup group;
    SyntaqliteDocConcat concat;
} SyntaqliteDoc;

// ============ Arena ============

typedef struct SyntaqliteDocArena {
    uint8_t *arena;
    uint32_t arena_size;
    uint32_t arena_capacity;
    uint32_t *offsets;
    uint32_t node_count;
    uint32_t node_capacity;
} SyntaqliteDocArena;

// Access node by ID
static inline SyntaqliteDoc* syntaqlite_doc_node(SyntaqliteDocArena *arena, uint32_t id) {
    if (id == SYNTAQLITE_NULL_DOC) return NULL;
    return (SyntaqliteDoc*)(arena->arena + arena->offsets[id]);
}
#define DOC_NODE(arena, id) syntaqlite_doc_node(arena, id)

// Arena management
void syntaqlite_doc_arena_init(SyntaqliteDocArena *arena);
void syntaqlite_doc_arena_free(SyntaqliteDocArena *arena);

// Builder functions - return doc ID (or SYNTAQLITE_NULL_DOC on error)
uint32_t doc_text(SyntaqliteDocArena *arena, const char *text, uint32_t length);
uint32_t doc_line(SyntaqliteDocArena *arena);
uint32_t doc_softline(SyntaqliteDocArena *arena);
uint32_t doc_hardline(SyntaqliteDocArena *arena);
uint32_t doc_nest(SyntaqliteDocArena *arena, int32_t indent, uint32_t child);
uint32_t doc_group(SyntaqliteDocArena *arena, uint32_t child);

// Concat: create empty, then append children one at a time
uint32_t doc_concat_empty(SyntaqliteDocArena *arena);
uint32_t doc_concat_append(SyntaqliteDocArena *arena, uint32_t concat_id, uint32_t child);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_FMT_DOC_H
