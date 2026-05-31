# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation API surface input/output tests.

For each (dialect, amalgamation mode, opt-out flag) combination, compile a
probe source against the generated amalgamation and assert that it
compiles successfully or fails as expected.

The matrix covers:
  - `syntaqlite_parser_create_<dialect>` / `syntaqlite_tokenizer_create_<dialect>`
    — always available
  - `syntaqlite_parser_create_with_dialect` / `syntaqlite_tokenizer_create_with_dialect`
    — blocked only when inline dispatch is active in this TU (default Full,
      without -DSYNTAQLITE_NO_INLINE_DIALECT_DISPATCH and without
      -DSYNTAQLITE_OMIT_RUNTIME)
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

from python.dev.diff_tests.amalg_executor import (
    AmalgMode,
    DialectConfig,
    _build_dialect_only,
    _build_full,
    _build_runtime_only,
)
from python.dev.diff_tests.utils import Colors, colorize
from python.dev.integration_tests.suite import SuiteContext

NAME = "amalg-api-surface"
DESCRIPTION = "Amalgamation API surface compile-pass/fail probes"


@dataclass(frozen=True)
class Case:
    """One probe compile expected to pass or fail."""
    label: str
    dialect: str
    mode: AmalgMode
    probe: str  # "create" or "with_dialect"
    extra_cflags: tuple[str, ...]
    expect_ok: bool


# ──────────────────────────────────────────────────────────────────────
# The matrix.
#
# `create_*` is always emitted, so every row is expected to compile.
# `with_dialect_*` is stripped only in (Full, no opt-out, runtime in TU).
# ──────────────────────────────────────────────────────────────────────
CASES: list[Case] = [
    # _create_<dialect>: works in every mode and with every flag combo.
    Case("sqlite full default          create_sqlite",
         "sqlite", AmalgMode.FULL, "create", (), True),
    Case("sqlite full no-inline opt-out create_sqlite",
         "sqlite", AmalgMode.FULL, "create",
         ("-DSYNTAQLITE_NO_INLINE_DIALECT_DISPATCH",), True),
    Case("sqlite full omit-runtime     create_sqlite",
         "sqlite", AmalgMode.FULL_OMIT_RUNTIME, "create", (), True),
    Case("sqlite dialect-only          create_sqlite",
         "sqlite", AmalgMode.DIALECT_ONLY, "create", (), True),
    Case("perfetto full default        create_perfetto",
         "perfetto", AmalgMode.FULL, "create", (), True),
    Case("perfetto dialect-only        create_perfetto",
         "perfetto", AmalgMode.DIALECT_ONLY, "create", (), True),

    # _with_dialect: blocked only in default Full mode (runtime in this TU).
    Case("sqlite full default          with_dialect BLOCKED",
         "sqlite", AmalgMode.FULL, "with_dialect", (), False),
    Case("sqlite full no-inline opt-out with_dialect OK",
         "sqlite", AmalgMode.FULL, "with_dialect",
         ("-DSYNTAQLITE_NO_INLINE_DIALECT_DISPATCH",), True),
    Case("sqlite full omit-runtime     with_dialect OK",
         "sqlite", AmalgMode.FULL_OMIT_RUNTIME, "with_dialect", (), True),
    Case("sqlite dialect-only          with_dialect OK",
         "sqlite", AmalgMode.DIALECT_ONLY, "with_dialect", (), True),
    Case("perfetto full default        with_dialect BLOCKED",
         "perfetto", AmalgMode.FULL, "with_dialect", (), False),
    Case("perfetto dialect-only        with_dialect OK",
         "perfetto", AmalgMode.DIALECT_ONLY, "with_dialect", (), True),
]


def _dialect_config(case: Case, root_dir: Path) -> DialectConfig:
    """Configure the amalgamation generator for this case's dialect."""
    if case.dialect == "perfetto":
        return DialectConfig(
            name="perfetto", mode=case.mode,
            actions_dir=str(root_dir / "dialects/perfetto/actions"),
            nodes_dir=str(root_dir / "dialects/perfetto/nodes"),
        )
    return DialectConfig(name=case.dialect, mode=case.mode)


def _build_amalg(cli_binary: Path, cfg: DialectConfig, amalg_dir: Path) -> None:
    """Generate the amalgamation files for the given config into `amalg_dir`."""
    amalg_dir.mkdir(parents=True, exist_ok=True)
    if cfg.mode in (AmalgMode.FULL, AmalgMode.FULL_OMIT_RUNTIME):
        _build_full(cli_binary, cfg, amalg_dir)
    elif cfg.mode == AmalgMode.DIALECT_ONLY:
        _build_dialect_only(cli_binary, cfg, amalg_dir)
    else:
        raise ValueError(f"unknown mode: {cfg.mode}")


def _compile_probe(
    probe_src: Path,
    amalg_dir: Path,
    runtime_dir: Path | None,
    case: Case,
    out: Path,
) -> tuple[bool, str]:
    """Try to compile (and link) `probe_src` against the amalgamation.

    Returns (ok, combined_stderr_stdout) — `ok` is True iff cc exited 0.
    """
    dialect = case.dialect
    grammar_header = f'"syntaqlite_{dialect}.h"'
    grammar_fn = f"syntaqlite_{dialect}_dialect"

    sources = [str(probe_src), str(amalg_dir / f"syntaqlite_{dialect}.c")]
    includes = [f"-I{amalg_dir}"]
    defines = [
        f"-DGRAMMAR_HEADER={grammar_header}",
        f"-DGRAMMAR_FN={grammar_fn}",
        f"-DDIALECT_NAME={dialect}",
    ]

    # Non-sqlite dialects need the sqlite convenience API stripped from the
    # runtime, since their dialect accessor differs from sqlite's.
    if dialect != "sqlite" and case.mode in (
        AmalgMode.DIALECT_ONLY, AmalgMode.FULL_OMIT_RUNTIME
    ):
        defines.append("-DSYNTAQLITE_OMIT_SQLITE_API")

    if case.mode == AmalgMode.FULL_OMIT_RUNTIME:
        assert runtime_dir is not None
        sources.append(str(runtime_dir / "syntaqlite_runtime.c"))
        includes.append(f"-I{runtime_dir}")
        defines.append("-DSYNTAQLITE_OMIT_RUNTIME")
    elif case.mode == AmalgMode.DIALECT_ONLY:
        assert runtime_dir is not None
        sources.append(str(runtime_dir / "syntaqlite_runtime.c"))
        includes.append(f"-I{runtime_dir}")

    cmd = [os.environ.get("CC", "cc"), "-o", str(out)]
    cmd += sources + includes + defines + list(case.extra_cflags) + ["-Werror"]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    return proc.returncode == 0, (proc.stderr or "") + (proc.stdout or "")


def run(ctx: SuiteContext) -> int:
    root_dir = ctx.root_dir
    cli_binary = ctx.binary
    probe_dir = root_dir / "tests/amalg_tests"

    with tempfile.TemporaryDirectory(prefix="syntaqlite_amalg_api_") as tmp_str:
        tmp = Path(tmp_str)

        # Build amalgamation artifacts once per unique config.
        configs: dict[str, tuple[DialectConfig, Path]] = {}
        for case in CASES:
            cfg = _dialect_config(case, root_dir)
            if cfg.key in configs:
                continue
            amalg_dir = tmp / cfg.key
            _build_amalg(cli_binary, cfg, amalg_dir)
            configs[cfg.key] = (cfg, amalg_dir)

        # Shared external runtime for DIALECT_ONLY and FULL_OMIT_RUNTIME.
        runtime_dir = tmp / "_runtime"
        runtime_dir.mkdir()
        _build_runtime_only(cli_binary, runtime_dir)

        passed: list[Case] = []
        failed: list[tuple[Case, str]] = []
        for case in CASES:
            cfg = _dialect_config(case, root_dir)
            _, amalg_dir = configs[cfg.key]
            probe_src = probe_dir / f"probe_{case.probe}.c"
            out = tmp / f"probe_{cfg.key}_{case.probe}_{len(passed) + len(failed)}"
            rt_dir = runtime_dir if case.mode in (
                AmalgMode.DIALECT_ONLY, AmalgMode.FULL_OMIT_RUNTIME
            ) else None
            ok, err = _compile_probe(probe_src, amalg_dir, rt_dir, case, out)
            if ok == case.expect_ok:
                passed.append(case)
                if ctx.verbose >= 1:
                    mark = colorize("[       OK ]", Colors.GREEN)
                    print(f"{mark} {case.label}")
            else:
                failed.append((case, err))
                mark = colorize("[  FAILED  ]", Colors.RED)
                what = "expected compile success" if case.expect_ok else "expected compile failure"
                print(f"{mark} {case.label} — {what}")
                if ctx.verbose >= 1 and err:
                    for line in err.splitlines()[:8]:
                        print(f"        {line}")

        total = len(passed) + len(failed)
        if failed:
            print(colorize(f"[  FAILED  ]", Colors.RED) + f" {len(failed)}/{total} cases")
            return 1
        print(colorize(f"[  PASSED  ]", Colors.GREEN) + f" {total}/{total} cases")
        return 0
