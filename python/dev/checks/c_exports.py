#!/usr/bin/env python3
# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check that the C library exports exactly the functions the headers declare.

The shared library exports the external functions listed in
tests/c_api/api-manifest.json (see python/tools/build_c_library.py). If a
function is renamed or removed from the headers but not the manifest, the
library fails to link; if one is added to the headers but not the manifest, it
is silently left out of the library. Both only show up in release builds, so
compare the two here.

Usage:
    python3 python/dev/checks/c_exports.py   # exits 0 on success
    tools/check-c-exports                    # thin wrapper
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).parent.parent.parent.parent  # repo root

MANIFEST = ROOT / "tests" / "c_api" / "api-manifest.json"
PUBLIC_INCLUDE_DIRS = [
    ROOT / "syntaqlite-syntax" / "include",
    ROOT / "syntaqlite" / "include",
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


def main() -> int:
    declared = declared_functions()
    exported = exported_functions()
    missing = sorted(set(declared) - exported)
    stale = sorted(exported - set(declared))
    if not missing and not stale:
        return 0

    manifest = MANIFEST.relative_to(ROOT)
    print(f"check-c-exports: {manifest} is out of date with the public headers.")
    for name in missing:
        header = declared[name].relative_to(ROOT)
        print(f"  declared in {header} but not exported: {name}")
    for name in stale:
        print(f"  exported but no longer declared: {name}")
    print(f"Update the \"functions\" (and any affected types) in {manifest}.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
