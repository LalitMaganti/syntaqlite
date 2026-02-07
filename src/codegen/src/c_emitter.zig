const std = @import("std");
const node_defs = @import("node_defs.zig");
const string_utils = @import("string_utils.zig");
const Definition = node_defs.Definition;
const EnumDef = node_defs.EnumDef;
const FlagsDef = node_defs.FlagsDef;
const NodeDef = node_defs.NodeDef;
const ListDef = node_defs.ListDef;
const FieldDef = node_defs.FieldDef;

const Allocator = std.mem.Allocator;

const isSourceSpanType = string_utils.isSourceSpanType;
const pascalToUpperSnake = string_utils.pascalToUpperSnake;
const pascalToSnakeCase = string_utils.pascalToSnakeCase;

/// Convert UPPER_SNAKE_CASE flag name to lowercase for struct fields.
/// e.g. "DISTINCT" -> "distinct", "WITHOUT_ROWID" -> "without_rowid"
fn flagNameToLower(allocator: Allocator, name: []const u8) ![]const u8 {
    const result = try allocator.alloc(u8, name.len);
    for (name, 0..) |c, i| {
        result[i] = std.ascii.toLower(c);
    }
    return result;
}

/// Get the name from a node or list definition, or null for other types.
fn defName(def: Definition) ?[]const u8 {
    return switch (def) {
        .node_def => |n| n.name,
        .list_def => |l| l.name,
        else => null,
    };
}

pub fn emit(allocator: Allocator, defs: []const Definition, writer: anytype) !void {
    // Header
    try writer.writeAll(
        \\// Copyright 2025 The syntaqlite Authors. All rights reserved.
        \\// Licensed under the Apache License, Version 2.0.
        \\
        \\// Generated from data/ast_nodes.py - DO NOT EDIT
        \\// Regenerate with: python3 python/tools/extract_sqlite.py
        \\
        \\#ifndef SYNTAQLITE_AST_NODES_GEN_H
        \\#define SYNTAQLITE_AST_NODES_GEN_H
        \\
        \\#ifdef SYNTAQLITE_CUSTOM_NODES
        \\#include SYNTAQLITE_CUSTOM_NODES
        \\#else
        \\
        \\#include <stddef.h>
        \\#include <stdint.h>
        \\
        \\#include "syntaqlite/ast.h"
        \\
        \\#ifdef __cplusplus
        \\extern "C" {
        \\#endif
        \\
        \\
    );

    // Value Enums
    try writer.writeAll("// ============ Value Enums ============\n");
    for (defs) |def| {
        switch (def) {
            .enum_def => |e| try emitEnum(allocator, e, writer),
            else => {},
        }
    }

    // Flags Types
    try writer.writeAll("\n// ============ Flags Types ============\n");
    for (defs) |def| {
        switch (def) {
            .flags_def => |f| try emitFlags(allocator, f, writer),
            else => {},
        }
    }

    // Node Tags
    try writer.writeAll("\n// ============ Node Tags ============\n\ntypedef enum {\n    SYNTAQLITE_NODE_NULL = 0,\n");
    for (defs) |def| {
        const name = defName(def) orelse continue;
        const upper = try pascalToUpperSnake(allocator, name);
        defer allocator.free(upper);
        try writer.print("    SYNTAQLITE_NODE_{s},\n", .{upper});
    }
    try writer.writeAll("    SYNTAQLITE_NODE_COUNT\n} SyntaqliteNodeTag;\n\n");

    // Node Structs
    try writer.writeAll("// ============ Node Structs (variable sizes) ============\n");
    for (defs) |def| {
        switch (def) {
            .node_def => |n| try emitNodeStruct(allocator, n, writer),
            .list_def => |l| try emitListStruct(allocator, l, writer),
            else => {},
        }
    }

    // Node Union
    try writer.writeAll("\n// ============ Node Union ============\n\ntypedef union SyntaqliteNode {\n    uint8_t tag;\n");
    for (defs) |def| {
        const name = defName(def) orelse continue;
        const snake = try pascalToSnakeCase(allocator, name);
        defer allocator.free(snake);
        try writer.print("    Syntaqlite{s} {s};\n", .{ name, snake });
    }
    try writer.writeAll(
        \\} SyntaqliteNode;
        \\
        \\#ifdef __cplusplus
        \\}
        \\#endif
        \\
        \\#endif /* SYNTAQLITE_CUSTOM_NODES */
        \\
        \\#endif  // SYNTAQLITE_AST_NODES_GEN_H
        \\
    );
}

fn emitEnum(allocator: Allocator, e: EnumDef, writer: anytype) !void {
    const upper_prefix = try pascalToUpperSnake(allocator, e.name);
    defer allocator.free(upper_prefix);

    try writer.writeAll("\ntypedef enum {\n");
    for (e.values, 0..) |value, i| {
        try writer.print("    SYNTAQLITE_{s}_{s} = {d},\n", .{ upper_prefix, value, i });
    }
    try writer.print("}} Syntaqlite{s};\n", .{e.name});
}

fn emitFlags(allocator: Allocator, f: FlagsDef, writer: anytype) !void {
    try writer.print("\ntypedef union Syntaqlite{s} {{\n    uint8_t raw;\n    struct {{\n", .{f.name});
    for (f.flags) |flag| {
        const lower = try flagNameToLower(allocator, flag.name);
        defer allocator.free(lower);
        try writer.print("        uint8_t {s} : 1;\n", .{lower});
    }
    try writer.print("    }};\n}} Syntaqlite{s};\n", .{f.name});
}

fn emitNodeStruct(_: Allocator, n: NodeDef, writer: anytype) !void {
    try writer.print("\ntypedef struct Syntaqlite{s} {{\n    uint8_t tag;\n", .{n.name});
    for (n.fields) |field| {
        switch (field.storage) {
            .@"inline" => {
                if (isSourceSpanType(field.type_name)) {
                    try writer.print("    SyntaqliteSourceSpan {s};\n", .{field.name});
                } else {
                    // It's an enum, flags, or Bool type
                    try writer.print("    Syntaqlite{s} {s};\n", .{ field.type_name, field.name });
                }
            },
            .index => {
                try writer.print("    uint32_t {s};\n", .{field.name});
            },
        }
    }
    try writer.print("}} Syntaqlite{s};\n", .{n.name});
}

fn emitListStruct(_: Allocator, l: ListDef, writer: anytype) !void {
    try writer.print("\n// List of {s}\ntypedef struct Syntaqlite{s} {{\n", .{ l.child_type, l.name });
    try writer.writeAll("    uint8_t tag;\n    uint8_t _pad[3];\n    uint32_t count;\n    uint32_t children[];  // flexible array of indices\n");
    try writer.print("}} Syntaqlite{s};\n", .{l.name});
}

/// Emit the internal header (src/parser/ast_nodes_gen.h).
///
/// This is a thin wrapper that re-exports the public header plus internal additions:
/// - _names[] string arrays (debug/print)
/// - synq_ast_node() inline (needs SynqArena)
/// - AST_NODE macro
/// - synq_node_base_size() declaration
pub fn emitInternal(allocator: Allocator, defs: []const Definition, writer: anytype) !void {
    // Header
    try writer.writeAll(
        \\// Copyright 2025 The syntaqlite Authors. All rights reserved.
        \\// Licensed under the Apache License, Version 2.0.
        \\
        \\// Generated from data/ast_nodes.py - DO NOT EDIT
        \\// Regenerate with: python3 python/tools/extract_sqlite.py
        \\
        \\#ifndef SYNQ_SRC_AST_AST_NODES_GEN_H
        \\#define SYNQ_SRC_AST_AST_NODES_GEN_H
        \\
        \\// Public types (enums, node structs, union)
        \\#include "syntaqlite/ast_nodes_gen.h"
        \\
        \\// Internal dependencies (SynqArena, etc.)
        \\#include "src/parser/ast_base.h"
        \\
        \\#ifdef __cplusplus
        \\extern "C" {
        \\#endif
        \\
        \\
    );

    // Enum name string arrays
    for (defs) |def| {
        switch (def) {
            .enum_def => |e| try emitEnumNameArray(allocator, e, writer),
            else => {},
        }
    }

    // Node access inline function
    try writer.writeAll(
        \\// Access node by ID
        \\inline SyntaqliteNode* synq_ast_node(SynqArena *ast, uint32_t id) {
        \\    if (id == SYNTAQLITE_NULL_NODE) return NULL;
        \\    return (SyntaqliteNode*)(ast->data + ast->offsets[id]);
        \\}
        \\
        \\#define AST_NODE(ast, id) synq_ast_node(ast, id)
        \\
        \\// ============ Node Size Table ============
        \\
        \\// Returns the fixed size of a node type (0 for variable-size nodes like lists)
        \\size_t synq_node_base_size(uint8_t tag);
        \\
        \\#ifdef __cplusplus
        \\}
        \\#endif
        \\
        \\#endif  // SYNQ_SRC_AST_AST_NODES_GEN_H
        \\
    );
}

fn emitEnumNameArray(allocator: Allocator, e: EnumDef, writer: anytype) !void {
    const snake = try pascalToSnakeCase(allocator, e.name);
    defer allocator.free(snake);

    try writer.print("static const char* const synq_{s}_names[] = {{\n", .{snake});
    for (e.values) |value| {
        try writer.print("    \"{s}\",\n", .{value});
    }
    try writer.writeAll("};\n\n");
}

// ============ Builder Header Emitter ============

/// Emit the builder header (src/parser/ast_builder_gen.h).
/// Contains synq_ast_build() declaration + static inline wrappers for each NodeDef.
pub fn emitBuilderH(allocator: Allocator, defs: []const Definition, writer: anytype) !void {
    try writer.writeAll(
        \\// Copyright 2025 The syntaqlite Authors. All rights reserved.
        \\// Licensed under the Apache License, Version 2.0.
        \\
        \\// Generated from data/ast_nodes.py - DO NOT EDIT
        \\// Regenerate with: python3 python/tools/generate_ast.py
        \\
        \\#ifndef SYNQ_SRC_AST_AST_BUILDER_GEN_H
        \\#define SYNQ_SRC_AST_AST_BUILDER_GEN_H
        \\
        \\#include "src/parser/ast_nodes_gen.h"
        \\
        \\#ifdef __cplusplus
        \\extern "C" {
        \\#endif
        \\
        \\// Generic node builder: arena alloc + memcpy + table-driven range computation
        \\uint32_t synq_ast_build(SynqAstContext *ctx, uint8_t tag,
        \\                        const void *node_data, size_t node_size);
        \\
        \\// ============ Builder Functions ============
        \\
        \\
    );

    for (defs) |def| {
        switch (def) {
            .node_def => |n| try emitNodeBuilderInline(allocator, n, writer),
            .list_def => |l| try emitListBuilderDecl(allocator, l, writer),
            else => {},
        }
    }

    try writer.writeAll(
        \\#ifdef __cplusplus
        \\}
        \\#endif
        \\
        \\#endif  // SYNQ_SRC_AST_AST_BUILDER_GEN_H
        \\
    );
}

/// Build the full parameter string for a node builder function.
fn buildParamString(allocator: Allocator, fields: []const FieldDef) !std.ArrayList(u8) {
    var buf = std.ArrayList(u8).init(allocator);
    errdefer buf.deinit();

    try buf.appendSlice("SynqAstContext *ctx");
    for (fields) |field| {
        try buf.appendSlice(", ");
        try writeFieldType(field, buf.writer());
        try buf.append(' ');
        try buf.appendSlice(field.name);
    }

    return buf;
}

/// Emit a static inline wrapper that constructs a compound literal and calls synq_ast_build().
fn emitNodeBuilderInline(allocator: Allocator, n: NodeDef, writer: anytype) !void {
    const snake = try pascalToSnakeCase(allocator, n.name);
    defer allocator.free(snake);
    const upper = try pascalToUpperSnake(allocator, n.name);
    defer allocator.free(upper);

    // Emit signature with "static inline " prefix
    var params_buf = try buildParamString(allocator, n.fields);
    defer params_buf.deinit();

    if (params_buf.items.len > 80) {
        try writer.print("static inline uint32_t synq_ast_{s}(\n    SynqAstContext *ctx,\n", .{snake});
        for (n.fields, 0..) |field, i| {
            try writer.writeAll("    ");
            try writeFieldType(field, writer);
            try writer.print(" {s}", .{field.name});
            try writer.writeAll(if (i < n.fields.len - 1) ",\n" else "\n");
        }
        try writer.writeAll(") {\n");
    } else {
        try writer.print("static inline uint32_t synq_ast_{s}({s}) {{\n", .{ snake, params_buf.items });
    }

    // Build compound literal to check its length
    var literal_buf = std.ArrayList(u8).init(allocator);
    defer literal_buf.deinit();
    try literal_buf.writer().print("&(Syntaqlite{s}){{.tag = SYNTAQLITE_NODE_{s}", .{ n.name, upper });
    for (n.fields) |field| {
        try literal_buf.writer().print(", .{s} = {s}", .{ field.name, field.name });
    }
    try literal_buf.append('}');

    if (literal_buf.items.len > 80) {
        // Multi-line compound literal
        try writer.print("    return synq_ast_build(ctx, SYNTAQLITE_NODE_{s},\n", .{upper});
        try writer.print("        &(Syntaqlite{s}){{\n", .{n.name});
        // .tag is always first
        try writer.print("            .tag = SYNTAQLITE_NODE_{s},\n", .{upper});
        for (n.fields, 0..) |field, i| {
            try writer.print("            .{s} = {s}", .{ field.name, field.name });
            if (i < n.fields.len - 1) {
                try writer.writeAll(",\n");
            } else {
                try writer.writeAll("\n");
            }
        }
        try writer.print("        }}, sizeof(Syntaqlite{s}));\n", .{n.name});
    } else {
        try writer.print("    return synq_ast_build(ctx, SYNTAQLITE_NODE_{s}, {s}, sizeof(Syntaqlite{s}));\n", .{ upper, literal_buf.items, n.name });
    }

    try writer.writeAll("}\n\n");
}

fn emitListBuilderDecl(allocator: Allocator, l: ListDef, writer: anytype) !void {
    const snake = try pascalToSnakeCase(allocator, l.name);
    defer allocator.free(snake);

    try writer.print("// Create empty {s}\n", .{l.name});
    try writer.print("uint32_t synq_ast_{s}_empty(SynqAstContext *ctx);\n\n", .{snake});
    try writer.print("// Create {s} with single child\n", .{l.name});
    try writer.print("uint32_t synq_ast_{s}(SynqAstContext *ctx, uint32_t first_child);\n\n", .{snake});
    try writer.print("// Append child to {s} (may reallocate, returns new list ID)\n", .{l.name});
    try writer.print("uint32_t synq_ast_{s}_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);\n\n", .{snake});
}

fn writeFieldType(field: FieldDef, writer: anytype) !void {
    switch (field.storage) {
        .index => try writer.writeAll("uint32_t"),
        .@"inline" => {
            if (isSourceSpanType(field.type_name)) {
                try writer.writeAll("SyntaqliteSourceSpan");
            } else {
                try writer.print("Syntaqlite{s}", .{field.type_name});
            }
        },
    }
}

// ============ Builder Implementation Emitter ============

/// Emit per-node range field metadata tables and dispatch array.
fn emitRangeMetadata(allocator: Allocator, defs: []const Definition, writer: anytype) !void {
    try writer.writeAll("// ============ Range Field Metadata ============\n\n");
    try writer.writeAll("typedef struct { uint16_t offset; uint8_t kind; } SynqFieldRangeMeta;\n\n");

    // Per-node arrays
    for (defs) |def| {
        switch (def) {
            .node_def => |n| {
                // Count range fields
                var range_count: usize = 0;
                for (n.fields) |field| {
                    if (field.storage == .index or
                        (field.storage == .@"inline" and isSourceSpanType(field.type_name)))
                    {
                        range_count += 1;
                    }
                }
                if (range_count == 0) continue;

                const snake = try pascalToSnakeCase(allocator, n.name);
                defer allocator.free(snake);

                try writer.print("static const SynqFieldRangeMeta range_meta_{s}[] = {{\n", .{snake});
                for (n.fields) |field| {
                    if (field.storage == .index) {
                        try writer.print("    {{offsetof(Syntaqlite{s}, {s}), 0}},\n", .{ n.name, field.name });
                    } else if (field.storage == .@"inline" and isSourceSpanType(field.type_name)) {
                        try writer.print("    {{offsetof(Syntaqlite{s}, {s}), 1}},\n", .{ n.name, field.name });
                    }
                }
                try writer.writeAll("};\n\n");
            },
            else => {},
        }
    }

    // Dispatch table
    try writer.writeAll("static const struct { const SynqFieldRangeMeta *fields; uint8_t count; } range_meta_table[] = {\n");
    try writer.writeAll("    [SYNTAQLITE_NODE_NULL] = {NULL, 0},\n");
    for (defs) |def| {
        switch (def) {
            .node_def => |n| {
                const upper = try pascalToUpperSnake(allocator, n.name);
                defer allocator.free(upper);

                var range_count: usize = 0;
                for (n.fields) |field| {
                    if (field.storage == .index or
                        (field.storage == .@"inline" and isSourceSpanType(field.type_name)))
                    {
                        range_count += 1;
                    }
                }
                if (range_count > 0) {
                    const snake = try pascalToSnakeCase(allocator, n.name);
                    defer allocator.free(snake);
                    try writer.print("    [SYNTAQLITE_NODE_{s}] = {{range_meta_{s}, {d}}},\n", .{ upper, snake, range_count });
                } else {
                    try writer.print("    [SYNTAQLITE_NODE_{s}] = {{NULL, 0}},\n", .{upper});
                }
            },
            .list_def => |l| {
                const upper = try pascalToUpperSnake(allocator, l.name);
                defer allocator.free(upper);
                try writer.print("    [SYNTAQLITE_NODE_{s}] = {{NULL, 0}},\n", .{upper});
            },
            else => {},
        }
    }
    try writer.writeAll("};\n\n");
}

/// Emit the builder implementation (src/parser/ast_builder_gen.c).
/// Contains size table, range metadata, generic synq_ast_build(), and list builders.
pub fn emitBuilderC(allocator: Allocator, defs: []const Definition, writer: anytype) !void {
    try writer.writeAll(
        \\// Copyright 2025 The syntaqlite Authors. All rights reserved.
        \\// Licensed under the Apache License, Version 2.0.
        \\
        \\// Generated from data/ast_nodes.py - DO NOT EDIT
        \\// Regenerate with: python3 python/tools/generate_ast.py
        \\
        \\#include "src/parser/ast_builder_gen.h"
        \\
        \\#include <stdlib.h>
        \\#include <string.h>
        \\
        \\// External definition for inline function (C99/C11).
        \\extern inline SyntaqliteNode* synq_ast_node(SynqArena *ast, uint32_t id);
        \\
        \\// ============ Node Size Table ============
        \\
        \\static const size_t node_base_sizes[] = {
        \\    [SYNTAQLITE_NODE_NULL] = 0,
        \\
    );

    for (defs) |def| {
        const name = defName(def) orelse continue;
        const upper = try pascalToUpperSnake(allocator, name);
        defer allocator.free(upper);
        try writer.print("    [SYNTAQLITE_NODE_{s}] = sizeof(Syntaqlite{s}),\n", .{ upper, name });
    }

    try writer.writeAll(
        \\};
        \\
        \\size_t synq_node_base_size(uint8_t tag) {
        \\    if (tag >= SYNTAQLITE_NODE_COUNT) return 0;
        \\    return node_base_sizes[tag];
        \\}
        \\
        \\
    );

    // Range metadata tables
    try emitRangeMetadata(allocator, defs, writer);

    // Generic synq_ast_build() implementation
    try writer.writeAll(
        \\// ============ Generic Node Builder ============
        \\
        \\uint32_t synq_ast_build(SynqAstContext *ctx, uint8_t tag,
        \\                        const void *node_data, size_t node_size) {
        \\    uint32_t id = synq_arena_alloc(&ctx->ast, tag, node_size);
        \\    void *dest = ctx->ast.data + ctx->ast.offsets[id];
        \\    memcpy(dest, node_data, node_size);
        \\
        \\    // Table-driven range computation
        \\    if (tag < SYNTAQLITE_NODE_COUNT && range_meta_table[tag].count > 0) {
        \\        synq_ast_ranges_sync(ctx);
        \\        SynqSourceRange _r = {UINT32_MAX, 0};
        \\        const SynqFieldRangeMeta *fields = range_meta_table[tag].fields;
        \\        uint8_t count = range_meta_table[tag].count;
        \\        const uint8_t *base = (const uint8_t *)dest;
        \\        for (uint8_t i = 0; i < count; i++) {
        \\            if (fields[i].kind == 0) {
        \\                uint32_t child_id;
        \\                memcpy(&child_id, base + fields[i].offset, sizeof(uint32_t));
        \\                synq_ast_range_union(ctx, &_r, child_id);
        \\            } else {
        \\                SyntaqliteSourceSpan span;
        \\                memcpy(&span, base + fields[i].offset, sizeof(SyntaqliteSourceSpan));
        \\                synq_ast_range_union_span(&_r, span);
        \\            }
        \\        }
        \\        if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;
        \\    }
        \\
        \\    return id;
        \\}
        \\
        \\// ============ List Builders ============
        \\
        \\
    );

    // List builders only
    for (defs) |def| {
        switch (def) {
            .list_def => |l| try emitListBuilderImpl(allocator, l, writer),
            else => {},
        }
    }
}

fn emitListBuilderImpl(allocator: Allocator, l: ListDef, writer: anytype) !void {
    const snake = try pascalToSnakeCase(allocator, l.name);
    defer allocator.free(snake);
    const upper = try pascalToUpperSnake(allocator, l.name);
    defer allocator.free(upper);

    // Empty list creator
    try writer.print("uint32_t synq_ast_{s}_empty(SynqAstContext *ctx) {{\n", .{snake});
    try writer.print("    uint32_t id = synq_arena_alloc(&ctx->ast, SYNTAQLITE_NODE_{s}, sizeof(Syntaqlite{s}));\n\n", .{ upper, l.name });
    try writer.print("    Syntaqlite{s} *node = (Syntaqlite{s}*)\n", .{ l.name, l.name });
    try writer.writeAll("        (ctx->ast.data + ctx->ast.offsets[id]);\n");
    try writer.writeAll("    node->count = 0;\n");
    try writer.writeAll("    synq_ast_ranges_sync(ctx);\n");
    try writer.writeAll("    return id;\n}\n\n");

    // Single-child creator
    try writer.print("uint32_t synq_ast_{s}(SynqAstContext *ctx, uint32_t first_child) {{\n", .{snake});
    try writer.print("    return synq_ast_list_start(ctx, SYNTAQLITE_NODE_{s}, first_child);\n", .{upper});
    try writer.writeAll("}\n\n");

    // Append function
    try writer.print("uint32_t synq_ast_{s}_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {{\n", .{snake});
    try writer.writeAll("    if (list_id == SYNTAQLITE_NULL_NODE) {\n");
    try writer.print("        return synq_ast_{s}(ctx, child);\n", .{snake});
    try writer.writeAll("    }\n");
    try writer.print("    return synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_{s});\n", .{upper});
    try writer.writeAll("}\n\n");
}

// ============ Print Implementation Emitter ============

/// Emit the print implementation (src/parser/ast_print_gen.c).
pub fn emitPrintC(allocator: Allocator, defs: []const Definition, writer: anytype) !void {
    // Collect enum names and flags for type lookups
    var enum_names = std.StringHashMap(void).init(allocator);
    defer enum_names.deinit();
    var flags_lookup = std.StringHashMap(FlagsDef).init(allocator);
    defer flags_lookup.deinit();
    for (defs) |def| {
        switch (def) {
            .enum_def => |e| try enum_names.put(e.name, {}),
            .flags_def => |f| try flags_lookup.put(f.name, f),
            else => {},
        }
    }

    try writer.writeAll(
        \\// Copyright 2025 The syntaqlite Authors. All rights reserved.
        \\// Licensed under the Apache License, Version 2.0.
        \\
        \\// Generated from data/ast_nodes.py - DO NOT EDIT
        \\// Regenerate with: python3 python/tools/generate_ast.py
        \\
        \\#include "src/parser/ast_nodes_gen.h"
        \\#include "src/parser/ast_print.h"
        \\
        \\static void print_node(FILE *out, SynqArena *ast, uint32_t node_id,
        \\                       const char *source, int depth,
        \\                       const char *field_name);
        \\
        \\static void print_node(FILE *out, SynqArena *ast, uint32_t node_id,
        \\                       const char *source, int depth,
        \\                       const char *field_name) {
        \\  if (node_id == SYNTAQLITE_NULL_NODE) {
        \\    if (field_name) {
        \\      synq_ast_print_indent(out, depth);
        \\      fprintf(out, "%s: null\n", field_name);
        \\    }
        \\    return;
        \\  }
        \\
        \\  SyntaqliteNode *node = AST_NODE(ast, node_id);
        \\  if (!node) {
        \\    return;
        \\  }
        \\
        \\  switch (node->tag) {
        \\
    );

    for (defs) |def| {
        switch (def) {
            .node_def => |n| try emitPrintNodeCase(allocator, n, enum_names, flags_lookup, writer),
            .list_def => |l| try emitPrintListCase(allocator, l, writer),
            else => {},
        }
    }

    try writer.writeAll(
        \\    default:
        \\      synq_ast_print_indent(out, depth);
        \\      fprintf(out, "Unknown(tag=%d)\n", node->tag);
        \\      break;
        \\  }
        \\}
        \\
        \\void synq_ast_print(FILE *out, SynqArena *ast, uint32_t node_id,
        \\                          const char *source) {
        \\  print_node(out, ast, node_id, source, 0, NULL);
        \\}
        \\
    );
}

fn emitPrintNodeCase(
    allocator: Allocator,
    n: NodeDef,
    enum_names: std.StringHashMap(void),
    flags_lookup: std.StringHashMap(FlagsDef),
    writer: anytype,
) !void {
    const upper = try pascalToUpperSnake(allocator, n.name);
    defer allocator.free(upper);
    const snake = try pascalToSnakeCase(allocator, n.name);
    defer allocator.free(snake);

    try writer.print("    case SYNTAQLITE_NODE_{s}: {{\n", .{upper});
    try writer.writeAll("      synq_ast_print_indent(out, depth);\n");
    try writer.writeAll("      if (field_name)\n");
    try writer.print("        fprintf(out, \"%s: {s}\\n\", field_name);\n", .{n.name});
    try writer.writeAll("      else\n");
    try writer.print("        fprintf(out, \"{s}\\n\");\n", .{n.name});

    for (n.fields) |field| {
        switch (field.storage) {
            .index => {
                try writer.print("      print_node(out, ast, node->{s}.{s}, source, depth + 1, \"{s}\");\n", .{ snake, field.name, field.name });
            },
            .@"inline" => {
                if (isSourceSpanType(field.type_name)) {
                    try writer.writeAll("      synq_ast_print_indent(out, depth + 1);\n");
                    try writer.print("      fprintf(out, \"{s}: \");\n", .{field.name});
                    try writer.print("      synq_ast_print_source_span(out, source, node->{s}.{s});\n", .{ snake, field.name });
                    try writer.writeAll("      fprintf(out, \"\\n\");\n");
                } else if (enum_names.contains(field.type_name)) {
                    const names_snake = try pascalToSnakeCase(allocator, field.type_name);
                    defer allocator.free(names_snake);
                    try writer.writeAll("      synq_ast_print_indent(out, depth + 1);\n");
                    try writer.print("      fprintf(out, \"{s}: %s\\n\", synq_{s}_names[node->{s}.{s}]);\n", .{ field.name, names_snake, snake, field.name });
                } else if (flags_lookup.get(field.type_name)) |fdef| {
                    try writer.writeAll("      synq_ast_print_indent(out, depth + 1);\n");
                    try writer.print("      fprintf(out, \"{s}:\");\n", .{field.name});
                    for (fdef.flags) |flag| {
                        const lower = try flagNameToLower(allocator, flag.name);
                        defer allocator.free(lower);
                        try writer.print("      if (node->{s}.{s}.{s}) fprintf(out, \" {s}\");\n", .{ snake, field.name, lower, flag.name });
                    }
                    try writer.print("      if (!node->{s}.{s}.raw) fprintf(out, \" (none)\");\n", .{ snake, field.name });
                    try writer.writeAll("      fprintf(out, \"\\n\");\n");
                } else {
                    try writer.writeAll("      synq_ast_print_indent(out, depth + 1);\n");
                    try writer.print("      fprintf(out, \"{s}: %u\\n\", node->{s}.{s});\n", .{ field.name, snake, field.name });
                }
            },
        }
    }

    try writer.writeAll("      break;\n    }\n\n");
}

fn emitPrintListCase(allocator: Allocator, l: ListDef, writer: anytype) !void {
    const upper = try pascalToUpperSnake(allocator, l.name);
    defer allocator.free(upper);
    const snake = try pascalToSnakeCase(allocator, l.name);
    defer allocator.free(snake);

    try writer.print("    case SYNTAQLITE_NODE_{s}: {{\n", .{upper});
    try writer.writeAll("      synq_ast_print_indent(out, depth);\n");
    try writer.writeAll("      if (field_name)\n");
    try writer.print("        fprintf(out, \"%s: {s}[%u]\\n\", field_name, node->{s}.count);\n", .{ l.name, snake });
    try writer.writeAll("      else\n");
    try writer.print("        fprintf(out, \"{s}[%u]\\n\", node->{s}.count);\n", .{ l.name, snake });
    try writer.print("      for (uint32_t i = 0; i < node->{s}.count; i++) {{\n", .{snake});
    try writer.print("        print_node(out, ast, node->{s}.children[i], source, depth + 1, NULL);\n", .{snake});
    try writer.writeAll("      }\n");
    try writer.writeAll("      break;\n    }\n\n");
}

// ============ Tests ============

// Helper to collect writer output into a string for testing.
fn collectOutput(allocator: Allocator, comptime emitFn: anytype, defs: []const Definition) ![]u8 {
    var buf = std.ArrayList(u8).init(allocator);
    errdefer buf.deinit();
    try emitFn(allocator, defs, buf.writer());
    return buf.toOwnedSlice();
}

test "emitInternal: enum name arrays" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .enum_def = .{ .name = "Bool", .values = &.{ "FALSE", "TRUE" } } },
    };

    const result = try collectOutput(allocator, emitInternal, &defs);
    defer allocator.free(result);

    // Should contain the name array
    try std.testing.expect(std.mem.indexOf(u8, result, "static const char* const synq_bool_names[] = {") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "    \"FALSE\",") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "    \"TRUE\",") != null);
    // Should contain the header guard
    try std.testing.expect(std.mem.indexOf(u8, result, "#ifndef SYNQ_SRC_AST_AST_NODES_GEN_H") != null);
    // Should contain synq_ast_node inline
    try std.testing.expect(std.mem.indexOf(u8, result, "inline SyntaqliteNode* synq_ast_node(") != null);
}

test "emitBuilderH: node builder inline wrapper" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .enum_def = .{ .name = "BinaryOp", .values = &.{"PLUS"} } },
        .{ .node_def = .{
            .name = "BinaryExpr",
            .fields = &.{
                .{ .name = "op", .storage = .@"inline", .type_name = "BinaryOp" },
                .{ .name = "left", .storage = .index, .type_name = "" },
                .{ .name = "right", .storage = .index, .type_name = "" },
            },
        } },
    };

    const result = try collectOutput(allocator, emitBuilderH, &defs);
    defer allocator.free(result);

    // synq_ast_build declaration
    try std.testing.expect(std.mem.indexOf(u8, result, "uint32_t synq_ast_build(SynqAstContext *ctx, uint8_t tag,") != null);
    // static inline wrapper
    try std.testing.expect(std.mem.indexOf(u8, result, "static inline uint32_t synq_ast_binary_expr(") != null);
    // Compound literal with synq_ast_build call
    try std.testing.expect(std.mem.indexOf(u8, result, "return synq_ast_build(ctx, SYNTAQLITE_NODE_BINARY_EXPR,") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, ".tag = SYNTAQLITE_NODE_BINARY_EXPR,") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, ".left = left,") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "#ifndef SYNQ_SRC_AST_AST_BUILDER_GEN_H") != null);
}

test "emitBuilderH: list builder declarations" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .list_def = .{ .name = "ExprList", .child_type = "Expr" } },
    };

    const result = try collectOutput(allocator, emitBuilderH, &defs);
    defer allocator.free(result);

    try std.testing.expect(std.mem.indexOf(u8, result, "uint32_t synq_ast_expr_list_empty(SynqAstContext *ctx);") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "uint32_t synq_ast_expr_list(SynqAstContext *ctx, uint32_t first_child);") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "uint32_t synq_ast_expr_list_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);") != null);
}

test "emitBuilderC: range metadata and generic builder" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .enum_def = .{ .name = "UnaryOp", .values = &.{"MINUS"} } },
        .{ .node_def = .{
            .name = "UnaryExpr",
            .fields = &.{
                .{ .name = "op", .storage = .@"inline", .type_name = "UnaryOp" },
                .{ .name = "operand", .storage = .index, .type_name = "" },
            },
        } },
    };

    const result = try collectOutput(allocator, emitBuilderC, &defs);
    defer allocator.free(result);

    // Size table entry
    try std.testing.expect(std.mem.indexOf(u8, result, "[SYNTAQLITE_NODE_UNARY_EXPR] = sizeof(SyntaqliteUnaryExpr),") != null);
    // Range metadata array
    try std.testing.expect(std.mem.indexOf(u8, result, "static const SynqFieldRangeMeta range_meta_unary_expr[] = {") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "{offsetof(SyntaqliteUnaryExpr, operand), 0},") != null);
    // Dispatch table entry
    try std.testing.expect(std.mem.indexOf(u8, result, "[SYNTAQLITE_NODE_UNARY_EXPR] = {range_meta_unary_expr, 1},") != null);
    // Generic builder function
    try std.testing.expect(std.mem.indexOf(u8, result, "uint32_t synq_ast_build(SynqAstContext *ctx, uint8_t tag,") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "memcpy(dest, node_data, node_size);") != null);
}

test "emitBuilderC: list builder implementation" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .list_def = .{ .name = "ExprList", .child_type = "Expr" } },
    };

    const result = try collectOutput(allocator, emitBuilderC, &defs);
    defer allocator.free(result);

    // Empty list creator
    try std.testing.expect(std.mem.indexOf(u8, result, "synq_ast_expr_list_empty(SynqAstContext *ctx)") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "node->count = 0;") != null);
    // Single-child creator
    try std.testing.expect(std.mem.indexOf(u8, result, "synq_ast_list_start(ctx, SYNTAQLITE_NODE_EXPR_LIST, first_child)") != null);
    // Append
    try std.testing.expect(std.mem.indexOf(u8, result, "synq_ast_list_append(ctx, list_id, child, SYNTAQLITE_NODE_EXPR_LIST)") != null);
}

test "emitPrintC: node with enum field" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .enum_def = .{ .name = "BinaryOp", .values = &.{"PLUS"} } },
        .{ .node_def = .{
            .name = "BinaryExpr",
            .fields = &.{
                .{ .name = "op", .storage = .@"inline", .type_name = "BinaryOp" },
                .{ .name = "left", .storage = .index, .type_name = "" },
                .{ .name = "right", .storage = .index, .type_name = "" },
            },
        } },
    };

    const result = try collectOutput(allocator, emitPrintC, &defs);
    defer allocator.free(result);

    // Node case
    try std.testing.expect(std.mem.indexOf(u8, result, "case SYNTAQLITE_NODE_BINARY_EXPR:") != null);
    // Enum field prints via names array
    try std.testing.expect(std.mem.indexOf(u8, result, "synq_binary_op_names[node->binary_expr.op]") != null);
    // Child nodes printed recursively
    try std.testing.expect(std.mem.indexOf(u8, result, "print_node(out, ast, node->binary_expr.left, source, depth + 1, \"left\")") != null);
}

test "emitPrintC: list node" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .list_def = .{ .name = "ExprList", .child_type = "Expr" } },
    };

    const result = try collectOutput(allocator, emitPrintC, &defs);
    defer allocator.free(result);

    try std.testing.expect(std.mem.indexOf(u8, result, "case SYNTAQLITE_NODE_EXPR_LIST:") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "node->expr_list.count") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "node->expr_list.children[i]") != null);
}

test "emitBuilderC: source span field in range metadata" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .node_def = .{
            .name = "Variable",
            .fields = &.{
                .{ .name = "source", .storage = .@"inline", .type_name = "SyntaqliteSourceSpan" },
            },
        } },
    };

    const result = try collectOutput(allocator, emitBuilderC, &defs);
    defer allocator.free(result);

    // Source span gets kind=1 in range metadata
    try std.testing.expect(std.mem.indexOf(u8, result, "static const SynqFieldRangeMeta range_meta_variable[] = {") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "{offsetof(SyntaqliteVariable, source), 1},") != null);
    try std.testing.expect(std.mem.indexOf(u8, result, "[SYNTAQLITE_NODE_VARIABLE] = {range_meta_variable, 1},") != null);
}

test "emitPrintC: source span field" {
    const allocator = std.testing.allocator;

    const defs = [_]Definition{
        .{ .node_def = .{
            .name = "Variable",
            .fields = &.{
                .{ .name = "source", .storage = .@"inline", .type_name = "SyntaqliteSourceSpan" },
            },
        } },
    };

    const result = try collectOutput(allocator, emitPrintC, &defs);
    defer allocator.free(result);

    try std.testing.expect(std.mem.indexOf(u8, result, "synq_ast_print_source_span(out, source, node->variable.source)") != null);
}
