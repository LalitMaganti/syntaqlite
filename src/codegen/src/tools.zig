// Symbol rename pipelines for SQLite extraction.

const std = @import("std");
const mem = std.mem;
const Allocator = mem.Allocator;
const transforms = @import("transforms.zig");
const Transform = transforms.Transform;
const Pipeline = transforms.Pipeline;

/// Create a pipeline for renaming sqlite3 symbols.
/// Uses the C tokenizer for safe renaming that preserves comments/strings.
pub fn createSymbolRenamePipeline(allocator: Allocator, prefix: []const u8) !Pipeline {
    const sqlite3_prefix = try std.fmt.allocPrint(allocator, "{s}_sqlite3", .{prefix});
    const testcase_name = try std.fmt.allocPrint(allocator, "{s}Testcase", .{sqlite3_prefix});
    const tolower_name = try std.fmt.allocPrint(allocator, "{s}Tolower", .{sqlite3_prefix});

    const xforms = try allocator.alloc(Transform, 4);
    // Rename sqlite3XxxYyy -> {prefix}_sqlite3XxxYyy
    xforms[0] = .{ .symbol_rename = .{ .old_prefix = "sqlite3", .new_prefix = sqlite3_prefix } };
    // Rename testcase() which doesn't have sqlite3 prefix
    xforms[1] = .{ .symbol_rename_exact = .{ .old_name = "testcase", .new_name = testcase_name } };
    // Remove the charMap macro - we'll use Tolower from sqlite_tables.h
    xforms[2] = .{ .remove_line_containing = .{ .needle = "define charMap" } };
    // Rename charMap -> Tolower
    xforms[3] = .{ .symbol_rename_exact = .{ .old_name = "charMap", .new_name = tolower_name } };

    return .{ .transforms = xforms };
}

/// Create a pipeline for renaming Lemon-generated parser symbols.
pub fn createParserSymbolRenamePipeline(allocator: Allocator, prefix: []const u8) !Pipeline {
    const sqlite3_prefix = try std.fmt.allocPrint(allocator, "{s}_sqlite3", .{prefix});

    const parser_name = try std.fmt.allocPrint(allocator, "{s}Parser", .{sqlite3_prefix});
    const parser_alloc = try std.fmt.allocPrint(allocator, "{s}ParserAlloc", .{sqlite3_prefix});
    const parser_free = try std.fmt.allocPrint(allocator, "{s}ParserFree", .{sqlite3_prefix});
    const parser_init = try std.fmt.allocPrint(allocator, "{s}ParserInit", .{sqlite3_prefix});
    const parser_finalize = try std.fmt.allocPrint(allocator, "{s}ParserFinalize", .{sqlite3_prefix});
    const parser_trace = try std.fmt.allocPrint(allocator, "{s}ParserTrace", .{sqlite3_prefix});
    const parser_fallback = try std.fmt.allocPrint(allocator, "{s}ParserFallback", .{sqlite3_prefix});
    const parser_coverage = try std.fmt.allocPrint(allocator, "{s}ParserCoverage", .{sqlite3_prefix});
    const parser_stack_peak = try std.fmt.allocPrint(allocator, "{s}ParserStackPeak", .{sqlite3_prefix});
    const yy_parser = try std.fmt.allocPrint(allocator, "{s}_yyParser", .{prefix});
    const yy_stack_entry = try std.fmt.allocPrint(allocator, "{s}_yyStackEntry", .{prefix});

    const xforms = try allocator.alloc(Transform, 11);
    xforms[0] = .{ .symbol_rename_exact = .{ .old_name = "Parse", .new_name = parser_name } };
    xforms[1] = .{ .symbol_rename_exact = .{ .old_name = "ParseAlloc", .new_name = parser_alloc } };
    xforms[2] = .{ .symbol_rename_exact = .{ .old_name = "ParseFree", .new_name = parser_free } };
    xforms[3] = .{ .symbol_rename_exact = .{ .old_name = "ParseInit", .new_name = parser_init } };
    xforms[4] = .{ .symbol_rename_exact = .{ .old_name = "ParseFinalize", .new_name = parser_finalize } };
    xforms[5] = .{ .symbol_rename_exact = .{ .old_name = "ParseTrace", .new_name = parser_trace } };
    xforms[6] = .{ .symbol_rename_exact = .{ .old_name = "ParseFallback", .new_name = parser_fallback } };
    xforms[7] = .{ .symbol_rename_exact = .{ .old_name = "ParseCoverage", .new_name = parser_coverage } };
    xforms[8] = .{ .symbol_rename_exact = .{ .old_name = "ParseStackPeak", .new_name = parser_stack_peak } };
    xforms[9] = .{ .symbol_rename_exact = .{ .old_name = "yyParser", .new_name = yy_parser } };
    xforms[10] = .{ .symbol_rename_exact = .{ .old_name = "yyStackEntry", .new_name = yy_stack_entry } };

    return .{ .transforms = xforms };
}


// ============ Tests ============

test "createSymbolRenamePipeline" {
    const allocator = std.testing.allocator;
    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();

    const pipeline = try createSymbolRenamePipeline(arena.allocator(), "synq");
    try std.testing.expectEqual(@as(usize, 4), pipeline.transforms.len);
}

test "createParserSymbolRenamePipeline" {
    const allocator = std.testing.allocator;
    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();

    const pipeline = try createParserSymbolRenamePipeline(arena.allocator(), "synq");
    try std.testing.expectEqual(@as(usize, 11), pipeline.transforms.len);
}

