# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Builds the emscripten-target wasm module used by web/playground."""

from __future__ import annotations

import argparse
import os
import platform
import shlex
import shutil
import subprocess
import sys

ROOT_DIR: str = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def strip_separate_dwarf_link_arg(flags: str) -> str:
    """Remove any existing -gseparate-dwarf linker arg from rustflags."""
    if not flags:
        return ""

    parts = shlex.split(flags)
    out: list[str] = []
    i = 0
    while i < len(parts):
        part = parts[i]
        if part == "-C" and i + 1 < len(parts):
            next_part = parts[i + 1]
            if next_part.startswith("link-arg=-gseparate-dwarf="):
                i += 2
                continue
        if part.startswith("-Clink-arg=-gseparate-dwarf="):
            i += 1
            continue
        out.append(part)
        i += 1
    return " ".join(out)


def get_platform_dir() -> str | None:
    sys_name = platform.system().lower()
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "amd64"

    if sys_name == "darwin":
        return "mac-" + arch
    elif sys_name == "linux":
        return "linux-" + arch
    else:
        return None


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build the emscripten-target wasm module for web/playground.",
    )
    parser.add_argument(
        "--rustflags",
        default="",
        help="extra target rustc flags appended for wasm32-unknown-emscripten",
    )
    parser.add_argument(
        "--hermetic",
        action="store_true",
        help="use third_party rustc/cargo instead of system binaries (for CI)",
    )
    args = parser.parse_args()

    platform_dir = get_platform_dir()
    if platform_dir is None:
        print("error: unsupported platform %s-%s" % (platform.system(), platform.machine()),
              file=sys.stderr)
        return 1

    bin_dir = os.path.join(ROOT_DIR, "third_party", "bin", platform_dir)
    emscripten_dir = os.path.join(bin_dir, "emscripten")
    nodejs_dir = os.path.join(bin_dir, "nodejs")

    if not os.path.isdir(emscripten_dir):
        print("emscripten not found at %s" % emscripten_dir, file=sys.stderr)
        print("run: tools/install-build-deps --ui", file=sys.stderr)
        return 1

    # Add hermetic node and emscripten to PATH.
    env = os.environ.copy()
    path_prepend = os.pathsep.join([
        os.path.join(nodejs_dir, "bin"),
        os.path.join(emscripten_dir, "emscripten"),
        os.path.join(emscripten_dir, "bin"),
    ])
    env["PATH"] = path_prepend + os.pathsep + env.get("PATH", "")
    env["EM_CONFIG"] = os.path.join(emscripten_dir, "emscripten_config")

    em_cache_dir = env.get("EM_CACHE", os.path.join(ROOT_DIR, ".cache", "emscripten-cache"))
    os.makedirs(em_cache_dir, exist_ok=True)
    env["EM_CACHE"] = em_cache_dir

    # Use a separate target directory so WASM builds don't invalidate native
    # cargo caches (different RUSTFLAGS, different toolchain).
    cargo_target_dir = os.path.join(ROOT_DIR, "target", "wasm-playground")
    env["CARGO_TARGET_DIR"] = cargo_target_dir

    wasm_release_dir = os.path.join(
        cargo_target_dir, "wasm32-unknown-emscripten", "release",
    )
    runtime_debug_relpath = os.path.relpath(
        os.path.join(wasm_release_dir, "syntaqlite-runtime.debug.wasm"),
        ROOT_DIR,
    )

    # Point rustc at the wasm32 standard library installed by install-build-deps --ui.
    # --no-sysroot prevents the global RUSTFLAGS --sysroot so that the target-specific
    # CARGO_TARGET_*_RUSTFLAGS can set it for wasm32 without conflict.
    wasm32_sysroot = os.path.join(
        bin_dir, "rust-wasm32", "rust-std-wasm32-unknown-emscripten",
    )
    # Keep inherited rustflags, but avoid duplicate/stale separate-dwarf outputs.
    target_rustflags = strip_separate_dwarf_link_arg(
        env.get("CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS", ""),
    )
    target_rustflags += " --sysroot " + wasm32_sysroot
    # Build runtime as emscripten main module and expose dynamic linker APIs.
    target_rustflags += " -C link-arg=-sMAIN_MODULE=2"
    target_rustflags += " -C link-arg=-sALLOW_TABLE_GROWTH"
    target_rustflags += " -C link-arg=-sALLOW_MEMORY_GROWTH"
    target_rustflags += (
        " -C link-arg=-sEXPORTED_RUNTIME_METHODS=[\"loadDynamicLibrary\",\"ccall\",\"cwrap\"]"
    )
    # Emit DWARF debug info into a separate file so Chrome DevTools can
    # symbolize stack traces without bloating the served .wasm.
    target_rustflags += " -C link-arg=-g"
    target_rustflags += " -C link-arg=-gseparate-dwarf=" + runtime_debug_relpath
    if args.rustflags:
        target_rustflags += " " + args.rustflags
    env["CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS"] = target_rustflags.strip()

    cargo = os.path.join(ROOT_DIR, "tools", "cargo")
    cargo_flags = ["--no-sysroot"] + (["--hermetic"] if args.hermetic else [])
    rc = subprocess.call(
        [
            sys.executable, cargo, *cargo_flags,
            "build", "-p", "syntaqlite-wasm",
            "--target", "wasm32-unknown-emscripten", "--release",
        ],
        cwd=ROOT_DIR,
        env=env,
    )
    if rc != 0:
        return rc

    # Copy the built wasm to web/playground/.
    wasm_target_dir = wasm_release_dir
    out_public_dir = os.path.join(ROOT_DIR, "web/playground", "public")
    os.makedirs(out_public_dir, exist_ok=True)
    out_runtime_js = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-runtime.js")
    out_runtime_wasm = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-runtime.wasm")
    out_dialect = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-sqlite.wasm")

    runtime_js_src = None
    runtime_wasm_src = None
    js_candidates = [
        os.path.join(wasm_target_dir, "syntaqlite-wasm.js"),
        os.path.join(wasm_target_dir, "syntaqlite_wasm.js"),
    ]
    wasm_candidates = [
        os.path.join(wasm_target_dir, "syntaqlite-wasm.wasm"),
        os.path.join(wasm_target_dir, "syntaqlite_wasm.wasm"),
    ]
    for js_path in js_candidates:
        if not os.path.isfile(js_path):
            continue
        for wasm_path in wasm_candidates:
            if os.path.isfile(wasm_path):
                runtime_js_src = js_path
                runtime_wasm_src = wasm_path
                break
        if runtime_js_src and runtime_wasm_src:
            break
    else:
        print("could not locate built runtime js/wasm in %s" % wasm_target_dir, file=sys.stderr)
        return 1

    shutil.copy2(runtime_js_src, out_runtime_js)
    shutil.copy2(runtime_wasm_src, out_runtime_wasm)

    # Copy separate DWARF debug info if present.
    runtime_debug_src = os.path.join(wasm_target_dir, "syntaqlite-runtime.debug.wasm")
    if os.path.isfile(runtime_debug_src):
        out_runtime_debug = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-runtime.debug.wasm")
        shutil.copy2(runtime_debug_src, out_runtime_debug)
        print("wrote %s" % out_runtime_debug)

    # Build SQLite dialect wasm side module from C sources directly.
    # dialect.c is generated by codegen and exports syntaqlite_sqlite_dialect()
    # returning *const SyntaqliteDialectTemplate — no wrapper needed.
    out_dialect_debug = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-sqlite.debug.wasm")
    emcc_cmd = [
        "emcc",
        "-O3",
        "-fPIC",
        "-g",
        "-gseparate-dwarf=" + out_dialect_debug,
        os.path.join(ROOT_DIR, "syntaqlite-syntax", "csrc", "sqlite", "dialect.c"),
        os.path.join(ROOT_DIR, "syntaqlite-syntax", "csrc", "sqlite", "sqlite_parse.c"),
        os.path.join(ROOT_DIR, "syntaqlite-syntax", "csrc", "sqlite", "sqlite_tokenize.c"),
        os.path.join(ROOT_DIR, "syntaqlite-syntax", "csrc", "sqlite", "sqlite_keyword.c"),
        "-I", os.path.join(ROOT_DIR, "syntaqlite-syntax"),
        "-I", os.path.join(ROOT_DIR, "syntaqlite-syntax", "include"),
        "-DSYNTAQLITE_OMIT_SQLITE_API",
        "-sWASM_BIGINT",
        "-sSIDE_MODULE=1",
        "--no-entry",
        "-o", out_dialect,
    ]
    rc = subprocess.call(emcc_cmd, cwd=ROOT_DIR, env=env)
    if rc != 0:
        print("failed to build sqlite dialect wasm from C sources", file=sys.stderr)
        return 1

    print("wrote %s" % out_runtime_js)
    print("wrote %s" % out_runtime_wasm)
    print("wrote %s" % out_dialect)
    if os.path.isfile(out_dialect_debug):
        print("wrote %s" % out_dialect_debug)

    # Also copy runtime + SQLite dialect to syntaqlite package for bundled distribution.
    js_pkg_wasm_dir = os.path.join(ROOT_DIR, "web", "syntaqlite-js", "wasm")
    os.makedirs(js_pkg_wasm_dir, exist_ok=True)
    for src, name in [
        (out_runtime_js, "syntaqlite-runtime.js"),
        (out_runtime_wasm, "syntaqlite-runtime.wasm"),
        (out_dialect, "syntaqlite-sqlite.wasm"),
    ]:
        dst = os.path.join(js_pkg_wasm_dir, name)
        shutil.copy2(src, dst)
        print("wrote %s" % dst)

    # Build Perfetto dialect wasm side module.
    perfetto_work_dir = os.path.join(ROOT_DIR, ".cache", "perfetto-wasm")
    perfetto_csrc_dir = os.path.join(perfetto_work_dir, "csrc")
    os.makedirs(perfetto_csrc_dir, exist_ok=True)

    rc = subprocess.call(
        [
            sys.executable, cargo, *cargo_flags,
            "run", "-p", "syntaqlite-cli", "--",
            "dialect", "--name", "perfetto",
            "--actions-dir", os.path.join(ROOT_DIR, "dialects", "perfetto", "actions"),
            "--nodes-dir", os.path.join(ROOT_DIR, "dialects", "perfetto", "nodes"),
            "--macro-style", "rust",
            "--output-dir", perfetto_csrc_dir,
        ],
        cwd=ROOT_DIR, env=env,
    )
    if rc != 0:
        print("failed to generate perfetto amalgamated C sources", file=sys.stderr)
        return rc

    # Write shim headers for side module compilation.
    with open(os.path.join(perfetto_csrc_dir, "syntaqlite_runtime.h"), "w", encoding="utf-8") as f:
        f.write("#ifndef SYNTAQLITE_RUNTIME_H\n#define SYNTAQLITE_RUNTIME_H\n"
                "#include \"syntaqlite/config.h\"\n#include \"syntaqlite/types.h\"\n"
                "#include \"syntaqlite/grammar.h\"\n#include \"syntaqlite/parser.h\"\n"
                "#include \"syntaqlite/tokenizer.h\"\n#endif\n")
    with open(os.path.join(perfetto_csrc_dir, "syntaqlite_dialect.h"), "w", encoding="utf-8") as f:
        f.write("#ifndef SYNTAQLITE_EXT_H\n#define SYNTAQLITE_EXT_H\n"
                "#include \"syntaqlite_dialect/sqlite_compat.h\"\n#include \"syntaqlite_dialect/arena.h\"\n"
                "#include \"syntaqlite_dialect/vec.h\"\n#include \"syntaqlite_dialect/dialect_types.h\"\n"
                "#include \"syntaqlite_dialect/dialect_macros.h\"\n"
                "#include \"syntaqlite_dialect/ast_builder.h\"\n"
                "#endif\n")

    out_perfetto = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-perfetto.wasm")
    out_perfetto_debug = os.path.join(ROOT_DIR, "web/playground", "public", "syntaqlite-perfetto.debug.wasm")
    rc = subprocess.call(
        [
            "emcc", "-O3", "-fPIC",
            "-g", "-gseparate-dwarf=" + out_perfetto_debug,
            os.path.join(perfetto_csrc_dir, "syntaqlite_perfetto.c"),
            "-I", perfetto_csrc_dir,
            "-I", os.path.join(ROOT_DIR, "syntaqlite-syntax", "include"),
            "-sWASM_BIGINT", "-sSIDE_MODULE=1", "--no-entry",
            "-o", out_perfetto,
        ],
        cwd=ROOT_DIR, env=env,
    )
    if rc != 0:
        print("failed to compile perfetto dialect wasm", file=sys.stderr)
        return rc

    print("wrote %s" % out_perfetto)
    if os.path.isfile(out_perfetto_debug):
        print("wrote %s" % out_perfetto_debug)
    return 0


if __name__ == "__main__":
    sys.exit(main())
