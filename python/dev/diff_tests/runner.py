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

from python.dev.diff_tests.test_executor import TestResult, execute_test
from python.dev.diff_tests.test_loader import load_all_tests
from python.dev.diff_tests.utils import Colors, colorize, format_diff


def _run_single_test(args: tuple) -> TestResult:
    """Worker function for parallel test execution."""
    binary, subcommand, name, blueprint = args
    # Lineage reads stdout but exits non-zero when error records are emitted;
    # accept that like `analyze` accepts non-zero exits.
    lineage = subcommand is not None and subcommand.split()[0] == "lineage"
    return execute_test(
        Path(binary), name, blueprint,
        subcommand=subcommand,
        use_stderr=(subcommand == "analyze"),
        ignore_exit_code=lineage,
    )


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


def print_failure_details(result: TestResult) -> None:
    """Print failure details."""
    if result.error:
        print(f"Error: {result.error}")
    else:
        print(f"SQL: {result.sql}")
        for line in format_diff(result.expected, result.actual):
            print(line)


def _apply_rebaseline(root_dir: Path, test_dir: str, results: List[TestResult]) -> None:
    """Rewrite failing test expectations in-place."""
    import importlib
    import inspect
    from glob import glob as _glob

    # Collect rebaseline-able failures (skip error/timeout results)
    updates = {r.name: r.actual for r in results if not r.passed and not r.error and r.actual}
    if not updates:
        return

    # Discover test suites to obtain method objects
    test_base = root_dir / test_dir
    # file_path -> [(method_obj, actual_output)]
    file_updates: dict = {}

    for test_file in sorted(_glob(str(test_base / "*.py"))):
        path = Path(test_file)
        if path.name == "__init__.py":
            continue
        relative = path.relative_to(root_dir)
        module_name = str(relative.with_suffix("")).replace("/", ".")
        module = importlib.import_module(module_name)

        from python.dev.diff_tests.testing import TestSuite
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
                if src_file not in file_updates:
                    file_updates[src_file] = []
                file_updates[src_file].append((method, updates[test_name]))

    rebaselined = 0
    for src_file, method_updates in file_updates.items():
        rebaselined += _rewrite_test_file(src_file, method_updates)

    print(f"Rebaselined {rebaselined} test(s) in {len(file_updates)} file(s).")


def _rewrite_test_file(file_path: str, updates: list) -> int:
    """Rewrite out= blocks in a single test file. Returns count of rewrites."""
    import inspect

    with open(file_path) as f:
        lines = f.readlines()

    # Collect (0-indexed start line, method, actual) and sort bottom-to-top
    # so replacements don't shift line numbers for earlier entries.
    located = []
    for method, actual in updates:
        _, start_line = inspect.getsourcelines(method)
        located.append((start_line - 1, method, actual))  # 1-indexed → 0-indexed
    located.sort(key=lambda x: x[0], reverse=True)

    count = 0
    for start_idx, method, actual in located:
        # Find `out="""\` within the next 50 lines of the method.
        out_start = None
        for i in range(start_idx, min(start_idx + 50, len(lines))):
            if 'out="""\\\n' in lines[i]:
                out_start = i
                break
        if out_start is None:
            print(f"Warning: no out block found for {method.__qualname__}", file=sys.stderr)
            continue

        # Detect indentation of the `out="""` line.
        indent = len(lines[out_start]) - len(lines[out_start].lstrip())
        indent_str = " " * indent

        # Find the closing `""",` line (unindented, at column 0).
        out_end = None
        for i in range(out_start + 1, len(lines)):
            if lines[i].rstrip("\n") == '""",':
                out_end = i
                break
        if out_end is None:
            print(f"Warning: no end of out block found for {method.__qualname__}", file=sys.stderr)
            continue

        # Build replacement lines.
        # Backslashes must be doubled so Python reads them back correctly
        # when the out block is a triple-quoted string in the test file.
        new_lines = [f'{indent_str}out="""\\\n']
        for line in actual.splitlines():
            escaped = line.replace('\\', '\\\\')
            new_lines.append(f'{indent_str}{escaped}\n' if line else '\n')
        new_lines.append('""",\n')

        lines[out_start:out_end + 1] = new_lines
        count += 1

    with open(file_path, "w") as f:
        f.writelines(lines)

    return count


def main(argv: Optional[List[str]] = None) -> int:
    """Main entry point for the test runner."""
    parser = argparse.ArgumentParser(description='Run AST diff tests')
    parser.add_argument('--binary', default='target/debug/syntaqlite',
                        help='Path to syntaqlite binary')
    parser.add_argument('--subcommand', default=None,
                        help='Subcommand to pass to binary (e.g., ast, fmt)')
    parser.add_argument('--filter', help='Run only tests matching pattern')
    parser.add_argument('--jobs', '-j', type=int, default=None,
                        help='Number of parallel jobs')
    parser.add_argument('--rebaseline', action='store_true',
                        help='Print suggested output for failures')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Increase verbosity (-v for results, -vv for RUN markers)')
    parser.add_argument('--root', default=None,
                        help='Project root directory')
    parser.add_argument('--test-dir', default='tests/ast_diff_tests',
                        help='Relative path to test directory (default: tests/ast_diff_tests)')

    args = parser.parse_args(argv)

    # Determine project root
    if args.root:
        root_dir = Path(args.root)
    else:
        root_dir = Path(__file__).parent.parent.parent.parent
        if not (root_dir / 'Cargo.toml').exists():
            print(f"Error: Could not find project root.", file=sys.stderr)
            return 1

    # Resolve binary path
    binary = Path(args.binary)
    if not binary.is_absolute():
        binary = root_dir / binary

    # Load tests
    try:
        tests = load_all_tests(root_dir, args.filter, args.test_dir)
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

    subcommand = args.subcommand
    test_args = [(str(binary), subcommand, name, blueprint) for name, blueprint in tests]

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
                if not args.rebaseline:
                    print_failure_details(result)
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


if __name__ == '__main__':
    sys.exit(main())
