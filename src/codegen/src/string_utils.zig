// Shared string utilities for codegen.
//
// Case conversion and type helpers used across c_emitter and fmt_compiler.

const std = @import("std");
const Allocator = std.mem.Allocator;

/// Convert PascalCase to UPPER_SNAKE_CASE or snake_case.
fn pascalToCase(allocator: Allocator, name: []const u8, comptime to_upper: bool) ![]const u8 {
    var result = std.ArrayList(u8).init(allocator);
    defer result.deinit();

    for (name, 0..) |c, i| {
        if (std.ascii.isUpper(c) and i > 0) {
            const prev = name[i - 1];
            if (std.ascii.isLower(prev)) {
                try result.append('_');
            } else if (std.ascii.isUpper(prev) and i + 1 < name.len and std.ascii.isLower(name[i + 1])) {
                try result.append('_');
            }
        }
        try result.append(if (to_upper) std.ascii.toUpper(c) else std.ascii.toLower(c));
    }

    return try result.toOwnedSlice();
}

/// Convert PascalCase to UPPER_SNAKE_CASE.
/// e.g. "BinaryOp" -> "BINARY_OP", "CteDefinition" -> "CTE_DEFINITION"
pub fn pascalToUpperSnake(allocator: Allocator, name: []const u8) ![]const u8 {
    return pascalToCase(allocator, name, true);
}

/// Convert PascalCase to snake_case.
/// e.g. "BinaryExpr" -> "binary_expr"
pub fn pascalToSnakeCase(allocator: Allocator, name: []const u8) ![]const u8 {
    return pascalToCase(allocator, name, false);
}

/// Known inline types that map to SyntaqliteSourceSpan in C.
pub fn isSourceSpanType(type_name: []const u8) bool {
    return std.mem.eql(u8, type_name, "SyntaqliteSourceSpan");
}

/// Produce the SYNTAQLITE_XXX_ enum prefix from a type name.
pub fn enumPrefix(allocator: Allocator, type_name: []const u8) ![]const u8 {
    const upper = try pascalToUpperSnake(allocator, type_name);
    return try std.fmt.allocPrint(allocator, "SYNTAQLITE_{s}", .{upper});
}

/// Case-insensitive comparison.
pub fn eqlIgnoreCase(a: []const u8, b: []const u8) bool {
    if (a.len != b.len) return false;
    for (a, b) |ca, cb| {
        if (std.ascii.toLower(ca) != std.ascii.toLower(cb)) return false;
    }
    return true;
}

// ============ Tests ============

test "pascalToUpperSnake" {
    const allocator = std.testing.allocator;

    const cases = .{
        .{ "BinaryOp", "BINARY_OP" },
        .{ "CteDefinition", "CTE_DEFINITION" },
        .{ "ForeignKeyClause", "FOREIGN_KEY_CLAUSE" },
        .{ "Bool", "BOOL" },
        .{ "SelectStmt", "SELECT_STMT" },
        .{ "IsOp", "IS_OP" },
        .{ "CreateTableStmt", "CREATE_TABLE_STMT" },
    };

    inline for (cases) |case| {
        const result = try pascalToUpperSnake(allocator, case[0]);
        defer allocator.free(result);
        try std.testing.expectEqualStrings(case[1], result);
    }
}

test "pascalToSnakeCase" {
    const allocator = std.testing.allocator;

    const cases = .{
        .{ "BinaryExpr", "binary_expr" },
        .{ "SelectStmt", "select_stmt" },
        .{ "CteDefinition", "cte_definition" },
        .{ "ExprList", "expr_list" },
        .{ "ValuesRowList", "values_row_list" },
    };

    inline for (cases) |case| {
        const result = try pascalToSnakeCase(allocator, case[0]);
        defer allocator.free(result);
        try std.testing.expectEqualStrings(case[1], result);
    }
}

test "enumPrefix" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const a = arena.allocator();
    try std.testing.expectEqualStrings("SYNTAQLITE_BINARY_OP", try enumPrefix(a, "BinaryOp"));
    try std.testing.expectEqualStrings("SYNTAQLITE_SORT_ORDER", try enumPrefix(a, "SortOrder"));
}

test "eqlIgnoreCase" {
    try std.testing.expect(eqlIgnoreCase("STAR", "star"));
    try std.testing.expect(eqlIgnoreCase("distinct", "DISTINCT"));
    try std.testing.expect(!eqlIgnoreCase("FOO", "BAR"));
    try std.testing.expect(!eqlIgnoreCase("FOO", "FOOO"));
}
