# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation test executor.

Generates dialect amalgamations, compiles test binaries, and runs
diff tests against them.

Three amalgamation modes are supported:

  FULL              -- runtime inlined into dialect amalgam; self-contained
                       syntaqlite_<name>.{h,c} that compiles with no extra deps.
  DIALECT_ONLY      -- dialect references an external syntaqlite_runtime.h;
                       runtime must be compiled and linked separately.
  FULL_OMIT_RUNTIME -- same checked-in full amalgamation, but compiled with
                       -DSYNTAQLITE_OMIT_RUNTIME to strip runtime bodies;
                       a separate runtime-only .c is linked in. Models the
                       cdylib dialect-plugin use case from issue #166.
"""

import enum
import os
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional

from python.dev.diff_tests.testing import DiffTestBlueprint


class AmalgMode(enum.Enum):
    FULL = "full"
    DIALECT_ONLY = "dialect_only"
    FULL_OMIT_RUNTIME = "full_omit_runtime"


@dataclass
class DialectConfig:
    """Configuration for a dialect under test."""
    name: str
    mode: AmalgMode = AmalgMode.FULL
    actions_dir: Optional[str] = None
    nodes_dir: Optional[str] = None
    extra_cflags: tuple[str, ...] = ()

    @property
    def key(self) -> str:
        """Unique build-cache key."""
        suffix = "_".join(f.lstrip("-D").lower() for f in self.extra_cflags)
        base = f"{self.name}_{self.mode.value}"
        return f"{base}_{suffix}" if suffix else base


# ---------------------------------------------------------------------------
# Amalgamation generation
# ---------------------------------------------------------------------------

def _build_full(cli_binary: Path, dialect: DialectConfig, output_dir: Path) -> None:
    cmd = [str(cli_binary), "dialect", "--name", dialect.name]
    if dialect.actions_dir:
        cmd += ["--actions-dir", dialect.actions_dir]
    if dialect.nodes_dir:
        cmd += ["--nodes-dir", dialect.nodes_dir]
    cmd += ["--output-type", "full", "--output-dir", str(output_dir)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"full dialect generation failed for {dialect.name}:\n{proc.stderr}"
        )


def _build_dialect_only(
    cli_binary: Path, dialect: DialectConfig, output_dir: Path
) -> None:
    cmd = [str(cli_binary), "dialect", "--name", dialect.name]
    if dialect.actions_dir:
        cmd += ["--actions-dir", dialect.actions_dir]
    if dialect.nodes_dir:
        cmd += ["--nodes-dir", dialect.nodes_dir]
    cmd += ["--output-dir", str(output_dir)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"dialect-only generation failed for {dialect.name}:\n{proc.stderr}"
        )


def _build_runtime_only(cli_binary: Path, output_dir: Path) -> None:
    cmd = [str(cli_binary), "dialect", "--name", "sqlite",
           "--output-type", "runtime-only", "--output-dir", str(output_dir)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"runtime-only generation failed:\n{proc.stderr}"
        )


# ---------------------------------------------------------------------------
# Compilation
# ---------------------------------------------------------------------------

def _compile_full_binary(
    test_c: Path, amalg_dir: Path, dialect_name: str, output_binary: Path,
    extra_cflags: tuple[str, ...] = (),
) -> None:
    """Compile test_ast.c against a self-contained full amalgamation."""
    grammar_header = f'"syntaqlite_{dialect_name}.h"'
    grammar_fn = f"syntaqlite_{dialect_name}_dialect"
    source = amalg_dir / f"syntaqlite_{dialect_name}.c"
    cmd = [
        "cc", "-o", str(output_binary),
        str(test_c), str(source),
        f"-I{amalg_dir}",
        f"-DGRAMMAR_HEADER={grammar_header}",
        f"-DGRAMMAR_FN={grammar_fn}",
        "-Werror",
        *extra_cflags,
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Compilation failed (full) for {dialect_name}:\n{proc.stderr}"
        )


def _compile_full_omit_runtime_binary(
    test_c: Path,
    amalg_dir: Path,
    runtime_dir: Path,
    dialect_name: str,
    output_binary: Path,
) -> None:
    """Compile test_ast.c against a full amalgamation built with
    -DSYNTAQLITE_OMIT_RUNTIME, linked against a separately-generated
    runtime-only .c.

    Models the cdylib dialect-plugin use case: the host binary already
    provides the runtime, so the plugin's copy of the full amalgamation
    strips runtime implementations to avoid duplication. Here we stand
    in for the host runtime by linking a runtime-only .c directly.
    """
    grammar_header = f'"syntaqlite_{dialect_name}.h"'
    grammar_fn = f"syntaqlite_{dialect_name}_dialect"
    dialect_src = amalg_dir / f"syntaqlite_{dialect_name}.c"
    runtime_src = runtime_dir / "syntaqlite_runtime.c"
    cmd = [
        "cc", "-o", str(output_binary),
        str(test_c), str(dialect_src), str(runtime_src),
        f"-I{amalg_dir}",
        f"-I{runtime_dir}",
        f"-DGRAMMAR_HEADER={grammar_header}",
        f"-DGRAMMAR_FN={grammar_fn}",
        "-DSYNTAQLITE_OMIT_RUNTIME",
        "-Werror",
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Compilation failed (full + OMIT_RUNTIME) for {dialect_name}:\n{proc.stderr}"
        )


def _compile_dialect_only_binary(
    test_c: Path,
    amalg_dir: Path,
    runtime_dir: Path,
    dialect_name: str,
    output_binary: Path,
) -> None:
    """Compile test_ast.c against dialect-only + separate runtime."""
    grammar_header = f'"syntaqlite_{dialect_name}.h"'
    grammar_fn = f"syntaqlite_{dialect_name}_dialect"
    dialect_src = amalg_dir / f"syntaqlite_{dialect_name}.c"
    runtime_src = runtime_dir / "syntaqlite_runtime.c"
    cmd = [
        "cc", "-o", str(output_binary),
        str(test_c), str(dialect_src), str(runtime_src),
        f"-I{amalg_dir}",
        f"-I{runtime_dir}",
        f"-DGRAMMAR_HEADER={grammar_header}",
        f"-DGRAMMAR_FN={grammar_fn}",
        "-Werror",
    ]
    # runtime.c is a separate TU from dialect.c, so the #define in the
    # dialect preamble doesn't apply to it.  Pass the flag explicitly for
    # non-sqlite dialects so the sqlite-specific convenience wrappers
    # (which call syntaqlite_sqlite_grammar) are omitted there too.
    if dialect_name != "sqlite":
        cmd.append("-DSYNTAQLITE_OMIT_SQLITE_API")
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Compilation failed (dialect-only) for {dialect_name}:\n{proc.stderr}"
        )


# ---------------------------------------------------------------------------
# Strict-warning compile check
# ---------------------------------------------------------------------------

def compile_strict_warning_check(
    test_cpp: Path, amalg_dir: Path, dialect_name: str, output_binary: Path,
    runtime_dir: Optional[Path] = None,
) -> None:
    """Compile a C++ driver with -Wall -Wextra -Werror.

    Verifies the amalgamation produces zero warnings under strict settings.
    The .c amalgamation sources are compiled as C; the .cpp driver is
    compiled as C++; then both are linked.  This mirrors how downstream
    projects actually consume the amalgamation.
    """
    grammar_header = f'"syntaqlite_{dialect_name}.h"'
    grammar_fn = f"syntaqlite_{dialect_name}_dialect"
    strict_flags = [
        "-Weverything", "-Werror",
        # Consumer-side suppressions: these are about the consumer's
        # toolchain or style preferences, not syntaqlite's code.
        # Mirrors what Perfetto uses with -Weverything.
        "-Wno-c++98-compat-pedantic",
        "-Wno-c++98-compat",
        "-Wno-disabled-macro-expansion",
        "-Wno-documentation-unknown-command",
        "-Wno-gnu-include-next",
        "-Wno-gnu-statement-expression",
        "-Wno-gnu-zero-variadic-macro-arguments",
        "-Wno-padded",
        "-Wno-poison-system-directories",
        "-Wno-pre-c11-compat",
        "-Wno-reserved-id-macro",
        "-Wno-reserved-identifier",
        "-Wno-shadow-uncaptured-local",
        "-Wno-unknown-sanitizers",
        "-Wno-unknown-warning-option",
        "-Wno-unsafe-buffer-usage",
        "-Wno-switch-default",
    ]
    include_flags = [f"-I{amalg_dir}"]
    define_flags = [
        f"-DGRAMMAR_HEADER={grammar_header}",
        f"-DGRAMMAR_FN={grammar_fn}",
    ]

    obj_dir = output_binary.parent
    objects: list[str] = []

    # --- Compile C amalgamation sources with strict warnings ---
    c_sources = [amalg_dir / f"syntaqlite_{dialect_name}.c"]
    if runtime_dir is not None:
        c_sources.append(runtime_dir / "syntaqlite_runtime.c")
        include_flags.append(f"-I{runtime_dir}")
        if dialect_name != "sqlite":
            define_flags.append("-DSYNTAQLITE_OMIT_SQLITE_API")

    for src in c_sources:
        obj = str(obj_dir / (src.stem + ".o"))
        cmd = [os.environ.get("CC", "cc"), "-c", "-std=c11"] + strict_flags + include_flags + define_flags
        cmd += ["-o", obj, str(src)]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            raise RuntimeError(
                f"Strict-warning C compilation failed for {src.name}:\n"
                f"{proc.stderr}"
            )
        objects.append(obj)

    # --- Compile C++ driver with strict warnings ---
    cpp_obj = str(obj_dir / "test_strict_warnings.o")
    cmd = [os.environ.get("CXX", "c++"), "-c", "-std=c++17"] + strict_flags + include_flags + define_flags
    cmd += ["-o", cpp_obj, str(test_cpp)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Strict-warning C++ compilation failed for {test_cpp.name}:\n"
            f"{proc.stderr}"
        )
    objects.append(cpp_obj)

    # --- Link ---
    cmd = [os.environ.get("CXX", "c++"), "-o", str(output_binary)] + objects
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Strict-warning link failed for {dialect_name}:\n{proc.stderr}"
        )


# ---------------------------------------------------------------------------
# Context
# ---------------------------------------------------------------------------

class AmalgTestContext:
    """Manages build artifacts for amalgamation tests.

    Generates amalgamations and compiles test binaries once per unique
    (dialect, mode) configuration, then provides the binary path for
    running individual test cases.
    """

    def __init__(self, root_dir: Path, cli_binary: Path):
        self.root_dir = root_dir
        self.cli_binary = cli_binary
        self.test_c = root_dir / "tests/amalg_tests/test_ast.c"
        self._temp_dir = tempfile.TemporaryDirectory(prefix="syntaqlite_amalg_test_")
        self._binaries: Dict[str, Path] = {}
        # Shared runtime dir — built once and reused by all DIALECT_ONLY configs.
        self._runtime_dir: Optional[Path] = None

    def cleanup(self):
        self._temp_dir.cleanup()

    def _ensure_runtime(self) -> Path:
        if self._runtime_dir is not None:
            return self._runtime_dir
        temp = Path(self._temp_dir.name)
        runtime_dir = temp / "_runtime"
        runtime_dir.mkdir(exist_ok=True)
        _build_runtime_only(self.cli_binary, runtime_dir)
        self._runtime_dir = runtime_dir
        return runtime_dir

    def get_binary(self, dialect: DialectConfig) -> Path:
        """Get the compiled test binary for a dialect+mode, building if needed."""
        key = dialect.key
        if key in self._binaries:
            return self._binaries[key]

        temp = Path(self._temp_dir.name)
        amalg_dir = temp / key
        amalg_dir.mkdir(exist_ok=True)

        if dialect.mode == AmalgMode.FULL:
            _build_full(self.cli_binary, dialect, amalg_dir)
            binary = temp / f"test_{key}"
            _compile_full_binary(self.test_c, amalg_dir, dialect.name, binary,
                                extra_cflags=dialect.extra_cflags)

        elif dialect.mode == AmalgMode.DIALECT_ONLY:
            _build_dialect_only(self.cli_binary, dialect, amalg_dir)
            runtime_dir = self._ensure_runtime()
            binary = temp / f"test_{key}"
            _compile_dialect_only_binary(
                self.test_c, amalg_dir, runtime_dir, dialect.name, binary
            )

        elif dialect.mode == AmalgMode.FULL_OMIT_RUNTIME:
            _build_full(self.cli_binary, dialect, amalg_dir)
            runtime_dir = self._ensure_runtime()
            binary = temp / f"test_{key}"
            _compile_full_omit_runtime_binary(
                self.test_c, amalg_dir, runtime_dir, dialect.name, binary
            )

        else:
            raise ValueError(f"Unknown AmalgMode: {dialect.mode}")

        self._binaries[key] = binary
        return binary

    def check_strict_warnings(self, dialect: DialectConfig) -> None:
        """Compile a C++ driver with -Wall -Wextra -Werror.

        Must be called after get_binary() so the amalgamation is already
        generated.
        """
        key = dialect.key
        temp = Path(self._temp_dir.name)
        amalg_dir = temp / key
        test_cpp = self.root_dir / "tests/amalg_tests/test_strict_warnings.cpp"
        output = temp / f"strict_{key}"
        runtime_dir = (
            self._runtime_dir
            if dialect.mode in (AmalgMode.DIALECT_ONLY, AmalgMode.FULL_OMIT_RUNTIME)
            else None
        )
        compile_strict_warning_check(
            test_cpp, amalg_dir, dialect.name, output, runtime_dir
        )
