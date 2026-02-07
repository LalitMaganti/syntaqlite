// Process mkkeywordhash output for both base and extension builds.
//
// Zig port of python/syntaqlite/sqlite_extractor/keywordhash.py.
// Splits raw mkkeywordhash output into data (hash tables, arrays) and
// keywordCode function, applying symbol renames and hash param extraction.

const std = @import("std");
const mem = std.mem;
const Allocator = mem.Allocator;
const transforms = @import("transforms.zig");

/// Structural markers in mkkeywordhash output.
const KEYWORDHASH_SCORE_MARKER = "/* Hash score:";
const KEYWORD_CODE_FUNC_MARKER = "static int keywordCode(";

/// Keywordhash array symbols that need renaming.
pub const KEYWORDHASH_ARRAY_SYMBOLS = [_][]const u8{
    "zKWText",
    "aKWHash",
    "aKWNext",
    "aKWLen",
    "aKWOffset",
    "aKWCode",
};

/// Result of processing mkkeywordhash output.
pub const KeywordhashResult = struct {
    /// Tables + hash param #defines (swapped by extensions).
    data_section: []const u8,
    /// Parameterized keywordCode function (generic, lives in base).
    keyword_code_func: []const u8,
};

/// Hash formula parameters extracted from keywordCode function.
const HashParams = struct {
    c0: u32,
    c1: u32,
    c2: u32,
    mod: u32,
};

/// Extract hash formula parameters from a keywordCode function.
///
/// The function contains a line like:
///     i = ((charMap(z[0])*4) ^ (charMap(z[n-1])*3) ^ n*1) % 127;
/// (or with renamed symbols like synq_sqlite3Tolower instead of charMap)
///
/// Matches the pattern: z[0])*N) ^ (FUNC(z[n-1])*N) ^ n*N) % N
const hash_anchors = [_][]const u8{ "z[0])*", "z[n-1])*", "n*", "% " };

fn extractHashParams(func_text: []const u8) !HashParams {
    var pos: usize = 0;
    var values: [4]u32 = undefined;

    for (hash_anchors, 0..) |anchor, i| {
        const anchor_pos = mem.indexOfPos(u8, func_text, pos, anchor) orelse
            return error.HashParamsNotFound;
        pos = anchor_pos + anchor.len;
        const num = try parseNextNumber(func_text, pos);
        values[i] = num.value;
        pos = num.end;
    }

    return .{ .c0 = values[0], .c1 = values[1], .c2 = values[2], .mod = values[3] };
}

const ParsedNumber = struct {
    value: u32,
    end: usize,
};

/// Parse a decimal number starting at `start`, skipping leading whitespace.
fn parseNextNumber(text: []const u8, start: usize) !ParsedNumber {
    var pos = start;
    // Skip whitespace
    while (pos < text.len and (text[pos] == ' ' or text[pos] == '\t')) {
        pos += 1;
    }
    if (pos >= text.len or !std.ascii.isDigit(text[pos])) {
        return error.HashParamsNotFound;
    }
    var value: u32 = 0;
    while (pos < text.len and std.ascii.isDigit(text[pos])) {
        value = value * 10 + @as(u32, text[pos] - '0');
        pos += 1;
    }
    return .{ .value = value, .end = pos };
}

/// Format hash parameter #defines for the data section.
fn formatHashParamDefines(allocator: Allocator, prefix: []const u8, params: HashParams) ![]u8 {
    return std.fmt.allocPrint(allocator,
        \\#define {s}_KW_HASH_C0  {d}
        \\#define {s}_KW_HASH_C1  {d}
        \\#define {s}_KW_HASH_C2  {d}
        \\#define {s}_KW_HASH_MOD {d}
        \\
    , .{
        prefix, params.c0,
        prefix, params.c1,
        prefix, params.c2,
        prefix, params.mod,
    });
}

/// Replace hardcoded hash numbers in keywordCode with #define references.
///
/// Finds the pattern: FUNC(z[0])*N) ^ (FUNC(z[n-1])*N) ^ n*N) % N
/// and replaces each N with the corresponding #define name.
fn replaceHashFormulaWithDefines(allocator: Allocator, func_text: []const u8, prefix: []const u8) ![]u8 {
    // Validate params exist first
    _ = try extractHashParams(func_text);

    const define_suffixes = [_][]const u8{ "_KW_HASH_C0", "_KW_HASH_C1", "_KW_HASH_C2", "_KW_HASH_MOD" };

    var result = std.ArrayList(u8).init(allocator);
    errdefer result.deinit();

    var pos: usize = 0;
    for (hash_anchors, define_suffixes) |anchor, suffix| {
        const anchor_pos = mem.indexOfPos(u8, func_text, pos, anchor) orelse
            return allocator.dupe(u8, func_text);
        const num_start = anchor_pos + anchor.len;
        try result.appendSlice(func_text[pos..num_start]);
        const num = try parseNextNumber(func_text, num_start);
        try result.writer().print("{s}{s}", .{ prefix, suffix });
        pos = num.end;
    }

    try result.appendSlice(func_text[pos..]);
    return result.toOwnedSlice();
}

/// Create the full set of transforms for keywordhash processing.
pub fn createKeywordhashTransforms(
    allocator: Allocator,
    prefix: []const u8,
) ![]transforms.Transform {
    var list = std.ArrayList(transforms.Transform).init(allocator);
    errdefer list.deinit();

    const sqlite3_prefix = try std.fmt.allocPrint(allocator, "{s}_sqlite3", .{prefix});
    const testcase_name = try std.fmt.allocPrint(allocator, "{s}Testcase", .{sqlite3_prefix});

    try list.append(.{ .symbol_rename = .{ .old_prefix = "sqlite3", .new_prefix = sqlite3_prefix } });
    try list.append(.{ .symbol_rename = .{ .old_prefix = "TK_", .new_prefix = "SYNTAQLITE_TOKEN_", .whole_word = false } });

    const exact_renames = [_][2][]const u8{
        .{ "testcase", testcase_name },
        .{ "charMap", try std.fmt.allocPrint(allocator, "{s}Tolower", .{sqlite3_prefix}) },
    };
    for (&exact_renames) |r| {
        try list.append(.{ .symbol_rename_exact = .{ .old_name = r[0], .new_name = r[1] } });
    }

    for (&KEYWORDHASH_ARRAY_SYMBOLS) |symbol| {
        try list.append(.{ .symbol_rename_exact = .{
            .old_name = symbol,
            .new_name = try std.fmt.allocPrint(allocator, "{s}_{s}", .{ prefix, symbol }),
        } });
    }

    try list.append(.{ .remove_function_calls = .{ .function_name = testcase_name } });

    return list.toOwnedSlice();
}

/// Process mkkeywordhash output into data section and keywordCode function.
pub fn processKeywordhash(
    allocator: Allocator,
    raw_output: []const u8,
    prefix: []const u8,
) !KeywordhashResult {
    const xforms = try createKeywordhashTransforms(allocator, prefix);
    const output = try (transforms.Pipeline{ .transforms = xforms }).apply(allocator, raw_output);

    // Split at structural markers
    const start = mem.indexOf(u8, output, KEYWORDHASH_SCORE_MARKER) orelse 0;
    const generated = output[start..];
    const func_offset = mem.indexOf(u8, generated, KEYWORD_CODE_FUNC_MARKER) orelse
        return error.KeywordCodeNotFound;

    const data_raw = mem.trim(u8, generated[0..func_offset], " \t\r\n");
    const func_raw = if (mem.indexOf(u8, generated[func_offset..], "\n}\n")) |end|
        generated[func_offset .. func_offset + end + 3]
    else
        generated[func_offset..];

    var upper_buf: [64]u8 = undefined;
    const upper_prefix = upperCase(&upper_buf, prefix);

    // Build data section: tables + hash param #defines
    const param_defines = try formatHashParamDefines(allocator, upper_prefix, try extractHashParams(func_raw));
    const data_section = try std.fmt.allocPrint(allocator, "{s}\n\n{s}", .{
        data_raw, mem.trim(u8, param_defines, " \t\r\n"),
    });

    // Build function: replace magic numbers with #defines, rename signature
    const func_with_defines = try replaceHashFormulaWithDefines(allocator, func_raw, upper_prefix);
    const new_sig = try std.fmt.allocPrint(allocator, "int {s}_keywordCode(", .{prefix});
    const keyword_code_func = try replaceFirst(allocator, func_with_defines, "static int keywordCode(", new_sig);

    return .{
        .data_section = try allocator.dupe(u8, mem.trim(u8, data_section, " \t\r\n")),
        .keyword_code_func = try allocator.dupe(u8, mem.trim(u8, keyword_code_func, " \t\r\n")),
    };
}

/// Replace first occurrence of needle with replacement.
fn replaceFirst(allocator: Allocator, haystack: []const u8, needle: []const u8, replacement: []const u8) ![]u8 {
    const idx = mem.indexOf(u8, haystack, needle) orelse {
        return try allocator.dupe(u8, haystack);
    };
    const new_len = haystack.len - needle.len + replacement.len;
    var result = try allocator.alloc(u8, new_len);
    @memcpy(result[0..idx], haystack[0..idx]);
    @memcpy(result[idx .. idx + replacement.len], replacement);
    @memcpy(result[idx + replacement.len ..], haystack[idx + needle.len ..]);
    return result;
}

/// Convert a string to uppercase, writing into a buffer.
fn upperCase(buf: []u8, s: []const u8) []const u8 {
    const len = @min(s.len, buf.len);
    for (0..len) |i| {
        buf[i] = std.ascii.toUpper(s[i]);
    }
    return buf[0..len];
}

// ============ Tests ============

const testing = std.testing;

test "extractHashParams: typical mkkeywordhash output" {
    const func_text =
        \\static int keywordCode(const char *z, int n, int *pType){
        \\  int i, j;
        \\  const char *zKW;
        \\  i = ((charMap(z[0])*4) ^ (charMap(z[n-1])*3) ^ n*1) % 127;
        \\  return 0;
        \\}
    ;
    const params = try extractHashParams(func_text);
    try testing.expectEqual(@as(u32, 4), params.c0);
    try testing.expectEqual(@as(u32, 3), params.c1);
    try testing.expectEqual(@as(u32, 1), params.c2);
    try testing.expectEqual(@as(u32, 127), params.mod);
}

test "extractHashParams: renamed symbols" {
    const func_text =
        \\static int keywordCode(const char *z, int n, int *pType){
        \\  i = ((synq_sqlite3Tolower(z[0])*11) ^ (synq_sqlite3Tolower(z[n-1])*7) ^ n*2) % 255;
        \\}
    ;
    const params = try extractHashParams(func_text);
    try testing.expectEqual(@as(u32, 11), params.c0);
    try testing.expectEqual(@as(u32, 7), params.c1);
    try testing.expectEqual(@as(u32, 2), params.c2);
    try testing.expectEqual(@as(u32, 255), params.mod);
}

test "extractHashParams: not found" {
    const result = extractHashParams("no hash formula here");
    try testing.expectError(error.HashParamsNotFound, result);
}

test "formatHashParamDefines" {
    const result = try formatHashParamDefines(testing.allocator, "SYNQ", .{
        .c0 = 4,
        .c1 = 3,
        .c2 = 1,
        .mod = 127,
    });
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "#define SYNQ_KW_HASH_C0  4\n") != null);
    try testing.expect(mem.indexOf(u8, result, "#define SYNQ_KW_HASH_C1  3\n") != null);
    try testing.expect(mem.indexOf(u8, result, "#define SYNQ_KW_HASH_C2  1\n") != null);
    try testing.expect(mem.indexOf(u8, result, "#define SYNQ_KW_HASH_MOD 127\n") != null);
}

test "replaceHashFormulaWithDefines" {
    const func_text = "i = ((f(z[0])*4) ^ (f(z[n-1])*3) ^ n*1) % 127;\n";
    const result = try replaceHashFormulaWithDefines(testing.allocator, func_text, "SYNQ");
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "SYNQ_KW_HASH_C0") != null);
    try testing.expect(mem.indexOf(u8, result, "SYNQ_KW_HASH_C1") != null);
    try testing.expect(mem.indexOf(u8, result, "SYNQ_KW_HASH_C2") != null);
    try testing.expect(mem.indexOf(u8, result, "SYNQ_KW_HASH_MOD") != null);
    // Original numbers should be gone
    try testing.expect(mem.indexOf(u8, result, "*4)") == null);
    try testing.expect(mem.indexOf(u8, result, "*3)") == null);
    try testing.expect(mem.indexOf(u8, result, "% 127") == null);
}

test "processKeywordhash: full pipeline" {
    const raw =
        \\/* some preamble */
        \\/* Hash score: 42 */
        \\static const char zKWText[] = "ABORT";
        \\static const unsigned char aKWHash[] = { 0 };
        \\static const unsigned char aKWNext[] = { 0 };
        \\static const unsigned char aKWLen[] = { 5 };
        \\static const unsigned int aKWOffset[] = { 0 };
        \\static const int aKWCode[] = { TK_ABORT };
        \\static int keywordCode(const char *z, int n, int *pType){
        \\  int i, j;
        \\  const char *zKW;
        \\  i = ((charMap(z[0])*4) ^ (charMap(z[n-1])*3) ^ n*1) % 127;
        \\  return 0;
        \\}
        \\
    ;

    // Use arena since processKeywordhash makes many intermediate allocations.
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    const result = try processKeywordhash(allocator, raw, "synq");

    // Data section should have renamed arrays
    try testing.expect(mem.indexOf(u8, result.data_section, "synq_zKWText") != null);
    try testing.expect(mem.indexOf(u8, result.data_section, "synq_aKWHash") != null);
    try testing.expect(mem.indexOf(u8, result.data_section, "SYNTAQLITE_TOKEN_ABORT") != null);

    // Data section should have hash param defines
    try testing.expect(mem.indexOf(u8, result.data_section, "#define SYNQ_KW_HASH_C0") != null);
    try testing.expect(mem.indexOf(u8, result.data_section, "#define SYNQ_KW_HASH_MOD") != null);

    // Function should be renamed (non-static, with prefix)
    try testing.expect(mem.indexOf(u8, result.keyword_code_func, "int synq_keywordCode(") != null);
    try testing.expect(mem.indexOf(u8, result.keyword_code_func, "static int keywordCode(") == null);

    // Function should use #define references
    try testing.expect(mem.indexOf(u8, result.keyword_code_func, "SYNQ_KW_HASH_C0") != null);
}

test "replaceFirst: basic" {
    const result = try replaceFirst(testing.allocator, "hello world hello", "hello", "bye");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("bye world hello", result);
}

test "replaceFirst: not found" {
    const result = try replaceFirst(testing.allocator, "hello world", "xyz", "abc");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello world", result);
}

test "upperCase" {
    var buf: [64]u8 = undefined;
    const result = upperCase(&buf, "synq");
    try testing.expectEqualStrings("SYNQ", result);
}
