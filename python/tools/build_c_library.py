# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build libsyntaqlite with the complete public C symbol surface.

Rust cdylibs export Rust-defined ``no_mangle`` functions but normally hide
symbols supplied by native static-library dependencies. The parser/tokenizer
implementation is one such dependency. This builder explicitly exports every
external function recorded in the C API manifest.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[2]
BASELINE = ROOT_DIR / "tests" / "c_api" / "api-manifest.json"


class BuildError(RuntimeError):
    """A user-facing build failure."""


def public_symbols(manifest: dict) -> list[str]:
    """Return external C functions from an API manifest."""
    return sorted(
        name
        for name, declaration in manifest["functions"].items()
        if declaration["linkage"] == "external"
    )


def _run(command: list[str], verbose: bool) -> None:
    if verbose:
        print("+", shlex.join(command))
    result = subprocess.run(
        command,
        cwd=ROOT_DIR,
        text=True,
        stdout=None if verbose else subprocess.PIPE,
        stderr=None if verbose else subprocess.PIPE,
    )
    if result.returncode != 0:
        detail = "\n".join(
            part.rstrip() for part in (result.stdout, result.stderr) if part
        )
        raise BuildError(
            f"command failed ({result.returncode}): {shlex.join(command)}"
            + (f"\n{detail}" if detail else "")
        )


def _cargo() -> list[str]:
    return [sys.executable, str(ROOT_DIR / "tools" / "cargo")]


def _output_dir(release: bool, target: str | None) -> Path:
    path = ROOT_DIR / "target"
    if target:
        path /= target
    return path / ("release" if release else "debug")


def _build_with_rustc_exports(
    symbols: list[str],
    *,
    release: bool,
    target: str | None,
    system: str,
    verbose: bool,
) -> Path:
    command = _cargo() + ["rustc", "-p", "syntaqlite", "--features", "rpc,dynload"]
    if release:
        command.append("--release")
    if target:
        command += ["--target", target]
    command += ["--crate-type", "cdylib", "--", "-C", "default-linker-libraries"]
    for symbol in symbols:
        if system == "Darwin":
            command += ["-C", f"link-arg=-Wl,-exported_symbol,_{symbol}"]
        else:
            command += [
                "-C",
                f"link-arg=/INCLUDE:{symbol}",
                "-C",
                f"link-arg=/EXPORT:{symbol}",
            ]
    _run(command, verbose)

    output = _output_dir(release, target)
    if system == "Darwin":
        return output / "libsyntaqlite.dylib"
    return output / "syntaqlite.dll"


def _build_linux(
    symbols: list[str], *, release: bool, target: str | None, verbose: bool
) -> Path:
    command = _cargo() + ["build", "-p", "syntaqlite", "--features", "rpc,dynload"]
    if release:
        command.append("--release")
    if target:
        command += ["--target", target]
    _run(command, verbose)

    output = _output_dir(release, target)
    static_library = output / "libsyntaqlite.a"
    shared_library = output / "libsyntaqlite.so"
    if not static_library.exists():
        raise BuildError(f"static library was not produced at {static_library}")

    with tempfile.TemporaryDirectory(prefix="syntaqlite-exports-") as temp:
        version_script = Path(temp) / "libsyntaqlite.map"
        body = ["{", "  global:"]
        body.extend(f"    {symbol};" for symbol in symbols)
        body.extend(["  local:", "    *;", "};", ""])
        version_script.write_text("\n".join(body))

        cc = shlex.split(os.environ.get("CC", "cc"))
        _run(
            cc
            + [
                "-shared",
                "-Wl,-z,defs",
                "-Wl,--whole-archive",
                str(static_library),
                "-Wl,--no-whole-archive",
                f"-Wl,--version-script={version_script}",
                "-Wl,-soname,libsyntaqlite.so",
                "-lpthread",
                "-ldl",
                "-lm",
                "-o",
                str(shared_library),
            ],
            verbose,
        )
    return shared_library


def build_c_library(
    symbols: list[str],
    *,
    release: bool = False,
    target: str | None = None,
    verbose: bool = False,
) -> Path:
    """Build and return the full C shared library."""
    system = platform.system()
    if system == "Linux":
        library = _build_linux(symbols, release=release, target=target, verbose=verbose)
    elif system in ("Darwin", "Windows"):
        library = _build_with_rustc_exports(
            symbols,
            release=release,
            target=target,
            system=system,
            verbose=verbose,
        )
    else:
        raise BuildError(f"unsupported platform: {system}")
    if not library.exists():
        raise BuildError(f"shared library was not produced at {library}")
    return library


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", action="store_true")
    parser.add_argument("--target")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args(argv)

    if not BASELINE.exists():
        print(
            "build-c-library: missing C API manifest; run tools/check-c-api --rebaseline",
            file=sys.stderr,
        )
        return 1
    manifest = json.loads(BASELINE.read_text())
    try:
        library = build_c_library(
            public_symbols(manifest),
            release=args.release,
            target=args.target,
            verbose=args.verbose,
        )
    except BuildError as error:
        print(f"build-c-library: {error}", file=sys.stderr)
        return 1
    print(library)
    return 0


if __name__ == "__main__":
    sys.exit(main())
