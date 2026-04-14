# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Amalgamation compilation and AST diff test suite.

Generates dialect amalgamations, compiles test binaries, and runs
AST diff tests against them. Covers multiple dialect configurations:
  - sqlite    (full)         — base grammar, runtime inlined
  - sqlite    (dialect-only) — base grammar, external runtime
  - perfetto  (full)         — perfetto extension, runtime inlined
  - perfetto  (dialect-only) — perfetto extension, external runtime
"""

import os
import sys
import time
from pathlib import Path

from python.dev.diff_tests.amalg_executor import (
    AmalgMode,
    AmalgTestContext,
    DialectConfig,
    compile_strict_warning_check,
)
from python.dev.diff_tests.test_executor import execute_test
from python.dev.diff_tests.test_loader import load_all_tests
from python.dev.diff_tests.utils import Colors, colorize, format_diff
from python.dev.integration_tests.suite import SuiteContext

NAME = "amalg"
DESCRIPTION = "Amalgamation compilation + AST diff tests (tests/amalg_tests/)"

# ---------------------------------------------------------------------------
# Dialect configurations keyed by lower-cased test suite class name.
# ---------------------------------------------------------------------------

def _dialect_configs(root_dir: Path) -> dict[str, DialectConfig]:
    perfetto_actions = str(root_dir / "dialects/perfetto/actions")
    perfetto_nodes = str(root_dir / "dialects/perfetto/nodes")
    return {
        "sqliteamalgfull": DialectConfig(name="sqlite", mode=AmalgMode.FULL),
        "sqliteamalgruntimeonly": DialectConfig(name="sqlite", mode=AmalgMode.DIALECT_ONLY),
        "perfettoamalgfull": DialectConfig(
            name="perfetto", mode=AmalgMode.FULL,
            actions_dir=perfetto_actions, nodes_dir=perfetto_nodes,
        ),
        "perfettoamalgdialectonly": DialectConfig(
            name="perfetto", mode=AmalgMode.DIALECT_ONLY,
            actions_dir=perfetto_actions, nodes_dir=perfetto_nodes,
        ),
    }

# Legacy test class name aliases.
_LEGACY_ALIASES = {
    "sqliteamalg": "sqliteamalgfull",
    "perfettoextension": "perfettoamalgfull",
}


def _dialect_for_test(test_name: str, configs: dict[str, DialectConfig]) -> str:
    suite_name = test_name.split(".")[0].lower()
    if suite_name in configs:
        return suite_name
    if suite_name in _LEGACY_ALIASES:
        return _LEGACY_ALIASES[suite_name]
    for key in configs:
        if suite_name.startswith(key) or key.startswith(suite_name):
            return key
    return "sqliteamalgfull"


def run(ctx: SuiteContext) -> int:
    root_dir = ctx.root_dir
    configs = _dialect_configs(root_dir)
    tests = load_all_tests(root_dir, ctx.filter_pattern, "tests/amalg_tests")
    if not tests:
        print("No amalgamation tests to run.")
        return 0

    suites = set(name.split(".")[0] for name, _ in tests)
    amalg_ctx = AmalgTestContext(root_dir, ctx.binary)
    try:
        needed = {_dialect_for_test(name, configs) for name, _ in tests}
        for key in sorted(needed):
            config = configs.get(key)
            if not config:
                print(f"Unknown dialect key: {key}", file=sys.stderr)
                return 1
            if ctx.verbose >= 1:
                print(f"Building {config.name} ({config.mode.value}) amalgamation...")
            try:
                amalg_ctx.get_binary(config)
            except RuntimeError as e:
                print(f"Error: {e}", file=sys.stderr)
                return 1
            if ctx.verbose >= 1:
                print(f"  {config.name} ({config.mode.value}): OK")

        # Verify the amalgamation compiles cleanly under strict C++ warnings.
        # This catches any -Wall/-Wextra warnings that would force downstream
        # consumers to add -Wno-* suppressions.
        for key in sorted(needed):
            config = configs[key]
            if ctx.verbose >= 1:
                print(f"Strict-warning C++ compile check: {config.name} ({config.mode.value})...")
            try:
                amalg_ctx.check_strict_warnings(config)
            except RuntimeError as e:
                print(f"Error: {e}", file=sys.stderr)
                return 1
            if ctx.verbose >= 1:
                print(f"  {config.name} ({config.mode.value}): OK")

        start_time = time.time()
        results = []
        failed_tests = []

        if ctx.verbose >= 1:
            print(f"[==========] Running {len(tests)} tests from {len(suites)} test suites.")

        for name, blueprint in tests:
            key = _dialect_for_test(name, configs)
            config = configs[key]
            binary = amalg_ctx.get_binary(config)
            result = execute_test(binary, name, blueprint)
            results.append(result)

            if result.passed:
                if ctx.verbose >= 1:
                    ok = colorize("[       OK ]", Colors.GREEN)
                    print(f"{ok} {name} ({result.elapsed_ms} ms)")
            else:
                if ctx.verbose >= 1:
                    failed = colorize("[  FAILED  ]", Colors.RED)
                    print(f"{failed} {name} ({result.elapsed_ms} ms)")
                if result.error:
                    print(f"Error: {result.error}")
                else:
                    print(f"SQL: {result.sql}")
                    for line in format_diff(result.expected, result.actual):
                        print(line)
                failed_tests.append(name)

        elapsed_ms = int((time.time() - start_time) * 1000)
        passed = sum(1 for r in results if r.passed)
        failed = len(failed_tests)

        if ctx.verbose >= 1:
            print(f"[==========] {len(results)} tests ran. ({elapsed_ms} ms total)")

        if passed > 0:
            msg = colorize("[  PASSED  ]", Colors.GREEN)
            print(f"{msg} {passed} tests.")

        if failed > 0:
            msg = colorize("[  FAILED  ]", Colors.RED)
            print(f"{msg} {failed} tests, listed below:")
            for name in failed_tests:
                print(f"{msg} {name}")
            print(f"\n {failed} FAILED TESTS")

        return 0 if failed == 0 else 1

    finally:
        amalg_ctx.cleanup()
