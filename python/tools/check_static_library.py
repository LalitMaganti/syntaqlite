# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Link a C program against the static libsyntaqlite and run it.

A static Rust library needs the system libraries its standard library
uses, which differ by platform. This asks rustc for that list, prints it
(it is what the C docs tell people to link), then links
tests/c_api/static_library_smoke.c against the library with it and runs
the result.

Run it after `cargo build --release -p syntaqlite-capi --features dynload`.
On Windows, the MSVC environment must be set up already.
"""

from __future__ import annotations

import os
import platform
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[2]
SMOKE_TEST = ROOT_DIR / "tests" / "c_api" / "static_library_smoke.c"
INCLUDE_DIRS = [ROOT_DIR / "syntaqlite-capi" / "include", ROOT_DIR / "syntaqlite-syntax" / "include"]


def _run(command: list[str]) -> None:
    print("+", shlex.join(command), flush=True)
    subprocess.run(command, check=True)


def native_static_libs() -> list[str]:
    """Returns the system libraries rustc says the static library needs."""
    proc = subprocess.run(
        ["cargo", "rustc", "--release", "-p", "syntaqlite-capi", "--features", "dynload",
         "--crate-type", "staticlib", "--", "--print", "native-static-libs"],
        cwd=ROOT_DIR, capture_output=True, text=True, check=True,
    )
    match = re.search(r"native-static-libs: (.*)", proc.stderr)
    if not match:
        raise RuntimeError("rustc did not print native-static-libs:\n" + proc.stderr)
    return shlex.split(match.group(1))


def main(argv: list[str] | None = None) -> int:
    libs = native_static_libs()
    print("native-static-libs:", " ".join(libs), flush=True)
    release = ROOT_DIR / "target" / "release"
    includes = [str(d) for d in INCLUDE_DIRS]
    with tempfile.TemporaryDirectory(prefix="syntaqlite-static-check-") as tmp:
        try:
            if platform.system() == "Windows":
                exe = Path(tmp) / "smoke.exe"
                cmd = ["cl", "/nologo", "/std:c11", "/MD"]
                for d in includes:
                    cmd += ["/I", d]
                _run(cmd + [str(SMOKE_TEST), str(release / "syntaqlite.lib"), *libs,
                            f"/Fe{exe}", f"/Fo{tmp}\\"])
            else:
                exe = Path(tmp) / "smoke"
                cmd = shlex.split(os.environ.get("CC", "cc")) + ["-std=c11"]
                for d in includes:
                    cmd += ["-I", d]
                _run(cmd + [str(SMOKE_TEST), str(release / "libsyntaqlite.a"), *libs,
                            "-o", str(exe)])
            _run([str(exe)])
        except subprocess.CalledProcessError as e:
            print(f"error: command failed with exit code {e.returncode}", file=sys.stderr)
            return 1
    return 0
