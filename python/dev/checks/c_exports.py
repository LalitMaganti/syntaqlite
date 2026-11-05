#!/usr/bin/env python3
# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check that the C library exports exactly the functions the headers declare.

The shared library exports the functions in syntaqlite-capi/exports.txt (see
syntaqlite-capi/build.rs), which must match the external functions in
tests/c_api/api-manifest.json. If a function is renamed or removed from the
headers but not the list, the library fails to link; if one is added to the
headers but not the list, it is silently left out of the library. Compare all
three here.

With --library, also check that a built shared library exports every function.

Usage:
    python3 python/dev/checks/c_exports.py   # exits 0 on success
    tools/check-c-exports                    # thin wrapper
    tools/check-c-exports --library target/release/libsyntaqlite.so
"""

import argparse
import json
import platform
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).parent.parent.parent.parent  # repo root

MANIFEST = ROOT / "tests" / "c_api" / "api-manifest.json"
EXPORTS = ROOT / "syntaqlite-capi" / "exports.txt"
PUBLIC_INCLUDE_DIRS = [
    ROOT / "syntaqlite-syntax" / "include",
    ROOT / "syntaqlite-capi" / "include",
]

_COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
_DECLARATION = re.compile(r"\bSYNTAQLITE_API\b[^;{(#]*?\b(syntaqlite_\w+)\s*\(", re.S)


def declared_functions() -> dict[str, Path]:
    """Maps each function declared SYNTAQLITE_API in a public header to it."""
    out: dict[str, Path] = {}
    for include_dir in PUBLIC_INCLUDE_DIRS:
        for header in sorted(include_dir.rglob("*.h")):
            text = _COMMENT.sub("", header.read_text(encoding="utf-8"))
            for name in _DECLARATION.findall(text):
                out[name] = header
    return out


def exported_functions() -> set[str]:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    return {
        name
        for name, declaration in manifest["functions"].items()
        if declaration["linkage"] == "external"
    }


def listed_exports() -> set[str]:
    lines = EXPORTS.read_text(encoding="utf-8").splitlines()
    return {line.strip() for line in lines if line.strip() and not line.startswith("#")}


def library_exports(library: Path) -> set[str]:
    """Returns the functions a shared library exports."""
    if platform.system() == "Windows":
        out = subprocess.run(["dumpbin", "/exports", str(library)],
                             capture_output=True, text=True, check=True).stdout
        return set(re.findall(r"^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\w+)", out, re.M))
    flags = ["-gU"] if platform.system() == "Darwin" else ["-D", "--defined-only"]
    out = subprocess.run(["nm", *flags, str(library)],
                         capture_output=True, text=True, check=True).stdout
    names = {line.split()[-1] for line in out.splitlines() if line.strip()}
    return {n[1:] if n.startswith("_") and platform.system() == "Darwin" else n for n in names}


def check_sources() -> bool:
    declared = declared_functions()
    exported = exported_functions()
    missing = sorted(set(declared) - exported)
    stale = sorted(exported - set(declared))
    ok = True
    if missing or stale:
        manifest = MANIFEST.relative_to(ROOT)
        print(f"check-c-exports: {manifest} is out of date with the public headers.")
        for name in missing:
            header = declared[name].relative_to(ROOT)
            print(f"  declared in {header} but not exported: {name}")
        for name in stale:
            print(f"  exported but no longer declared: {name}")
        print(f"Update the \"functions\" (and any affected types) in {manifest}.")
        ok = False
    listed = listed_exports()
    if listed != exported:
        print(f"check-c-exports: {EXPORTS.relative_to(ROOT)} does not match the manifest.")
        for name in sorted(exported - listed):
            print(f"  missing from exports.txt: {name}")
        for name in sorted(listed - exported):
            print(f"  in exports.txt but not the manifest: {name}")
        ok = False
    return ok


def check_library(library: Path) -> bool:
    missing = sorted(exported_functions() - library_exports(library))
    if missing:
        print(f"check-c-exports: {library} does not export {len(missing)} public functions:")
        for name in missing:
            print(f"  {name}")
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--library", type=Path, help="Built shared library to check.")
    args = parser.parse_args()
    ok = check_sources()
    if args.library:
        ok = check_library(args.library) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
