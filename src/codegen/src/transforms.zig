// Transform pipeline for SQLite code extraction.
//
// Zig port of python/syntaqlite/sqlite_extractor/transforms.py.
// Defines composable text transformations used by the extraction pipeline.

const std = @import("std");
const mem = std.mem;
const Allocator = mem.Allocator;
const c_tokenizer = @import("c_tokenizer.zig");

/// A transform takes content and returns new (allocated) content.
/// Caller owns the returned slice.
pub const Transform = union(enum) {
    replace_text: ReplaceText,
    truncate_at: TruncateAt,
    symbol_rename: SymbolRename,
    symbol_rename_exact: SymbolRenameExact,
    remove_function_calls: RemoveFunctionCalls,
    remove_section: RemoveSection,
    strip_blessing_comment: StripBlessingComment,
    insert_before: InsertBefore,
    insert_after: InsertAfter,
    remove_line_containing: RemoveLineContaining,
    replace_section: ReplaceSection,
    collapse_blank_lines: CollapseBlankLines,

    /// Apply this transform to content, returning newly allocated result.
    pub fn apply(self: Transform, allocator: Allocator, content: []const u8) ![]u8 {
        return switch (self) {
            .replace_text => |t| t.apply(allocator, content),
            .truncate_at => |t| t.apply(allocator, content),
            .symbol_rename => |t| t.apply(allocator, content),
            .symbol_rename_exact => |t| t.apply(allocator, content),
            .remove_function_calls => |t| t.apply(allocator, content),
            .remove_section => |t| t.apply(allocator, content),
            .strip_blessing_comment => |t| t.apply(allocator, content),
            .insert_before => |t| t.apply(allocator, content),
            .insert_after => |t| t.apply(allocator, content),
            .remove_line_containing => |t| t.apply(allocator, content),
            .replace_section => |t| t.apply(allocator, content),
            .collapse_blank_lines => |t| t.apply(allocator, content),
        };
    }
};

/// A sequence of transforms applied in order.
pub const Pipeline = struct {
    transforms: []const Transform,

    /// Apply all transforms in sequence. Returns newly allocated result.
    pub fn apply(self: Pipeline, allocator: Allocator, content: []const u8) ![]u8 {
        var current = try allocator.dupe(u8, content);

        for (self.transforms) |transform| {
            const next = try transform.apply(allocator, current);
            allocator.free(current);
            current = next;
        }

        return current;
    }
};

/// Simple text replacement transform.
pub const ReplaceText = struct {
    old: []const u8,
    new: []const u8,
    /// Maximum number of replacements. 0 means replace all.
    count: usize = 0,

    pub fn apply(self: ReplaceText, allocator: Allocator, content: []const u8) ![]u8 {
        if (self.count == 0) {
            return replaceAll(allocator, content, self.old, self.new);
        }
        return replaceN(allocator, content, self.old, self.new, self.count);
    }
};

/// Truncate content at a marker string.
pub const TruncateAt = struct {
    marker: []const u8,
    include_marker: bool = false,

    pub fn apply(self: TruncateAt, allocator: Allocator, content: []const u8) ![]u8 {
        const idx = mem.indexOf(u8, content, self.marker) orelse {
            return try allocator.dupe(u8, content);
        };
        const end = if (self.include_marker) idx + self.marker.len else idx;
        return try allocator.dupe(u8, content[0..end]);
    }
};

/// Safe symbol renaming using the C tokenizer.
/// Only renames actual identifiers; strings and comments are left untouched.
pub const SymbolRename = struct {
    old_prefix: []const u8,
    new_prefix: []const u8,
    whole_word: bool = true,

    pub fn apply(self: SymbolRename, allocator: Allocator, content: []const u8) ![]u8 {
        return c_tokenizer.renameSymbolsSafe(
            allocator,
            content,
            self.old_prefix,
            self.new_prefix,
            self.whole_word,
        );
    }
};

/// Rename an exact symbol name using the C tokenizer.
pub const SymbolRenameExact = struct {
    old_name: []const u8,
    new_name: []const u8,

    pub fn apply(self: SymbolRenameExact, allocator: Allocator, content: []const u8) ![]u8 {
        return c_tokenizer.renameSymbolExact(
            allocator,
            content,
            self.old_name,
            self.new_name,
        );
    }
};

/// Remove all calls to a specific function, including the entire line.
/// Matches: optional leading whitespace, function_name(...);rest-of-line\n
pub const RemoveFunctionCalls = struct {
    function_name: []const u8,

    pub fn apply(self: RemoveFunctionCalls, allocator: Allocator, content: []const u8) ![]u8 {
        var result = std.ArrayList(u8).init(allocator);
        errdefer result.deinit();

        var pos: usize = 0;
        while (pos < content.len) {
            // Find start of next line
            const line_start = pos;
            const line_end = mem.indexOfScalarPos(u8, content, pos, '\n');
            const next_line = if (line_end) |end| end + 1 else content.len;
            const line = content[line_start .. line_end orelse content.len];

            // Check if this line contains a call to function_name
            if (lineContainsFunctionCall(line, self.function_name)) {
                // Skip this entire line
                pos = next_line;
            } else {
                try result.appendSlice(content[line_start..next_line]);
                pos = next_line;
            }
        }

        return result.toOwnedSlice();
    }
};

/// Remove everything between start_marker and end_marker (inclusive, line-aligned).
pub const RemoveSection = struct {
    start_marker: []const u8,
    end_marker: []const u8,

    pub fn apply(self: RemoveSection, allocator: Allocator, content: []const u8) ![]u8 {
        const start_pos = mem.indexOf(u8, content, self.start_marker) orelse {
            return try allocator.dupe(u8, content);
        };
        const end_pos = mem.indexOfPos(u8, content, start_pos, self.end_marker) orelse {
            return try allocator.dupe(u8, content);
        };

        // Expand to line boundaries
        const line_start = if (mem.lastIndexOfScalar(u8, content[0..start_pos], '\n')) |nl| nl + 1 else 0;

        const after_end_marker = end_pos + self.end_marker.len;
        const line_end = if (mem.indexOfScalarPos(u8, content, after_end_marker, '\n')) |nl| nl + 1 else content.len;

        var result = try allocator.alloc(u8, line_start + (content.len - line_end));
        @memcpy(result[0..line_start], content[0..line_start]);
        @memcpy(result[line_start..], content[line_end..]);
        return result;
    }
};

/// Remove SQLite's blessing comment from the start of content.
pub const StripBlessingComment = struct {
    pub fn apply(_: StripBlessingComment, allocator: Allocator, content: []const u8) ![]u8 {
        const marker = "May you share freely";
        const idx = mem.indexOf(u8, content, marker) orelse {
            return try allocator.dupe(u8, content);
        };

        // Find end of comment
        const end_comment = mem.indexOfPos(u8, content, idx, "*/") orelse {
            return try allocator.dupe(u8, content);
        };

        // Skip past the comment and any trailing whitespace
        var end = end_comment + 2;
        while (end < content.len and (content[end] == ' ' or content[end] == '\t' or content[end] == '\n')) {
            end += 1;
        }

        return try allocator.dupe(u8, content[end..]);
    }
};

/// Insert text at a position in content.
fn insertAt(allocator: Allocator, content: []const u8, pos: usize, text: []const u8) ![]u8 {
    var result = try allocator.alloc(u8, content.len + text.len);
    @memcpy(result[0..pos], content[0..pos]);
    @memcpy(result[pos .. pos + text.len], text);
    @memcpy(result[pos + text.len ..], content[pos..]);
    return result;
}

/// Insert text before a marker string.
pub const InsertBefore = struct {
    marker: []const u8,
    text: []const u8,

    pub fn apply(self: InsertBefore, allocator: Allocator, content: []const u8) ![]u8 {
        const idx = mem.indexOf(u8, content, self.marker) orelse {
            return try allocator.dupe(u8, content);
        };
        return insertAt(allocator, content, idx, self.text);
    }
};

/// Insert text after a marker string.
pub const InsertAfter = struct {
    marker: []const u8,
    text: []const u8,

    pub fn apply(self: InsertAfter, allocator: Allocator, content: []const u8) ![]u8 {
        const idx = mem.indexOf(u8, content, self.marker) orelse {
            return try allocator.dupe(u8, content);
        };
        return insertAt(allocator, content, idx + self.marker.len, self.text);
    }
};

/// Remove all lines containing a given substring.
/// Equivalent to Python's RemoveRegex for simple line-matching patterns.
pub const RemoveLineContaining = struct {
    needle: []const u8,

    pub fn apply(self: RemoveLineContaining, allocator: Allocator, content: []const u8) ![]u8 {
        var result = std.ArrayList(u8).init(allocator);
        errdefer result.deinit();

        var pos: usize = 0;
        while (pos < content.len) {
            const line_start = pos;
            const line_end = mem.indexOfScalarPos(u8, content, pos, '\n');
            const next_line = if (line_end) |end| end + 1 else content.len;
            const line = content[line_start .. line_end orelse content.len];

            if (mem.indexOf(u8, line, self.needle) == null) {
                try result.appendSlice(content[line_start..next_line]);
            }
            pos = next_line;
        }

        return result.toOwnedSlice();
    }
};

/// Replace everything from start_marker through end_marker (inclusive) with replacement text.
pub const ReplaceSection = struct {
    start_marker: []const u8,
    end_marker: []const u8,
    replacement: []const u8,

    pub fn apply(self: ReplaceSection, allocator: Allocator, content: []const u8) ![]u8 {
        const start_pos = mem.indexOf(u8, content, self.start_marker) orelse {
            return try allocator.dupe(u8, content);
        };
        const end_pos = mem.indexOfPos(u8, content, start_pos + self.start_marker.len, self.end_marker) orelse {
            return try allocator.dupe(u8, content);
        };
        const after_end = end_pos + self.end_marker.len;

        const new_len = start_pos + self.replacement.len + (content.len - after_end);
        var result = try allocator.alloc(u8, new_len);
        @memcpy(result[0..start_pos], content[0..start_pos]);
        @memcpy(result[start_pos..][0..self.replacement.len], self.replacement);
        @memcpy(result[start_pos + self.replacement.len ..], content[after_end..]);
        return result;
    }
};

/// Collapse runs of consecutive blank lines beyond a threshold.
pub const CollapseBlankLines = struct {
    max_consecutive: usize = 3,

    pub fn apply(self: CollapseBlankLines, allocator: Allocator, content: []const u8) ![]u8 {
        var result = std.ArrayList(u8).init(allocator);
        errdefer result.deinit();
        var newline_count: usize = 0;

        for (content) |c| {
            if (c == '\n') {
                newline_count += 1;
                if (newline_count <= self.max_consecutive) {
                    try result.append(c);
                }
            } else {
                newline_count = 0;
                try result.append(c);
            }
        }

        return result.toOwnedSlice();
    }
};

// ============ Helpers ============

/// Replace all occurrences of `needle` with `replacement` in `haystack`.
fn replaceAll(allocator: Allocator, haystack: []const u8, needle: []const u8, replacement: []const u8) ![]u8 {
    if (needle.len == 0) {
        return try allocator.dupe(u8, haystack);
    }

    // Count occurrences first for exact allocation
    var count: usize = 0;
    var pos: usize = 0;
    while (mem.indexOfPos(u8, haystack, pos, needle)) |idx| {
        count += 1;
        pos = idx + needle.len;
    }

    if (count == 0) {
        return try allocator.dupe(u8, haystack);
    }

    const new_len = haystack.len - (count * needle.len) + (count * replacement.len);
    var result = try allocator.alloc(u8, new_len);
    var write_pos: usize = 0;
    pos = 0;

    while (mem.indexOfPos(u8, haystack, pos, needle)) |idx| {
        const chunk_len = idx - pos;
        @memcpy(result[write_pos .. write_pos + chunk_len], haystack[pos..idx]);
        write_pos += chunk_len;
        @memcpy(result[write_pos .. write_pos + replacement.len], replacement);
        write_pos += replacement.len;
        pos = idx + needle.len;
    }

    // Copy remaining
    const remaining = haystack.len - pos;
    @memcpy(result[write_pos .. write_pos + remaining], haystack[pos..]);

    return result;
}

/// Replace the first `max_count` occurrences.
fn replaceN(allocator: Allocator, haystack: []const u8, needle: []const u8, replacement: []const u8, max_count: usize) ![]u8 {
    if (needle.len == 0 or max_count == 0) {
        return try allocator.dupe(u8, haystack);
    }

    var result = std.ArrayList(u8).init(allocator);
    errdefer result.deinit();

    var pos: usize = 0;
    var replacements_done: usize = 0;

    while (replacements_done < max_count) {
        const idx = mem.indexOfPos(u8, haystack, pos, needle) orelse break;
        try result.appendSlice(haystack[pos..idx]);
        try result.appendSlice(replacement);
        pos = idx + needle.len;
        replacements_done += 1;
    }

    try result.appendSlice(haystack[pos..]);
    return result.toOwnedSlice();
}

/// Check if a line contains a function call pattern: optional whitespace, name, optional whitespace, (...), optional whitespace, ;
fn lineContainsFunctionCall(line: []const u8, function_name: []const u8) bool {
    // Skip leading whitespace
    var i: usize = 0;
    while (i < line.len and (line[i] == ' ' or line[i] == '\t')) {
        i += 1;
    }

    // Check if the function name matches at current position
    if (i + function_name.len > line.len) return false;
    if (!mem.eql(u8, line[i .. i + function_name.len], function_name)) return false;
    i += function_name.len;

    // Skip whitespace before '('
    while (i < line.len and (line[i] == ' ' or line[i] == '\t' or line[i] == '\n')) {
        i += 1;
    }

    // Expect '('
    if (i >= line.len or line[i] != '(') return false;

    // Find matching ')'
    var depth: usize = 1;
    i += 1;
    while (i < line.len and depth > 0) {
        if (line[i] == '(') depth += 1;
        if (line[i] == ')') depth -= 1;
        i += 1;
    }
    if (depth != 0) return false;

    // Skip whitespace before ';'
    while (i < line.len and (line[i] == ' ' or line[i] == '\t')) {
        i += 1;
    }

    // Expect ';'
    if (i >= line.len or line[i] != ';') return false;

    return true;
}

// ============ Tests ============

const testing = std.testing;

test "ReplaceText: replace all occurrences" {
    const t = ReplaceText{ .old = "foo", .new = "bar" };
    const result = try t.apply(testing.allocator, "foo baz foo");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("bar baz bar", result);
}

test "ReplaceText: no match returns copy" {
    const t = ReplaceText{ .old = "xyz", .new = "abc" };
    const result = try t.apply(testing.allocator, "hello world");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello world", result);
}

test "ReplaceText: count limits replacements" {
    const t = ReplaceText{ .old = "a", .new = "b", .count = 2 };
    const result = try t.apply(testing.allocator, "a a a a");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("b b a a", result);
}

test "ReplaceText: replace with longer string" {
    const t = ReplaceText{ .old = "x", .new = "xyz" };
    const result = try t.apply(testing.allocator, "axbxc");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("axyzbxyzc", result);
}

test "ReplaceText: replace with shorter string" {
    const t = ReplaceText{ .old = "abc", .new = "x" };
    const result = try t.apply(testing.allocator, "abcdefabc");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("xdefx", result);
}

test "ReplaceText: replace with empty string (deletion)" {
    const t = ReplaceText{ .old = "remove", .new = "" };
    const result = try t.apply(testing.allocator, "keep remove keep");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("keep  keep", result);
}

test "TruncateAt: basic truncation" {
    const t = TruncateAt{ .marker = "STOP" };
    const result = try t.apply(testing.allocator, "hello STOP world");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello ", result);
}

test "TruncateAt: include marker" {
    const t = TruncateAt{ .marker = "STOP", .include_marker = true };
    const result = try t.apply(testing.allocator, "hello STOP world");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello STOP", result);
}

test "TruncateAt: marker not found" {
    const t = TruncateAt{ .marker = "MISSING" };
    const result = try t.apply(testing.allocator, "hello world");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello world", result);
}

test "SymbolRename: renames identifiers only" {
    const t = SymbolRename{ .old_prefix = "sqlite3", .new_prefix = "my_sqlite3" };
    const result = try t.apply(testing.allocator, "sqlite3Init(); // calls sqlite3Init");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("my_sqlite3Init(); // calls sqlite3Init", result);
}

test "SymbolRenameExact: exact match only" {
    const t = SymbolRenameExact{ .old_name = "sqlite3Init", .new_name = "myInit" };
    const result = try t.apply(testing.allocator, "sqlite3Init(); sqlite3InitAll();");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("myInit(); sqlite3InitAll();", result);
}

test "RemoveFunctionCalls: removes matching lines" {
    const t = RemoveFunctionCalls{ .function_name = "testcase" };
    const input = "int x = 1;\n  testcase(foo);\nint y = 2;\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("int x = 1;\nint y = 2;\n", result);
}

test "RemoveFunctionCalls: preserves non-matching lines" {
    const t = RemoveFunctionCalls{ .function_name = "testcase" };
    const input = "int x = 1;\nint y = 2;\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("int x = 1;\nint y = 2;\n", result);
}

test "RemoveFunctionCalls: nested parens" {
    const t = RemoveFunctionCalls{ .function_name = "testcase" };
    const input = "  testcase(foo(bar));\nkeep;\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("keep;\n", result);
}

test "RemoveSection: removes between markers" {
    const t = RemoveSection{ .start_marker = "START", .end_marker = "END" };
    const input = "before\nSTART stuff END\nafter\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("before\nafter\n", result);
}

test "RemoveSection: markers not found" {
    const t = RemoveSection{ .start_marker = "START", .end_marker = "END" };
    const input = "no markers here\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("no markers here\n", result);
}

test "RemoveSection: multiline section" {
    const t = RemoveSection{ .start_marker = "#ifdef FOO", .end_marker = "#endif" };
    const input = "before\n#ifdef FOO\nstuff\nmore stuff\n#endif /* FOO */\nafter\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("before\nafter\n", result);
}

test "StripBlessingComment: removes blessing" {
    const input = "/*\n** May you share freely, never taking more than you give.\n*/\nint x;";
    const t = StripBlessingComment{};
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("int x;", result);
}

test "StripBlessingComment: no blessing returns unchanged" {
    const t = StripBlessingComment{};
    const result = try t.apply(testing.allocator, "int x;");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("int x;", result);
}

test "InsertBefore: inserts before marker" {
    const t = InsertBefore{ .marker = "world", .text = "hello " };
    const result = try t.apply(testing.allocator, "world!");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello world!", result);
}

test "InsertBefore: marker not found" {
    const t = InsertBefore{ .marker = "missing", .text = "prefix" };
    const result = try t.apply(testing.allocator, "hello");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello", result);
}

test "InsertAfter: inserts after marker" {
    const t = InsertAfter{ .marker = "hello", .text = " world" };
    const result = try t.apply(testing.allocator, "hello!");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello world!", result);
}

test "InsertAfter: marker not found" {
    const t = InsertAfter{ .marker = "missing", .text = "suffix" };
    const result = try t.apply(testing.allocator, "hello");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello", result);
}

test "Pipeline: empty pipeline returns copy" {
    const pipeline = Pipeline{ .transforms = &.{} };
    const result = try pipeline.apply(testing.allocator, "hello");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello", result);
}

test "Pipeline: single transform" {
    const transforms = [_]Transform{
        .{ .replace_text = .{ .old = "a", .new = "b" } },
    };
    const pipeline = Pipeline{ .transforms = &transforms };
    const result = try pipeline.apply(testing.allocator, "aaa");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("bbb", result);
}

test "Pipeline: multiple transforms compose" {
    const transforms = [_]Transform{
        .{ .replace_text = .{ .old = "hello", .new = "goodbye" } },
        .{ .truncate_at = .{ .marker = "!" } },
        .{ .insert_after = .{ .marker = "goodbye", .text = " cruel" } },
    };
    const pipeline = Pipeline{ .transforms = &transforms };
    const result = try pipeline.apply(testing.allocator, "hello world! extra");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("goodbye cruel world", result);
}

test "Transform union dispatch: all variants" {
    // Verify each variant dispatches correctly through the union
    const t1 = Transform{ .replace_text = .{ .old = "x", .new = "y" } };
    const r1 = try t1.apply(testing.allocator, "x");
    defer testing.allocator.free(r1);
    try testing.expectEqualStrings("y", r1);

    const t2 = Transform{ .truncate_at = .{ .marker = "!" } };
    const r2 = try t2.apply(testing.allocator, "hello! world");
    defer testing.allocator.free(r2);
    try testing.expectEqualStrings("hello", r2);

    const t3 = Transform{ .strip_blessing_comment = .{} };
    const r3 = try t3.apply(testing.allocator, "no blessing here");
    defer testing.allocator.free(r3);
    try testing.expectEqualStrings("no blessing here", r3);
}

test "replaceAll helper: empty needle" {
    const result = try replaceAll(testing.allocator, "hello", "", "x");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("hello", result);
}

test "replaceAll helper: adjacent matches" {
    const result = try replaceAll(testing.allocator, "aaa", "a", "bb");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("bbbbbb", result);
}

test "lineContainsFunctionCall: simple" {
    try testing.expect(lineContainsFunctionCall("  foo(bar);", "foo"));
    try testing.expect(lineContainsFunctionCall("foo(bar);", "foo"));
    try testing.expect(!lineContainsFunctionCall("  foobar(x);", "foo"));
    try testing.expect(!lineContainsFunctionCall("int foo = 1;", "foo"));
}

test "RemoveLineContaining: removes matching lines" {
    const t = RemoveLineContaining{ .needle = "define charMap" };
    const input = "int x;\n# define charMap(X) sqlite3UpperToLower[(unsigned char)X]\nint y;\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("int x;\nint y;\n", result);
}

test "RemoveLineContaining: no match preserves all" {
    const t = RemoveLineContaining{ .needle = "MISSING" };
    const input = "line1\nline2\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("line1\nline2\n", result);
}

test "RemoveLineContaining: multiple matching lines" {
    const t = RemoveLineContaining{ .needle = "charMap" };
    const input = "a\n# define charMap(X) foo\n# define charMap(X) bar\nb\n";
    const result = try t.apply(testing.allocator, input);
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("a\nb\n", result);
}

test "ReplaceSection: replaces between markers" {
    const t = ReplaceSection{ .start_marker = "<start>", .end_marker = "<end>", .replacement = "NEW" };
    const result = try t.apply(testing.allocator, "before <start> middle <end> after");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("before NEW after", result);
}

test "ReplaceSection: markers not found" {
    const t = ReplaceSection{ .start_marker = "<start>", .end_marker = "<end>", .replacement = "NEW" };
    const result = try t.apply(testing.allocator, "no markers here");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("no markers here", result);
}

test "CollapseBlankLines: collapses excess newlines" {
    const t = CollapseBlankLines{};
    const result = try t.apply(testing.allocator, "a\n\n\n\n\nb");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("a\n\n\nb", result);
}

test "CollapseBlankLines: no collapse needed" {
    const t = CollapseBlankLines{};
    const result = try t.apply(testing.allocator, "a\n\nb");
    defer testing.allocator.free(result);
    try testing.expectEqualStrings("a\n\nb", result);
}
