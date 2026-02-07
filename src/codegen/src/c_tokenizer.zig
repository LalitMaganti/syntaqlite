// C tokenizer for safe symbol renaming.
//
// Zig port of python/syntaqlite/sqlite_extractor/c_tokenizer.py.
// Provides a basic C tokenizer that distinguishes between identifiers,
// strings, and comments, enabling safe symbol renaming that won't
// accidentally modify text in comments or string literals.

const std = @import("std");
const mem = std.mem;

pub const TokenType = enum {
    identifier,
    string,
    char,
    line_comment,
    block_comment,
    whitespace,
    other,
};

pub const Token = struct {
    type: TokenType,
    value: []const u8,
    start: usize,
    end: usize,
};

/// Iterator that yields C tokens from source code.
pub const Tokenizer = struct {
    content: []const u8,
    pos: usize,

    pub fn init(content: []const u8) Tokenizer {
        return .{ .content = content, .pos = 0 };
    }

    pub fn next(self: *Tokenizer) ?Token {
        if (self.pos >= self.content.len) return null;

        const start = self.pos;
        const ch = self.content[self.pos];

        // Whitespace
        if (ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r') {
            self.pos += 1;
            while (self.pos < self.content.len) {
                const c = self.content[self.pos];
                if (c != ' ' and c != '\t' and c != '\n' and c != '\r') break;
                self.pos += 1;
            }
            return .{ .type = .whitespace, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // Line comment: //
        if (self.pos + 1 < self.content.len and ch == '/' and self.content[self.pos + 1] == '/') {
            self.pos += 2;
            while (self.pos < self.content.len and self.content[self.pos] != '\n') {
                self.pos += 1;
            }
            return .{ .type = .line_comment, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // Block comment: /* ... */
        if (self.pos + 1 < self.content.len and ch == '/' and self.content[self.pos + 1] == '*') {
            self.pos += 2;
            while (self.pos + 1 < self.content.len) {
                if (self.content[self.pos] == '*' and self.content[self.pos + 1] == '/') {
                    self.pos += 2;
                    break;
                }
                self.pos += 1;
            } else {
                // Unterminated block comment: consume rest
                self.pos = self.content.len;
            }
            return .{ .type = .block_comment, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // String literal
        if (ch == '"') {
            self.pos = self.scanQuoted('"');
            return .{ .type = .string, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // Char literal
        if (ch == '\'') {
            self.pos = self.scanQuoted('\'');
            return .{ .type = .char, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // Identifier
        if (isIdentStart(ch)) {
            self.pos += 1;
            while (self.pos < self.content.len and isIdentCont(self.content[self.pos])) {
                self.pos += 1;
            }
            return .{ .type = .identifier, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // Number
        if (isDigit(ch)) {
            if (ch == '0' and self.pos + 1 < self.content.len and
                (self.content[self.pos + 1] == 'x' or self.content[self.pos + 1] == 'X'))
            {
                self.pos += 2;
                while (self.pos < self.content.len and isHexDigit(self.content[self.pos])) {
                    self.pos += 1;
                }
            } else {
                self.pos += 1;
                while (self.pos < self.content.len and isNumberCont(self.content, self.pos)) {
                    self.pos += 1;
                }
            }
            return .{ .type = .other, .value = self.content[start..self.pos], .start = start, .end = self.pos };
        }

        // Everything else (single character)
        self.pos += 1;
        return .{ .type = .other, .value = self.content[start..self.pos], .start = start, .end = self.pos };
    }

    fn scanQuoted(self: *Tokenizer, quote: u8) usize {
        var pos = self.pos + 1;
        while (pos < self.content.len) {
            if (self.content[pos] == '\\') {
                pos += 2;
            } else if (self.content[pos] == quote) {
                return pos + 1;
            } else if (self.content[pos] == '\n') {
                return pos;
            } else {
                pos += 1;
            }
        }
        return pos;
    }
};

fn isIdentStart(c: u8) bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
}

fn isIdentCont(c: u8) bool {
    return isIdentStart(c) or isDigit(c);
}

fn isDigit(c: u8) bool {
    return c >= '0' and c <= '9';
}

fn isHexDigit(c: u8) bool {
    return isDigit(c) or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F');
}

fn isNumberCont(content: []const u8, pos: usize) bool {
    const c = content[pos];
    if (isDigit(c) or c == '.' or c == 'e' or c == 'E' or
        c == 'f' or c == 'F' or c == 'l' or c == 'L' or
        c == 'u' or c == 'U')
    {
        return true;
    }
    // +/- only after e/E
    if ((c == '+' or c == '-') and pos > 0 and (content[pos - 1] == 'e' or content[pos - 1] == 'E')) {
        return true;
    }
    return false;
}

pub const MatchMode = enum { prefix, prefix_all, exact };

/// Rename identifiers in C source, leaving strings and comments untouched.
///   - .prefix: replace `old` when it appears as a prefix of an identifier
///   - .prefix_all: replace every occurrence of `old` within each identifier
///   - .exact: replace only identifiers that match `old` exactly
pub fn renameSymbols(
    allocator: mem.Allocator,
    content: []const u8,
    old: []const u8,
    new: []const u8,
    mode: MatchMode,
) ![]u8 {
    var result = std.ArrayList(u8).init(allocator);
    errdefer result.deinit();

    var tokenizer = Tokenizer.init(content);
    while (tokenizer.next()) |tok| {
        if (tok.type == .identifier) {
            switch (mode) {
                .prefix => {
                    if (mem.startsWith(u8, tok.value, old)) {
                        try result.appendSlice(new);
                        try result.appendSlice(tok.value[old.len..]);
                    } else {
                        try result.appendSlice(tok.value);
                    }
                },
                .prefix_all => {
                    var remaining = tok.value;
                    while (mem.indexOf(u8, remaining, old)) |idx| {
                        try result.appendSlice(remaining[0..idx]);
                        try result.appendSlice(new);
                        remaining = remaining[idx + old.len ..];
                    }
                    try result.appendSlice(remaining);
                },
                .exact => {
                    if (mem.eql(u8, tok.value, old)) {
                        try result.appendSlice(new);
                    } else {
                        try result.appendSlice(tok.value);
                    }
                },
            }
        } else {
            try result.appendSlice(tok.value);
        }
    }

    return result.toOwnedSlice();
}

/// Rename symbols that start with `old_prefix`, replacing the prefix with `new_prefix`.
/// Only renames identifier tokens; strings and comments are left untouched.
/// When `whole_word` is true (default), only matches identifiers that start with `old_prefix`.
/// When false, replaces all occurrences of `old_prefix` within each identifier.
pub fn renameSymbolsSafe(
    allocator: mem.Allocator,
    content: []const u8,
    old_prefix: []const u8,
    new_prefix: []const u8,
    whole_word: bool,
) ![]u8 {
    return renameSymbols(allocator, content, old_prefix, new_prefix, if (whole_word) .prefix else .prefix_all);
}

/// Rename an exact symbol (identifier must match exactly).
/// Only renames identifier tokens; strings and comments are left untouched.
pub fn renameSymbolExact(
    allocator: mem.Allocator,
    content: []const u8,
    old_name: []const u8,
    new_name: []const u8,
) ![]u8 {
    return renameSymbols(allocator, content, old_name, new_name, .exact);
}

// =============================================================================
// Tests
// =============================================================================

const testing = std.testing;

fn collectTokens(content: []const u8) !std.ArrayList(Token) {
    var tokens = std.ArrayList(Token).init(testing.allocator);
    var tokenizer = Tokenizer.init(content);
    while (tokenizer.next()) |tok| {
        try tokens.append(tok);
    }
    return tokens;
}

test "tokenize simple C snippet" {
    const code = "int main() { return 0; }";
    var tokens = try collectTokens(code);
    defer tokens.deinit();

    // Expected: int WS main ( ) WS { WS return WS 0 ; WS }
    try testing.expect(tokens.items.len > 0);

    // First token should be identifier "int"
    try testing.expectEqual(TokenType.identifier, tokens.items[0].type);
    try testing.expectEqualStrings("int", tokens.items[0].value);

    // "main" is an identifier
    try testing.expectEqual(TokenType.identifier, tokens.items[2].type);
    try testing.expectEqualStrings("main", tokens.items[2].value);

    // "return" is an identifier (index 8: int WS main ( ) WS { WS return ...)
    try testing.expectEqual(TokenType.identifier, tokens.items[8].type);
    try testing.expectEqualStrings("return", tokens.items[8].value);
}

test "tokenize string literals are preserved" {
    const code =
        \\printf("sqlite3_open");
    ;
    var tokens = try collectTokens(code);
    defer tokens.deinit();

    // "printf" = identifier, "(" = other, "\"sqlite3_open\"" = string, ")" = other, ";" = other
    try testing.expectEqual(TokenType.identifier, tokens.items[0].type);
    try testing.expectEqualStrings("printf", tokens.items[0].value);

    try testing.expectEqual(TokenType.string, tokens.items[2].type);
    try testing.expectEqualStrings("\"sqlite3_open\"", tokens.items[2].value);
}

test "tokenize comments" {
    const code = "x // line comment\ny /* block */ z";
    var tokens = try collectTokens(code);
    defer tokens.deinit();

    // Find the line comment
    var found_line = false;
    var found_block = false;
    for (tokens.items) |tok| {
        if (tok.type == .line_comment) {
            try testing.expectEqualStrings("// line comment", tok.value);
            found_line = true;
        }
        if (tok.type == .block_comment) {
            try testing.expectEqualStrings("/* block */", tok.value);
            found_block = true;
        }
    }
    try testing.expect(found_line);
    try testing.expect(found_block);
}

test "tokenize char literal" {
    const code = "'a'";
    var tokens = try collectTokens(code);
    defer tokens.deinit();

    try testing.expectEqual(1, tokens.items.len);
    try testing.expectEqual(TokenType.char, tokens.items[0].type);
    try testing.expectEqualStrings("'a'", tokens.items[0].value);
}

test "tokenize hex number" {
    const code = "0xFF00";
    var tokens = try collectTokens(code);
    defer tokens.deinit();

    try testing.expectEqual(1, tokens.items.len);
    try testing.expectEqual(TokenType.other, tokens.items[0].type);
    try testing.expectEqualStrings("0xFF00", tokens.items[0].value);
}

test "tokenize escaped string" {
    const code =
        \\"hello \"world\""
    ;
    var tokens = try collectTokens(code);
    defer tokens.deinit();

    try testing.expectEqual(1, tokens.items.len);
    try testing.expectEqual(TokenType.string, tokens.items[0].type);
    try testing.expectEqualStrings("\"hello \\\"world\\\"\"", tokens.items[0].value);
}

test "renameSymbolsSafe: whole word prefix rename" {
    const code = "sqlite3Init(); // calls sqlite3Init";
    const result = try renameSymbolsSafe(testing.allocator, code, "sqlite3", "my_sqlite3", true);
    defer testing.allocator.free(result);

    // Identifier sqlite3Init is renamed, comment sqlite3Init is NOT
    try testing.expectEqualStrings("my_sqlite3Init(); // calls sqlite3Init", result);
}

test "renameSymbolsSafe: no rename inside strings" {
    const code =
        \\printf("sqlite3_open"); sqlite3_close();
    ;
    const result = try renameSymbolsSafe(testing.allocator, code, "sqlite3", "synq_sqlite3", true);
    defer testing.allocator.free(result);

    try testing.expectEqualStrings(
        \\printf("sqlite3_open"); synq_sqlite3_close();
    , result);
}

test "renameSymbolExact: exact match only" {
    const code = "sqlite3Init(); sqlite3InitAll();";
    const result = try renameSymbolExact(testing.allocator, code, "sqlite3Init", "myInit");
    defer testing.allocator.free(result);

    // Only exact match "sqlite3Init" is renamed, not "sqlite3InitAll"
    try testing.expectEqualStrings("myInit(); sqlite3InitAll();", result);
}

test "renameSymbolExact: no rename inside comments" {
    const code = "foo /* foo */ foo";
    const result = try renameSymbolExact(testing.allocator, code, "foo", "bar");
    defer testing.allocator.free(result);

    try testing.expectEqualStrings("bar /* foo */ bar", result);
}

test "roundtrip: all tokens reconstruct original" {
    const code =
        \\#include <stdio.h>
        \\
        \\int main(void) {
        \\    /* comment */
        \\    printf("hello %d\n", 42);
        \\    return 0; // done
        \\}
    ;
    var tokenizer = Tokenizer.init(code);
    var reconstructed = std.ArrayList(u8).init(testing.allocator);
    defer reconstructed.deinit();

    while (tokenizer.next()) |tok| {
        try reconstructed.appendSlice(tok.value);
    }

    try testing.expectEqualStrings(code, reconstructed.items);
}
