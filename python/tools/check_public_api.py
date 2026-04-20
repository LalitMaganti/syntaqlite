# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Verify or regenerate the public-API golden files for library crates.

All crates are checked in parallel. Uses a dedicated Cargo target directory
so nightly rustdoc builds don't invalidate the main workspace's stable cache.

Usage:
    tools/check-public-api               — check all crates
    tools/check-public-api --rebaseline  — regenerate all golden files
"""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed

ROOT_DIR: str = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, ROOT_DIR)

from python.tools.cargo_slots import acquire_slot

_BLUE: str = "\033[1;34m"
_RED: str = "\033[1;31m"
_GREEN: str = "\033[1;32m"
_RESET: str = "\033[0m"

_GOLDEN_DIR: str = os.path.join(ROOT_DIR, "tests", "public-api")

# (crate_name, features_or_None)
_CRATES: list[tuple[str, str | None]] = [
    ("syntaqlite",            "fmt,analysis,sqlite,lsp,experimental-embedded,serde-json"),
    ("syntaqlite-syntax",     None),
    ("syntaqlite-buildtools", None),
]


def _cargo_path() -> str:
    return os.path.join(ROOT_DIR, "tools", "cargo")


def _verbose_log(verbose: bool, msg: str) -> None:
    if verbose:
        print(msg, file=sys.stderr, flush=True)


def _run_public_api(crate_name: str, features: str | None, verbose: bool) -> tuple[bool, str]:
    """Run cargo public-api for *crate_name* and return (ok, output_or_error)."""
    cmd = [sys.executable, _cargo_path(), "public-api", "-p", crate_name]
    if features:
        cmd += ["--features", features]
    cmd += ["--omit", "blanket-impls", "--omit", "auto-trait-impls", "--omit", "auto-derived-impls"]
    with acquire_slot() as target_dir:
        env = {**os.environ, "CARGO_TARGET_DIR": target_dir}
        _verbose_log(verbose, f"[check-public-api] {crate_name}: {shlex.join(cmd)}")
        _verbose_log(verbose, f"[check-public-api] {crate_name}: CARGO_TARGET_DIR={target_dir}")
        proc = subprocess.run(
            cmd, cwd=ROOT_DIR, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        )
        if proc.returncode != 0:
            return False, (
                f"{_RED}public-api command failed for {crate_name} (exit {proc.returncode}){_RESET}\n"
                f"{proc.stderr}\n"
            )
        _verbose_log(
            verbose,
            f"[check-public-api] {crate_name}: success, stdout lines={len(proc.stdout.splitlines())}",
        )
        lines = sorted(dict.fromkeys(proc.stdout.splitlines()))
        return True, "\n".join(lines) + ("\n" if lines else "")


def _check_crate(crate_name: str, features: str | None, verbose: bool) -> tuple[bool, str]:
    """Check one crate against its golden file. Returns (ok, message)."""
    golden = os.path.join(_GOLDEN_DIR, f"{crate_name}.txt")
    if not os.path.exists(golden):
        return False, (
            f"{_RED}MISSING golden file: public-api/{crate_name}.txt{_RESET}\n"
            f"  Run: tools/check-public-api --rebaseline\n"
        )

    ok, actual = _run_public_api(crate_name, features, verbose)
    if not ok:
        return False, actual

    with open(golden) as f:
        expected = f.read()

    if actual == expected:
        return True, f"{_BLUE}==> {crate_name}: OK{_RESET}\n"

    with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as tmp:
        tmp.write(actual)
        tmp_path = tmp.name
    try:
        diff = subprocess.run(
            ["diff", "-u", golden, tmp_path],
            capture_output=True, text=True,
        ).stdout
    finally:
        os.unlink(tmp_path)

    return False, (
        f"{_RED}Public API changed: {crate_name}{_RESET}\n"
        + diff
        + f"\n  If intentional, run: tools/check-public-api --rebaseline\n"
    )


def _rebaseline_crate(crate_name: str, features: str | None, verbose: bool) -> tuple[bool, str]:
    """Regenerate the golden file for one crate. Returns (ok, message)."""
    ok, actual = _run_public_api(crate_name, features, verbose)
    if not ok:
        return False, actual

    golden = os.path.join(_GOLDEN_DIR, f"{crate_name}.txt")
    os.makedirs(_GOLDEN_DIR, exist_ok=True)
    with open(golden, "w") as f:
        f.write(actual)
    lines = actual.count("\n")
    return True, f"Rebaselined {crate_name} ({lines} lines)\n"


def _run_all(rebaseline: bool, verbose: bool) -> bool:
    worker = _rebaseline_crate if rebaseline else _check_crate

    results: list[tuple[bool, str] | None] = [None] * len(_CRATES)
    with ThreadPoolExecutor(max_workers=len(_CRATES)) as pool:
        futures = {
            pool.submit(worker, name, feats, verbose): i
            for i, (name, feats) in enumerate(_CRATES)
        }
        for future in as_completed(futures):
            results[futures[future]] = future.result()

    all_ok = True
    for ok, msg in results:
        sys.stdout.write(msg)
        if not ok:
            all_ok = False

    return all_ok


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check or regenerate public-API golden files.",
    )
    parser.add_argument(
        "--rebaseline", action="store_true",
        help="Regenerate all golden files instead of checking them",
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Print per-crate command/debug logs (including command failures).",
    )
    args = parser.parse_args()

    ok = _run_all(args.rebaseline, args.verbose)
    if ok and args.rebaseline:
        print(f"{_GREEN}All golden files rebaselined.{_RESET}")
    elif not ok:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
