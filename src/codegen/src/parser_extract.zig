// Parser data extraction from Lemon-generated parse.c.
//
// Zig port of python/syntaqlite/sqlite_extractor/parser_extract.py.
// Extracts grammar data from a Lemon-generated parser file
// and formats it for inclusion in amalgamated extension headers.

const std = @import("std");
const mem = std.mem;
const Allocator = std.mem.Allocator;
pub const m = @import("markers.zig");

/// Extracted parser data from a Lemon-generated parse.c.
pub const ParserData = struct {
    token_defines: []const u8,
    control_defines: []const u8,
    parsing_tables: []const u8,
    fallback_table: []const u8,
    token_names: []const u8,
    rule_names: []const u8,
    rule_info_lhs: []const u8,
    rule_info_nrhs: []const u8,
    reduce_actions: []const u8,
    struct_defs: []const u8,
};

/// Extract a section of content between markers.
pub fn extractSection(
    content: []const u8,
    start_marker: []const u8,
    end_marker: []const u8,
    include_markers: bool,
) []const u8 {
    const start_pos = mem.indexOf(u8, content, start_marker) orelse return "";

    const extract_start = if (include_markers) start_pos else start_pos + start_marker.len;

    const end_pos = mem.indexOfPos(u8, content, extract_start, end_marker) orelse return "";

    const extract_end = if (include_markers) end_pos + end_marker.len else end_pos;

    return mem.trim(u8, content[extract_start..extract_end], " \t\r\n");
}

/// Extract an array definition from content.
/// Finds a line starting with `array_prefix` and returns everything from there
/// to the closing `};`.
pub fn extractArray(content: []const u8, array_prefix: []const u8) []const u8 {
    const start = mem.indexOf(u8, content, array_prefix) orelse return "";

    var brace_count: isize = 0;
    var i = start;
    while (i < content.len) : (i += 1) {
        if (content[i] == '{') {
            brace_count += 1;
        } else if (content[i] == '}') {
            brace_count -= 1;
            if (brace_count == 0) {
                // Find the semicolon
                const semicolon = mem.indexOfPos(u8, content, i, ";");
                const end = if (semicolon) |s| s + 1 else i + 1;
                return mem.trim(u8, content[start..end], " \t\r\n");
            }
        }
    }
    return "";
}

/// Extract parser struct definitions (yyStackEntry + yyParser).
pub fn extractStructDefs(content: []const u8, prefix: []const u8, buf: []u8) []const u8 {
    const start_pos = mem.indexOf(u8, content, m.STRUCT_COMMENT_START) orelse return "";

    // Build the end marker: "typedef struct <prefix>_yyParser <prefix>_yyParser;"
    const end_marker = std.fmt.bufPrint(buf, "typedef struct {s}_yyParser {s}_yyParser;", .{ prefix, prefix }) catch return "";

    const end_pos = mem.indexOfPos(u8, content, start_pos, end_marker) orelse return "";

    return content[start_pos .. end_pos + end_marker.len];
}

/// Extract token defines (#define SYNTAQLITE_TOKEN_* lines).
pub fn extractTokenDefines(allocator: Allocator, content: []const u8) ![]const u8 {
    var result = std.ArrayList(u8).init(allocator);

    var line_iter = mem.splitScalar(u8, content, '\n');
    while (line_iter.next()) |line| {
        const trimmed = mem.trim(u8, line, " \t\r");
        if (mem.startsWith(u8, trimmed, "#define SYNTAQLITE_TOKEN_")) {
            // Make sure it ends with a number
            if (mem.lastIndexOfAny(u8, trimmed, "0123456789")) |_| {
                if (result.items.len > 0) try result.append('\n');
                try result.appendSlice(trimmed);
            }
        }
    }

    return try result.toOwnedSlice();
}

/// Extract parser data from Lemon-generated parse.c.
pub fn extractParserData(allocator: Allocator, parse_c_content: []const u8, prefix: []const u8) !ParserData {
    var struct_buf: [256]u8 = undefined;

    const token_defines = try extractTokenDefines(allocator, parse_c_content);

    return .{
        .token_defines = token_defines,
        .control_defines = extractSection(parse_c_content, m.CONTROL_DEFS_START, m.CONTROL_DEFS_END, false),
        .parsing_tables = extractSection(parse_c_content, m.PARSING_TABLES_START, m.PARSING_TABLES_END, false),
        .reduce_actions = extractSection(parse_c_content, m.REDUCE_ACTIONS_START, m.REDUCE_ACTIONS_END, false),
        .struct_defs = extractStructDefs(parse_c_content, prefix, &struct_buf),
        .fallback_table = extractArray(parse_c_content, "static const YYCODETYPE yyFallback[]"),
        .token_names = extractArray(parse_c_content, "static const char *const yyTokenName[]"),
        .rule_names = extractArray(parse_c_content, "static const char *const yyRuleName[]"),
        .rule_info_lhs = extractArray(parse_c_content, "static const YYCODETYPE yyRuleInfoLhs[]"),
        .rule_info_nrhs = extractArray(parse_c_content, "static const signed char yyRuleInfoNRhs[]"),
    };
}

/// Format the extension reduce function.
pub fn formatExtensionReduceFunction(allocator: Allocator, data: ParserData, prefix: []const u8) ![]const u8 {
    var parts = std.ArrayList(u8).init(allocator);
    const writer = parts.writer();

    const sqlite3_prefix = try std.fmt.allocPrint(allocator, "{s}_sqlite3", .{prefix});
    defer allocator.free(sqlite3_prefix);

    try writer.print(
        \\#ifndef yytestcase
        \\# define yytestcase(X)
        \\#endif
        \\
        \\/* Reduce function */
        \\static inline void {s}_parser_reduce(
        \\  {s}_yyParser *yypParser,
        \\  unsigned int yyruleno,
        \\  {s}_yyStackEntry *yymsp,
        \\  int yyLookahead,
        \\  {s}ParserTOKENTYPE yyLookaheadToken
        \\  {s}ParserCTX_PDECL
        \\) {{
        \\  {s}ParserARG_FETCH
        \\  (void)yypParser;
        \\  (void)yyLookahead;
        \\  (void)yyLookaheadToken;
        \\  (void)yymsp;
        \\
        \\  switch( yyruleno ){{
        \\{s}
        \\  }};
        \\}}
        \\
    , .{
        prefix,
        prefix,
        prefix,
        sqlite3_prefix,
        sqlite3_prefix,
        sqlite3_prefix,
        data.reduce_actions,
    });

    return try parts.toOwnedSlice();
}

// ============ Tests ============

test "extractSection basic" {
    const content = "before /* START */ middle /* END */ after";
    const result = extractSection(content, "/* START */", "/* END */", false);
    try std.testing.expectEqualStrings("middle", result);
}

test "extractSection with markers" {
    const content = "before /* START */ middle /* END */ after";
    const result = extractSection(content, "/* START */", "/* END */", true);
    try std.testing.expectEqualStrings("/* START */ middle /* END */", result);
}

test "extractSection not found" {
    const content = "no markers here";
    const result = extractSection(content, "/* START */", "/* END */", false);
    try std.testing.expectEqualStrings("", result);
}

test "extractArray basic" {
    const content = "static const int arr[] = { 1, 2, 3 };";
    const result = extractArray(content, "static const int arr[]");
    try std.testing.expectEqualStrings("static const int arr[] = { 1, 2, 3 };", result);
}

test "extractArray nested braces" {
    const content = "static const int arr[] = { {1, 2}, {3, 4} };";
    const result = extractArray(content, "static const int arr[]");
    try std.testing.expectEqualStrings("static const int arr[] = { {1, 2}, {3, 4} };", result);
}

test "extractArray not found" {
    const content = "no array here";
    const result = extractArray(content, "static const int arr[]");
    try std.testing.expectEqualStrings("", result);
}

test "extractTokenDefines" {
    const allocator = std.testing.allocator;
    const content =
        \\#include <stdio.h>
        \\#define SYNTAQLITE_TOKEN_SEMI 1
        \\#define SYNTAQLITE_TOKEN_EXPLAIN 2
        \\#define OTHER_STUFF 42
        \\#define SYNTAQLITE_TOKEN_SELECT 3
    ;
    const result = try extractTokenDefines(allocator, content);
    defer allocator.free(result);

    try std.testing.expect(mem.indexOf(u8, result, "#define SYNTAQLITE_TOKEN_SEMI 1") != null);
    try std.testing.expect(mem.indexOf(u8, result, "#define SYNTAQLITE_TOKEN_EXPLAIN 2") != null);
    try std.testing.expect(mem.indexOf(u8, result, "#define SYNTAQLITE_TOKEN_SELECT 3") != null);
    try std.testing.expect(mem.indexOf(u8, result, "OTHER_STUFF") == null);
}

test "extractStructDefs" {
    var buf: [256]u8 = undefined;
    const content = "some stuff before\n" ++
        "/* The following structure represents a single element of the\n" ++
        " * parser stack. */\n" ++
        "struct synq_yyStackEntry { int major; };\n" ++
        "struct synq_yyParser { int something; };\n" ++
        "typedef struct synq_yyParser synq_yyParser;\n" ++
        "some stuff after\n";
    const result = extractStructDefs(content, "synq", &buf);
    try std.testing.expect(result.len > 0);
    try std.testing.expect(mem.startsWith(u8, result, "/* The following structure"));
    try std.testing.expect(mem.endsWith(u8, result, "typedef struct synq_yyParser synq_yyParser;"));
}
