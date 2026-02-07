// File generation utilities for SQLite code extraction.
//
// Zig port of python/syntaqlite/sqlite_extractor/generators.py.
// Handles copyright headers, include guards, and file generation boilerplate.

const std = @import("std");
const mem = std.mem;
const Allocator = mem.Allocator;

/// SQLite's blessing comment for generated files.
pub const sqlite_blessing =
    \\**
    \\** The author disclaims copyright to this source code.  In place of
    \\** a legal notice, here is a blessing:
    \\**
    \\**    May you do good and not evil.
    \\**    May you find forgiveness for yourself and forgive others.
    \\**    May you share freely, never taking more than you give.
    \\**
    \\
;

/// Configuration for generating a C file with optional include guard.
/// For headers, provide a guard name. For source files, leave guard as null.
pub const FileGenerator = struct {
    guard: ?[]const u8 = null,
    description: []const u8 = "",
    regenerate_cmd: []const u8 = "",
    includes: []const []const u8 = &.{},
    use_blessing: bool = true,

    /// Generate a complete file with optional guards.
    /// Returns allocated string. Caller owns the returned slice.
    pub fn generate(self: FileGenerator, allocator: Allocator, content: []const u8) ![]u8 {
        var result = std.ArrayList(u8).init(allocator);
        errdefer result.deinit();

        try result.appendSlice("/*\n");
        if (self.use_blessing) {
            try result.appendSlice(sqlite_blessing);
        }
        if (self.description.len > 0) {
            try result.appendSlice("** ");
            try result.appendSlice(self.description);
            try result.append('\n');
        }
        if (self.regenerate_cmd.len > 0) {
            try result.appendSlice("** DO NOT EDIT - regenerate with: ");
            try result.appendSlice(self.regenerate_cmd);
            try result.append('\n');
        }
        try result.appendSlice("*/\n");

        if (self.guard) |guard| {
            try result.appendSlice("#ifndef ");
            try result.appendSlice(guard);
            try result.appendSlice("\n#define ");
            try result.appendSlice(guard);
            try result.appendSlice("\n\n");
        } else {
            try result.append('\n');
        }

        for (self.includes) |inc| {
            try result.appendSlice("#include ");
            if (inc.len > 0 and inc[0] == '<') {
                try result.appendSlice(inc);
            } else {
                try result.append('"');
                try result.appendSlice(inc);
                try result.append('"');
            }
            try result.append('\n');
        }
        if (self.includes.len > 0) {
            try result.append('\n');
        }

        // Trim leading/trailing whitespace from content
        const trimmed = mem.trim(u8, content, " \t\n\r");
        if (trimmed.len > 0) {
            try result.appendSlice(trimmed);
            try result.append('\n');
        }

        if (self.guard) |guard| {
            try result.appendSlice("\n#endif /* ");
            try result.appendSlice(guard);
            try result.appendSlice(" */\n");
        }

        return result.toOwnedSlice();
    }

};

/// Create a FileGenerator configured for header files.
pub fn headerGenerator(config: struct {
    guard: []const u8,
    description: []const u8 = "",
    regenerate_cmd: []const u8 = "",
    includes: []const []const u8 = &.{},
    use_blessing: bool = true,
}) FileGenerator {
    return .{
        .guard = config.guard,
        .description = config.description,
        .regenerate_cmd = config.regenerate_cmd,
        .includes = config.includes,
        .use_blessing = config.use_blessing,
    };
}

/// Create a FileGenerator configured for source files.
pub fn sourceFileGenerator(config: struct {
    description: []const u8 = "",
    regenerate_cmd: []const u8 = "",
    use_blessing: bool = true,
}) FileGenerator {
    return .{
        .guard = null,
        .description = config.description,
        .regenerate_cmd = config.regenerate_cmd,
        .use_blessing = config.use_blessing,
    };
}

/// Extract SYNTAQLITE_TOKEN_* define lines from lemon-generated parse.h content.
pub fn extractTokenDefines(allocator: Allocator, parse_h_content: []const u8) ![]const []const u8 {
    var defines = std.ArrayList([]const u8).init(allocator);
    errdefer defines.deinit();

    const prefix = "#define SYNTAQLITE_TOKEN_";
    var pos: usize = 0;

    while (pos < parse_h_content.len) {
        // Find start of next line
        const line_start = pos;
        const line_end = mem.indexOfScalarPos(u8, parse_h_content, pos, '\n') orelse parse_h_content.len;

        const line = parse_h_content[line_start..line_end];
        if (mem.startsWith(u8, line, prefix)) {
            try defines.append(try allocator.dupe(u8, line));
        }

        pos = if (line_end < parse_h_content.len) line_end + 1 else parse_h_content.len;
    }

    return defines.toOwnedSlice();
}

// ============ Tests ============

const testing = std.testing;

test "FileGenerator: source file with blessing" {
    const gen = sourceFileGenerator(.{
        .description = "Test file",
    });
    const result = try gen.generate(testing.allocator, "int x = 1;");
    defer testing.allocator.free(result);

    try testing.expect(mem.startsWith(u8, result, "/*\n**\n"));
    try testing.expect(mem.indexOf(u8, result, "May you share freely") != null);
    try testing.expect(mem.indexOf(u8, result, "** Test file\n") != null);
    try testing.expect(mem.indexOf(u8, result, "int x = 1;\n") != null);
    // No include guard
    try testing.expect(mem.indexOf(u8, result, "#ifndef") == null);
}

test "FileGenerator: header file with guard" {
    const gen = headerGenerator(.{
        .guard = "MY_HEADER_H",
        .description = "My header",
    });
    const result = try gen.generate(testing.allocator, "void foo(void);");
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "#ifndef MY_HEADER_H\n#define MY_HEADER_H\n") != null);
    try testing.expect(mem.indexOf(u8, result, "void foo(void);\n") != null);
    try testing.expect(mem.indexOf(u8, result, "\n#endif /* MY_HEADER_H */\n") != null);
}

test "FileGenerator: with includes" {
    const includes = [_][]const u8{ "<stdint.h>", "src/foo.h" };
    const gen = headerGenerator(.{
        .guard = "TEST_H",
        .includes = &includes,
    });
    const result = try gen.generate(testing.allocator, "");
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "#include <stdint.h>\n") != null);
    try testing.expect(mem.indexOf(u8, result, "#include \"src/foo.h\"\n") != null);
}

test "FileGenerator: no blessing" {
    const gen = sourceFileGenerator(.{
        .use_blessing = false,
        .description = "No blessing",
    });
    const result = try gen.generate(testing.allocator, "int x;");
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "May you share freely") == null);
    try testing.expect(mem.indexOf(u8, result, "** No blessing\n") != null);
}

test "FileGenerator: regenerate command" {
    const gen = sourceFileGenerator(.{
        .regenerate_cmd = "python3 extract.py",
    });
    const result = try gen.generate(testing.allocator, "int x;");
    defer testing.allocator.free(result);

    try testing.expect(mem.indexOf(u8, result, "** DO NOT EDIT - regenerate with: python3 extract.py\n") != null);
}

test "FileGenerator: empty content" {
    const gen = sourceFileGenerator(.{});
    const result = try gen.generate(testing.allocator, "  \n  ");
    defer testing.allocator.free(result);

    // Should not contain any content between header and end
    try testing.expect(mem.indexOf(u8, result, "*/\n\n") != null);
}

test "headerGenerator convenience" {
    const gen = headerGenerator(.{
        .guard = "FOO_H",
    });
    try testing.expect(gen.guard != null);
    try testing.expectEqualStrings("FOO_H", gen.guard.?);
}

test "sourceFileGenerator convenience" {
    const gen = sourceFileGenerator(.{});
    try testing.expect(gen.guard == null);
    try testing.expect(gen.use_blessing);
}

test "extractTokenDefines: extracts matching lines" {
    const content =
        \\#define SYNTAQLITE_TOKEN_ABORT 1
        \\#define SYNTAQLITE_TOKEN_SELECT 2
        \\#define SOMETHING_ELSE 3
        \\#define SYNTAQLITE_TOKEN_WHERE 4
        \\
    ;

    const defines = try extractTokenDefines(testing.allocator, content);
    defer {
        for (defines) |d| testing.allocator.free(d);
        testing.allocator.free(defines);
    }

    try testing.expectEqual(@as(usize, 3), defines.len);
    try testing.expectEqualStrings("#define SYNTAQLITE_TOKEN_ABORT 1", defines[0]);
    try testing.expectEqualStrings("#define SYNTAQLITE_TOKEN_SELECT 2", defines[1]);
    try testing.expectEqualStrings("#define SYNTAQLITE_TOKEN_WHERE 4", defines[2]);
}

test "extractTokenDefines: empty input" {
    const defines = try extractTokenDefines(testing.allocator, "");
    defer testing.allocator.free(defines);
    try testing.expectEqual(@as(usize, 0), defines.len);
}
