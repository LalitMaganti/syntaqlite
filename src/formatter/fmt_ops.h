// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Format opcode definitions for the data-driven formatter interpreter.
//
// Format recipes are static const arrays of SynqFmtOp, encoded as a
// prefix tree: each op is followed inline by its children. A recursive
// interpreter walks the array to build Wadler-Lindig documents.
//
// Usage:
//   static const SynqFmtOp my_node_fmt[] = {
//       FOP_SEQ(2),
//           FOP_KW("SELECT "),
//           FOP_CHILD(MyNode, columns),
//   };

#ifndef SYNQ_SRC_FORMATTER_FMT_OPS_H
#define SYNQ_SRC_FORMATTER_FMT_OPS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // SYNQ_SRC_FORMATTER_FMT_OPS_H

// ============ Op Kinds ============

typedef enum {
    // Leaves (0 children in prefix stream)
    SYNQ_FOP_KW,           // keyword string: data = text
    SYNQ_FOP_SPAN,         // source span field: field_offset -> SyntaqliteSourceSpan
    SYNQ_FOP_CHILD,        // format child node: field_offset -> uint32_t node_id
    SYNQ_FOP_CHILD_ITEM,   // current loop child (inside FOR_EACH)
    SYNQ_FOP_LINE,         // space when flat, newline when broken
    SYNQ_FOP_SOFTLINE,     // empty when flat, newline when broken
    SYNQ_FOP_HARDLINE,     // always newline
    SYNQ_FOP_CLAUSE,       // SQL clause: field_offset -> node_id, data = keyword
    SYNQ_FOP_ENUM_DISPLAY, // enum -> string: field_offset, count, data = entries

    // Wrappers (1 child)
    SYNQ_FOP_GROUP,        // try flat, break if doesn't fit
    SYNQ_FOP_NEST,         // increase indent for child

    // Sequence (count children)
    SYNQ_FOP_SEQ,          // concat count children

    // Conditionals (1 or 2 children depending on HAS_ELSE)
    SYNQ_FOP_IF_SET,       // if index field != SYNTAQLITE_NULL_NODE
    SYNQ_FOP_IF_FLAG,      // if uint8_t field & mask (for flags unions)
    SYNQ_FOP_IF_ENUM,      // if enum field == value
    SYNQ_FOP_IF_SPAN,      // if source span has non-zero length

    // Switch (count CASE ops + optional default body)
    SYNQ_FOP_SWITCH,       // multi-branch on enum field
    SYNQ_FOP_CASE,         // switch case marker (1 child: body)

    // List iteration (1 or 2 children depending on HAS_SEPARATOR)
    SYNQ_FOP_FOR_EACH,     // iterate list children: template [+ separator]
} SynqFmtOpKind;

// ============ Op Flags ============

#define SYNQ_FOP_HAS_ELSE      0x01
#define SYNQ_FOP_HAS_SEPARATOR 0x02
#define SYNQ_FOP_HAS_DEFAULT   0x04

// ============ Enum Display Entry ============

// Maps an enum value to a display string (used by ENUM_DISPLAY).
typedef struct SynqFmtEnumEntry {
    int value;
    const char *text;
} SynqFmtEnumEntry;

// ============ Op Struct ============

// A single opcode in a format recipe.
// Encoding is prefix-tree: children follow inline after the parent op.
typedef struct SynqFmtOp {
    uint8_t kind;           // SynqFmtOpKind
    uint8_t flags;          // SYNQ_FOP_HAS_ELSE, etc.
    uint16_t count;         // SEQ: child count, ENUM_DISPLAY/SWITCH: case count
    uint16_t field_offset;  // byte offset of field in node struct
    uint16_t enum_value;    // IF_ENUM/CASE: value to compare
    const void *data;       // KW/CLAUSE: string, ENUM_DISPLAY: SynqFmtEnumEntry*
} SynqFmtOp;

// ============ List Layout Constants ============

// All list nodes share the same layout:
//   uint8_t tag; uint8_t _pad[3]; uint32_t count; uint32_t children[];
#define SYNQ_LIST_COUNT_OFFSET  4
#define SYNQ_LIST_CHILDREN_OFFSET 8

// ============ Helper Macros ============
//
// Node type T is PascalCase without the Syntaqlite prefix (e.g., SelectStmt).
// Field f is the struct member name (e.g., columns).
// The macro adds the Syntaqlite prefix for offsetof.

// --- Leaves ---

#define FOP_KW(s) \
    { .kind = SYNQ_FOP_KW, .data = (s) }

#define FOP_SPAN(T, f) \
    { .kind = SYNQ_FOP_SPAN, .field_offset = offsetof(Syntaqlite##T, f) }

#define FOP_CHILD(T, f) \
    { .kind = SYNQ_FOP_CHILD, .field_offset = offsetof(Syntaqlite##T, f) }

#define FOP_CHILD_ITEM \
    { .kind = SYNQ_FOP_CHILD_ITEM }

#define FOP_LINE \
    { .kind = SYNQ_FOP_LINE }

#define FOP_SOFTLINE \
    { .kind = SYNQ_FOP_SOFTLINE }

#define FOP_HARDLINE \
    { .kind = SYNQ_FOP_HARDLINE }

// CLAUSE: reads node_id from field, calls synq_format_clause(ctx, kw, id).
// Returns SYNQ_NULL_DOC when the field is SYNTAQLITE_NULL_NODE.
#define FOP_CLAUSE(T, f, kw) \
    { .kind = SYNQ_FOP_CLAUSE, .field_offset = offsetof(Syntaqlite##T, f), \
      .data = (kw) }

// ENUM_DISPLAY: maps enum field to display strings via lookup table.
//   entries: pointer to static SynqFmtEnumEntry array
//   n:       number of entries
#define FOP_ENUM_DISPLAY(T, f, entries, n) \
    { .kind = SYNQ_FOP_ENUM_DISPLAY, .field_offset = offsetof(Syntaqlite##T, f), \
      .count = (n), .data = (entries) }

// --- Wrappers (1 child follows) ---

#define FOP_GROUP \
    { .kind = SYNQ_FOP_GROUP }

#define FOP_NEST \
    { .kind = SYNQ_FOP_NEST }

// --- Sequence (count children follow) ---

#define FOP_SEQ(n) \
    { .kind = SYNQ_FOP_SEQ, .count = (n) }

// --- Conditionals ---
//
// IF_SET: tests uint32_t index field != SYNTAQLITE_NULL_NODE.
// IF_FLAG: reads uint8_t at field_offset, tests (raw & mask) != 0.
//   Use mask=0 to test raw != 0 (any flag set / Bool truthy).
// IF_ENUM: reads int (C enum) at field_offset, tests == value.
// IF_SPAN: reads SyntaqliteSourceSpan at field_offset, tests length > 0.
//
// Each has a plain variant (1 child: then) and _ELSE variant (2 children:
// then, else).

#define FOP_IF_SET(T, f) \
    { .kind = SYNQ_FOP_IF_SET, .field_offset = offsetof(Syntaqlite##T, f) }

#define FOP_IF_SET_ELSE(T, f) \
    { .kind = SYNQ_FOP_IF_SET, .field_offset = offsetof(Syntaqlite##T, f), \
      .flags = SYNQ_FOP_HAS_ELSE }

#define FOP_IF_FLAG(T, f, mask) \
    { .kind = SYNQ_FOP_IF_FLAG, .field_offset = offsetof(Syntaqlite##T, f), \
      .enum_value = (mask) }

#define FOP_IF_FLAG_ELSE(T, f, mask) \
    { .kind = SYNQ_FOP_IF_FLAG, .field_offset = offsetof(Syntaqlite##T, f), \
      .enum_value = (mask), .flags = SYNQ_FOP_HAS_ELSE }

#define FOP_IF_ENUM(T, f, v) \
    { .kind = SYNQ_FOP_IF_ENUM, .field_offset = offsetof(Syntaqlite##T, f), \
      .enum_value = (v) }

#define FOP_IF_ENUM_ELSE(T, f, v) \
    { .kind = SYNQ_FOP_IF_ENUM, .field_offset = offsetof(Syntaqlite##T, f), \
      .enum_value = (v), .flags = SYNQ_FOP_HAS_ELSE }

#define FOP_IF_SPAN(T, f) \
    { .kind = SYNQ_FOP_IF_SPAN, .field_offset = offsetof(Syntaqlite##T, f) }

#define FOP_IF_SPAN_ELSE(T, f) \
    { .kind = SYNQ_FOP_IF_SPAN, .field_offset = offsetof(Syntaqlite##T, f), \
      .flags = SYNQ_FOP_HAS_ELSE }

// --- Switch ---
//
// SWITCH: multi-branch on enum field. Followed by count CASE ops, each
// with 1 child (body subtree). Use _DEFAULT variant to add a fallback
// body after all cases.

#define FOP_SWITCH(T, f, n) \
    { .kind = SYNQ_FOP_SWITCH, .field_offset = offsetof(Syntaqlite##T, f), \
      .count = (n) }

#define FOP_SWITCH_DEFAULT(T, f, n) \
    { .kind = SYNQ_FOP_SWITCH, .field_offset = offsetof(Syntaqlite##T, f), \
      .count = (n), .flags = SYNQ_FOP_HAS_DEFAULT }

#define FOP_CASE(v) \
    { .kind = SYNQ_FOP_CASE, .enum_value = (v) }

// --- List iteration ---
//
// FOR_EACH: iterate list children. Followed by 1 child (template).
// Use _SEP variant to add a separator (2 children: template, separator).
// Inside the template, use FOP_CHILD_ITEM to format the current child.

#define FOP_FOR_EACH \
    { .kind = SYNQ_FOP_FOR_EACH }

#define FOP_FOR_EACH_SEP \
    { .kind = SYNQ_FOP_FOR_EACH, .flags = SYNQ_FOP_HAS_SEPARATOR }

// ============ Recipe Interpreter ============

// Forward declaration -- defined in fmt_helpers.h.
typedef struct SynqFmtCtx SynqFmtCtx;

// Look up recipe by node tag and interpret. This is the main dispatch entry.
uint32_t synq_fmt_interpret(SynqFmtCtx *ctx, uint32_t node_id);

// Interpret a specific op array against a raw node pointer (used by tests).
uint32_t synq_fmt_interpret_ops(SynqFmtCtx *ctx, const void *node_ptr,
                                const SynqFmtOp *ops);

// Default recipe for list nodes without explicit format annotations.
extern const SynqFmtOp synq_fmt_default_comma_list[];

// Generated table mapping node tags to format recipes (indexed by SyntaqliteNodeTag).
// NULL entries mean the node type has no formatter.
extern const SynqFmtOp *synq_fmt_recipes[];

#ifdef __cplusplus
}
#endif  // SYNQ_SRC_FORMATTER_FMT_OPS_H

#endif  // SYNQ_SRC_FORMATTER_FMT_OPS_H
