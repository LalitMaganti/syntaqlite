const std = @import("std");
const mem = std.mem;
const fs = std.fs;
const node_defs = @import("node_defs.zig");
const c_emitter = @import("c_emitter.zig");
const fmt_compiler = @import("fmt_compiler.zig");
const grammar_build = @import("grammar_build.zig");
const parser_split = @import("parser_split.zig");
const parser_extract = @import("parser_extract.zig");
const generators = @import("generators.zig");
const transforms = @import("transforms.zig");
const tools = @import("tools.zig");
const keywordhash = @import("keywordhash.zig");
const lempar_transform = @import("lempar_transform.zig");
const embedded_tools = @import("embedded_tools.zig");

const version = "0.1.0";

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 2) {
        printUsage();
        return;
    }

    const cmd = args[1];
    if (std.mem.eql(u8, cmd, "version")) {
        const stdout = std.io.getStdOut().writer();
        try stdout.print("synq-codegen {s}\n", .{version});
    } else if (std.mem.eql(u8, cmd, "generate")) {
        try runGenerateAll(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "generate-ast")) {
        try runGenerateAst(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "build-grammar")) {
        try runBuildGrammar(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "transform-lempar")) {
        try runTransformLempar(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "run-lemon")) {
        try runRunLemon(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "process-parser")) {
        try runProcessParser(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "process-tokenizer")) {
        try runProcessTokenizer(allocator, args[2..]);
    } else {
        const stderr = std.io.getStdErr().writer();
        try stderr.print("unknown command: {s}\n", .{cmd});
        printUsage();
        std.process.exit(1);
    }
}

// ============ Helpers ============

fn readFileAlloc(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    const file = try fs.cwd().openFile(path, .{});
    defer file.close();
    return try file.readToEndAlloc(allocator, 50 * 1024 * 1024);
}

fn writeFileContent(path: []const u8, content: []const u8) !void {
    if (fs.path.dirname(path)) |dir| {
        fs.cwd().makePath(dir) catch |err| switch (err) {
            error.PathAlreadyExists => {},
            else => return err,
        };
    }
    const file = try fs.cwd().createFile(path, .{});
    defer file.close();
    try file.writeAll(content);
}

/// Convert field_name to CLI flag: "foo_bar" → "--foo-bar", "o" → "-o".
fn flagName(comptime name: []const u8) []const u8 {
    if (name.len == 1) return comptime "-" ++ name;
    const flag = comptime blk: {
        var buf: [name.len + 2]u8 = undefined;
        buf[0] = '-';
        buf[1] = '-';
        for (name, 0..) |c, i| {
            buf[i + 2] = if (c == '_') '-' else c;
        }
        break :blk buf;
    };
    return &flag;
}

/// Parse CLI args into a struct. Fields with ?[]const u8 start as null;
/// fields with []const u8 use their default value.
fn parseArgs(comptime T: type, args: []const []const u8) T {
    var result: T = .{};
    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        var matched = false;
        inline for (std.meta.fields(T)) |field| {
            if (std.mem.eql(u8, args[i], comptime flagName(field.name))) {
                if (i + 1 >= args.len) dieMissing(comptime flagName(field.name));
                i += 1;
                @field(result, field.name) = args[i];
                matched = true;
            }
        }
        if (!matched) dieUnknown(args[i]);
    }
    return result;
}

/// Unwrap a required ?[]const u8 field, or exit with error.
fn require(opts: anytype, comptime field_name: []const u8) []const u8 {
    return @field(opts, field_name) orelse dieMissing(comptime flagName(field_name));
}

fn dieUnknown(opt: []const u8) noreturn {
    const stderr = std.io.getStdErr().writer();
    stderr.print("unknown option: {s}\n", .{opt}) catch {};
    std.process.exit(1);
}

fn dieMissing(name: []const u8) noreturn {
    const stderr = std.io.getStdErr().writer();
    stderr.print("error: {s} is required\n", .{name}) catch {};
    std.process.exit(1);
}

// ============ generate (full pipeline) ============

fn runGenerateAll(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        base_grammar: ?[]const u8 = null,
        actions_dir: ?[]const u8 = null,
        nodes_dir: ?[]const u8 = null,
        tokenize_c: ?[]const u8 = null,
        global_c: ?[]const u8 = null,
        sqliteint_h: ?[]const u8 = null,
        prefix: []const u8 = "synq",
        out_dir: ?[]const u8 = null,
    }, args);

    const bg = require(opts, "base_grammar");
    const nd = require(opts, "nodes_dir");
    const tc = require(opts, "tokenize_c");
    const gc = require(opts, "global_c");
    const si = require(opts, "sqliteint_h");
    const od = require(opts, "out_dir");

    const stderr = std.io.getStdErr().writer();

    // Stage 1: build-grammar
    var build_grammar_args = std.ArrayList([]const u8).init(allocator);
    defer build_grammar_args.deinit();
    try build_grammar_args.appendSlice(&.{ "--base-grammar", bg });
    if (opts.actions_dir) |ad| {
        try build_grammar_args.appendSlice(&.{ "--actions-dir", ad });
    }
    try build_grammar_args.appendSlice(&.{ "--prefix", opts.prefix });

    const grammar_out = try fs.path.join(allocator, &.{ od, "synq_parse.y" });
    defer allocator.free(grammar_out);
    try build_grammar_args.appendSlice(&.{ "-o", grammar_out });

    try stderr.writeAll("[1/5] Building grammar...\n");
    try runBuildGrammar(allocator, build_grammar_args.items);

    // Stage 2: run-lemon
    const lemon_out_dir = try fs.path.join(allocator, &.{ od, "_lemon_tmp" });
    defer allocator.free(lemon_out_dir);

    try stderr.writeAll("[2/5] Running Lemon parser generator...\n");
    try runRunLemon(allocator, &.{ "--grammar", grammar_out, "--out-dir", lemon_out_dir });

    // Stage 3: process-parser
    const parse_c = try fs.path.join(allocator, &.{ lemon_out_dir, "synq_parse.c" });
    defer allocator.free(parse_c);
    const parse_h = try fs.path.join(allocator, &.{ lemon_out_dir, "synq_parse.h" });
    defer allocator.free(parse_h);
    const parser_out = try fs.path.join(allocator, &.{ od, "parser" });
    defer allocator.free(parser_out);

    try stderr.writeAll("[3/5] Processing parser output...\n");
    try runProcessParser(allocator, &.{
        "--parse-c", parse_c,
        "--parse-h", parse_h,
        "--prefix",  opts.prefix,
        "--out-dir", parser_out,
    });

    // Stage 4: process-tokenizer
    const tokenizer_out = try fs.path.join(allocator, &.{ od, "tokenizer" });
    defer allocator.free(tokenizer_out);

    try stderr.writeAll("[4/5] Processing tokenizer...\n");
    try runProcessTokenizer(allocator, &.{
        "--tokenize-c",  tc,
        "--global-c",    gc,
        "--sqliteint-h", si,
        "--prefix",      opts.prefix,
        "--out-dir",     tokenizer_out,
    });

    // Stage 5: generate-ast
    try stderr.writeAll("[5/5] Generating AST code...\n");
    try runGenerateAst(allocator, &.{
        "--nodes-dir", nd,
        "--out-dir",   parser_out,
    });

    // Move fmt_gen.c and copy public headers using a temp arena
    {
        var arena = std.heap.ArenaAllocator.init(allocator);
        defer arena.deinit();
        const a = arena.allocator();

        // Move fmt_gen.c to formatter directory
        const fmt_src = try fs.path.join(a, &.{ parser_out, "fmt_gen.c" });
        const fmt_dst = try fs.path.join(a, &.{ od, "formatter", "fmt_gen.c" });
        try writeFileContent(fmt_dst, try readFileAlloc(a, fmt_src));
        try fs.cwd().deleteFile(fmt_src);

        // Copy public headers
        const include_dir = try fs.path.join(a, &.{ od, "include", "syntaqlite" });

        const ast_nodes_src = try fs.path.join(a, &.{ parser_out, "ast_nodes_gen.h" });
        const ast_nodes_dst = try fs.path.join(a, &.{ include_dir, "ast_nodes_gen.h" });
        try writeFileContent(ast_nodes_dst, try readFileAlloc(a, ast_nodes_src));

        const tokens_src = try fs.path.join(a, &.{ parser_out, "sqlite_tokens_gen.h" });
        const tokens_dst = try fs.path.join(a, &.{ include_dir, "sqlite_tokens_gen.h" });
        try writeFileContent(tokens_dst, try readFileAlloc(a, tokens_src));
    }

    // Clean up temp lemon output
    fs.cwd().deleteTree(lemon_out_dir) catch {};
    fs.cwd().deleteFile(grammar_out) catch {};

    try stderr.print("All files generated in {s}\n", .{od});
}

// ============ generate-ast ============

fn runGenerateAst(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        nodes_dir: ?[]const u8 = null,
        out_dir: ?[]const u8 = null,
    }, args);

    const nd = require(opts, "nodes_dir");

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const arena_alloc = arena.allocator();

    const abs_nodes_dir = try fs.cwd().realpathAlloc(arena_alloc, nd);

    // Load definitions (shared by both generators)
    const files = try node_defs.loadIndex(arena_alloc, abs_nodes_dir);
    const defs = try node_defs.loadDefinitions(arena_alloc, abs_nodes_dir, files);

    if (opts.out_dir) |od| {
        const emitters = .{
            .{ "ast_nodes_gen.h", c_emitter.emit },
            .{ "ast_nodes_gen_internal.h", c_emitter.emitInternal },
            .{ "ast_builder_gen.h", c_emitter.emitBuilderH },
            .{ "ast_builder_gen.c", c_emitter.emitBuilderC },
            .{ "ast_print_gen.c", c_emitter.emitPrintC },
        };
        inline for (emitters) |entry| {
            const path = try fs.path.join(arena_alloc, &.{ od, entry[0] });
            const file = try fs.cwd().createFile(path, .{});
            defer file.close();
            var buffered = std.io.bufferedWriter(file.writer());
            try entry[1](arena_alloc, defs, buffered.writer());
            try buffered.flush();
        }
        // fmt_gen.c is special — uses nodes_dir directly
        {
            const path = try fs.path.join(arena_alloc, &.{ od, "fmt_gen.c" });
            const file = try fs.cwd().createFile(path, .{});
            defer file.close();
            var buffered = std.io.bufferedWriter(file.writer());
            try fmt_compiler.generateFmtC(arena_alloc, abs_nodes_dir, buffered.writer());
            try buffered.flush();
        }

        const stderr = std.io.getStdErr().writer();
        try stderr.print("Generated AST headers, builders, printer, and formatter in {s}\n", .{od});
    } else {
        // No --out-dir: print ast_nodes_gen.h to stdout (backwards compatible)
        const stdout = std.io.getStdOut().writer();
        var buffered = std.io.bufferedWriter(stdout);
        try c_emitter.emit(arena_alloc, defs, buffered.writer());
        try buffered.flush();
    }
}

// ============ build-grammar ============

fn runBuildGrammar(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        base_grammar: ?[]const u8 = null,
        actions_dir: ?[]const u8 = null,
        prefix: []const u8 = "synq",
        o: ?[]const u8 = null,
    }, args);

    const bg = require(opts, "base_grammar");
    const out = require(opts, "o");

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const a = arena.allocator();

    // Read base grammar
    const base_grammar_content = try readFileAlloc(a, bg);

    // Resolve to absolute path for embedded lemon
    const abs_bg = try fs.cwd().realpathAlloc(a, bg);

    // Run embedded lemon -g to extract clean grammar rules
    const lemon_g_content = try embedded_tools.runLemonCapture(a, &.{ "lemon", "-g", abs_bg });

    // Parse lemon -g output to get terminals and rules
    var lemon_output = try grammar_build.parseLemonGOutput(a, lemon_g_content);
    defer lemon_output.deinit();

    // Build a set of valid terminal names from lemon -g output
    var valid_tokens = std.StringHashMap(void).init(a);
    for (lemon_output.terminals.items) |t| {
        try valid_tokens.put(t, {});
    }

    // Extract fallback, precedence, wildcard from base grammar
    var fallback = try grammar_build.extractFallbackFromGrammar(a, base_grammar_content, &valid_tokens);
    defer fallback.tokens.deinit();

    var precedence = try grammar_build.extractPrecedenceFromGrammar(a, base_grammar_content);
    defer precedence.deinit();

    const wildcard = try grammar_build.extractWildcardFromGrammar(a, base_grammar_content);

    // Load action files (if directory provided)
    var action_rules: ?std.StringHashMap([]const u8) = null;
    if (opts.actions_dir) |ad| {
        const abs_ad = try fs.cwd().realpathAlloc(a, ad);
        action_rules = try grammar_build.loadAllActions(a, abs_ad);
    }

    const grammar_content = try grammar_build.generateGrammarFile(a, .{
        .prefix = opts.prefix,
        .terminals = lemon_output.terminals.items,
        .rules = lemon_output.rules.items,
        .fallback_id = fallback.fallback_id,
        .fallback_tokens = fallback.tokens.items,
        .precedence_rules = precedence.items,
        .wildcard = wildcard,
        .action_rules = if (action_rules != null) &action_rules.? else null,
    });

    try writeFileContent(out, grammar_content);

    const stderr = std.io.getStdErr().writer();
    try stderr.print("Grammar written to {s}\n", .{out});
}

// ============ transform-lempar ============

fn runTransformLempar(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        input: ?[]const u8 = null,
        o: ?[]const u8 = null,
    }, args);

    const inp = require(opts, "input");
    const out = require(opts, "o");

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const a = arena.allocator();

    const content = try readFileAlloc(a, inp);
    const transformed = try lempar_transform.transformForSplit(a, content);

    try writeFileContent(out, transformed);

    const stderr = std.io.getStdErr().writer();
    try stderr.print("Transformed lempar written to {s}\n", .{out});
}

// ============ process-parser ============

fn runProcessParser(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        parse_c: ?[]const u8 = null,
        parse_h: ?[]const u8 = null,
        prefix: []const u8 = "synq",
        out_dir: ?[]const u8 = null,
    }, args);

    const pc = require(opts, "parse_c");
    const ph = require(opts, "parse_h");
    const od = require(opts, "out_dir");

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const a = arena.allocator();

    // Read parse.c, strip Lemon's generated comment, and apply symbol renaming
    var parse_c = try readFileAlloc(a, pc);

    var xforms = std.ArrayList(transforms.Transform).init(a);
    try xforms.append(.{ .remove_section = .{
        .start_marker = "/* This file is automatically generated by Lemon",
        .end_marker = "*/",
    } });
    const rename_pipeline = try tools.createParserSymbolRenamePipeline(a, opts.prefix);
    try xforms.appendSlice(rename_pipeline.transforms);
    parse_c = try (transforms.Pipeline{ .transforms = xforms.items }).apply(a, parse_c);

    const split = try parser_split.splitParserOutput(a, parse_c, opts.prefix);

    // Write parser output files
    const defs_path = try fs.path.join(a, &.{ od, "sqlite_parse_defs.h" });
    const tables_path = try fs.path.join(a, &.{ od, "sqlite_parse_tables.h" });
    const engine_path = try fs.path.join(a, &.{ od, "sqlite_parse_gen.c" });
    try writeFileContent(defs_path, split.defs_h);
    try writeFileContent(tables_path, split.tables_h);

    // Wrap engine.c in a source file header
    const engine_gen = generators.sourceFileGenerator(.{
        .description = "SQLite parser for synq.\n** Generated from SQLite's parse.y via Lemon.",
    });
    const engine_wrapped = try engine_gen.generate(a, split.engine_c);
    try writeFileContent(engine_path, engine_wrapped);

    // Read parse.h and extract token defines
    const parse_h = try readFileAlloc(a, ph);
    const token_defines = try generators.extractTokenDefines(a, parse_h);

    // Build token defines body with extension grammar ifdef
    var token_body = std.ArrayList(u8).init(a);
    const tw = token_body.writer();
    try tw.writeAll("#ifdef SYNTAQLITE_EXTENSION_GRAMMAR\n");
    try tw.writeAll("#include SYNTAQLITE_EXTENSION_GRAMMAR\n");
    try tw.writeAll("#else\n\n");
    for (token_defines) |define| {
        try tw.writeAll(define);
        try tw.writeAll("\n");
    }
    try tw.writeAll("\n#endif /* SYNTAQLITE_EXTENSION_GRAMMAR */");

    const token_gen = generators.headerGenerator(.{
        .guard = "SYNTAQLITE_SQLITE_TOKENS_GEN_H",
        .description = "Token definitions from SQLite's parse.y via Lemon.",
    });
    const token_content = try token_gen.generate(a, token_body.items);
    const tokens_path = try fs.path.join(a, &.{ od, "sqlite_tokens_gen.h" });
    try writeFileContent(tokens_path, token_content);

    const stderr = std.io.getStdErr().writer();
    try stderr.print("Parser files written to {s}\n", .{od});
}

// ============ process-tokenizer ============

fn runProcessTokenizer(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        tokenize_c: ?[]const u8 = null,
        global_c: ?[]const u8 = null,
        sqliteint_h: ?[]const u8 = null,
        prefix: []const u8 = "synq",
        out_dir: ?[]const u8 = null,
    }, args);

    const tc = require(opts, "tokenize_c");
    const gc = require(opts, "global_c");
    const si = require(opts, "sqliteint_h");
    const od = require(opts, "out_dir");

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const a = arena.allocator();

    const sqlite3_prefix = try std.fmt.allocPrint(a, "{s}_sqlite3", .{opts.prefix});

    const kh_raw = try embedded_tools.runMkkeywordhash(a);
    const kh_result = try keywordhash.processKeywordhash(a, kh_raw, opts.prefix);

    // Build inlined header to replace #include "sqliteInt.h"
    var inlined = std.ArrayList(u8).init(a);
    const iw = inlined.writer();
    try iw.writeAll("#include \"src/common/synq_sqlite_defs.h\"\n");
    try iw.writeAll("#include \"src/tokenizer/sqlite_charmap_gen.h\"\n\n");
    try iw.writeAll("/*\n** Synq tokenizer injection support.\n");
    try iw.writeAll("** When SYNTAQLITE_EXTENSION_GRAMMAR is defined, external keyword data is used.\n*/\n");
    try iw.writeAll("#ifdef SYNTAQLITE_EXTENSION_GRAMMAR\n");
    try iw.writeAll("#include SYNTAQLITE_EXTENSION_GRAMMAR\n");
    try iw.writeAll("#define _SYNQ_EXTERNAL_KEYWORDHASH 1\n");
    try iw.writeAll("#endif\n\n");
    try iw.writeAll("#ifndef _SYNQ_EXTERNAL_KEYWORDHASH\n");
    try iw.writeAll("#include \"syntaqlite/sqlite_tokens_gen.h\"\n");
    try iw.writeAll(kh_result.data_section);
    try iw.writeAll("\n#endif /* _SYNQ_EXTERNAL_KEYWORDHASH */\n\n");
    try iw.writeAll("/* Keyword lookup function — driven by hash tables + SYNQ_KW_HASH_* params */\n");
    try iw.writeAll(kh_result.keyword_code_func);
    try iw.writeAll("\n");
    try iw.print("/* Stub for parser fallback - not needed for pure tokenization */\n", .{});
    try iw.print("static inline int {s}ParserFallback(int token) {{\n", .{sqlite3_prefix});
    try iw.writeAll("  (void)token;\n  return 0;\n}\n\n");
    try iw.print("/* Forward declaration */\n", .{});
    try iw.print("i64 {s}GetToken(const unsigned char *z, int *tokenType);\n", .{sqlite3_prefix});

    // Build tokenizer transform pipeline
    const testcase_name = try std.fmt.allocPrint(a, "{s}Testcase", .{sqlite3_prefix});
    const keyword_code_new = try std.fmt.allocPrint(a, "{s}_keywordCode", .{opts.prefix});

    var xform_list = std.ArrayList(transforms.Transform).init(a);
    try xform_list.append(.{ .truncate_at = .{ .marker = "/*\n** Run the parser on the given SQL string." } });
    try xform_list.append(.{ .replace_text = .{ .old = "#include \"sqliteInt.h\"", .new = inlined.items } });
    try xform_list.append(.{ .replace_text = .{ .old = "#include \"keywordhash.h\"\n", .new = "" } });
    try xform_list.append(.{ .replace_text = .{ .old = "#include <stdlib.h>\n", .new = "" } });

    // Rename analyze{Window,Over,Filter}Keyword definitions and calls
    const analyze_funcs = [_][]const u8{ "Window", "Over", "Filter" };
    for (&analyze_funcs) |func| {
        const def_old = try std.fmt.allocPrint(a, "static int analyze{s}Keyword", .{func});
        const def_new = try std.fmt.allocPrint(a, "int {s}Analyze{s}Keyword", .{ sqlite3_prefix, func });
        const call_old = try std.fmt.allocPrint(a, "analyze{s}Keyword(", .{func});
        const call_new = try std.fmt.allocPrint(a, "{s}Analyze{s}Keyword(", .{ sqlite3_prefix, func });
        try xform_list.append(.{ .replace_text = .{ .old = def_old, .new = def_new } });
        try xform_list.append(.{ .replace_text = .{ .old = call_old, .new = call_new } });
    }

    try xform_list.append(.{ .strip_blessing_comment = .{} });
    try xform_list.append(.{ .symbol_rename = .{ .old_prefix = "TK_", .new_prefix = "SYNTAQLITE_TOKEN_" } });
    try xform_list.append(.{ .symbol_rename_exact = .{ .old_name = "keywordCode", .new_name = keyword_code_new } });
    try xform_list.append(.{ .remove_function_calls = .{ .function_name = testcase_name } });
    const xforms = try xform_list.toOwnedSlice();

    const pipeline = transforms.Pipeline{ .transforms = xforms };

    // Read and transform tokenize.c
    var tokenize_content = try readFileAlloc(a, tc);
    tokenize_content = try pipeline.apply(a, tokenize_content);

    // Apply sqlite3 symbol rename pipeline
    const sym_pipeline = try tools.createSymbolRenamePipeline(a, opts.prefix);
    tokenize_content = try sym_pipeline.apply(a, tokenize_content);

    const tok_gen = generators.sourceFileGenerator(.{
        .description = "SQLite tokenizer for synq.\n** Extracted from SQLite's tokenize.c with minimal modifications.",
    });
    const tok_wrapped = try tok_gen.generate(a, tokenize_content);
    const tok_path = try fs.path.join(a, &.{ od, "sqlite_tokenize_gen.c" });
    try writeFileContent(tok_path, tok_wrapped);

    const charmap_content = try buildCharmapHeader(a, gc, si, opts.prefix);
    const charmap_path = try fs.path.join(a, &.{ od, "sqlite_charmap_gen.h" });
    try writeFileContent(charmap_path, charmap_content);

    const stderr = std.io.getStdErr().writer();
    try stderr.print("Tokenizer files written to {s}\n", .{od});
}

/// Build the sqlite_charmap_gen.h content from global.c + sqliteInt.h.
fn buildCharmapHeader(
    a: std.mem.Allocator,
    global_c_path: []const u8,
    sqliteint_h_path: []const u8,
    prefix: []const u8,
) ![]const u8 {
    const global_c = try readFileAlloc(a, global_c_path);
    const sqliteint_h = try readFileAlloc(a, sqliteint_h_path);
    const sqlite3_prefix = try std.fmt.allocPrint(a, "{s}_sqlite3", .{prefix});

    var body = std.ArrayList(u8).init(a);
    const w = body.writer();

    // Extract sqlite3UpperToLower — ASCII version only
    const upper_start_marker = "const unsigned char sqlite3UpperToLower[]";
    if (mem.indexOf(u8, global_c, upper_start_marker)) |upper_pos| {
        const ifdef_ascii = "#ifdef SQLITE_ASCII";
        if (mem.indexOfPos(u8, global_c, upper_pos, ifdef_ascii)) |ifdef_pos| {
            const endif_marker = "#endif";
            if (mem.indexOfPos(u8, global_c, ifdef_pos, endif_marker)) |endif_pos| {
                // Get data between #ifdef and #endif (skip the #ifdef line)
                const after_ifdef = ifdef_pos + ifdef_ascii.len;
                const data_start = (mem.indexOfPos(u8, global_c, after_ifdef, "\n") orelse after_ifdef) + 1;
                const upper_data = mem.trim(u8, global_c[data_start..endif_pos], " \t\r\n");

                try w.print(
                    \\/*
                    \\** Upper-to-lower case conversion table.
                    \\** SQLite only considers US-ASCII characters.
                    \\*/
                    \\static const unsigned char {s}UpperToLower[] = {{
                    \\
                , .{sqlite3_prefix});
                try w.writeAll(upper_data);
                try w.writeAll("\n};\n\n");
            }
        }
    }

    // Extract sqlite3CtypeMap
    const ctype_marker = "const unsigned char sqlite3CtypeMap[256]";
    if (mem.indexOf(u8, global_c, ctype_marker)) |ctype_pos| {
        // Find the comment block before it
        const comment_marker = "/*";
        var comment_start = ctype_pos;
        // Search backwards for the nearest /* before ctype_pos
        var search = ctype_pos;
        while (search > 0) : (search -= 1) {
            if (search + 1 < global_c.len and global_c[search] == '/' and global_c[search + 1] == '*') {
                comment_start = search;
                break;
            }
        }
        _ = comment_marker;

        const comment_end_marker = "*/";
        if (mem.indexOfPos(u8, global_c, comment_start, comment_end_marker)) |ce| {
            const comment = global_c[comment_start .. ce + comment_end_marker.len];
            try w.writeAll(comment);
            try w.writeAll("\n");
        }

        // Find the array end
        if (mem.indexOfPos(u8, global_c, ctype_pos, "};")) |array_end| {
            var ctype_block = try a.dupe(u8, global_c[ctype_pos .. array_end + 2]);
            // Rename sqlite3CtypeMap -> {prefix}_sqlite3CtypeMap
            const old_name = "sqlite3CtypeMap";
            const new_name = try std.fmt.allocPrint(a, "{s}CtypeMap", .{sqlite3_prefix});
            if (mem.indexOf(u8, ctype_block, old_name)) |name_pos| {
                var result = std.ArrayList(u8).init(a);
                try result.appendSlice(ctype_block[0..name_pos]);
                try result.appendSlice(new_name);
                try result.appendSlice(ctype_block[name_pos + old_name.len ..]);
                ctype_block = try result.toOwnedSlice();
            }
            try w.writeAll("static ");
            try w.writeAll(ctype_block);
            try w.writeAll("\n\n");
        }
    }

    // Extract character classification macros from sqliteInt.h
    const macro_comment_marker = "** The following macros mimic the standard library functions";
    if (mem.indexOf(u8, sqliteint_h, macro_comment_marker)) |macro_pos| {
        // Find the comment start (/* before this)
        var cs = macro_pos;
        while (cs > 0) : (cs -= 1) {
            if (cs + 1 < sqliteint_h.len and sqliteint_h[cs] == '/' and sqliteint_h[cs + 1] == '*') break;
        }
        const comment_end = (mem.indexOfPos(u8, sqliteint_h, cs, "*/") orelse cs) + 2;
        const macro_comment = sqliteint_h[cs..comment_end];
        try w.writeAll(macro_comment);
        try w.writeAll("\n");

        // Find #ifdef SQLITE_ASCII block
        if (mem.indexOfPos(u8, sqliteint_h, comment_end, "#ifdef SQLITE_ASCII")) |ifdef_pos| {
            const after_ifdef_line = (mem.indexOfPos(u8, sqliteint_h, ifdef_pos, "\n") orelse ifdef_pos) + 1;
            if (mem.indexOfPos(u8, sqliteint_h, ifdef_pos, "#else")) |else_pos| {
                const ascii_macros = mem.trim(u8, sqliteint_h[after_ifdef_line..else_pos], " \t\r\n");

                // Rename sqlite3 symbols in macros
                const sym_pipeline = try tools.createSymbolRenamePipeline(a, prefix);
                const renamed = try sym_pipeline.apply(a, ascii_macros);
                try w.writeAll(renamed);
                try w.writeAll("\n");
            }
        }
    }

    // Wrap in header generator
    const guard = try std.fmt.allocPrint(a, "{s}_SRC_TOKENIZER_SQLITE_CHARMAP_GEN_H", .{prefix});
    // uppercase the guard
    for (guard) |*c| {
        c.* = std.ascii.toUpper(c.*);
    }
    const gen = generators.headerGenerator(.{
        .guard = guard,
        .description = "Character tables extracted from SQLite's global.c",
    });
    return try gen.generate(a, body.items);
}

// ============ run-lemon ============

fn runRunLemon(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const opts = parseArgs(struct {
        grammar: ?[]const u8 = null,
        out_dir: ?[]const u8 = null,
    }, args);

    const gp = require(opts, "grammar");
    const od = require(opts, "out_dir");

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const a = arena.allocator();

    // Read grammar file
    const grammar_content = try readFileAlloc(a, gp);

    // Create temp directory for lemon to work in
    var tmp_path_buf: [std.fs.max_path_bytes]u8 = undefined;
    const timestamp = @as(u64, @intCast(std.time.nanoTimestamp()));
    const tmp_dir = try std.fmt.bufPrint(&tmp_path_buf, "/tmp/synq-lemon-{d}", .{timestamp});

    try fs.cwd().makePath(tmp_dir);
    defer fs.cwd().deleteTree(tmp_dir) catch {};

    // Write grammar and embedded lempar.c into temp dir
    const tmp_grammar = try fs.path.join(a, &.{ tmp_dir, "synq_parse.y" });
    try writeFileContent(tmp_grammar, grammar_content);

    const tmp_lempar = try fs.path.join(a, &.{ tmp_dir, "lempar.c" });
    try writeFileContent(tmp_lempar, embedded_tools.lempar_c);

    // Resolve to absolute paths for embedded lemon
    const abs_grammar = try fs.cwd().realpathAlloc(a, tmp_grammar);
    const abs_lempar = try fs.cwd().realpathAlloc(a, tmp_lempar);

    // Run embedded lemon -l (suppress #line directives, write parse.c + parse.h)
    // -T specifies template explicitly since embedded lemon can't find it via argv[0]
    const t_flag = try std.fmt.allocPrint(a, "-T{s}", .{abs_lempar});
    try embedded_tools.runLemon(&.{ "lemon", "-l", t_flag, abs_grammar });

    // Copy outputs to out-dir
    try fs.cwd().makePath(od);

    const src_parse_c = try fs.path.join(a, &.{ tmp_dir, "synq_parse.c" });
    const src_parse_h = try fs.path.join(a, &.{ tmp_dir, "synq_parse.h" });
    const dst_parse_c = try fs.path.join(a, &.{ od, "synq_parse.c" });
    const dst_parse_h = try fs.path.join(a, &.{ od, "synq_parse.h" });

    const parse_c_data = try readFileAlloc(a, src_parse_c);
    const parse_h_data = try readFileAlloc(a, src_parse_h);
    try writeFileContent(dst_parse_c, parse_c_data);
    try writeFileContent(dst_parse_h, parse_h_data);

    const stderr = std.io.getStdErr().writer();
    try stderr.print("Lemon output written to {s}\n", .{od});
}

// ============ Usage ============

fn printUsage() void {
    const stderr = std.io.getStdErr().writer();
    stderr.print(
        \\Usage: synq-codegen <command> [options]
        \\
        \\Commands:
        \\  generate           Run the full codegen pipeline
        \\  generate-ast       Generate AST code only (header + formatter)
        \\  build-grammar      Assemble grammar from parse.y + action files
        \\  transform-lempar   Add SYNQ markers to lempar.c template
        \\  run-lemon          Run embedded Lemon parser generator
        \\  process-parser     Split + rename Lemon parser output
        \\  process-tokenizer  Transform tokenizer + extract charmap
        \\  version            Print version information
        \\
        \\Run 'synq-codegen <command> --help' for command-specific options.
        \\
    , .{}) catch {};
}
