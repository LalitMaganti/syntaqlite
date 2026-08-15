#!/usr/bin/env python3
# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build and run syntaqlite-buildtools to generate parser and tokenizer.

Multi-stage bootstrap pipeline:
  Stage 1  (--extract): Extract C fragments from raw SQLite source.
  Stage 1b (always):    Generate built-in catalog Rust modules.
  Stage 2  (always):    Generate base syntaqlite crate C + Rust code.

The bootstrap tool (syntaqlite-buildtools) has no dependency on any generated
files, so it can be built from a completely clean checkout.

Usage:
    python3 python/tools/run_codegen.py              # Stage 1b + 2
    python3 python/tools/run_codegen.py --extract     # Stage 1 + 1b + 2
    tools/run-codegen                             # Stage 1b + 2
    tools/run-codegen --extract                   # Stage 1 + 1b + 2
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build and run syntaqlite-buildtools to generate parser and tokenizer."
    )
    parser.add_argument(
        "--extract",
        action="store_true",
        help="Run Stage 1: extract C fragments from raw SQLite source",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Print progress messages",
    )
    args = parser.parse_args()

    def log(msg: str) -> None:
        if args.verbose:
            print(msg)

    project_root = Path(__file__).parent.parent.parent
    sqlite_src = project_root / "third_party" / "src" / "sqlite"
    dialect_crate_dir = project_root / "syntaqlite-syntax"
    shared_crate_dir = project_root / "syntaqlite-syntax"
    buildtools_dir = project_root / "syntaqlite-buildtools"
    actions_dir = buildtools_dir / "parser-actions"
    nodes_dir = buildtools_dir / "parser-nodes"
    vendored_dir = project_root / "syntaqlite-buildtools" / "sqlite-vendored"

    do_extract = args.extract

    # Validate input files
    if not actions_dir.is_dir():
        print(f"Error: Parser actions directory not found at {actions_dir}", file=sys.stderr)
        return 1

    if do_extract:
        if not sqlite_src.is_dir():
            print(f"Error: SQLite source not found at {sqlite_src}", file=sys.stderr)
            return 1

        # Stage 1: Extract SQLite fragments, vendor sources, generate base_files_tables.rs
        log("Stage 1: Extracting SQLite fragments and vendoring sources...")
        result = subprocess.run(
            [
                "cargo", "run", "--release", "-p", "syntaqlite-buildtools",
                "--",
                "sqlite-extract",
                "--sqlite-src", str(sqlite_src),
                "--output-dir", str(vendored_dir),
                "--actions-dir", str(actions_dir),
                "--nodes-dir", str(nodes_dir),
            ],
            cwd=project_root,
            capture_output=not args.verbose,
        )
        if result.returncode != 0:
            if not args.verbose and result.stderr:
                sys.stderr.buffer.write(result.stderr)
            print("Stage 1 extraction failed", file=sys.stderr)
            return result.returncode

    # Build the bootstrap tool for stages 1b and 2.
    # syntaqlite-buildtools has no dependency on any generated files, so this
    # works even from a completely clean checkout.
    log("Building syntaqlite-buildtools...")
    result = subprocess.run(
        ["cargo", "build", "--release", "-p", "syntaqlite-buildtools"],
        cwd=project_root,
        capture_output=not args.verbose,
    )
    if result.returncode != 0:
        if not args.verbose and result.stderr:
            sys.stderr.buffer.write(result.stderr)
        print("Build failed", file=sys.stderr)
        return result.returncode

    tools_bin = project_root / "target" / "release" / "syntaqlite-buildtools"

    # Stage 1b: Generate built-in catalogs and cflag Rust modules.
    # Output paths are hardcoded in the Rust binary.
    functions_json = vendored_dir / "data" / "functions.json"
    relations_json = vendored_dir / "data" / "relations.json"
    version_cflags_json = vendored_dir / "data" / "version_cflags.json"

    log("Stage 1b: Generating built-in catalogs and cflag modules...")
    result = subprocess.run(
        [
            str(tools_bin),
            "codegen-sqlite-parser",
            "--functions-json", str(functions_json),
            "--relations-json", str(relations_json),
            "--version-cflags-json", str(version_cflags_json),
        ],
        cwd=project_root,
        capture_output=not args.verbose,
    )
    if result.returncode != 0:
        if not args.verbose and result.stderr:
            sys.stderr.buffer.write(result.stderr)
        print("Stage 1b: codegen-sqlite-parser failed", file=sys.stderr)
        return result.returncode

    # Stage 2: Generate base SQLite dialect C + Rust code.
    log("Stage 2: Generating base SQLite dialect...")
    result = subprocess.run(
        [
            str(tools_bin),
            "codegen-sqlite",
            "--actions-dir", str(actions_dir),
            "--nodes-dir", str(nodes_dir),
        ],
        capture_output=not args.verbose,
    )

    if result.returncode != 0:
        if not args.verbose and result.stderr:
            sys.stderr.buffer.write(result.stderr)
        print("Codegen failed", file=sys.stderr)
        return result.returncode

    # Format generated C code
    format_c = project_root / "tools" / "format-c"
    result = subprocess.run([str(format_c)], cwd=project_root, capture_output=not args.verbose)
    if result.returncode != 0:
        if not args.verbose and result.stderr:
            sys.stderr.buffer.write(result.stderr)
        print("format-c failed", file=sys.stderr)
        return result.returncode

    # Format generated Rust code
    result = subprocess.run(
        ["cargo", "fmt", "--all"],
        cwd=project_root,
        capture_output=not args.verbose,
    )
    if result.returncode != 0:
        if not args.verbose and result.stderr:
            sys.stderr.buffer.write(result.stderr)
        print("cargo fmt failed", file=sys.stderr)
        return result.returncode

    print("Codegen complete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
