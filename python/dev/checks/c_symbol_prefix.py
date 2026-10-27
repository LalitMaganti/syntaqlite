#!/usr/bin/env python3
# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check that SYNTAQLITE_SYMBOL_PREFIX renames every global C symbol.

Compiles each C source in syntaqlite-syntax/csrc with a test prefix and
fails if any global symbol it defines, or any reference to one, is left
unprefixed. That happens when a new global symbol is not added to
include/syntaqlite/symbol_prefix.h.

Usage:
    python3 python/dev/checks/c_symbol_prefix.py   # exits 0 on success
    tools/check-c-symbol-prefix                    # thin wrapper
"""

import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parent.parent.parent.parent  # repo root
SYNTAX = ROOT / "syntaqlite-syntax"
HEADER = SYNTAX / "include" / "syntaqlite" / "symbol_prefix.h"
PREFIX = "synq_prefix_check_"


def _symbols(obj: Path) -> list[tuple[str, str]]:
    """(type, name) for each global symbol in `obj`."""
    out = subprocess.run(
        ["nm", "-g", str(obj)], capture_output=True, text=True, check=True,
    ).stdout
    syms = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        kind, name = parts[-2], parts[-1]
        if sys.platform == "darwin":
            name = name.removeprefix("_")
        syms.append((kind, name))
    return syms


def main() -> int:
    cc = shlex.split(os.environ.get("CC", "cc"))
    sources = sorted((SYNTAX / "csrc").rglob("*.c"))
    defined: set[str] = set()
    undefined: set[str] = set()
    with tempfile.TemporaryDirectory(prefix="syntaqlite-prefix-") as tmp:
        for i, src in enumerate(sources):
            obj = Path(tmp) / f"{i}.o"
            subprocess.run(
                cc + [
                    "-c", str(src), "-o", str(obj),
                    f"-I{SYNTAX}", f"-I{SYNTAX / 'include'}",
                    "-DSYNTAQLITE_FMT", "-DSYNTAQLITE_VALIDATION",
                    f"-DSYNTAQLITE_SYMBOL_PREFIX={PREFIX}",
                    "-w",
                ],
                check=True,
            )
            for kind, name in _symbols(obj):
                (undefined if kind == "U" else defined).add(name)

    unprefixed = sorted(n for n in defined if not n.startswith(PREFIX))
    ours = {n.removeprefix(PREFIX) for n in defined}
    stray = sorted(n for n in undefined if n in ours)
    if not unprefixed and not stray:
        return 0

    header = HEADER.relative_to(ROOT)
    print(f"check-c-symbol-prefix: {header} is missing global symbols.")
    for name in unprefixed:
        print(f"  defined without prefix: {name}")
    for name in stray:
        print(f"  referenced without prefix: {name}")
    print(f"Add a #define for each to {header}.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
