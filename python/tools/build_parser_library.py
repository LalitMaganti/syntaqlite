# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build prebuilt libsyntaqlite-parser libraries from its C source.

Compiles libsyntaqlite_parser.c into a static and a shared library for the
host platform, then links tests/c_api/parser_library_smoke.c against each
and runs it, unless --no-run is given (for cross-compiled Windows builds).

Output names follow libsyntaqlite's:

    Linux    libsyntaqlite_parser.a    libsyntaqlite_parser.so
    macOS    libsyntaqlite_parser.a    libsyntaqlite_parser.dylib
    Windows  syntaqlite_parser.lib     syntaqlite_parser.dll + syntaqlite_parser.dll.lib

On Windows the libraries are built with clang-cl and the smoke test with
MSVC's cl, as most users would. Both expect the MSVC environment for the
target architecture to be set up already.
"""

from __future__ import annotations

import argparse
import os
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[2]
SMOKE_TEST = ROOT_DIR / "tests" / "c_api" / "parser_library_smoke.c"
EXPORT_DEFINES = ["-DSYNTAQLITE_SHARED", "-DSYNTAQLITE_BUILDING"]


def _run(command: list[str], cwd: Path | None = None) -> None:
    print("+", shlex.join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def _build_unix(src: Path, out: Path, work: Path, run: bool) -> None:
    cc = shlex.split(os.environ.get("CC", "cc"))
    source = src / "libsyntaqlite_parser.c"
    is_mac = platform.system() == "Darwin"

    obj = work / "libsyntaqlite_parser.o"
    _run(cc + ["-O2", "-fPIC", "-c", str(source), "-o", str(obj)])
    _run(["ar", "rcs", str(out / "libsyntaqlite_parser.a"), str(obj)])

    if is_mac:
        shared = out / "libsyntaqlite_parser.dylib"
        soname = ["-Wl,-install_name,@rpath/libsyntaqlite_parser.dylib"]
    else:
        shared = out / "libsyntaqlite_parser.so"
        soname = ["-Wl,-soname,libsyntaqlite_parser.so"]
    _run(cc + ["-O2", "-fPIC", "-fvisibility=hidden", "-shared", *EXPORT_DEFINES,
               str(source), "-o", str(shared), *soname])

    static_exe = work / "smoke_static"
    _run(cc + ["-I", str(src), str(SMOKE_TEST), str(out / "libsyntaqlite_parser.a"),
               "-o", str(static_exe)])
    shared_exe = work / "smoke_shared"
    _run(cc + ["-I", str(src), str(SMOKE_TEST),
               "-L", str(out), "-lsyntaqlite_parser", f"-Wl,-rpath,{out}",
               "-o", str(shared_exe)])
    if run:
        _run([str(static_exe)])
        _run([str(shared_exe)])


def _build_windows(src: Path, out: Path, work: Path, target: str | None, run: bool) -> None:
    cl = ["clang-cl", "/nologo", "/O2", "/MD"]
    if target:
        cl.append(f"--target={target}")
    source = src / "libsyntaqlite_parser.c"

    obj = work / "libsyntaqlite_parser.obj"
    _run(cl + ["/c", str(source), f"/Fo{obj}"])
    _run(["lib", "/nologo", f"/OUT:{out / 'syntaqlite_parser.lib'}", str(obj)])

    dll = out / "syntaqlite_parser.dll"
    _run(cl + ["/LD", *EXPORT_DEFINES, str(source), f"/Fe{dll}", f"/Fo{work}\\",
               "/link", f"/IMPLIB:{out / 'syntaqlite_parser.dll.lib'}"])
    for leftover in out.glob("syntaqlite_parser.exp"):
        leftover.unlink()

    msvc = ["cl", "/nologo", "/std:c11", "/MD"]
    static_exe = work / "smoke_static.exe"
    _run(msvc + ["/I", str(src), str(SMOKE_TEST), str(out / "syntaqlite_parser.lib"),
                 f"/Fe{static_exe}", f"/Fo{work}\\"])
    shared_dir = work / "shared"
    shared_dir.mkdir()
    shared_exe = shared_dir / "smoke_shared.exe"
    _run(msvc + ["/I", str(src), str(SMOKE_TEST), str(out / "syntaqlite_parser.dll.lib"),
                 f"/Fe{shared_exe}", f"/Fo{work}\\"])
    shutil.copy(dll, shared_dir)
    if run:
        _run([str(static_exe)])
        _run([str(shared_exe)])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source-dir", type=Path, required=True,
                        help="Directory containing libsyntaqlite_parser.{c,h}.")
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="Directory to write the libraries to.")
    parser.add_argument("--target", help="clang-cl target triple, for Windows cross builds.")
    parser.add_argument("--no-run", action="store_true",
                        help="Link the smoke test but do not run it.")
    args = parser.parse_args(argv)

    src = args.source_dir.resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    try:
        with tempfile.TemporaryDirectory(prefix="syntaqlite-parser-build-") as tmp:
            if platform.system() == "Windows":
                _build_windows(src, out, Path(tmp), args.target, not args.no_run)
            else:
                _build_unix(src, out, Path(tmp), not args.no_run)
    except subprocess.CalledProcessError as e:
        print(f"error: command failed with exit code {e.returncode}", file=sys.stderr)
        return 1
    print("built:", ", ".join(sorted(p.name for p in out.iterdir())))
    return 0
