# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Main entry point for LSP declarative diff tests.

Loads `LspDiffTestBlueprint` instances from a test directory, drives them
serially against a single spawned LSP server, and either reports diffs or
rewrites expected outputs in-place when `--rebaseline` is given.
"""

from __future__ import annotations

import argparse
import fnmatch
import importlib
import inspect
import sys
import time
from glob import glob as _glob
from pathlib import Path

from python.dev.diff_tests.lsp_client import spawn_lsp
from python.dev.diff_tests.lsp_executor import execute_lsp_test
from python.dev.diff_tests.runner import (
    _rewrite_test_file,
    print_failed,
    print_failure_details,
    print_ok,
    print_run,
)
from python.dev.diff_tests.test_executor import TestResult
from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite
from python.dev.diff_tests.utils import Colors, colorize


def _load_tests(
    root_dir: Path,
    test_dir: str,
    filter_pattern: str | None,
) -> list[tuple[str, LspDiffTestBlueprint]]:
    test_base = root_dir / test_dir
    tests: list[tuple[str, LspDiffTestBlueprint]] = []
    for test_file in sorted(_glob(str(test_base / "*.py"))):
        path = Path(test_file)
        if path.name == "__init__.py":
            continue
        relative = path.relative_to(root_dir)
        module_name = str(relative.with_suffix("")).replace("/", ".")
        module = importlib.import_module(module_name)
        for _, obj in inspect.getmembers(module, inspect.isclass):
            if not (issubclass(obj, TestSuite) and obj is not TestSuite):
                continue
            suite_instance = obj()
            for name, blueprint in suite_instance.fetch():
                if isinstance(blueprint, LspDiffTestBlueprint):
                    if filter_pattern is None or fnmatch.fnmatch(
                        name, filter_pattern
                    ):
                        tests.append((name, blueprint))
    return tests


def _apply_rebaseline(
    root_dir: Path,
    test_dir: str,
    results: list[TestResult],
) -> None:
    updates = {r.name: r.actual for r in results if not r.passed and not r.error and r.actual}
    if not updates:
        return

    test_base = root_dir / test_dir
    file_updates: dict[str, list] = {}

    for test_file in sorted(_glob(str(test_base / "*.py"))):
        path = Path(test_file)
        if path.name == "__init__.py":
            continue
        relative = path.relative_to(root_dir)
        module_name = str(relative.with_suffix("")).replace("/", ".")
        module = importlib.import_module(module_name)
        for _, obj in inspect.getmembers(module, inspect.isclass):
            if not (issubclass(obj, TestSuite) and obj is not TestSuite):
                continue
            for attr in dir(obj):
                if not attr.startswith("test_"):
                    continue
                test_name = f"{obj.__name__}.{attr[5:]}"
                if test_name not in updates:
                    continue
                method = getattr(obj, attr)
                src_file = inspect.getsourcefile(method)
                file_updates.setdefault(src_file, []).append(
                    (method, updates[test_name])
                )

    rebaselined = 0
    for src_file, method_updates in file_updates.items():
        rebaselined += _rewrite_test_file(src_file, method_updates)
    print(f"Rebaselined {rebaselined} test(s) in {len(file_updates)} file(s).")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run LSP diff tests")
    parser.add_argument(
        "--binary",
        default="target/debug/syntaqlite",
        help="Path to syntaqlite binary",
    )
    parser.add_argument("--filter", help="Run only tests matching glob pattern")
    parser.add_argument(
        "--rebaseline",
        action="store_true",
        help="Rewrite expected output for failures",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Increase verbosity (-v for results, -vv for RUN markers)",
    )
    parser.add_argument("--root", default=None, help="Project root directory")
    parser.add_argument(
        "--test-dir",
        default="tests/lsp_diff_tests",
        help="Relative path to test directory",
    )
    args = parser.parse_args(argv)

    if args.root:
        root_dir = Path(args.root)
    else:
        root_dir = Path(__file__).parent.parent.parent.parent
        if not (root_dir / "Cargo.toml").exists():
            print("Error: Could not find project root.", file=sys.stderr)
            return 1

    binary = Path(args.binary)
    if not binary.is_absolute():
        binary = root_dir / binary

    try:
        tests = _load_tests(root_dir, args.test_dir, args.filter)
    except ImportError as e:
        print(f"Error loading tests: {e}", file=sys.stderr)
        return 1

    if not tests:
        print("No tests to run.")
        return 0

    suites = set(name.split(".")[0] for name, _ in tests)
    verbosity = args.verbose

    if verbosity >= 1:
        print(
            f"[==========] Running {len(tests)} tests from {len(suites)} test suites."
        )

    # One LSP subprocess serves all tests; each test uses a unique URI and
    # closes its document afterwards, so state doesn't bleed.
    client = spawn_lsp(binary)
    start_time = time.time()
    results: list[TestResult] = []
    failed_tests: list[str] = []

    try:
        for name, blueprint in tests:
            if verbosity >= 2:
                print_run(name)
            result = execute_lsp_test(client, name, blueprint)
            results.append(result)
            if result.passed:
                if verbosity >= 1:
                    print_ok(result.name, result.elapsed_ms)
            else:
                if verbosity >= 1:
                    print_failed(result.name, result.elapsed_ms)
                if not args.rebaseline:
                    print_failure_details(result)
                failed_tests.append(result.name)
    finally:
        client.shutdown()

    elapsed_ms = int((time.time() - start_time) * 1000)
    passed = sum(1 for r in results if r.passed)
    failed = len(failed_tests)

    if verbosity >= 1:
        print(
            f"[==========] {len(results)} tests from {len(suites)} test suites ran. "
            f"({elapsed_ms} ms total)"
        )

    if passed > 0:
        msg = colorize("[  PASSED  ]", Colors.GREEN)
        print(f"{msg} {passed} tests.")

    if failed > 0:
        if args.rebaseline:
            _apply_rebaseline(root_dir, args.test_dir, results)
            return 0
        msg = colorize("[  FAILED  ]", Colors.RED)
        print(f"{msg} {failed} tests, listed below:")
        for name in failed_tests:
            print(f"{msg} {name}")
        print()
        print(f" {failed} FAILED TESTS")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
