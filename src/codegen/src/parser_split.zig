// Split Lemon-generated parser into three files.
//
// Zig port of python/syntaqlite/sqlite_extractor/parser_split.py.
// Uses extractParserData() to pull grammar data from the stock Lemon output,
// then assembles:
//   - sqlite_parse_defs.h   -- grammar-independent struct definitions
//   - sqlite_parse_tables.h -- grammar data + synq_parser_reduce()
//   - sqlite_parse_gen.c    -- parsing engine functions (includes tables.h)

const std = @import("std");
const mem = std.mem;
const Allocator = std.mem.Allocator;

const parser_extract = @import("parser_extract.zig");
const mk = @import("markers.zig");
const transforms = @import("transforms.zig");
const ParserData = parser_extract.ParserData;

pub const SplitResult = struct {
    defs_h: []const u8,
    tables_h: []const u8,
    engine_c: []const u8,

    pub fn deinit(self: *SplitResult, allocator: Allocator) void {
        allocator.free(self.defs_h);
        allocator.free(self.tables_h);
        allocator.free(self.engine_c);
    }
};

/// Split Lemon-generated parser into three files.
pub fn splitParserOutput(
    allocator: Allocator,
    content: []const u8,
    prefix: []const u8,
) !SplitResult {
    const data = try parser_extract.extractParserData(allocator, content, prefix);
    defer allocator.free(data.token_defines);

    if (data.struct_defs.len == 0) {
        return error.MissingStructDefs;
    }

    return .{
        .defs_h = try buildDefsH(allocator, data, prefix),
        .tables_h = try buildTablesH(allocator, data, prefix),
        .engine_c = try buildEngineC(allocator, content, data, prefix),
    };
}

/// Build sqlite_parse_defs.h -- control defines + parser struct definitions.
fn buildDefsH(allocator: Allocator, data: ParserData, prefix: []const u8) ![]const u8 {
    var parts = std.ArrayList(u8).init(allocator);
    const writer = parts.writer();

    var upper_prefix_buf: [64]u8 = undefined;
    const upper_prefix = upperPrefix(prefix, &upper_prefix_buf);

    try writer.print("#ifndef {s}_SRC_PARSER_SQLITE_PARSE_DEFS_H\n", .{upper_prefix});
    try writer.print("#define {s}_SRC_PARSER_SQLITE_PARSE_DEFS_H\n\n", .{upper_prefix});

    try writer.writeAll(
        \\#include "src/common/synq_sqlite_defs.h"
        \\#include "src/parser/ast_base.h"
        \\#include "src/parser/ast_builder_gen.h"
        \\
        \\
    );

    try writer.writeAll("/* Control defines */\n");
    if (data.control_defines.len > 0) {
        try writer.writeAll(data.control_defines);
        try writer.writeAll("\n\n");
    }

    try writer.writeAll("/* Parser struct definitions */\n");
    try writer.writeAll(data.struct_defs);
    try writer.writeAll("\n\n");

    try writer.print("#endif /* {s}_SRC_PARSER_SQLITE_PARSE_DEFS_H */\n", .{upper_prefix});

    return try parts.toOwnedSlice();
}

/// Write data section: comment + content (if non-empty) + trailing newlines.
fn writeDataSection(writer: anytype, comment: []const u8, data: []const u8) !void {
    try writer.writeAll(comment);
    if (data.len > 0) {
        try writer.writeAll(data);
        try writer.writeAll("\n\n");
    }
}

/// Write guarded data section: comment + guard_start + content + guard_end.
fn writeGuardedSection(writer: anytype, comment: []const u8, data: []const u8, guard_start: []const u8, guard_end: []const u8) !void {
    try writer.writeAll(comment);
    try writer.writeAll(guard_start);
    if (data.len > 0) try writer.writeAll(data);
    try writer.writeAll(guard_end);
}

/// Build sqlite_parse_tables.h from grammar data sections.
fn buildTablesH(allocator: Allocator, data: ParserData, prefix: []const u8) ![]const u8 {
    var parts = std.ArrayList(u8).init(allocator);
    const w = parts.writer();

    var upper_prefix_buf: [64]u8 = undefined;
    const upper_prefix = upperPrefix(prefix, &upper_prefix_buf);

    try w.print("#ifndef {s}_SRC_PARSER_SQLITE_PARSE_TABLES_H\n", .{upper_prefix});
    try w.print("#define {s}_SRC_PARSER_SQLITE_PARSE_TABLES_H\n\n", .{upper_prefix});
    try w.writeAll("#include \"src/parser/sqlite_parse_defs.h\"\n\n");

    try writeDataSection(w, "/* Token definitions */\n", data.token_defines);
    try writeDataSection(w, "/* Parsing tables */\n", data.parsing_tables);
    try writeGuardedSection(w, "/* Fallback table */\n", data.fallback_table, "#ifdef YYFALLBACK\n", "\n#endif /* YYFALLBACK */\n\n");
    try writeGuardedSection(w, "/* Token names (debug) */\n", data.token_names, "#if defined(YYCOVERAGE) || !defined(NDEBUG)\n", "\n#endif\n\n");
    try writeGuardedSection(w, "/* Rule names (debug) */\n", data.rule_names, "#ifndef NDEBUG\n", "\n#endif\n\n");
    try writeDataSection(w, "/* Rule info */\n", data.rule_info_lhs);
    try writeDataSection(w, "", data.rule_info_nrhs);

    const reduce_fn = try parser_extract.formatExtensionReduceFunction(allocator, data, prefix);
    defer allocator.free(reduce_fn);
    try w.writeAll(reduce_fn);
    try w.writeAll("\n");

    try w.print("#endif /* {s}_SRC_PARSER_SQLITE_PARSE_TABLES_H */\n", .{upper_prefix});

    return try parts.toOwnedSlice();
}

/// Build sqlite_parse_gen.c by removing data sections from the Lemon output.
fn buildEngineC(
    allocator: Allocator,
    content: []const u8,
    data: ParserData,
    prefix: []const u8,
) ![]const u8 {
    const sqlite3_prefix = try std.fmt.allocPrint(allocator, "{s}_sqlite3", .{prefix});
    defer allocator.free(sqlite3_prefix);

    const struct_typedef = try std.fmt.allocPrint(allocator, "typedef struct {s}_yyParser {s}_yyParser;", .{ prefix, prefix });
    defer allocator.free(struct_typedef);

    const reduce_call = try std.fmt.allocPrint(
        allocator,
        "    default:\n      {s}_parser_reduce(yypParser, yyruleno, yymsp, yyLookahead,\n                              yyLookaheadToken {s}ParserCTX_PARAM);\n      break;",
        .{ prefix, sqlite3_prefix },
    );
    defer allocator.free(reduce_call);

    const switch_replacement = try std.mem.concat(allocator, u8, &.{
        "  switch( yyruleno ){\n",
        reduce_call,
        "\n  };",
    });
    defer allocator.free(switch_replacement);

    const conditional_include =
        "\n" ++
        "#ifdef SYNTAQLITE_EXTENSION_GRAMMAR\n" ++
        "#include SYNTAQLITE_EXTENSION_GRAMMAR\n" ++
        "#else\n" ++
        "#include \"src/parser/sqlite_parse_tables.h\"\n" ++
        "#endif\n";

    const include_end_with_nl = mk.INCLUDE_END ++ "\n";
    const include_replacement = try std.fmt.allocPrint(allocator, "{s}{s}", .{ include_end_with_nl, conditional_include });
    defer allocator.free(include_replacement);

    // Build transform pipeline
    var xforms = std.ArrayList(transforms.Transform).init(allocator);
    defer xforms.deinit();

    try xforms.append(.{ .replace_text = .{ .old = include_end_with_nl, .new = include_replacement, .count = 1 } });

    for ([_][2][]const u8{
        .{ mk.TOKEN_DEFS_START, mk.TOKEN_DEFS_END },
        .{ mk.CONTROL_DEFS_COMMENT_START, mk.CONTROL_DEFS_END },
        .{ mk.PARSING_TABLES_COMMENT_START, mk.PARSING_TABLES_END },
        .{ mk.FALLBACK_COMMENT_START, "#endif /* YYFALLBACK */" },
        .{ mk.STRUCT_COMMENT_START, struct_typedef },
        .{ mk.REDUCE_ACTIONS_START, mk.REDUCE_ACTIONS_END },
        .{ "/* Beginning here are the reduction cases.", "*/" },
    }) |s| {
        try xforms.append(.{ .remove_section = .{ .start_marker = s[0], .end_marker = s[1] } });
    }

    try xforms.append(.{ .replace_text = .{ .old = "#ifndef INTERFACE\n# define INTERFACE 1\n#endif\n", .new = "", .count = 1 } });

    for ([_][]const u8{ data.token_names, data.rule_names }) |arr| {
        if (arr.len > 0)
            try xforms.append(.{ .replace_text = .{ .old = arr, .new = "", .count = 1 } });
    }

    for ([_]struct { data: []const u8, comment: []const u8 }{
        .{ .data = data.rule_info_lhs, .comment = "/* For rule J, yyRuleInfoLhs[J]" },
        .{ .data = data.rule_info_nrhs, .comment = "/* For rule J, yyRuleInfoNRhs[J]" },
    }) |ri| {
        if (ri.data.len > 0)
            try xforms.append(.{ .remove_section = .{ .start_marker = ri.comment, .end_marker = lastLine(ri.data) } });
    }

    try xforms.append(.{ .replace_section = .{
        .start_marker = "  switch( yyruleno ){",
        .end_marker = "};",
        .replacement = switch_replacement,
    } });
    try xforms.append(.{ .collapse_blank_lines = .{} });

    const pipeline = transforms.Pipeline{ .transforms = xforms.items };
    return try pipeline.apply(allocator, content);
}

/// Get the last line of text.
fn lastLine(text: []const u8) []const u8 {
    const trimmed = mem.trimRight(u8, text, " \t\r\n");
    if (mem.lastIndexOf(u8, trimmed, "\n")) |pos| {
        return mem.trim(u8, trimmed[pos + 1 ..], " \t\r");
    }
    return trimmed;
}

fn upperPrefix(prefix: []const u8, buf: []u8) []const u8 {
    for (prefix, 0..) |c, i| {
        buf[i] = std.ascii.toUpper(c);
    }
    return buf[0..prefix.len];
}

// ============ Tests ============

test "lastLine" {
    try std.testing.expectEqualStrings("last", lastLine("first\nsecond\nlast"));
    try std.testing.expectEqualStrings("only", lastLine("only"));
    try std.testing.expectEqualStrings("last", lastLine("first\nlast\n"));
}

test "upperPrefix" {
    var buf: [64]u8 = undefined;
    try std.testing.expectEqualStrings("SYNQ", upperPrefix("synq", &buf));
    try std.testing.expectEqualStrings("FOO", upperPrefix("foo", &buf));
}

