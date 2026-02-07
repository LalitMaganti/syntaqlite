// Transform lempar.c into syntaqlite templates.
//
// Zig port of python/syntaqlite/sqlite_extractor/lempar_transform.py.
// Transforms Lemon's lempar.c template to add split markers around each
// data section. After Lemon generates the parser, parser_split uses these
// markers to split the output into three files:
//   - sqlite_parse_defs.h   (struct definitions)
//   - sqlite_parse_tables.h (grammar data + reduce function)
//   - sqlite_parse_gen.c    (parsing engine)

const std = @import("std");
const mem = std.mem;
const Allocator = mem.Allocator;
const m = @import("markers.zig");

/// Transform lempar.c by adding split markers around data sections.
///
/// This creates a template that, after Lemon processes it, produces output
/// with clearly marked sections that parser_split can extract.
///
/// Markers use the format: /* SYNQ:<category>:<name>:BEGIN/END */
pub fn transformForSplit(allocator: Allocator, lempar_content: []const u8) ![]u8 {
    var content = try allocator.dupe(u8, lempar_content);

    const marker_defs = .{
        .{ "DATA", "token_defs", m.TOKEN_DEFS_START, m.TOKEN_DEFS_END, @as(?[]const u8, null) },
        .{ "DATA", "control_defs", m.CONTROL_DEFS_START, m.CONTROL_DEFS_END, @as(?[]const u8, null) },
        .{ "DATA", "tables", m.PARSING_TABLES_START, m.PARSING_TABLES_END, @as(?[]const u8, null) },
        .{ "DATA", "fallback", m.FALLBACK_ARRAY_DECL, "};", @as(?[]const u8, m.YYFALLBACK_IFDEF) },
        .{ "DATA", "token_names", m.TOKEN_NAMES_ARRAY_DECL, "};", @as(?[]const u8, m.YYCOVERAGE_NDEBUG_IFDEF) },
        .{ "DATA", "rule_names", m.RULE_NAMES_ARRAY_DECL, "};", @as(?[]const u8, m.NDEBUG_IFNDEF) },
        .{ "DATA", "rule_info_lhs", m.RULE_INFO_LHS_ARRAY_DECL, "};", @as(?[]const u8, null) },
        .{ "DATA", "rule_info_nrhs", m.RULE_INFO_NRHS_ARRAY_DECL, "};", @as(?[]const u8, null) },
        .{ "STRUCTS", "defs", m.STRUCT_COMMENT_START, m.STRUCT_END, @as(?[]const u8, null) },
        .{ "REDUCE", "actions", m.REDUCE_ACTIONS_START, m.REDUCE_ACTIONS_END, @as(?[]const u8, null) },
    };
    inline for (marker_defs) |md| {
        content = try addMarkerFree(allocator, content, md[0], md[1], md[2], md[3], md[4]);
    }

    return content;
}

/// Add markers and free old content. Convenience wrapper for chaining.
fn addMarkerFree(
    allocator: Allocator,
    old_content: []u8,
    category: []const u8,
    name: []const u8,
    start_marker: []const u8,
    end_marker: []const u8,
    after_marker: ?[]const u8,
) ![]u8 {
    const result = try addMarker(allocator, old_content, category, name, start_marker, end_marker, after_marker);
    if (result.ptr != old_content.ptr) {
        allocator.free(old_content);
    }
    return result;
}

/// Add SYNQ split markers around a section.
fn addMarker(
    allocator: Allocator,
    content: []const u8,
    category: []const u8,
    name: []const u8,
    start_marker: []const u8,
    end_marker: []const u8,
    after_marker: ?[]const u8,
) ![]u8 {
    var search_start: usize = 0;
    if (after_marker) |am| {
        const after_pos = mem.indexOf(u8, content, am) orelse {
            return try allocator.dupe(u8, content);
        };
        search_start = after_pos;
    }

    const start_pos = mem.indexOfPos(u8, content, search_start, start_marker) orelse {
        return try allocator.dupe(u8, content);
    };

    const end_pos = mem.indexOfPos(u8, content, start_pos, end_marker) orelse {
        return try allocator.dupe(u8, content);
    };

    const after_end_marker = end_pos + end_marker.len;

    // Find the start of the line containing start_marker
    const line_start = if (mem.lastIndexOfScalar(u8, content[0..start_pos], '\n')) |nl| nl + 1 else 0;

    // Find the end of the line containing end_marker
    const line_end = mem.indexOfScalarPos(u8, content, after_end_marker, '\n') orelse content.len;

    const section = content[line_start..line_end];

    var result = std.ArrayList(u8).init(allocator);
    errdefer result.deinit();
    try result.appendSlice(content[0..line_start]);
    try result.writer().print("/* SYNQ:{s}:{s}:BEGIN */\n", .{ category, name });
    try result.appendSlice(section);
    try result.writer().print("\n/* SYNQ:{s}:{s}:END */", .{ category, name });
    try result.appendSlice(content[line_end..]);

    return result.toOwnedSlice();
}

// ============ Tests ============

const testing = std.testing;

test "addMarker: basic section marking" {
    const content = "before\nSTART stuff END\nafter\n";
    const result = try addMarker(testing.allocator, content, "DATA", "test", "START", "END", null);
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:test:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "START stuff END") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:test:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "before\n") != null);
    try testing.expect(mem.indexOf(u8, result, "after\n") != null);
}

test "addMarker: with after_marker" {
    const content = "preamble\n#ifdef GUARD\narray = {1};\nrest\n";
    const result = try addMarker(testing.allocator, content, "DATA", "arr", "array", "};", "#ifdef GUARD");
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:arr:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "array = {1};") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:arr:END */") != null);
}

test "addMarker: after_marker not found returns copy" {
    const content = "no guard here\narray = {1};\n";
    const result = try addMarker(testing.allocator, content, "DATA", "arr", "array", "};", "#ifdef MISSING");
    defer testing.allocator.free(result);

    try testing.expectEqualStrings(content, result);
}

test "addMarker: start_marker not found returns copy" {
    const content = "no markers here\n";
    const result = try addMarker(testing.allocator, content, "DATA", "test", "MISSING", "END", null);
    defer testing.allocator.free(result);

    try testing.expectEqualStrings(content, result);
}

test "addMarker: end_marker not found returns copy" {
    const content = "START but no end\n";
    const result = try addMarker(testing.allocator, content, "DATA", "test", "START", "MISSING", null);
    defer testing.allocator.free(result);

    try testing.expectEqualStrings(content, result);
}

test "addMarker: multiline section" {
    const content =
        \\line 1
        \\/* BEGIN section */
        \\data line 1
        \\data line 2
        \\/* END section */
        \\line after
        \\
    ;
    const result = try addMarker(testing.allocator, content, "STRUCTS", "defs", "/* BEGIN section */", "/* END section */", null);
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:STRUCTS:defs:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* BEGIN section */") != null);
    try testing.expect(mem.indexOf(u8, result, "data line 1") != null);
    try testing.expect(mem.indexOf(u8, result, "data line 2") != null);
    try testing.expect(mem.indexOf(u8, result, "/* END section */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:STRUCTS:defs:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "line after") != null);
}

test "transformForSplit: adds all expected markers" {
    // Build a minimal lempar.c-like template with all the markers
    const template =
        \\preamble
        \\/* These constants specify the various numeric values for terminal symbols.
        \\ * ...
        \\/**************** End token definitions ***************************************/
        \\/************* Begin control #defines *****************************************/
        \\control stuff
        \\/************* End control #defines *******************************************/
        \\some code
        \\/*********** Begin parsing tables **********************************************/
        \\table data
        \\/********** End of lemon-generated parsing tables *****************************/
        \\#ifdef YYFALLBACK
        \\static const YYCODETYPE yyFallback[] = {
        \\  0
        \\};
        \\#endif
        \\#if defined(YYCOVERAGE) || !defined(NDEBUG)
        \\static const char *const yyTokenName[] = {
        \\  "SEMI"
        \\};
        \\#endif
        \\#ifndef NDEBUG
        \\static const char *const yyRuleName[] = {
        \\  "rule1"
        \\};
        \\#endif
        \\static const YYCODETYPE yyRuleInfoLhs[] = {
        \\  42
        \\};
        \\static const signed char yyRuleInfoNRhs[] = {
        \\  -1
        \\};
        \\/* The following structure represents a single element of the
        \\ * parser stack. */
        \\struct yyStackEntry { int major; };
        \\struct yyParser { int x; };
        \\typedef struct yyParser yyParser;
        \\engine code
        \\/********** Begin reduce actions **********************************************/
        \\case 0: break;
        \\/********** End reduce actions ************************************************/
        \\more engine code
        \\
    ;

    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    const result = try transformForSplit(allocator, template);

    // Check that all expected markers are present
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:token_defs:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:token_defs:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:control_defs:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:control_defs:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:tables:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:tables:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:fallback:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:fallback:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:token_names:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:token_names:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:rule_names:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:rule_names:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:rule_info_lhs:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:rule_info_lhs:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:rule_info_nrhs:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:DATA:rule_info_nrhs:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:STRUCTS:defs:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:STRUCTS:defs:END */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:REDUCE:actions:BEGIN */") != null);
    try testing.expect(mem.indexOf(u8, result, "/* SYNQ:REDUCE:actions:END */") != null);

    // Verify the section content is preserved between markers
    try testing.expect(mem.indexOf(u8, result, "control stuff") != null);
    try testing.expect(mem.indexOf(u8, result, "table data") != null);
    try testing.expect(mem.indexOf(u8, result, "case 0: break;") != null);
    try testing.expect(mem.indexOf(u8, result, "preamble") != null);
    try testing.expect(mem.indexOf(u8, result, "more engine code") != null);
}

test "transformForSplit: missing markers are silently skipped" {
    const minimal = "just some random C code\n";

    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();

    const result = try transformForSplit(arena.allocator(), minimal);

    // No markers should be added
    try testing.expect(mem.indexOf(u8, result, "SYNQ:") == null);
    try testing.expectEqualStrings(minimal, result);
}
