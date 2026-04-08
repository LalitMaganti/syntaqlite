# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Shared helpers for Perfetto dialect diff test runners."""

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[3]


def _has_opt(argv: list[str], opt: str) -> bool:
    return any(a == opt or a.startswith(f"{opt}=") for a in argv)


def _get_opt(argv: list[str], opt: str) -> str | None:
    for i, a in enumerate(argv):
        if a == opt and i + 1 < len(argv):
            return argv[i + 1]
        prefix = f"{opt}="
        if a.startswith(prefix):
            return a[len(prefix):]
    return None


def _replace_opt(argv: list[str], opt: str, value: str) -> list[str]:
    out = list(argv)
    for i, a in enumerate(out):
        if a == opt:
            if i + 1 < len(out):
                out[i + 1] = value
                return out
        prefix = f"{opt}="
        if a.startswith(prefix):
            out[i] = f"{opt}={value}"
            return out
    return [opt, value] + out


def _write_runtime_shims(csrc_dir: Path) -> None:
    (csrc_dir / "syntaqlite_runtime.h").write_text(
        """\
#ifndef SYNTAQLITE_RUNTIME_H
#define SYNTAQLITE_RUNTIME_H
#include \"syntaqlite/config.h\"
#include \"syntaqlite/types.h\"
#include \"syntaqlite/grammar.h\"
#include \"syntaqlite/parser.h\"
#include \"syntaqlite/tokenizer.h\"
#endif
""",
        encoding="utf-8",
    )
    (csrc_dir / "syntaqlite_dialect.h").write_text(
        """\
#ifndef SYNTAQLITE_EXT_H
#define SYNTAQLITE_EXT_H
#include \"syntaqlite_dialect/sqlite_compat.h\"
#include \"syntaqlite_dialect/dialect_types.h\"
#include \"syntaqlite_dialect/dialect_macros.h\"
#include \"syntaqlite_dialect/arena.h\"
#include \"syntaqlite_dialect/vec.h\"
#include \"syntaqlite_dialect/ast_builder.h\"
#endif
""",
        encoding="utf-8",
    )


def _compile_perfetto_dialect(cli_binary: Path, work_dir: Path) -> Path:
    csrc_dir = work_dir / "csrc"
    csrc_dir.mkdir(parents=True, exist_ok=True)

    subprocess.run(
        [
            str(cli_binary),
            "dialect",
            "--name",
            "perfetto",
            "--actions-dir",
            str(ROOT_DIR / "dialects" / "perfetto" / "actions"),
            "--nodes-dir",
            str(ROOT_DIR / "dialects" / "perfetto" / "nodes"),
            "--macro-style",
            "rust",
            "--output-dir",
            str(csrc_dir),
        ],
        cwd=ROOT_DIR,
        check=True,
    )

    _write_runtime_shims(csrc_dir)

    ext = ".dylib" if sys.platform == "darwin" else ".so"
    out_lib = work_dir / f"libsyntaqlite_perfetto{ext}"

    cc_cmd = ["cc"]
    if sys.platform == "darwin":
        cc_cmd += ["-dynamiclib", "-fPIC"]
    else:
        cc_cmd += ["-shared", "-fPIC"]

    parser_sys = ROOT_DIR / "syntaqlite-syntax"
    cc_cmd += [
        str(csrc_dir / "syntaqlite_perfetto.c"),
        str(parser_sys / "csrc" / "parser.c"),
        str(parser_sys / "csrc" / "token_wrapped.c"),
        "-DSYNTAQLITE_OMIT_SQLITE_API",
        "-I",
        str(csrc_dir),
        "-I",
        str(parser_sys),
        "-I",
        str(parser_sys / "include"),
        "-o",
        str(out_lib),
    ]
    subprocess.run(cc_cmd, cwd=ROOT_DIR, check=True)
    return out_lib


def _write_wrapper(path: Path, binary: Path, dialect_lib: Path) -> None:
    path.write_text(
        f"""#!/bin/sh
if [ "$1" = "fmt" ]; then
  shift
  exec "{binary}" --dialect "{dialect_lib}" --dialect-name perfetto fmt --semicolons false "$@"
fi
exec "{binary}" --dialect "{dialect_lib}" --dialect-name perfetto "$@"
""",
        encoding="utf-8",
    )
    path.chmod(0o755)


def run_perfetto_tests(
    subcommand: str, test_dir: str, tempfile_prefix: str, argv: list[str]
) -> int:
    """Entry point for Perfetto dialect test runners.

    Args:
        subcommand: CLI subcommand (e.g., "fmt" or "validate").
        test_dir: Relative path to test directory.
        tempfile_prefix: Prefix for temporary directory name.
        argv: Command-line arguments (sys.argv[1:]).
    """
    from python.dev.diff_tests.runner import main

    if not _has_opt(argv, "--binary"):
        argv = ["--binary", "target/debug/syntaqlite"] + argv
    if not _has_opt(argv, "--subcommand"):
        argv = ["--subcommand", subcommand] + argv
    if not _has_opt(argv, "--test-dir"):
        argv = ["--test-dir", test_dir] + argv

    binary_opt = _get_opt(argv, "--binary")
    if binary_opt is None:
        print("error: --binary option missing", file=sys.stderr)
        return 1

    binary_path = Path(binary_opt)
    if not binary_path.is_absolute():
        binary_path = ROOT_DIR / binary_path

    with tempfile.TemporaryDirectory(prefix=tempfile_prefix) as temp_dir:
        temp = Path(temp_dir)
        dialect_lib = _compile_perfetto_dialect(binary_path, temp)
        wrapper = temp / "syntaqlite-perfetto"
        _write_wrapper(wrapper, binary_path, dialect_lib)
        argv = _replace_opt(argv, "--binary", str(wrapper))
        return main(argv)
