// Grammar building pipeline for syntaqlite.
//
// Zig port of python/syntaqlite/sqlite_extractor/grammar_build.py.
// Extracts and reconstructs a clean grammar file from SQLite's parse.y
// that can be processed by Lemon with custom templates.

const std = @import("std");
const mem = std.mem;
const Allocator = std.mem.Allocator;

// ============ Parsed types ============

pub const LemonGrammarOutput = struct {
    terminals: std.ArrayList([]const u8),
    nonterminals: std.ArrayList([]const u8),
    rules: std.ArrayList([]const u8),

    pub fn init(allocator: Allocator) LemonGrammarOutput {
        return .{
            .terminals = std.ArrayList([]const u8).init(allocator),
            .nonterminals = std.ArrayList([]const u8).init(allocator),
            .rules = std.ArrayList([]const u8).init(allocator),
        };
    }

    pub fn deinit(self: *LemonGrammarOutput) void {
        self.terminals.deinit();
        self.nonterminals.deinit();
        self.rules.deinit();
    }
};

pub const FallbackResult = struct {
    fallback_id: []const u8,
    tokens: std.ArrayList([]const u8),

    pub fn deinit(self: *FallbackResult) void {
        self.tokens.deinit();
    }
};

// ============ Action rule parsing ============

/// Parse grammar content with action code and extract rules with their full text.
/// Returns a map from rule signatures (without type annotations) to full rule text
/// (including type annotations and action code).
pub fn parseActionsContent(allocator: Allocator, content: []const u8) !std.StringHashMap([]const u8) {
    var rules = std.StringHashMap([]const u8).init(allocator);

    var i: usize = 0;
    while (i < content.len) {
        // Skip line comments
        if (i + 1 < content.len and content[i] == '/' and content[i + 1] == '/') {
            while (i < content.len and content[i] != '\n') : (i += 1) {}
            if (i < content.len) i += 1;
            continue;
        }

        // Look for ::=
        const rule_start = mem.indexOfPos(u8, content, i, "::=") orelse break;

        // Find the LHS (before ::=)
        const lhs_start = if (mem.lastIndexOf(u8, content[0..rule_start], "\n")) |pos| pos + 1 else 0;
        const lhs = mem.trim(u8, content[lhs_start..rule_start], " \t\r\n");

        // Find the action block { ... }
        const action_start = mem.indexOfPos(u8, content, rule_start, "{") orelse {
            i = rule_start + 3;
            continue;
        };

        // Find the RHS (between ::= and {), excluding the . and precedence marker
        var rhs_text = mem.trim(u8, content[rule_start + 3 .. action_start], " \t\r\n");

        // Extract and preserve precedence marker like [IN], [BITNOT]
        var prec_marker: []const u8 = "";
        var prec_buf: [64]u8 = undefined;
        if (findPrecMarker(rhs_text)) |pm| {
            const marker_text = std.fmt.bufPrint(&prec_buf, " [{s}]", .{pm.name}) catch "";
            prec_marker = marker_text;
            rhs_text = mem.trim(u8, rhs_text[0..pm.start], " \t\r\n");
        }
        if (rhs_text.len > 0 and rhs_text[rhs_text.len - 1] == '.') {
            rhs_text = mem.trim(u8, rhs_text[0 .. rhs_text.len - 1], " \t\r\n");
        }

        // Find matching closing brace
        var brace_depth: usize = 1;
        var j: usize = action_start + 1;
        while (j < content.len and brace_depth > 0) : (j += 1) {
            if (content[j] == '{') {
                brace_depth += 1;
            } else if (content[j] == '}') {
                brace_depth -= 1;
            }
        }

        const action_code = mem.trim(u8, content[action_start..j], " \t\r\n");

        // Build full rule text with type annotations and action code
        const full_rule = try std.fmt.allocPrint(allocator, "{s} ::= {s}.{s} {s}", .{
            lhs, rhs_text, prec_marker, action_code,
        });

        // Build signature without type annotations for matching
        const lhs_clean = try stripAnnotations(allocator, lhs);
        defer allocator.free(lhs_clean);
        const rhs_clean = try stripAnnotationsFromTokens(allocator, rhs_text);
        defer allocator.free(rhs_clean);

        var sig_buf = std.ArrayList(u8).init(allocator);
        try sig_buf.appendSlice(lhs_clean);
        try sig_buf.appendSlice(" ::=");
        if (rhs_clean.len > 0) {
            try sig_buf.append(' ');
            try sig_buf.appendSlice(rhs_clean);
        }
        try sig_buf.append('.');
        try sig_buf.appendSlice(prec_marker);
        const signature = try sig_buf.toOwnedSlice();

        try rules.put(signature, full_rule);
        i = j;
    }

    return rules;
}

const PrecMarkerResult = struct {
    name: []const u8,
    start: usize,
};

/// Find a precedence marker like [IN] at the end of text.
fn findPrecMarker(text: []const u8) ?PrecMarkerResult {
    // Scan backwards for ] at end (ignoring trailing whitespace)
    var end = text.len;
    while (end > 0 and (text[end - 1] == ' ' or text[end - 1] == '\t' or text[end - 1] == '\n' or text[end - 1] == '\r')) {
        end -= 1;
    }
    if (end == 0 or text[end - 1] != ']') return null;

    // Find matching [
    var pos = end - 2;
    while (pos > 0) : (pos -= 1) {
        if (text[pos] == '[') {
            const name = text[pos + 1 .. end - 1];
            // Verify it's all word characters (uppercase letters/digits/underscore)
            for (name) |c| {
                if (!std.ascii.isAlphanumeric(c) and c != '_') return null;
            }
            return .{ .name = name, .start = pos };
        }
    }
    if (text[0] == '[') {
        const name = text[1 .. end - 1];
        for (name) |c| {
            if (!std.ascii.isAlphanumeric(c) and c != '_') return null;
        }
        return .{ .name = name, .start = 0 };
    }
    return null;
}

/// Strip type annotations like (A), (B) from a symbol.
/// e.g. "lhs(A)" -> "lhs"
fn stripAnnotations(allocator: Allocator, text: []const u8) ![]const u8 {
    var result = std.ArrayList(u8).init(allocator);
    var i: usize = 0;
    while (i < text.len) {
        if (text[i] == '(' and i > 0) {
            // Check if this is a single uppercase letter annotation
            if (i + 2 < text.len and std.ascii.isUpper(text[i + 1]) and text[i + 2] == ')') {
                i += 3; // skip (X)
                continue;
            }
        }
        try result.append(text[i]);
        i += 1;
    }
    return try result.toOwnedSlice();
}

/// Strip type annotations from all tokens in a space-separated list.
fn stripAnnotationsFromTokens(allocator: Allocator, text: []const u8) ![]const u8 {
    var result = std.ArrayList(u8).init(allocator);
    var iter = mem.tokenizeAny(u8, text, " \t");
    var first = true;
    while (iter.next()) |tok| {
        const stripped = try stripAnnotations(allocator, tok);
        const trimmed = mem.trim(u8, stripped, " \t");
        if (trimmed.len == 0) {
            allocator.free(stripped);
            continue;
        }
        if (!first) try result.append(' ');
        try result.appendSlice(trimmed);
        allocator.free(stripped);
        first = false;
    }
    return try result.toOwnedSlice();
}

/// Load and merge action rules from all .y files in a directory.
pub fn loadAllActions(allocator: Allocator, actions_dir: []const u8) !std.StringHashMap([]const u8) {
    var merged = std.StringHashMap([]const u8).init(allocator);

    var dir = std.fs.openDirAbsolute(actions_dir, .{ .iterate = true }) catch {
        return merged;
    };
    defer dir.close();

    // Collect .y files and sort them
    var filenames = std.ArrayList([]const u8).init(allocator);
    defer filenames.deinit();

    var it = dir.iterate();
    while (try it.next()) |entry| {
        if (entry.kind != .file) continue;
        if (!mem.endsWith(u8, entry.name, ".y")) continue;
        try filenames.append(try allocator.dupe(u8, entry.name));
    }

    mem.sort([]const u8, filenames.items, {}, struct {
        fn lessThan(_: void, a: []const u8, b: []const u8) bool {
            return mem.order(u8, a, b) == .lt;
        }
    }.lessThan);

    for (filenames.items) |filename| {
        const path = try std.fs.path.join(allocator, &.{ actions_dir, filename });
        defer allocator.free(path);

        const file = std.fs.openFileAbsolute(path, .{}) catch continue;
        defer file.close();

        const content = try file.readToEndAlloc(allocator, 1024 * 1024);
        defer allocator.free(content);

        var file_rules = try parseActionsContent(allocator, content);
        defer file_rules.deinit();

        var file_it = file_rules.iterator();
        while (file_it.next()) |entry| {
            if (merged.contains(entry.key_ptr.*)) {
                return error.DuplicateRuleSignature;
            }
            try merged.put(
                try allocator.dupe(u8, entry.key_ptr.*),
                try allocator.dupe(u8, entry.value_ptr.*),
            );
        }
    }

    return merged;
}

// ============ Lemon -g output parsing ============

/// Parse a symbol line from lemon -g output.
/// Lines look like: "//   0 $                      162 INDEX"
/// Each line has pairs of (id, name).
fn parseSymbolLine(line: []const u8) !std.ArrayList([]const u8) {
    const allocator = std.heap.page_allocator;
    var symbols = std.ArrayList([]const u8).init(allocator);

    // Strip leading "//"
    const stripped = if (line.len >= 2 and mem.startsWith(u8, line, "//"))
        line[2..]
    else
        line;

    var parts = mem.tokenizeAny(u8, stripped, " \t");
    while (parts.next()) |part| {
        // Check if it's a digit (the ID)
        if (isAllDigits(part)) {
            // Next token is the name
            if (parts.next()) |name| {
                try symbols.append(name);
            }
        }
    }
    return symbols;
}

fn isAllDigits(s: []const u8) bool {
    if (s.len == 0) return false;
    for (s) |c| {
        if (!std.ascii.isDigit(c)) return false;
    }
    return true;
}

/// Parse the output of lemon -g.
pub fn parseLemonGOutput(allocator: Allocator, output: []const u8) !LemonGrammarOutput {
    var result = LemonGrammarOutput.init(allocator);
    var in_symbols = false;

    var line_iter = mem.splitScalar(u8, mem.trim(u8, output, " \t\r\n"), '\n');
    while (line_iter.next()) |line| {
        if (mem.startsWith(u8, line, "// Symbols:")) {
            in_symbols = true;
        } else if (in_symbols and mem.startsWith(u8, line, "//")) {
            var symbols = try parseSymbolLine(line);
            defer symbols.deinit();
            for (symbols.items) |name| {
                if (mem.eql(u8, name, "$")) continue;
                if (name.len > 0 and std.ascii.isLower(name[0])) {
                    try result.nonterminals.append(try allocator.dupe(u8, name));
                } else {
                    try result.terminals.append(try allocator.dupe(u8, name));
                }
            }
        } else if (mem.indexOf(u8, line, "::=") != null) {
            in_symbols = false;
            try result.rules.append(try allocator.dupe(u8, mem.trim(u8, line, " \t\r\n")));
        } else if (result.rules.items.len > 0) {
            const trimmed = mem.trim(u8, line, " \t\r\n");
            if (trimmed.len > 0 and !mem.startsWith(u8, trimmed, "//")) {
                try result.rules.append(try allocator.dupe(u8, trimmed));
            }
        }
    }

    return result;
}

// ============ Fallback extraction ============

/// Extract fallback declaration from grammar, filtering to valid tokens.
///
/// Handles SQLite conditional directives:
///   %ifdef SQLITE_OMIT_X ... %endif  -> EXCLUDE (we never define OMIT macros)
///   %ifndef SQLITE_OMIT_X ... %endif -> INCLUDE (strip directives only)
pub fn extractFallbackFromGrammar(
    allocator: Allocator,
    grammar_content: []const u8,
    valid_tokens: *const std.StringHashMap(void),
) !FallbackResult {
    var result = FallbackResult{
        .fallback_id = "",
        .tokens = std.ArrayList([]const u8).init(allocator),
    };

    // Find %fallback declaration
    const fallback_start = findFallbackStart(grammar_content) orelse return result;

    // Extract fallback ID (first word after %fallback)
    const after_fallback = mem.trim(u8, grammar_content[fallback_start + "%fallback".len ..], " \t");
    const fallback_id_end = mem.indexOfAny(u8, after_fallback, " \t\n\r") orelse return result;
    result.fallback_id = try allocator.dupe(u8, after_fallback[0..fallback_id_end]);

    // Find the end of the fallback declaration (terminated by '.')
    const decl_start = fallback_start + "%fallback".len + fallback_id_end;
    const dot_pos = mem.indexOfPos(u8, grammar_content, decl_start, ".") orelse return result;

    var tokens_text_buf = std.ArrayList(u8).init(allocator);
    defer tokens_text_buf.deinit();

    // Process the text between the fallback ID and the dot, handling conditional directives.
    // We need to handle:
    //   %ifdef SQLITE_OMIT_X ... %endif SQLITE_OMIT_X -> EXCLUDE
    //   %ifndef SQLITE_OMIT_X ... %endif SQLITE_OMIT_X -> INCLUDE (strip directives)
    const tokens_text = grammar_content[decl_start..dot_pos];
    try processConditionalDirectives(allocator, tokens_text, &tokens_text_buf);

    // Extract token names (uppercase identifiers) and filter to valid ones
    var seen = std.StringHashMap(void).init(allocator);
    defer seen.deinit();

    var tok_iter = mem.tokenizeAny(u8, tokens_text_buf.items, " \t\n\r");
    while (tok_iter.next()) |tok| {
        if (tok.len == 0) continue;
        if (!isUpperIdent(tok)) continue;
        if (!valid_tokens.contains(tok)) continue;
        if (seen.contains(tok)) continue;

        const duped = try allocator.dupe(u8, tok);
        try result.tokens.append(duped);
        try seen.put(duped, {});
    }

    return result;
}

fn findFallbackStart(content: []const u8) ?usize {
    var pos: usize = 0;
    while (pos < content.len) {
        const line_start = pos;
        // Find end of line
        const line_end = mem.indexOfPos(u8, content, pos, "\n") orelse content.len;

        const line = mem.trim(u8, content[line_start..line_end], " \t\r");
        if (mem.startsWith(u8, line, "%fallback")) {
            return line_start + (mem.indexOf(u8, content[line_start..line_end], "%fallback") orelse 0);
        }
        pos = if (line_end < content.len) line_end + 1 else content.len;
    }
    return null;
}

/// Process conditional directives in fallback token text.
/// %ifdef blocks are EXCLUDED (we never define SQLITE_OMIT_* macros).
/// %ifndef blocks are INCLUDED (the condition is true for us).
fn processConditionalDirectives(
    allocator: Allocator,
    text: []const u8,
    output: *std.ArrayList(u8),
) !void {
    _ = allocator;
    var i: usize = 0;
    while (i < text.len) {
        // Look for %ifdef or %ifndef
        if (text[i] == '%') {
            if (mem.startsWith(u8, text[i..], "%ifdef ")) {
                // Skip everything until matching %endif
                const endif_pos = mem.indexOfPos(u8, text, i, "%endif") orelse {
                    i = text.len;
                    continue;
                };
                // Skip past %endif and its argument
                i = endif_pos + "%endif".len;
                while (i < text.len and text[i] != '\n') : (i += 1) {}
                if (i < text.len) i += 1;
                continue;
            } else if (mem.startsWith(u8, text[i..], "%ifndef ")) {
                // Include the content, but skip the directive line itself
                while (i < text.len and text[i] != '\n') : (i += 1) {}
                if (i < text.len) i += 1;
                continue;
            } else if (mem.startsWith(u8, text[i..], "%endif")) {
                // Skip the %endif line
                while (i < text.len and text[i] != '\n') : (i += 1) {}
                if (i < text.len) i += 1;
                continue;
            }
        }
        try output.append(text[i]);
        i += 1;
    }
}

fn isUpperIdent(s: []const u8) bool {
    if (s.len == 0) return false;
    if (!std.ascii.isUpper(s[0])) return false;
    for (s) |c| {
        if (!std.ascii.isUpper(c) and !std.ascii.isDigit(c) and c != '_') return false;
    }
    return true;
}

// ============ Precedence extraction ============

/// Extract %left/%right/%nonassoc precedence rules from grammar.
pub fn extractPrecedenceFromGrammar(allocator: Allocator, grammar_content: []const u8) !std.ArrayList([]const u8) {
    var rules = std.ArrayList([]const u8).init(allocator);

    var line_iter = mem.splitScalar(u8, grammar_content, '\n');
    while (line_iter.next()) |raw_line| {
        const line = mem.trim(u8, raw_line, " \t\r");
        if (line.len == 0) continue;

        var direction: []const u8 = "";
        if (mem.startsWith(u8, line, "%left ")) {
            direction = "left";
        } else if (mem.startsWith(u8, line, "%right ")) {
            direction = "right";
        } else if (mem.startsWith(u8, line, "%nonassoc ")) {
            direction = "nonassoc";
        } else continue;

        // Find the tokens between the directive and the dot
        const dir_prefix_len = 1 + direction.len + 1; // %<direction><space>
        const rest = line[dir_prefix_len..];
        const dot_pos = mem.indexOf(u8, rest, ".") orelse continue;
        const tokens = mem.trim(u8, rest[0..dot_pos], " \t");
        const rule = try std.fmt.allocPrint(allocator, "%{s} {s}.", .{ direction, tokens });
        try rules.append(rule);
    }

    return rules;
}

// ============ Wildcard extraction ============

/// Extract %wildcard directive from grammar.
pub fn extractWildcardFromGrammar(allocator: Allocator, grammar_content: []const u8) !?[]const u8 {
    var line_iter = mem.splitScalar(u8, grammar_content, '\n');
    while (line_iter.next()) |raw_line| {
        const line = mem.trim(u8, raw_line, " \t\r");
        if (mem.startsWith(u8, line, "%wildcard ")) {
            const rest = line["%wildcard ".len..];
            const dot_pos = mem.indexOf(u8, rest, ".") orelse continue;
            return try allocator.dupe(u8, mem.trim(u8, rest[0..dot_pos], " \t"));
        }
    }
    return null;
}

// ============ Extension grammar helpers ============

/// Strip { ... } action blocks from grammar content (brace-aware).
/// Produces bare rules suitable for lemon -g.
pub fn stripActionBlocks(allocator: Allocator, content: []const u8) ![]const u8 {
    var result = std.ArrayList(u8).init(allocator);
    var i: usize = 0;
    while (i < content.len) {
        if (content[i] == '{') {
            var depth: usize = 1;
            i += 1;
            while (i < content.len and depth > 0) : (i += 1) {
                if (content[i] == '{') {
                    depth += 1;
                } else if (content[i] == '}') {
                    depth -= 1;
                }
            }
        } else {
            try result.append(content[i]);
            i += 1;
        }
    }
    return try result.toOwnedSlice();
}

/// Split extension grammar into directives and bare rules.
/// Returns (directives, bare_rules).
pub fn splitExtensionGrammar(
    allocator: Allocator,
    extension_grammar: []const u8,
) !struct { directives: []const u8, bare_rules: []const u8 } {
    const stripped = try stripActionBlocks(allocator, extension_grammar);
    defer allocator.free(stripped);

    var directives_buf = std.ArrayList(u8).init(allocator);
    var rules_buf = std.ArrayList(u8).init(allocator);

    var line_iter = mem.splitScalar(u8, stripped, '\n');
    while (line_iter.next()) |raw_line| {
        const line = mem.trim(u8, raw_line, " \t\r");
        if (line.len == 0) continue;
        if (mem.startsWith(u8, line, "//")) continue;

        if (line[0] == '%' and mem.indexOf(u8, line, "::=") == null) {
            if (directives_buf.items.len > 0) try directives_buf.append('\n');
            try directives_buf.appendSlice(line);
        } else if (mem.indexOf(u8, line, "::=") != null) {
            if (rules_buf.items.len > 0) try rules_buf.append('\n');
            try rules_buf.appendSlice(line);
        }
    }

    return .{
        .directives = try directives_buf.toOwnedSlice(),
        .bare_rules = try rules_buf.toOwnedSlice(),
    };
}

// ============ Grammar file generation ============

pub const GenerateGrammarOptions = struct {
    prefix: []const u8 = "synq",
    terminals: []const []const u8 = &.{},
    rules: []const []const u8 = &.{},
    fallback_id: []const u8 = "",
    fallback_tokens: []const []const u8 = &.{},
    precedence_rules: []const []const u8 = &.{},
    wildcard: ?[]const u8 = null,
    extension_terminals: []const []const u8 = &.{},
    extension_action_rules: ?*std.StringHashMap([]const u8) = null,
    action_rules: ?*std.StringHashMap([]const u8) = null,
};

/// Write %token declarations in batches of 10.
fn writeTokenBatch(writer: anytype, tokens: []const []const u8) !void {
    var i: usize = 0;
    while (i < tokens.len) {
        try writer.writeAll("%token");
        const end = @min(i + 10, tokens.len);
        for (tokens[i..end]) |tok| {
            try writer.print(" {s}", .{tok});
        }
        try writer.writeAll(".\n");
        i = end;
    }
}

/// Generate the grammar file content.
pub fn generateGrammarFile(allocator: Allocator, opts: GenerateGrammarOptions) ![]const u8 {
    var parts = std.ArrayList(u8).init(allocator);
    const writer = parts.writer();

    const sqlite3_prefix_buf = try std.fmt.allocPrint(allocator, "{s}_sqlite3", .{opts.prefix});
    defer allocator.free(sqlite3_prefix_buf);

    var prefix_cap_buf: [64]u8 = undefined;
    const prefix_cap = blk: {
        if (opts.prefix.len == 0) break :blk "";
        @memcpy(prefix_cap_buf[0..opts.prefix.len], opts.prefix);
        prefix_cap_buf[0] = std.ascii.toUpper(prefix_cap_buf[0]);
        break :blk prefix_cap_buf[0..opts.prefix.len];
    };

    // Header comment
    try writer.print(
        \\// Grammar file for {s} parser.
        \\// Generated from SQLite's parse.y with clean rules via lemon -g.
        \\// DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py
        \\
        \\
    , .{opts.prefix});

    // Parser name and token prefix
    try writer.print(
        \\%name {s}Parser
        \\%token_prefix SYNTAQLITE_TOKEN_
        \\%start_symbol input
        \\
        \\
    , .{sqlite3_prefix_buf});

    // Include section
    try writer.writeAll(
        \\%include {
        \\#include "src/common/synq_sqlite_defs.h"
        \\#include "syntaqlite/sqlite_tokens_gen.h"
        \\#include "src/parser/ast_base.h"
        \\#include "src/parser/ast_builder_gen.h"
        \\
        \\#define YYNOERRORRECOVERY 1
        \\#define YYPARSEFREENEVERNULL 1
        \\}
        \\
        \\
    );

    // Token type and extra context
    try writer.print(
        \\%token_type {{SynqToken}}
        \\%default_type {{u32}}
        \\%extra_context {{{s}ParseContext *pCtx}}
        \\
        \\
    , .{prefix_cap});

    // Type directives (hardcoded, not from action files)
    try writer.writeAll(
        \\// Non-terminals that pass through token info (not node IDs)
        \\%type nm {SynqToken}
        \\%type as {SynqToken}
        \\%type scanpt {SynqToken}
        \\%type likeop {SynqToken}
        \\%type dbnm {SynqToken}
        \\%type multiselect_op {int}
        \\%type in_op {int}
        \\%type typetoken {SynqToken}
        \\%type typename {SynqToken}
        \\%type withnm {SynqToken}
        \\%type wqas {int}
        \\%type collate {int}
        \\%type raisetype {int}
        \\%type joinop {int}
        \\%type indexed_by {SynqToken}
        \\
        \\// DML support
        \\%type insert_cmd {int}
        \\%type orconf {int}
        \\%type resolvetype {int}
        \\%type indexed_opt {SynqToken}
        \\
        \\// Schema/Transaction support
        \\%type ifexists {int}
        \\%type transtype {int}
        \\%type trans_opt {int}
        \\%type savepoint_opt {int}
        \\%type kwcolumn_opt {int}
        \\%type columnname {SynqColumnNameValue}
        \\
        \\// Utility/Create support
        \\%type ifnotexists {int}
        \\%type temp {int}
        \\%type uniqueflag {int}
        \\%type database_kw_opt {int}
        \\%type explain {int}
        \\%type createkw {SynqToken}
        \\%type signed {SynqToken}
        \\%type plus_num {SynqToken}
        \\%type minus_num {SynqToken}
        \\%type nmnum {SynqToken}
        \\
        \\// CREATE TABLE support
        \\%type autoinc {int}
        \\%type refargs {int}
        \\%type refarg {int}
        \\%type refact {int}
        \\%type defer_subclause {int}
        \\%type init_deferred_pred_opt {int}
        \\%type defer_subclause_opt {int}
        \\%type table_option_set {int}
        \\%type table_option {int}
        \\%type ccons {SynqConstraintValue}
        \\%type carglist {SynqConstraintListValue}
        \\%type tcons {SynqConstraintValue}
        \\%type conslist {SynqConstraintListValue}
        \\%type conslist_opt {u32}
        \\%type tconscomma {int}
        \\%type onconf {int}
        \\%type scantok {SynqToken}
        \\%type generated {SynqConstraintValue}
        \\
        \\// JOIN ON/USING discriminator
        \\%type on_using {SynqOnUsingValue}
        \\
        \\// WITH clause (recursive flag + cte list)
        \\%type with {SynqWithValue}
        \\
        \\// Window function support
        \\%type range_or_rows {int}
        \\%type frame_exclude_opt {int}
        \\%type frame_exclude {int}
        \\
        \\// Trigger support
        \\%type trigger_time {int}
        \\%type trnm {SynqToken}
        \\
    );

    // Syntax/stack error handlers
    try writer.writeAll(
        \\
        \\%syntax_error {
        \\  (void)yymajor;
        \\  (void)TOKEN;
        \\  if( pCtx && pCtx->onSyntaxError ) {
        \\    pCtx->onSyntaxError(pCtx);
        \\  }
        \\}
        \\
        \\%stack_overflow {
        \\  if( pCtx && pCtx->onStackOverflow ) {
        \\    pCtx->onStackOverflow(pCtx);
        \\  }
        \\}
        \\
        \\
    );

    // Token declarations
    if (opts.terminals.len > 0) {
        try writer.writeAll("// Base token declarations\n");
        try writeTokenBatch(writer, opts.terminals);
        try writer.writeAll("\n");
    }

    // Precedence rules
    if (opts.precedence_rules.len > 0) {
        try writer.writeAll("// Operator precedence\n");
        for (opts.precedence_rules) |rule| {
            try writer.print("{s}\n", .{rule});
        }
        try writer.writeAll("\n");
    }

    // Fallback declaration
    if (opts.fallback_id.len > 0 and opts.fallback_tokens.len > 0) {
        try writer.print("// Fallback tokens to {s}\n", .{opts.fallback_id});
        try writer.print("%fallback {s}\n", .{opts.fallback_id});
        var fi: usize = 0;
        while (fi < opts.fallback_tokens.len) {
            try writer.writeAll("  ");
            const end = @min(fi + 8, opts.fallback_tokens.len);
            for (opts.fallback_tokens[fi..end], 0..) |tok, j| {
                if (j > 0) try writer.writeAll(" ");
                try writer.print("{s}", .{tok});
            }
            try writer.writeAll("\n");
            fi = end;
        }
        try writer.writeAll(".\n\n");
    }

    // Wildcard
    if (opts.wildcard) |wc| {
        try writer.print("%wildcard {s}.\n\n", .{wc});
    }

    // Grammar rules - use action version if available, otherwise bare rule
    try writer.writeAll("// Grammar rules\n");
    for (opts.rules) |rule| {
        var found = false;
        if (opts.action_rules) |ar| {
            if (ar.get(rule)) |action_rule| {
                try writer.print("{s}\n", .{action_rule});
                found = true;
            }
        }
        if (!found) {
            if (opts.extension_action_rules) |ear| {
                if (ear.get(rule)) |ext_rule| {
                    try writer.print("{s}\n", .{ext_rule});
                    found = true;
                }
            }
        }
        if (!found) {
            try writer.print("{s}\n", .{rule});
        }
    }

    // Extension token declarations
    if (opts.extension_terminals.len > 0) {
        try writer.writeAll("\n// Extension token declarations\n");
        try writeTokenBatch(writer, opts.extension_terminals);
    }

    // Tokenizer-only tokens
    try writer.writeAll(
        \\
        \\// Tokenizer-only tokens (not used in grammar rules)
        \\%token SPACE COMMENT ILLEGAL.
        \\
    );

    return try parts.toOwnedSlice();
}

/// Parse %token declarations from extension grammar content.
pub fn parseExtensionKeywords(allocator: Allocator, grammar_content: []const u8) !std.ArrayList([]const u8) {
    var keywords = std.ArrayList([]const u8).init(allocator);
    var seen = std.StringHashMap(void).init(allocator);
    defer seen.deinit();

    var line_iter = mem.splitScalar(u8, grammar_content, '\n');
    while (line_iter.next()) |raw_line| {
        const line = mem.trim(u8, raw_line, " \t\r");
        if (!mem.startsWith(u8, line, "%token ")) continue;

        // Extract keyword names from after %token
        var tok_iter = mem.tokenizeAny(u8, line["%token ".len..], " \t.");
        while (tok_iter.next()) |tok| {
            if (!isUpperIdent(tok)) continue;
            if (seen.contains(tok)) continue;
            const duped = try allocator.dupe(u8, tok);
            try keywords.append(duped);
            try seen.put(duped, {});
        }
    }

    return keywords;
}

// ============ Tests ============

test "parseActionsContent basic rule" {
    const allocator = std.testing.allocator;
    const content =
        \\oneselect(A) ::= values(B). {
        \\    A = synq_ast_values_clause(pCtx->astCtx, B);
        \\}
    ;
    var rules = try parseActionsContent(allocator, content);
    defer {
        var it = rules.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        rules.deinit();
    }

    try std.testing.expectEqual(@as(usize, 1), rules.count());
    try std.testing.expect(rules.contains("oneselect ::= values."));
}

test "parseActionsContent with precedence marker" {
    const allocator = std.testing.allocator;
    const content =
        \\expr(A) ::= expr(B) PLUS expr(C). [IN] {
        \\    A = build(B, C);
        \\}
    ;
    var rules = try parseActionsContent(allocator, content);
    defer {
        var it = rules.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        rules.deinit();
    }

    try std.testing.expectEqual(@as(usize, 1), rules.count());
    // The signature should preserve the precedence marker
    try std.testing.expect(rules.contains("expr ::= expr PLUS expr. [IN]"));
}

test "parseActionsContent empty rhs" {
    const allocator = std.testing.allocator;
    const content =
        \\input(A) ::= . {
        \\    A = 0;
        \\}
    ;
    var rules = try parseActionsContent(allocator, content);
    defer {
        var it = rules.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        rules.deinit();
    }

    try std.testing.expectEqual(@as(usize, 1), rules.count());
    try std.testing.expect(rules.contains("input ::=."));
}

test "extractFallbackFromGrammar basic" {
    const allocator = std.testing.allocator;
    const content =
        \\%fallback ID
        \\  ABORT ACTION AFTER
        \\.
    ;
    var valid = std.StringHashMap(void).init(allocator);
    defer valid.deinit();
    try valid.put("ABORT", {});
    try valid.put("ACTION", {});
    try valid.put("AFTER", {});
    try valid.put("ID", {});

    var fb = try extractFallbackFromGrammar(allocator, content, &valid);
    defer {
        allocator.free(fb.fallback_id);
        for (fb.tokens.items) |tok| allocator.free(tok);
        fb.deinit();
    }

    try std.testing.expectEqualStrings("ID", fb.fallback_id);
    try std.testing.expectEqual(@as(usize, 3), fb.tokens.items.len);
    try std.testing.expectEqualStrings("ABORT", fb.tokens.items[0]);
    try std.testing.expectEqualStrings("ACTION", fb.tokens.items[1]);
    try std.testing.expectEqualStrings("AFTER", fb.tokens.items[2]);
}

test "extractFallbackFromGrammar ifdef OMIT excluded" {
    const allocator = std.testing.allocator;
    const content =
        \\%fallback ID
        \\  ABORT
        \\%ifdef SQLITE_OMIT_COMPOUND_SELECT
        \\  UNION EXCEPT INTERSECT
        \\%endif SQLITE_OMIT_COMPOUND_SELECT
        \\  ACTION
        \\.
    ;
    var valid = std.StringHashMap(void).init(allocator);
    defer valid.deinit();
    try valid.put("ABORT", {});
    try valid.put("UNION", {});
    try valid.put("EXCEPT", {});
    try valid.put("INTERSECT", {});
    try valid.put("ACTION", {});
    try valid.put("ID", {});

    var fb = try extractFallbackFromGrammar(allocator, content, &valid);
    defer {
        allocator.free(fb.fallback_id);
        for (fb.tokens.items) |tok| allocator.free(tok);
        fb.deinit();
    }

    // UNION, EXCEPT, INTERSECT should be EXCLUDED (inside %ifdef SQLITE_OMIT_*)
    try std.testing.expectEqual(@as(usize, 2), fb.tokens.items.len);
    try std.testing.expectEqualStrings("ABORT", fb.tokens.items[0]);
    try std.testing.expectEqualStrings("ACTION", fb.tokens.items[1]);
}

test "extractFallbackFromGrammar ifndef OMIT included" {
    const allocator = std.testing.allocator;
    const content =
        \\%fallback ID
        \\  ABORT
        \\%ifndef SQLITE_OMIT_TRIGGER
        \\  BEFORE AFTER
        \\%endif SQLITE_OMIT_TRIGGER
        \\  ACTION
        \\.
    ;
    var valid = std.StringHashMap(void).init(allocator);
    defer valid.deinit();
    try valid.put("ABORT", {});
    try valid.put("BEFORE", {});
    try valid.put("AFTER", {});
    try valid.put("ACTION", {});
    try valid.put("ID", {});

    var fb = try extractFallbackFromGrammar(allocator, content, &valid);
    defer {
        allocator.free(fb.fallback_id);
        for (fb.tokens.items) |tok| allocator.free(tok);
        fb.deinit();
    }

    // BEFORE, AFTER should be INCLUDED (inside %ifndef SQLITE_OMIT_*)
    try std.testing.expectEqual(@as(usize, 4), fb.tokens.items.len);
    try std.testing.expectEqualStrings("ABORT", fb.tokens.items[0]);
    try std.testing.expectEqualStrings("BEFORE", fb.tokens.items[1]);
    try std.testing.expectEqualStrings("AFTER", fb.tokens.items[2]);
    try std.testing.expectEqualStrings("ACTION", fb.tokens.items[3]);
}

test "extractPrecedenceFromGrammar" {
    const allocator = std.testing.allocator;
    const content =
        \\%left OR.
        \\%left AND.
        \\%right NOT.
        \\%nonassoc EQ NE.
    ;

    var rules = try extractPrecedenceFromGrammar(allocator, content);
    defer {
        for (rules.items) |r| allocator.free(r);
        rules.deinit();
    }

    try std.testing.expectEqual(@as(usize, 4), rules.items.len);
    try std.testing.expectEqualStrings("%left OR.", rules.items[0]);
    try std.testing.expectEqualStrings("%left AND.", rules.items[1]);
    try std.testing.expectEqualStrings("%right NOT.", rules.items[2]);
    try std.testing.expectEqualStrings("%nonassoc EQ NE.", rules.items[3]);
}

test "extractWildcardFromGrammar" {
    const allocator = std.testing.allocator;
    const content =
        \\%wildcard ANY.
    ;

    const result = try extractWildcardFromGrammar(allocator, content);
    try std.testing.expect(result != null);
    defer if (result) |r| allocator.free(r);
    try std.testing.expectEqualStrings("ANY", result.?);
}

test "extractWildcardFromGrammar none" {
    const allocator = std.testing.allocator;
    const content = "// no wildcard here\n";
    const result = try extractWildcardFromGrammar(allocator, content);
    try std.testing.expect(result == null);
}

test "stripActionBlocks" {
    const allocator = std.testing.allocator;
    const content = "rule ::= stuff. { code { nested } more }";
    const result = try stripActionBlocks(allocator, content);
    defer allocator.free(result);

    try std.testing.expectEqualStrings("rule ::= stuff. ", result);
}

test "parseLemonGOutput" {
    const allocator = std.testing.allocator;
    const output =
        \\// Symbols:
        \\//   0 $                      1 SEMI
        \\//   2 EXPLAIN                 3 input
        \\input ::= cmdlist.
        \\cmdlist ::= cmdlist ecmd.
    ;

    var parsed = try parseLemonGOutput(allocator, output);
    defer {
        for (parsed.terminals.items) |t| allocator.free(t);
        for (parsed.nonterminals.items) |t| allocator.free(t);
        for (parsed.rules.items) |r| allocator.free(r);
        parsed.deinit();
    }

    try std.testing.expectEqual(@as(usize, 2), parsed.terminals.items.len);
    try std.testing.expectEqualStrings("SEMI", parsed.terminals.items[0]);
    try std.testing.expectEqualStrings("EXPLAIN", parsed.terminals.items[1]);

    try std.testing.expectEqual(@as(usize, 1), parsed.nonterminals.items.len);
    try std.testing.expectEqualStrings("input", parsed.nonterminals.items[0]);

    try std.testing.expectEqual(@as(usize, 2), parsed.rules.items.len);
    try std.testing.expectEqualStrings("input ::= cmdlist.", parsed.rules.items[0]);
}

test "splitExtensionGrammar" {
    const allocator = std.testing.allocator;
    const content =
        \\%token FOO BAR.
        \\// comment
        \\mynt ::= FOO BAR. { action(); }
    ;

    const result = try splitExtensionGrammar(allocator, content);
    defer {
        allocator.free(result.directives);
        allocator.free(result.bare_rules);
    }

    try std.testing.expectEqualStrings("%token FOO BAR.", result.directives);
    try std.testing.expectEqualStrings("mynt ::= FOO BAR.", result.bare_rules);
}

test "parseExtensionKeywords" {
    const allocator = std.testing.allocator;
    const content =
        \\%token FOO BAR.
        \\%token BAZ.
        \\%token FOO.
    ;

    var keywords = try parseExtensionKeywords(allocator, content);
    defer {
        for (keywords.items) |kw| allocator.free(kw);
        keywords.deinit();
    }

    // FOO should only appear once (deduped)
    try std.testing.expectEqual(@as(usize, 3), keywords.items.len);
    try std.testing.expectEqualStrings("FOO", keywords.items[0]);
    try std.testing.expectEqualStrings("BAR", keywords.items[1]);
    try std.testing.expectEqualStrings("BAZ", keywords.items[2]);
}

test "stripAnnotations" {
    const allocator = std.testing.allocator;

    {
        const result = try stripAnnotations(allocator, "lhs(A)");
        defer allocator.free(result);
        try std.testing.expectEqualStrings("lhs", result);
    }
    {
        const result = try stripAnnotations(allocator, "expr(B) PLUS expr(C)");
        defer allocator.free(result);
        try std.testing.expectEqualStrings("expr PLUS expr", result);
    }
    {
        const result = try stripAnnotations(allocator, "VALUES");
        defer allocator.free(result);
        try std.testing.expectEqualStrings("VALUES", result);
    }
}

test "findPrecMarker" {
    {
        const result = findPrecMarker("expr PLUS expr. [IN]");
        try std.testing.expect(result != null);
        try std.testing.expectEqualStrings("IN", result.?.name);
    }
    {
        const result = findPrecMarker("expr BITNOT expr. [BITNOT]");
        try std.testing.expect(result != null);
        try std.testing.expectEqualStrings("BITNOT", result.?.name);
    }
    {
        const result = findPrecMarker("no marker here");
        try std.testing.expect(result == null);
    }
}
