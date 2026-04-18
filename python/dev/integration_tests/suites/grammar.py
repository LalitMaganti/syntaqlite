# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Grammar token-ordering invariant suite.

Generates full amalgamations for the base SQLite dialect and each registered
extension, then verifies:
  - Shared tokens have identical IDs in every dialect.
  - Extension-only tokens have IDs strictly greater than the max base token ID.

This guards against the class of bug where an extension .y file sorts before a
base file in the grammar concatenation, shifting all subsequent token IDs.
"""

import re
import subprocess
import tempfile
from pathlib import Path

from python.dev.integration_tests.suite import SuiteContext

NAME = "grammar"
DESCRIPTION = "Grammar token-ID ordering invariants across all dialect extensions"

TOKEN_DEFINE_RE = re.compile(r"^#define\s+SYNTAQLITE_TK_(\w+)\s+(\d+)", re.MULTILINE)

# Extension dialects to check. Add an entry here when a new dialect is added.
_EXTENSIONS = [
    {
        "name": "perfetto",
        "actions_dir_rel": "dialects/perfetto/actions",
        "nodes_dir_rel": "dialects/perfetto/nodes",
    },
]


def _parse_token_ids(header_text: str) -> dict[str, int]:
    return {m.group(1): int(m.group(2)) for m in TOKEN_DEFINE_RE.finditer(header_text)}


def _generate_full_amalg(
    binary: Path,
    name: str,
    output_dir: Path,
    actions_dir: str | None = None,
    nodes_dir: str | None = None,
) -> None:
    cmd = [str(binary), "dialect", "generate", "--name", name]
    if actions_dir:
        cmd += ["--actions-dir", actions_dir]
    if nodes_dir:
        cmd += ["--nodes-dir", nodes_dir]
    cmd += ["--output-type", "full", "--output-dir", str(output_dir)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Failed to generate amalgamation for '{name}':\n{proc.stderr}"
        )


def _check_extension(
    binary: Path,
    base_ids: dict[str, int],
    max_base_id: int,
    ext: dict,
    root_dir: Path,
    verbose: bool,
) -> list[str]:
    name = ext["name"]
    actions_dir = str(root_dir / ext["actions_dir_rel"])
    nodes_dir = str(root_dir / ext["nodes_dir_rel"])
    errors: list[str] = []

    with tempfile.TemporaryDirectory(prefix=f"syntaqlite_grammar_{name}_") as tmp:
        tmp_path = Path(tmp)
        try:
            _generate_full_amalg(binary, name, tmp_path, actions_dir, nodes_dir)
        except RuntimeError as e:
            return [str(e)]

        header = tmp_path / f"syntaqlite_{name}.h"
        if not header.exists():
            return [f"Expected header not found: {header}"]
        ext_ids = _parse_token_ids(header.read_text())

    if not ext_ids:
        return [f"No SYNTAQLITE_TK_* defines found in syntaqlite_{name}.h"]

    for tok, base_id in base_ids.items():
        if tok in ext_ids and ext_ids[tok] != base_id:
            errors.append(
                f"  SYNTAQLITE_TK_{tok}: sqlite={base_id}, {name}={ext_ids[tok]}"
                f" (ID shifted — extension .y sorted before base .y files)"
            )

    for tok, ext_id in ext_ids.items():
        if tok not in base_ids:
            if ext_id <= max_base_id:
                errors.append(
                    f"  Extension-only SYNTAQLITE_TK_{tok}={ext_id}"
                    f" is not above max base ID {max_base_id}"
                )
            elif verbose:
                print(f"    ext-only: SYNTAQLITE_TK_{tok}={ext_id} (max_base={max_base_id}) OK")

    return errors


def run(ctx: SuiteContext) -> int:
    verbose = ctx.verbose >= 1

    print("Generating sqlite amalgamation...", end=" ", flush=True)
    with tempfile.TemporaryDirectory(prefix="syntaqlite_grammar_sqlite_") as sqlite_tmp:
        try:
            _generate_full_amalg(ctx.binary, "sqlite", Path(sqlite_tmp))
        except RuntimeError as e:
            print("FAILED")
            print(str(e))
            return 1

        header = Path(sqlite_tmp) / "syntaqlite_sqlite.h"
        if not header.exists():
            print("FAILED")
            print(f"Expected header not found: {header}")
            return 1
        base_ids = _parse_token_ids(header.read_text())

    if not base_ids:
        print("FAILED")
        print("No SYNTAQLITE_TK_* defines found in sqlite amalgam header")
        return 1

    max_base_id = max(base_ids.values())
    print(f"OK ({len(base_ids)} tokens, max id={max_base_id})")

    if verbose:
        for tok, tid in sorted(base_ids.items(), key=lambda x: x[1]):
            print(f"  SYNTAQLITE_TK_{tok}={tid}")

    all_errors: dict[str, list[str]] = {}
    for ext in _EXTENSIONS:
        name = ext["name"]
        print(f"Checking extension '{name}'...", end=" ", flush=True)
        errors = _check_extension(ctx.binary, base_ids, max_base_id, ext, ctx.root_dir, verbose)
        if errors:
            print("FAILED")
            all_errors[name] = errors
        else:
            print("OK")

    if all_errors:
        print()
        print("FAILURES:")
        for name, errors in all_errors.items():
            print(f"  [{name}]")
            for err in errors:
                print(err)
        return 1

    print(f"All grammar checks passed ({len(_EXTENSIONS)} extension(s) verified).")
    return 0
