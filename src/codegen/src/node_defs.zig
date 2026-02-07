const std = @import("std");
const Allocator = std.mem.Allocator;

// ============ Parsed definition types ============

pub const EnumDef = struct {
    name: []const u8,
    values: []const []const u8,
};

pub const FlagEntry = struct {
    name: []const u8,
    bit: u32,
};

pub const FlagsDef = struct {
    name: []const u8,
    flags: []const FlagEntry,
};

pub const FieldDef = struct {
    name: []const u8,
    storage: Storage,
    type_name: []const u8,

    pub const Storage = enum { @"inline", index };
};

pub const NodeDef = struct {
    name: []const u8,
    fields: []const FieldDef,
};

pub const ListDef = struct {
    name: []const u8,
    child_type: []const u8,
};

pub const Definition = union(enum) {
    enum_def: EnumDef,
    flags_def: FlagsDef,
    node_def: NodeDef,
    list_def: ListDef,
};

pub const NodeDefinitions = struct {
    defs: []const Definition,
    allocator: Allocator,

    pub fn deinit(self: *NodeDefinitions) void {
        self.allocator.free(self.defs);
    }
};

// ============ JSON parsing ============

pub fn loadIndex(allocator: Allocator, nodes_dir: []const u8) ![]const []const u8 {
    const index_path = try std.fs.path.join(allocator, &.{ nodes_dir, "index.json" });
    defer allocator.free(index_path);

    const file = try std.fs.openFileAbsolute(index_path, .{});
    defer file.close();

    const content = try file.readToEndAlloc(allocator, 1024 * 1024);
    defer allocator.free(content);

    const parsed = try std.json.parseFromSlice(std.json.Value, allocator, content, .{});
    defer parsed.deinit();

    const files_arr = parsed.value.object.get("files") orelse return error.MissingFiles;
    const items = files_arr.array.items;

    const result = try allocator.alloc([]const u8, items.len);
    for (items, 0..) |item, i| {
        result[i] = try allocator.dupe(u8, item.string);
    }
    return result;
}

pub fn loadDefinitions(allocator: Allocator, nodes_dir: []const u8, files: []const []const u8) ![]const Definition {
    var defs = std.ArrayList(Definition).init(allocator);
    defer defs.deinit();

    for (files) |filename| {
        const file_path = try std.fs.path.join(allocator, &.{ nodes_dir, filename });
        defer allocator.free(file_path);

        const file = try std.fs.openFileAbsolute(file_path, .{});
        defer file.close();

        const content = try file.readToEndAlloc(allocator, 4 * 1024 * 1024);
        defer allocator.free(content);

        const parsed = try std.json.parseFromSlice(std.json.Value, allocator, content, .{});
        defer parsed.deinit();

        const definitions = parsed.value.object.get("definitions") orelse return error.MissingDefinitions;

        for (definitions.array.items) |def_val| {
            const obj = def_val.object;
            const kind = obj.get("kind").?.string;

            if (std.mem.eql(u8, kind, "enum")) {
                try defs.append(.{ .enum_def = try parseEnumDef(allocator, obj) });
            } else if (std.mem.eql(u8, kind, "flags")) {
                try defs.append(.{ .flags_def = try parseFlagsDef(allocator, obj) });
            } else if (std.mem.eql(u8, kind, "node")) {
                try defs.append(.{ .node_def = try parseNodeDef(allocator, obj) });
            } else if (std.mem.eql(u8, kind, "list")) {
                try defs.append(.{ .list_def = try parseListDef(allocator, obj) });
            }
        }
    }

    return try defs.toOwnedSlice();
}

fn parseEnumDef(allocator: Allocator, obj: std.json.ObjectMap) !EnumDef {
    const name = try allocator.dupe(u8, obj.get("name").?.string);
    const values_arr = obj.get("values").?.array.items;
    const values = try allocator.alloc([]const u8, values_arr.len);
    for (values_arr, 0..) |v, i| {
        values[i] = try allocator.dupe(u8, v.string);
    }
    return .{ .name = name, .values = values };
}

fn parseFlagsDef(allocator: Allocator, obj: std.json.ObjectMap) !FlagsDef {
    const name = try allocator.dupe(u8, obj.get("name").?.string);
    const flags_obj = obj.get("flags").?.object;

    // Collect entries and sort by bit value
    var entries = std.ArrayList(FlagEntry).init(allocator);
    defer entries.deinit();

    var it = flags_obj.iterator();
    while (it.next()) |entry| {
        const bit: u32 = @intCast(entry.value_ptr.integer);
        try entries.append(.{
            .name = try allocator.dupe(u8, entry.key_ptr.*),
            .bit = bit,
        });
    }

    // Sort by bit value to maintain consistent ordering
    std.mem.sort(FlagEntry, entries.items, {}, struct {
        fn lessThan(_: void, a: FlagEntry, b: FlagEntry) bool {
            return a.bit < b.bit;
        }
    }.lessThan);

    return .{ .name = name, .flags = try entries.toOwnedSlice() };
}

fn parseNodeDef(allocator: Allocator, obj: std.json.ObjectMap) !NodeDef {
    const name = try allocator.dupe(u8, obj.get("name").?.string);
    const fields_obj = obj.get("fields").?.object;

    // Preserve insertion order from JSON (json.ObjectMap maintains order)
    var fields = try allocator.alloc(FieldDef, fields_obj.count());
    var idx: usize = 0;
    var it = fields_obj.iterator();
    while (it.next()) |entry| {
        const field_obj = entry.value_ptr.object;
        const storage_str = field_obj.get("storage").?.string;
        const storage: FieldDef.Storage = if (std.mem.eql(u8, storage_str, "inline"))
            .@"inline"
        else
            .index;

        fields[idx] = .{
            .name = try allocator.dupe(u8, entry.key_ptr.*),
            .storage = storage,
            .type_name = try allocator.dupe(u8, field_obj.get("type").?.string),
        };
        idx += 1;
    }

    return .{ .name = name, .fields = fields };
}

fn parseListDef(allocator: Allocator, obj: std.json.ObjectMap) !ListDef {
    const name = try allocator.dupe(u8, obj.get("name").?.string);
    const child_type = try allocator.dupe(u8, obj.get("child_type").?.string);
    return .{ .name = name, .child_type = child_type };
}
