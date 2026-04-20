# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Unified integration test runner.

Discovers all suite modules under integration_tests/suites/ and runs the
selected ones.  Each suite is a Python module that exposes:

    NAME: str           — short identifier for --suite
    DESCRIPTION: str    — shown in --list output
    run(ctx) -> int     — returns 0 on pass, non-zero on failure

Usage (via tools/run-integration-tests):
    tools/run-integration-tests               # run all suites
    tools/run-integration-tests --suite ast   # run one suite
    tools/run-integration-tests --list        # list available suites
"""

from __future__ import annotations

import argparse
import importlib
import sys
import time
from pathlib import Path
from types import ModuleType

ROOT_DIR = Path(__file__).resolve().parents[3]

# Suites in canonical run order.
_SUITE_MODULES = [
    "python.dev.integration_tests.suites.ast",
    "python.dev.integration_tests.suites.fmt",
    "python.dev.integration_tests.suites.perfetto_fmt",
    "python.dev.integration_tests.suites.perfetto_val",
    "python.dev.integration_tests.suites.amalg",
    "python.dev.integration_tests.suites.grammar",
    "python.dev.integration_tests.suites.sql_idempotency",
    "python.dev.integration_tests.suites.upstream_sqlite",
    "python.dev.integration_tests.suites.lsp",
    "python.dev.integration_tests.suites.lsp_diff",
    "python.dev.integration_tests.suites.validate",
    "python.dev.integration_tests.suites.semantic",
    "python.dev.integration_tests.suites.lineage",
    "python.dev.integration_tests.suites.introspect",
    "python.dev.integration_tests.suites.serve",
]

_BLUE = "\033[1;34m"
_GREEN = "\033[1;32m"
_RED = "\033[1;31m"
_RESET = "\033[0m"


def _load_suites() -> list[ModuleType]:
    return [importlib.import_module(m) for m in _SUITE_MODULES]


def _print_suite_header(name: str) -> None:
    bar = "=" * (len(name) + 4)
    print(f"\n{_BLUE}{bar}")
    print(f"  {name}")
    print(f"{bar}{_RESET}")


def main(argv: list[str] | None = None) -> int:
    from python.dev.integration_tests.suite import SuiteContext

    parser = argparse.ArgumentParser(
        description="Run syntaqlite integration test suites.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--suite", metavar="NAME", action="append", dest="suites",
        help="Suite(s) to run (repeatable). Omit to run all suites.",
    )
    parser.add_argument(
        "--list", action="store_true",
        help="List available suites and exit.",
    )
    parser.add_argument(
        "--binary", default="target/debug/syntaqlite",
        help="Path to the syntaqlite CLI binary.",
    )
    parser.add_argument(
        "--filter", metavar="PATTERN", dest="filter_pattern",
        help="Run only tests whose names match PATTERN (regex, applies to diff suites).",
    )
    parser.add_argument(
        "--rebaseline", action="store_true",
        help="Rewrite failing test expectations in-place (diff suites only).",
    )
    parser.add_argument(
        "--jobs", "-j", type=int, default=None,
        help="Parallel jobs for diff test suites.",
    )
    parser.add_argument(
        "-v", "--verbose", action="count", default=0,
        help="Increase verbosity.",
    )
    parser.add_argument(
        "--analyze-only", action="store_true",
        help="Skip running tests; analyze existing logs from a previous run.",
    )
    parser.add_argument(
        "--validate", action="store_true",
        help="Enable semantic validation (upstream-sqlite suite).",
    )
    args = parser.parse_args(argv)

    all_suites = _load_suites()

    if args.list:
        print("Available suites:")
        for suite in all_suites:
            print(f"  {suite.NAME:<20} {suite.DESCRIPTION}")
        return 0

    # Filter to requested suites.
    if args.suites:
        by_name = {s.NAME: s for s in all_suites}
        selected = []
        for name in args.suites:
            if name not in by_name:
                print(
                    f"Unknown suite '{name}'. Available: {', '.join(by_name)}",
                    file=sys.stderr,
                )
                return 1
            selected.append(by_name[name])
    else:
        selected = [s for s in all_suites if getattr(s, "ENABLED_BY_DEFAULT", True)]

    binary = Path(args.binary)
    if not binary.is_absolute():
        binary = ROOT_DIR / binary

    needs_binary = any(getattr(s, "NEEDS_BINARY", True) for s in selected)
    if needs_binary and not binary.exists():
        print(f"Error: binary not found: {binary}", file=sys.stderr)
        print("Build it with: cargo build -p syntaqlite-cli", file=sys.stderr)
        return 1

    ctx = SuiteContext(
        root_dir=ROOT_DIR,
        binary=binary,
        verbose=args.verbose,
        filter_pattern=args.filter_pattern,
        rebaseline=args.rebaseline,
        jobs=args.jobs,
        analyze_only=args.analyze_only,
        validate=args.validate,
    )

    results: list[tuple[str, bool, float]] = []

    for suite in selected:
        _print_suite_header(suite.NAME)
        t0 = time.monotonic()
        rc = suite.run(ctx)
        elapsed = time.monotonic() - t0
        results.append((suite.NAME, rc == 0, elapsed))

    # Summary
    print(f"\n{_BLUE}{'=' * 40}{_RESET}")
    passed = sum(1 for _, ok, _ in results if ok)
    failed = len(results) - passed

    for name, ok, elapsed in results:
        status = colorize("[  PASS  ]", _GREEN) if ok else colorize("[  FAIL  ]", _RED)
        print(f"  {status} {name} ({elapsed:.1f}s)")

    print()
    if failed == 0:
        print(colorize(f"All {passed} suite(s) passed.", _GREEN))
    else:
        print(colorize(f"{failed} suite(s) FAILED, {passed} passed.", _RED))

    return 0 if failed == 0 else 1


def colorize(text: str, color: str) -> str:
    import sys as _sys
    if hasattr(_sys.stdout, "isatty") and _sys.stdout.isatty():
        return f"{color}{text}{_RESET}"
    return text


if __name__ == "__main__":
    sys.exit(main())
