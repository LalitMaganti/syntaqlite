# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Main test runner logic."""

import argparse
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from typing import List, Optional

from python.syntaqlite.diff_tests.test_executor import TestResult, execute_test
from python.syntaqlite.diff_tests.test_loader import load_all_tests
from python.syntaqlite.diff_tests.utils import Colors, colorize, format_diff


def _run_single_test(args: tuple) -> TestResult:
    """Worker function for parallel test execution."""
    binary, name, blueprint = args
    return execute_test(Path(binary), name, blueprint)


def print_run(name: str) -> None:
    """Print test start marker."""
    print(f"[ RUN      ] {name}")


def print_ok(name: str, elapsed_ms: int) -> None:
    """Print test pass marker."""
    ok = colorize("[       OK ]", Colors.GREEN)
    print(f"{ok} {name} ({elapsed_ms} ms)")


def print_failed(name: str, elapsed_ms: int) -> None:
    """Print test fail marker."""
    failed = colorize("[  FAILED  ]", Colors.RED)
    print(f"{failed} {name} ({elapsed_ms} ms)")


def print_failure_details(result: TestResult, rebaseline: bool = False) -> None:
    """Print failure details."""
    if result.error:
        print(f"Error: {result.error}")
    else:
        print(f"SQL: {result.sql}")
        for line in format_diff(result.expected, result.actual):
            print(line)

        if rebaseline:
            print()
            print("Suggested update:")
            print('            out="""')
            for line in result.actual.splitlines():
                print(f"                {line}")
            print('            """,')


def main(argv: Optional[List[str]] = None) -> int:
    """Main entry point for the test runner."""
    parser = argparse.ArgumentParser(description='Run AST diff tests')
    parser.add_argument('--binary', default='out/debug/ast_test',
                        help='Path to ast_test binary')
    parser.add_argument('--filter', help='Run only tests matching pattern')
    parser.add_argument('--jobs', '-j', type=int, default=None,
                        help='Number of parallel jobs')
    parser.add_argument('--rebaseline', action='store_true',
                        help='Print suggested output for failures')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Increase verbosity (-v for results, -vv for RUN markers)')
    parser.add_argument('--root', default=None,
                        help='Project root directory')

    args = parser.parse_args(argv)

    # Determine project root
    if args.root:
        root_dir = Path(args.root)
    else:
        root_dir = Path(__file__).parent.parent.parent.parent
        if not (root_dir / '.gn').exists():
            print(f"Error: Could not find project root.", file=sys.stderr)
            return 1

    # Resolve binary path
    binary = Path(args.binary)
    if not binary.is_absolute():
        binary = root_dir / binary

    # Load tests
    try:
        tests = load_all_tests(root_dir, args.filter)
    except ImportError as e:
        print(f"Error loading tests: {e}", file=sys.stderr)
        return 1

    if not tests:
        print("No tests to run.")
        return 0

    # Count test suites
    suites = set(name.split('.')[0] for name, _ in tests)

    verbosity = args.verbose

    if verbosity >= 1:
        print(f"[==========] Running {len(tests)} tests from {len(suites)} test suites.")

    # Run tests
    start_time = time.time()
    results: List[TestResult] = []
    failed_tests: List[str] = []

    test_args = [(str(binary), name, blueprint) for name, blueprint in tests]

    # Submit all tests to the pool for parallel execution, then iterate
    # futures in submission order so output is serialized per-test.
    max_workers = args.jobs if args.jobs else (os.cpu_count() or 1)
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futures = [executor.submit(_run_single_test, arg) for arg in test_args]

        for future in futures:
            result = future.result()
            results.append(result)
            if result.passed:
                if verbosity >= 2:
                    print_run(result.name)
                if verbosity >= 1:
                    print_ok(result.name, result.elapsed_ms)
            else:
                if verbosity >= 2:
                    print_run(result.name)
                if verbosity >= 1:
                    print_failed(result.name, result.elapsed_ms)
                print_failure_details(result, args.rebaseline)
                failed_tests.append(result.name)

    elapsed_ms = int((time.time() - start_time) * 1000)

    # Summary
    passed = sum(1 for r in results if r.passed)
    failed = len(failed_tests)

    if verbosity >= 1:
        print(f"[==========] {len(results)} tests from {len(suites)} test suites ran. ({elapsed_ms} ms total)")

    if passed > 0:
        msg = colorize("[  PASSED  ]", Colors.GREEN)
        print(f"{msg} {passed} tests.")

    if failed > 0:
        msg = colorize("[  FAILED  ]", Colors.RED)
        print(f"{msg} {failed} tests, listed below:")
        for name in failed_tests:
            print(f"{msg} {name}")
        print()
        print(f" {failed} FAILED TESTS")

    return 0 if failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
