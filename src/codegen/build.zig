const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "synq-codegen",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.setCwd(b.path("."));
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run synq-codegen");
    run_step.dependOn(&run_cmd.step);

    const test_step = b.step("test", "Run all tests");

    const test_files = [_][]const u8{
        "src/string_utils.zig",
        "src/c_tokenizer.zig",
        "src/main.zig",
        "src/c_emitter.zig",
        "src/transforms.zig",
        "src/generators.zig",
        "src/grammar_build.zig",
        "src/parser_extract.zig",
        "src/parser_split.zig",
        "src/fmt_compiler.zig",
        "src/keywordhash.zig",
        "src/lempar_transform.zig",
    };

    for (test_files) |file| {
        const t = b.addTest(.{
            .root_source_file = b.path(file),
            .target = target,
            .optimize = optimize,
        });
        const run_t = b.addRunArtifact(t);
        run_t.setCwd(b.path("."));
        test_step.dependOn(&run_t.step);
    }
}
