// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Format recipe interpreter: walks a prefix-encoded SynqFmtOp array
// and builds Wadler-Lindig documents.

#include "src/formatter/fmt_helpers.h"

#include "src/ast/ast_nodes_gen.h"

#include <stdbool.h>
#include <string.h>

// ============ Forward Declarations ============

static uint32_t interpret_op(SynqFmtCtx *ctx, const void *node_ptr,
                             const SynqFmtOp *ops, uint32_t *pos,
                             uint32_t loop_child_id);

static void skip_op(const SynqFmtOp *ops, uint32_t *pos);

// ============ Skip ============
//
// Advance *pos past one op and all its children without building docs.
// Used to skip non-taken branches in conditionals.

static void skip_op(const SynqFmtOp *ops, uint32_t *pos) {
    const SynqFmtOp *op = &ops[(*pos)++];

    switch (op->kind) {
    // Leaves: no children
    case SYNQ_FOP_KW:
    case SYNQ_FOP_SPAN:
    case SYNQ_FOP_CHILD:
    case SYNQ_FOP_CHILD_ITEM:
    case SYNQ_FOP_LINE:
    case SYNQ_FOP_SOFTLINE:
    case SYNQ_FOP_HARDLINE:
    case SYNQ_FOP_CLAUSE:
    case SYNQ_FOP_ENUM_DISPLAY:
        break;

    // Wrappers: 1 child
    case SYNQ_FOP_GROUP:
    case SYNQ_FOP_NEST:
        skip_op(ops, pos);
        break;

    // Sequence: count children
    case SYNQ_FOP_SEQ:
        for (uint16_t i = 0; i < op->count; i++)
            skip_op(ops, pos);
        break;

    // Conditionals: 1 or 2 children
    case SYNQ_FOP_IF_SET:
    case SYNQ_FOP_IF_FLAG:
    case SYNQ_FOP_IF_ENUM:
    case SYNQ_FOP_IF_SPAN:
        skip_op(ops, pos);  // then
        if (op->flags & SYNQ_FOP_HAS_ELSE) skip_op(ops, pos);
        break;

    // Switch: count CASE markers (each with 1 child) + optional default
    case SYNQ_FOP_SWITCH:
        for (uint16_t i = 0; i < op->count; i++) {
            (*pos)++;  // skip CASE marker
            skip_op(ops, pos);  // skip case body
        }
        if (op->flags & SYNQ_FOP_HAS_DEFAULT)
            skip_op(ops, pos);
        break;

    // Case marker: 1 child
    case SYNQ_FOP_CASE:
        skip_op(ops, pos);
        break;

    // For each: 1 or 2 children (template + optional separator)
    case SYNQ_FOP_FOR_EACH:
        skip_op(ops, pos);  // template
        if (op->flags & SYNQ_FOP_HAS_SEPARATOR)
            skip_op(ops, pos);  // separator
        break;
    }
}

// ============ Interpret ============

static uint32_t interpret_op(SynqFmtCtx *ctx, const void *node_ptr,
                             const SynqFmtOp *ops, uint32_t *pos,
                             uint32_t loop_child_id) {
    const SynqFmtOp *op = &ops[(*pos)++];
    const char *base = (const char *)node_ptr;

    switch (op->kind) {

    case SYNQ_FOP_KW:
        return synq_kw(ctx, (const char *)op->data);

    case SYNQ_FOP_SPAN: {
        SynqSourceSpan span;
        memcpy(&span, base + op->field_offset, sizeof(span));
        return synq_span_text(ctx, span);
    }

    case SYNQ_FOP_CHILD: {
        uint32_t child_id;
        memcpy(&child_id, base + op->field_offset, sizeof(child_id));
        return synq_format_node(ctx, child_id);
    }

    case SYNQ_FOP_CHILD_ITEM:
        return synq_format_node(ctx, loop_child_id);

    case SYNQ_FOP_LINE:
        return synq_doc_line(&ctx->docs);

    case SYNQ_FOP_SOFTLINE:
        return synq_doc_softline(&ctx->docs);

    case SYNQ_FOP_HARDLINE:
        return synq_doc_hardline(&ctx->docs);

    case SYNQ_FOP_CLAUSE: {
        uint32_t body_id;
        memcpy(&body_id, base + op->field_offset, sizeof(body_id));
        return synq_format_clause(ctx, (const char *)op->data, body_id);
    }

    case SYNQ_FOP_ENUM_DISPLAY: {
        int val;
        memcpy(&val, base + op->field_offset, sizeof(val));
        const SynqFmtEnumEntry *entries = (const SynqFmtEnumEntry *)op->data;
        for (uint16_t i = 0; i < op->count; i++) {
            if (entries[i].value == val)
                return synq_kw(ctx, entries[i].text);
        }
        return SYNQ_NULL_DOC;
    }

    case SYNQ_FOP_GROUP: {
        uint32_t child = interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return synq_doc_group(&ctx->docs, child);
    }

    case SYNQ_FOP_NEST: {
        uint32_t child = interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return synq_doc_nest(&ctx->docs,
                             (int32_t)ctx->options->indent_width, child);
    }

    case SYNQ_FOP_SEQ: {
        uint16_t n = op->count;
        uint32_t items[n > 0 ? n : 1];
        for (uint16_t i = 0; i < n; i++)
            items[i] = interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return synq_doc_concat(&ctx->docs, items, n);
    }

    case SYNQ_FOP_IF_SET: {
        uint32_t field_val;
        memcpy(&field_val, base + op->field_offset, sizeof(field_val));
        if (field_val != SYNQ_NULL_NODE) {
            uint32_t then_doc = interpret_op(ctx, node_ptr, ops, pos,
                                             loop_child_id);
            if (op->flags & SYNQ_FOP_HAS_ELSE) skip_op(ops, pos);
            return then_doc;
        }
        skip_op(ops, pos);  // skip then
        if (op->flags & SYNQ_FOP_HAS_ELSE)
            return interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return SYNQ_NULL_DOC;
    }

    case SYNQ_FOP_IF_FLAG: {
        uint8_t raw;
        memcpy(&raw, base + op->field_offset, sizeof(raw));
        bool test = op->enum_value ? (raw & (uint8_t)op->enum_value)
                                   : (raw != 0);
        if (test) {
            uint32_t then_doc = interpret_op(ctx, node_ptr, ops, pos,
                                             loop_child_id);
            if (op->flags & SYNQ_FOP_HAS_ELSE) skip_op(ops, pos);
            return then_doc;
        }
        skip_op(ops, pos);
        if (op->flags & SYNQ_FOP_HAS_ELSE)
            return interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return SYNQ_NULL_DOC;
    }

    case SYNQ_FOP_IF_ENUM: {
        int val;
        memcpy(&val, base + op->field_offset, sizeof(val));
        if (val == (int)op->enum_value) {
            uint32_t then_doc = interpret_op(ctx, node_ptr, ops, pos,
                                             loop_child_id);
            if (op->flags & SYNQ_FOP_HAS_ELSE) skip_op(ops, pos);
            return then_doc;
        }
        skip_op(ops, pos);
        if (op->flags & SYNQ_FOP_HAS_ELSE)
            return interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return SYNQ_NULL_DOC;
    }

    case SYNQ_FOP_IF_SPAN: {
        SynqSourceSpan span;
        memcpy(&span, base + op->field_offset, sizeof(span));
        if (span.length > 0) {
            uint32_t then_doc = interpret_op(ctx, node_ptr, ops, pos,
                                             loop_child_id);
            if (op->flags & SYNQ_FOP_HAS_ELSE) skip_op(ops, pos);
            return then_doc;
        }
        skip_op(ops, pos);
        if (op->flags & SYNQ_FOP_HAS_ELSE)
            return interpret_op(ctx, node_ptr, ops, pos, loop_child_id);
        return SYNQ_NULL_DOC;
    }

    case SYNQ_FOP_SWITCH: {
        int val;
        memcpy(&val, base + op->field_offset, sizeof(val));
        uint32_t result = SYNQ_NULL_DOC;
        bool matched = false;

        for (uint16_t i = 0; i < op->count; i++) {
            const SynqFmtOp *case_op = &ops[(*pos)++];
            // case_op->kind should be SYNQ_FOP_CASE
            if (!matched && (int)case_op->enum_value == val) {
                result = interpret_op(ctx, node_ptr, ops, pos,
                                      loop_child_id);
                matched = true;
            } else {
                skip_op(ops, pos);
            }
        }

        if (op->flags & SYNQ_FOP_HAS_DEFAULT) {
            if (!matched)
                result = interpret_op(ctx, node_ptr, ops, pos,
                                      loop_child_id);
            else
                skip_op(ops, pos);
        }

        return result;
    }

    case SYNQ_FOP_CASE:
        // Should not be reached directly; handled by SWITCH.
        return interpret_op(ctx, node_ptr, ops, pos, loop_child_id);

    case SYNQ_FOP_FOR_EACH: {
        bool has_sep = op->flags & SYNQ_FOP_HAS_SEPARATOR;

        // Pre-compute sub-expression boundaries.
        uint32_t template_start = *pos;
        uint32_t sep_start = template_start;
        skip_op(ops, &sep_start);  // sep_start = past template

        uint32_t end_pos = sep_start;
        if (has_sep) skip_op(ops, &end_pos);  // end_pos = past separator

        // Read list layout at fixed offsets.
        uint32_t count;
        memcpy(&count, base + SYNQ_LIST_COUNT_OFFSET, sizeof(count));
        const uint32_t *children =
            (const uint32_t *)(base + SYNQ_LIST_CHILDREN_OFFSET);

        uint32_t max = has_sep ? count * 2 : count;
        uint32_t buf[max > 0 ? max : 1];
        uint32_t n = 0;

        for (uint32_t i = 0; i < count; i++) {
            if (i > 0 && has_sep) {
                uint32_t sp = sep_start;
                buf[n++] = interpret_op(ctx, node_ptr, ops, &sp,
                                        children[i]);
            }
            uint32_t tp = template_start;
            buf[n++] = interpret_op(ctx, node_ptr, ops, &tp, children[i]);
        }

        *pos = end_pos;
        return synq_doc_concat(&ctx->docs, buf, n);
    }

    }

    // Unreachable for valid ops.
    return SYNQ_NULL_DOC;
}

// ============ Public API ============

uint32_t synq_fmt_interpret_ops(SynqFmtCtx *ctx, const void *node_ptr,
                                const SynqFmtOp *ops) {
    uint32_t pos = 0;
    return interpret_op(ctx, node_ptr, ops, &pos, SYNQ_NULL_NODE);
}

uint32_t synq_fmt_interpret(SynqFmtCtx *ctx, uint32_t node_id) {
    SynqNode *node = AST_NODE(ctx->ast, node_id);
    if (!node) return SYNQ_NULL_DOC;

    const SynqFmtOp *ops = node->tag < SYNQ_NODE_COUNT
        ? synq_fmt_recipes[node->tag] : NULL;
    if (!ops) return synq_kw(ctx, "/* UNSUPPORTED */");

    uint32_t pos = 0;
    return interpret_op(ctx, node, ops, &pos, SYNQ_NULL_NODE);
}

// ============ Default Comma List Recipe ============

// Generic recipe for list nodes without explicit format annotations.
// All list structs share the same layout (tag, _pad, count, children[])
// so this works for any list type.
const SynqFmtOp synq_fmt_default_comma_list[] = {
    FOP_GROUP,
    FOP_FOR_EACH_SEP,
        FOP_CHILD_ITEM,
        FOP_SEQ(2),
            FOP_KW(","),
            FOP_LINE,
};
