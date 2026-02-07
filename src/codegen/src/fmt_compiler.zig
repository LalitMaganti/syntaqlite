// Fmt compiler: compiles FmtDoc JSON annotations into C FOP_ opcode arrays.
//
// Zig port of python/syntaqlite/ast_codegen/fmt_codegen.py.
// Reads the "fmt" key from node/list JSON definitions and produces
// static const SynqFmtOp arrays (format recipes) for the formatter interpreter.

const std = @import("std");
const node_defs = @import("node_defs.zig");
const string_utils = @import("string_utils.zig");
const Allocator = std.mem.Allocator;

const FlagEntry = node_defs.FlagEntry;
const FlagsDef = node_defs.FlagsDef;
const FieldDef = node_defs.FieldDef;
const Definition = node_defs.Definition;

const pascalToUpperSnake = string_utils.pascalToUpperSnake;
const pascalToSnakeCase = string_utils.pascalToSnakeCase;
const enumPrefix = string_utils.enumPrefix;
const eqlIgnoreCase = string_utils.eqlIgnoreCase;

// ============ Op Compiler ============

const OpCompiler = struct {
    allocator: Allocator,
    node_name: []const u8,
    fields: ?[]const FieldDef,
    flags_lookup: *const std.StringHashMap(FlagsDef),
    ops: std.ArrayList([]const u8),
    aux_lines: std.ArrayList([]const u8),
    aux_counter: u32,

    fn init(
        allocator: Allocator,
        node_name: []const u8,
        fields: ?[]const FieldDef,
        flags_lookup: *const std.StringHashMap(FlagsDef),
    ) OpCompiler {
        return .{
            .allocator = allocator,
            .node_name = node_name,
            .fields = fields,
            .flags_lookup = flags_lookup,
            .ops = std.ArrayList([]const u8).init(allocator),
            .aux_lines = std.ArrayList([]const u8).init(allocator),
            .aux_counter = 0,
        };
    }

    const CompileError = error{
        UnknownFmtDocType,
        InvalidClauseBody,
        CannotResolveFlagMask,
        OutOfMemory,
    };

    fn compile(self: *OpCompiler, doc: std.json.Value) CompileError!void {
        const obj = doc.object;
        const doc_type = obj.get("type").?.string;

        if (try self.tryOptimize(obj, doc_type)) return;

        if (std.mem.eql(u8, doc_type, "kw")) {
            const text = obj.get("text").?.string;
            try self.ops.append(try allocCStr(self.allocator, "FOP_KW(", text, ")"));
        } else if (std.mem.eql(u8, doc_type, "span")) {
            const field = obj.get("field").?.string;
            try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_SPAN({s}, {s})", .{ self.node_name, field }));
        } else if (std.mem.eql(u8, doc_type, "child")) {
            const field = obj.get("field").?.string;
            if (std.mem.eql(u8, field, "_item")) {
                try self.ops.append("FOP_CHILD_ITEM");
            } else {
                try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_CHILD({s}, {s})", .{ self.node_name, field }));
            }
        } else if (std.mem.eql(u8, doc_type, "line")) {
            try self.ops.append("FOP_LINE");
        } else if (std.mem.eql(u8, doc_type, "softline")) {
            try self.ops.append("FOP_SOFTLINE");
        } else if (std.mem.eql(u8, doc_type, "hardline")) {
            try self.ops.append("FOP_HARDLINE");
        } else if (std.mem.eql(u8, doc_type, "group")) {
            try self.ops.append("FOP_GROUP");
            try self.compile(obj.get("child").?);
        } else if (std.mem.eql(u8, doc_type, "nest")) {
            try self.ops.append("FOP_NEST");
            try self.compile(obj.get("child").?);
        } else if (std.mem.eql(u8, doc_type, "seq")) {
            const items = obj.get("items").?.array.items;
            try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_SEQ({d})", .{items.len}));
            for (items) |item| try self.compile(item);
        } else if (std.mem.eql(u8, doc_type, "if_set")) {
            try self.compileIfSet(obj);
        } else if (std.mem.eql(u8, doc_type, "if_flag")) {
            try self.compileIfFlag(obj);
        } else if (std.mem.eql(u8, doc_type, "if_enum")) {
            try self.compileIfEnum(obj);
        } else if (std.mem.eql(u8, doc_type, "if_span")) {
            try self.compileIfSpan(obj);
        } else if (std.mem.eql(u8, doc_type, "clause")) {
            try self.compileClause(obj);
        } else if (std.mem.eql(u8, doc_type, "enum_display")) {
            try self.compileEnumDisplay(obj);
        } else if (std.mem.eql(u8, doc_type, "switch")) {
            try self.compileSwitch(obj);
        } else if (std.mem.eql(u8, doc_type, "for_each_child")) {
            try self.compileForEach(obj);
        } else {
            return error.UnknownFmtDocType;
        }
    }

    // ---- Optimizations ----

    fn tryOptimize(self: *OpCompiler, obj: std.json.ObjectMap, doc_type: []const u8) !bool {
        if (!std.mem.eql(u8, doc_type, "if_set")) return false;
        if (obj.get("else") != null) return false;

        const field = obj.get("field").?.string;
        const then_obj = obj.get("then").?.object;
        const then_type = then_obj.get("type").?.string;

        // if_set(field, clause(kw, child(field))) -> FOP_CLAUSE
        if (std.mem.eql(u8, then_type, "clause")) {
            const body = then_obj.get("body").?.object;
            if (std.mem.eql(u8, body.get("type").?.string, "child")) {
                if (std.mem.eql(u8, body.get("field").?.string, field)) {
                    const keyword = then_obj.get("keyword").?.string;
                    try self.ops.append(try allocClauseOp(self.allocator, self.node_name, field, keyword));
                    return true;
                }
            }
        }

        // if_set(field, child(field)) -> FOP_CHILD
        if (std.mem.eql(u8, then_type, "child")) {
            if (std.mem.eql(u8, then_obj.get("field").?.string, field)) {
                try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_CHILD({s}, {s})", .{ self.node_name, field }));
                return true;
            }
        }

        return false;
    }

    // ---- Conditional compilers ----

    /// Emit an if/else conditional opcode and compile then/else branches.
    fn emitConditional(self: *OpCompiler, obj: std.json.ObjectMap, op: []const u8, op_else: []const u8) !void {
        const has_else = obj.get("else") != null;
        try self.ops.append(if (has_else) op_else else op);
        try self.compile(obj.get("then").?);
        if (has_else) try self.compile(obj.get("else").?);
    }

    fn compileIfSet(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const field = obj.get("field").?.string;
        try self.emitConditional(obj, try std.fmt.allocPrint(self.allocator, "FOP_IF_SET({s}, {s})", .{ self.node_name, field }), try std.fmt.allocPrint(self.allocator, "FOP_IF_SET_ELSE({s}, {s})", .{ self.node_name, field }));
    }

    fn compileIfFlag(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const field = obj.get("field").?.string;

        if (std.mem.indexOfScalar(u8, field, '.')) |dot_pos| {
            const field_base = field[0..dot_pos];
            const bit_name = field[dot_pos + 1 ..];
            const mask = try self.resolveFlagMask(field_base, bit_name);
            try self.emitConditional(obj, try std.fmt.allocPrint(self.allocator, "FOP_IF_FLAG({s}, {s}, {s})", .{ self.node_name, field_base, mask }), try std.fmt.allocPrint(self.allocator, "FOP_IF_FLAG_ELSE({s}, {s}, {s})", .{ self.node_name, field_base, mask }));
        } else {
            // Bool field: use IF_ENUM (Bool is int-sized, IF_FLAG reads uint8_t)
            try self.emitConditional(obj, try std.fmt.allocPrint(self.allocator, "FOP_IF_ENUM({s}, {s}, SYNTAQLITE_BOOL_TRUE)", .{ self.node_name, field }), try std.fmt.allocPrint(self.allocator, "FOP_IF_ENUM_ELSE({s}, {s}, SYNTAQLITE_BOOL_TRUE)", .{ self.node_name, field }));
        }
    }

    fn compileIfEnum(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const field = obj.get("field").?.string;
        const value = obj.get("value").?.string;
        const c_value = try self.resolveEnumValue(field, value);
        try self.emitConditional(obj, try std.fmt.allocPrint(self.allocator, "FOP_IF_ENUM({s}, {s}, {s})", .{ self.node_name, field, c_value }), try std.fmt.allocPrint(self.allocator, "FOP_IF_ENUM_ELSE({s}, {s}, {s})", .{ self.node_name, field, c_value }));
    }

    fn compileIfSpan(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const field = obj.get("field").?.string;
        try self.emitConditional(obj, try std.fmt.allocPrint(self.allocator, "FOP_IF_SPAN({s}, {s})", .{ self.node_name, field }), try std.fmt.allocPrint(self.allocator, "FOP_IF_SPAN_ELSE({s}, {s})", .{ self.node_name, field }));
    }

    // ---- Pattern compilers ----

    fn compileClause(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const keyword = obj.get("keyword").?.string;
        const body = obj.get("body").?.object;
        if (!std.mem.eql(u8, body.get("type").?.string, "child")) return error.InvalidClauseBody;
        const body_field = body.get("field").?.string;
        if (std.mem.eql(u8, body_field, "_item")) return error.InvalidClauseBody;
        try self.ops.append(try allocClauseOp(self.allocator, self.node_name, body_field, keyword));
    }

    fn compileEnumDisplay(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const field = obj.get("field").?.string;
        const mapping = obj.get("mapping").?.object;

        self.aux_counter += 1;
        const snake = try pascalToSnakeCase(self.allocator, self.node_name);
        const arr_name = try std.fmt.allocPrint(self.allocator, "fmt_{s}_entries_{d}", .{ snake, self.aux_counter });

        try self.aux_lines.append(try std.fmt.allocPrint(self.allocator, "static const SynqFmtEnumEntry {s}[] = {{", .{arr_name}));

        var count: usize = 0;
        var it = mapping.iterator();
        while (it.next()) |entry| {
            const c_enum = try self.resolveEnumValue(field, entry.key_ptr.*);
            const display_str = entry.value_ptr.string;
            try self.aux_lines.append(try allocEnumEntry(self.allocator, c_enum, display_str));
            count += 1;
        }

        try self.aux_lines.append("};");
        try self.aux_lines.append("");

        try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_ENUM_DISPLAY({s}, {s}, {s}, {d})", .{ self.node_name, field, arr_name, count }));
    }

    fn compileSwitch(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        const field = obj.get("field").?.string;
        const cases = obj.get("cases").?.object;
        const has_default = obj.get("default") != null;

        const case_count = cases.count();

        if (has_default) {
            try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_SWITCH_DEFAULT({s}, {s}, {d})", .{ self.node_name, field, case_count }));
        } else {
            try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_SWITCH({s}, {s}, {d})", .{ self.node_name, field, case_count }));
        }

        var it = cases.iterator();
        while (it.next()) |entry| {
            const c_enum = try self.resolveEnumValue(field, entry.key_ptr.*);
            try self.ops.append(try std.fmt.allocPrint(self.allocator, "FOP_CASE({s})", .{c_enum}));
            try self.compile(entry.value_ptr.*);
        }

        if (has_default) {
            try self.compile(obj.get("default").?);
        }
    }

    fn compileForEach(self: *OpCompiler, obj: std.json.ObjectMap) !void {
        if (obj.get("separator") != null) {
            try self.ops.append("FOP_FOR_EACH_SEP");
            try self.compile(obj.get("template").?);
            try self.compile(obj.get("separator").?);
        } else {
            try self.ops.append("FOP_FOR_EACH");
            try self.compile(obj.get("template").?);
        }
    }

    // ---- Type resolution ----

    fn resolveEnumValue(self: *OpCompiler, field_name: []const u8, value: []const u8) ![]const u8 {
        const type_name = self.getFieldTypeName(field_name);
        const prefix = try enumPrefix(self.allocator, type_name);
        return try std.fmt.allocPrint(self.allocator, "{s}_{s}", .{ prefix, value });
    }

    fn getFieldTypeName(self: *OpCompiler, field_name: []const u8) []const u8 {
        if (self.fields) |fields| {
            for (fields) |f| {
                if (std.mem.eql(u8, f.name, field_name) and f.storage == .@"inline") {
                    return f.type_name;
                }
            }
        }
        return field_name;
    }

    fn resolveFlagMask(self: *OpCompiler, field_base: []const u8, bit_name: []const u8) ![]const u8 {
        if (self.fields) |fields| {
            for (fields) |f| {
                if (std.mem.eql(u8, f.name, field_base) and f.storage == .@"inline") {
                    if (self.flags_lookup.get(f.type_name)) |flags_def| {
                        for (flags_def.flags) |flag| {
                            if (eqlIgnoreCase(flag.name, bit_name)) {
                                return try std.fmt.allocPrint(self.allocator, "0x{x:0>2}", .{flag.bit});
                            }
                        }
                    }
                }
            }
        }
        return error.CannotResolveFlagMask;
    }
};

// ============ Public API ============

/// Full pipeline: load definitions + fmt annotations from JSON files and emit fmt_gen.c.
pub fn generateFmtC(
    allocator: Allocator,
    nodes_dir: []const u8,
    writer: anytype,
) !void {
    const files = try node_defs.loadIndex(allocator, nodes_dir);
    const defs = try node_defs.loadDefinitions(allocator, nodes_dir, files);

    // Build lookup tables
    var flags_lookup = std.StringHashMap(FlagsDef).init(allocator);
    defer flags_lookup.deinit();
    var node_fields_lookup = std.StringHashMap([]const FieldDef).init(allocator);
    defer node_fields_lookup.deinit();

    for (defs) |def| {
        switch (def) {
            .flags_def => |f| try flags_lookup.put(f.name, f),
            .node_def => |n| try node_fields_lookup.put(n.name, n.fields),
            else => {},
        }
    }

    const DefInfo = struct { name: []const u8, is_list: bool, has_fmt: bool };
    var all_defs = std.ArrayList(DefInfo).init(allocator);
    defer all_defs.deinit();

    const CompiledRecipe = struct { snake_name: []const u8, ops: []const []const u8, aux_lines: []const []const u8 };
    var all_compiled = std.ArrayList(CompiledRecipe).init(allocator);
    defer all_compiled.deinit();

    // Load raw JSON to get fmt annotations
    for (files) |filename| {
        const file_path = try std.fs.path.join(allocator, &.{ nodes_dir, filename });
        defer allocator.free(file_path);

        const file = try std.fs.openFileAbsolute(file_path, .{});
        defer file.close();

        const content = try file.readToEndAlloc(allocator, 4 * 1024 * 1024);
        // Content ownership transfers to arena (don't free individually)

        const parsed = try std.json.parseFromSlice(std.json.Value, allocator, content, .{});
        // Don't deinit - JSON values are referenced by the compiler

        const definitions = parsed.value.object.get("definitions") orelse return error.MissingDefinitions;

        for (definitions.array.items) |def_val| {
            const obj = def_val.object;
            const kind = obj.get("kind").?.string;
            const name_str = obj.get("name").?.string;
            const name = try allocator.dupe(u8, name_str);
            const fmt = obj.get("fmt");
            const is_list = std.mem.eql(u8, kind, "list");

            if (!std.mem.eql(u8, kind, "node") and !is_list) continue;

            if (fmt) |fmt_val| {
                const fields = if (is_list) null else node_fields_lookup.get(name);
                var compiler = OpCompiler.init(allocator, name, fields, &flags_lookup);
                try compiler.compile(fmt_val);

                const snake = try pascalToSnakeCase(allocator, name);
                try all_compiled.append(.{
                    .snake_name = snake,
                    .ops = try compiler.ops.toOwnedSlice(),
                    .aux_lines = try compiler.aux_lines.toOwnedSlice(),
                });
                try all_defs.append(.{ .name = name, .is_list = is_list, .has_fmt = true });
            } else {
                try all_defs.append(.{ .name = name, .is_list = is_list, .has_fmt = false });
            }
        }
    }

    // Emit header
    try writer.writeAll(
        \\// Copyright 2025 The syntaqlite Authors. All rights reserved.
        \\// Licensed under the Apache License, Version 2.0.
        \\
        \\// Generated from ast_codegen node definitions - DO NOT EDIT
        \\// Regenerate with: python3 python/tools/extract_sqlite.py
        \\
        \\#include "src/formatter/fmt_ops.h"
        \\
        \\#include "src/parser/ast_nodes_gen.h"
        \\
        \\// ============ Format Recipes ============
        \\
        \\
    );

    // Emit aux lines and op arrays
    for (all_compiled.items) |recipe| {
        for (recipe.aux_lines) |line| {
            try writer.writeAll(line);
            try writer.writeByte('\n');
        }
        try writer.print("static const SynqFmtOp fmt_{s}[] = {{\n", .{recipe.snake_name});
        for (recipe.ops) |op| {
            try writer.print("    {s},\n", .{op});
        }
        try writer.writeAll("};\n\n");
    }

    // Build fmt name set
    var fmt_names = std.StringHashMap(void).init(allocator);
    defer fmt_names.deinit();
    for (all_compiled.items) |recipe| {
        try fmt_names.put(recipe.snake_name, {});
    }

    // Recipe table
    try writer.writeAll(
        \\// ============ Recipe Table ============
        \\
        \\
    );
    try writer.writeAll("const SynqFmtOp *synq_fmt_recipes[SYNTAQLITE_NODE_COUNT] = {\n");

    for (all_defs.items) |info| {
        const tag = try tagName(allocator, info.name);
        const snake = try pascalToSnakeCase(allocator, info.name);

        if (info.has_fmt) {
            try writer.print("    [{s}] = fmt_{s},\n", .{ tag, snake });
        } else if (info.is_list) {
            try writer.print("    [{s}] = synq_fmt_default_comma_list,\n", .{tag});
        }
    }

    try writer.writeAll("};\n");
}

// ============ String utilities ============

/// Build "FOP_KW("text")" with C-escaped string.
fn allocCStr(allocator: Allocator, prefix: []const u8, text: []const u8, suffix: []const u8) ![]const u8 {
    // Count how much space we need for the escaped string
    var escaped_len: usize = 0;
    for (text) |c| {
        if (c == '"' or c == '\\') {
            escaped_len += 2;
        } else {
            escaped_len += 1;
        }
    }

    const total = prefix.len + 1 + escaped_len + 1 + suffix.len; // prefix + " + escaped + " + suffix
    var buf = try allocator.alloc(u8, total);
    var pos: usize = 0;

    @memcpy(buf[pos..][0..prefix.len], prefix);
    pos += prefix.len;
    buf[pos] = '"';
    pos += 1;
    for (text) |c| {
        if (c == '"' or c == '\\') {
            buf[pos] = '\\';
            pos += 1;
        }
        buf[pos] = c;
        pos += 1;
    }
    buf[pos] = '"';
    pos += 1;
    @memcpy(buf[pos..][0..suffix.len], suffix);
    pos += suffix.len;

    return buf[0..pos];
}

/// Build 'FOP_CLAUSE(T, f, "keyword")'.
fn allocClauseOp(allocator: Allocator, node_name: []const u8, field: []const u8, keyword: []const u8) ![]const u8 {
    // FOP_CLAUSE(NodeName, field, "keyword")
    const prefix = try std.fmt.allocPrint(allocator, "FOP_CLAUSE({s}, {s}, ", .{ node_name, field });
    defer allocator.free(prefix);
    return try allocCStr(allocator, prefix, keyword, ")");
}

/// Build '    { ENUM_VALUE, "display_str" },' for SynqFmtEnumEntry.
fn allocEnumEntry(allocator: Allocator, c_enum: []const u8, display_str: []const u8) ![]const u8 {
    const prefix = try std.fmt.allocPrint(allocator, "    {{ {s}, ", .{c_enum});
    defer allocator.free(prefix);
    return try allocCStr(allocator, prefix, display_str, " },");
}

/// Produce the SYNTAQLITE_NODE_XXX tag name.
fn tagName(allocator: Allocator, name: []const u8) ![]const u8 {
    const upper = try pascalToUpperSnake(allocator, name);
    return try std.fmt.allocPrint(allocator, "SYNTAQLITE_NODE_{s}", .{upper});
}

// ============ Tests ============

test "allocCStr basic" {
    const allocator = std.testing.allocator;
    const result = try allocCStr(allocator, "FOP_KW(", "SELECT", ")");
    defer allocator.free(result);
    try std.testing.expectEqualStrings("FOP_KW(\"SELECT\")", result);
}

test "allocCStr with quotes" {
    const allocator = std.testing.allocator;
    const result = try allocCStr(allocator, "FOP_KW(", "say \"hi\"", ")");
    defer allocator.free(result);
    try std.testing.expectEqualStrings("FOP_KW(\"say \\\"hi\\\"\")", result);
}

const TestResult = struct { ops: []const []const u8, aux_lines: []const []const u8 };

fn compileJson(allocator: Allocator, json_str: []const u8, node_name: []const u8, fields: ?[]const FieldDef, flags_lookup: *const std.StringHashMap(FlagsDef)) !TestResult {
    const parsed = try std.json.parseFromSlice(std.json.Value, allocator, json_str, .{});
    // Don't deinit - arena will clean up
    var compiler = OpCompiler.init(allocator, node_name, fields, flags_lookup);
    try compiler.compile(parsed.value);
    return .{
        .ops = try compiler.ops.toOwnedSlice(),
        .aux_lines = try compiler.aux_lines.toOwnedSlice(),
    };
}

test "compile simple kw" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a, "{\"type\": \"kw\", \"text\": \"SELECT\"}", "TestNode", null, &fl);
    try std.testing.expectEqual(@as(usize, 1), r.ops.len);
    try std.testing.expectEqualStrings("FOP_KW(\"SELECT\")", r.ops[0]);
}

test "compile span" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a, "{\"type\": \"span\", \"field\": \"source\"}", "Literal", null, &fl);
    try std.testing.expectEqual(@as(usize, 1), r.ops.len);
    try std.testing.expectEqualStrings("FOP_SPAN(Literal, source)", r.ops[0]);
}

test "compile seq" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const json =
        \\{"type": "seq", "items": [
        \\  {"type": "kw", "text": "CAST("},
        \\  {"type": "child", "field": "expr"},
        \\  {"type": "kw", "text": ")"}
        \\]}
    ;
    const r = try compileJson(a, json, "CastExpr", null, &fl);
    try std.testing.expectEqual(@as(usize, 4), r.ops.len);
    try std.testing.expectEqualStrings("FOP_SEQ(3)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_KW(\"CAST(\")", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_CHILD(CastExpr, expr)", r.ops[2]);
    try std.testing.expectEqualStrings("FOP_KW(\")\")", r.ops[3]);
}

test "if_set+child optimization" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "if_set", "field": "columns", "then": {"type": "child", "field": "columns"}}
    , "SelectStmt", null, &fl);
    try std.testing.expectEqual(@as(usize, 1), r.ops.len);
    try std.testing.expectEqualStrings("FOP_CHILD(SelectStmt, columns)", r.ops[0]);
}

test "if_set+clause optimization" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "if_set", "field": "where", "then": {"type": "clause", "keyword": "WHERE", "body": {"type": "child", "field": "where"}}}
    , "SelectStmt", null, &fl);
    try std.testing.expectEqual(@as(usize, 1), r.ops.len);
    try std.testing.expectEqualStrings("FOP_CLAUSE(SelectStmt, where, \"WHERE\")", r.ops[0]);
}

test "if_flag with flags union" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    try fl.put("SelectStmtFlags", .{
        .name = "SelectStmtFlags",
        .flags = &[_]FlagEntry{.{ .name = "DISTINCT", .bit = 1 }},
    });
    const fields = &[_]FieldDef{
        .{ .name = "flags", .storage = .@"inline", .type_name = "SelectStmtFlags" },
    };
    const r = try compileJson(a,
        \\{"type": "if_flag", "field": "flags.distinct", "then": {"type": "kw", "text": "SELECT DISTINCT"}, "else": {"type": "kw", "text": "SELECT"}}
    , "SelectStmt", fields, &fl);
    try std.testing.expectEqual(@as(usize, 3), r.ops.len);
    try std.testing.expectEqualStrings("FOP_IF_FLAG_ELSE(SelectStmt, flags, 0x01)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_KW(\"SELECT DISTINCT\")", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_KW(\"SELECT\")", r.ops[2]);
}

test "if_flag with bool field" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "if_flag", "field": "is_temp", "then": {"type": "kw", "text": " TEMP"}}
    , "CreateTableStmt", null, &fl);
    try std.testing.expectEqual(@as(usize, 2), r.ops.len);
    try std.testing.expectEqualStrings("FOP_IF_ENUM(CreateTableStmt, is_temp, SYNTAQLITE_BOOL_TRUE)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_KW(\" TEMP\")", r.ops[1]);
}

test "for_each with separator" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "for_each_child", "template": {"type": "child", "field": "_item"}, "separator": {"type": "kw", "text": " "}}
    , "ColumnConstraintList", null, &fl);
    try std.testing.expectEqual(@as(usize, 3), r.ops.len);
    try std.testing.expectEqualStrings("FOP_FOR_EACH_SEP", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_CHILD_ITEM", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_KW(\" \")", r.ops[2]);
}

test "switch without default" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const fields = &[_]FieldDef{
        .{ .name = "raise_type", .storage = .@"inline", .type_name = "RaiseType" },
    };
    const r = try compileJson(a,
        \\{"type": "switch", "field": "raise_type", "cases": {
        \\  "IGNORE": {"type": "kw", "text": "IGNORE"},
        \\  "ROLLBACK": {"type": "kw", "text": "ROLLBACK"}
        \\}}
    , "RaiseExpr", fields, &fl);
    try std.testing.expectEqual(@as(usize, 5), r.ops.len);
    try std.testing.expectEqualStrings("FOP_SWITCH(RaiseExpr, raise_type, 2)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_CASE(SYNTAQLITE_RAISE_TYPE_IGNORE)", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_KW(\"IGNORE\")", r.ops[2]);
    try std.testing.expectEqualStrings("FOP_CASE(SYNTAQLITE_RAISE_TYPE_ROLLBACK)", r.ops[3]);
    try std.testing.expectEqualStrings("FOP_KW(\"ROLLBACK\")", r.ops[4]);
}

test "enum_display" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const fields = &[_]FieldDef{
        .{ .name = "op", .storage = .@"inline", .type_name = "UnaryOp" },
    };
    const r = try compileJson(a,
        \\{"type": "enum_display", "field": "op", "mapping": {"MINUS": "-", "PLUS": "+"}}
    , "UnaryExpr", fields, &fl);
    try std.testing.expectEqual(@as(usize, 1), r.ops.len);
    try std.testing.expectEqualStrings("FOP_ENUM_DISPLAY(UnaryExpr, op, fmt_unary_expr_entries_1, 2)", r.ops[0]);
    try std.testing.expect(r.aux_lines.len >= 3);
}

test "if_enum" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const fields = &[_]FieldDef{
        .{ .name = "sort_order", .storage = .@"inline", .type_name = "SortOrder" },
    };
    const r = try compileJson(a,
        \\{"type": "if_enum", "field": "sort_order", "value": "DESC", "then": {"type": "kw", "text": " DESC"}}
    , "OrderingTerm", fields, &fl);
    try std.testing.expectEqual(@as(usize, 2), r.ops.len);
    try std.testing.expectEqualStrings("FOP_IF_ENUM(OrderingTerm, sort_order, SYNTAQLITE_SORT_ORDER_DESC)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_KW(\" DESC\")", r.ops[1]);
}

test "if_span with else" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "if_span", "field": "column", "then": {"type": "span", "field": "column"}, "else": {"type": "kw", "text": "?"}}
    , "SetClause", null, &fl);
    try std.testing.expectEqual(@as(usize, 3), r.ops.len);
    try std.testing.expectEqualStrings("FOP_IF_SPAN_ELSE(SetClause, column)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_SPAN(SetClause, column)", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_KW(\"?\")", r.ops[2]);
}

test "group + nest + line" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "group", "child": {"type": "nest", "child": {"type": "seq", "items": [{"type": "line"}, {"type": "softline"}, {"type": "hardline"}]}}}
    , "T", null, &fl);
    try std.testing.expectEqual(@as(usize, 6), r.ops.len);
    try std.testing.expectEqualStrings("FOP_GROUP", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_NEST", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_SEQ(3)", r.ops[2]);
    try std.testing.expectEqualStrings("FOP_LINE", r.ops[3]);
    try std.testing.expectEqualStrings("FOP_SOFTLINE", r.ops[4]);
    try std.testing.expectEqualStrings("FOP_HARDLINE", r.ops[5]);
}

test "child _item" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "child", "field": "_item"}
    , "T", null, &fl);
    try std.testing.expectEqual(@as(usize, 1), r.ops.len);
    try std.testing.expectEqualStrings("FOP_CHILD_ITEM", r.ops[0]);
}

test "switch with default" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const fields = &[_]FieldDef{
        .{ .name = "op", .storage = .@"inline", .type_name = "BinaryOp" },
    };
    const r = try compileJson(a,
        \\{"type": "switch", "field": "op", "cases": {
        \\  "AND": {"type": "kw", "text": "AND"}
        \\}, "default": {"type": "kw", "text": "?"}}
    , "BinaryExpr", fields, &fl);
    try std.testing.expectEqual(@as(usize, 4), r.ops.len);
    try std.testing.expectEqualStrings("FOP_SWITCH_DEFAULT(BinaryExpr, op, 1)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_CASE(SYNTAQLITE_BINARY_OP_AND)", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_KW(\"AND\")", r.ops[2]);
    try std.testing.expectEqualStrings("FOP_KW(\"?\")", r.ops[3]);
}

test "for_each without separator" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const r = try compileJson(a,
        \\{"type": "for_each_child", "template": {"type": "child", "field": "_item"}}
    , "CaseWhenList", null, &fl);
    try std.testing.expectEqual(@as(usize, 2), r.ops.len);
    try std.testing.expectEqualStrings("FOP_FOR_EACH", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_CHILD_ITEM", r.ops[1]);
}

test "if_enum with else" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    var fl = std.StringHashMap(FlagsDef).init(a);
    const fields = &[_]FieldDef{
        .{ .name = "negated", .storage = .@"inline", .type_name = "Bool" },
    };
    const r = try compileJson(a,
        \\{"type": "if_enum", "field": "negated", "value": "TRUE", "then": {"type": "kw", "text": " NOT"}, "else": {"type": "kw", "text": ""}}
    , "BetweenExpr", fields, &fl);
    try std.testing.expectEqual(@as(usize, 3), r.ops.len);
    try std.testing.expectEqualStrings("FOP_IF_ENUM_ELSE(BetweenExpr, negated, SYNTAQLITE_BOOL_TRUE)", r.ops[0]);
    try std.testing.expectEqualStrings("FOP_KW(\" NOT\")", r.ops[1]);
    try std.testing.expectEqualStrings("FOP_KW(\"\")", r.ops[2]);
}
