# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""C API integration test suite.

Compiles small C drivers against the syntaqlite static library and runs
each scenario through the matching driver's stdin, diffing stdout
against the scenario's expected output.

Scenarios are defined in Python under `tests/c_api_tests/<area>.py` as
classes deriving from `CApiTestSuite`; the module filename selects the
driver area (e.g. `formatter.py` → `formatter_driver.c`).
"""

from __future__ import annotations

import importlib
import inspect
import re
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

from python.dev.diff_tests.utils import Colors, colorize, format_diff
from python.dev.integration_tests.suite import SuiteContext

NAME = "c-api"
DESCRIPTION = "C API integration tests (tests/c_api_tests/)"
NEEDS_BINARY = False


# ---------------------------------------------------------------------------
# Scenario dataclass and test-suite base class
# ---------------------------------------------------------------------------

@dataclass
class CApiScenario:
    """A single C API test scenario.

    Attributes:
        input: Line-protocol input fed to the driver's stdin.
        expected: Expected stdout, byte-for-byte.
    """
    input: str
    expected: str


class CApiTestSuite:
    """Base for C API scenario classes.

    Subclass this and add methods prefixed with `test_` that return
    `CApiScenario` instances; `fetch()` collects them as
    `(class.method, scenario)` tuples.
    """

    def fetch(self) -> list[tuple[str, CApiScenario]]:
        out: list[tuple[str, CApiScenario]] = []
        for name in sorted(dir(self)):
            if not name.startswith("test_"):
                continue
            method = getattr(self, name)
            if not callable(method):
                continue
            scenario = method()
            if not isinstance(scenario, CApiScenario):
                continue
            out.append((f"{self.__class__.__name__}.{name[5:]}", scenario))
        return out


# ---------------------------------------------------------------------------
# Driver compilation
# ---------------------------------------------------------------------------

@dataclass
class Driver:
    area: str           # scenario module stem (e.g. "formatter")
    source: str         # path of the driver .c relative to root_dir

_DRIVERS = [
    Driver(area="formatter", source="tests/c_api_tests/formatter_driver.c"),
    Driver(area="validator", source="tests/c_api_tests/validator_driver.c"),
    Driver(area="parser",    source="tests/c_api_tests/parser_driver.c"),
]

_STATIC_LIB_REL = "target/debug/libsyntaqlite.a"


def _ensure_static_lib(root_dir: Path, verbose: int) -> Path:
    lib = root_dir / _STATIC_LIB_REL
    if lib.exists():
        return lib
    if verbose >= 1:
        print(f"Building {lib.name} via cargo...")
    subprocess.check_call(
        ["cargo", "build", "-p", "syntaqlite"],
        cwd=root_dir,
    )
    if not lib.exists():
        raise RuntimeError(f"cargo build did not produce {lib}")
    return lib


def _platform_link_args() -> list[str]:
    if sys.platform == "darwin":
        return ["-framework", "Security", "-framework", "SystemConfiguration",
                "-framework", "CoreFoundation", "-lc++"]
    if sys.platform.startswith("linux"):
        return ["-lpthread", "-ldl", "-lm"]
    return []


def _compile_driver(
    root_dir: Path, driver: Driver, static_lib: Path, out_dir: Path,
    verbose: int,
) -> Path:
    out_bin = out_dir / (Path(driver.source).stem + (".exe" if sys.platform == "win32" else ""))
    src = root_dir / driver.source
    inc_dirs = [
        root_dir / "syntaqlite" / "include",
        root_dir / "syntaqlite-syntax" / "include",
    ]
    cmd = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O0", "-g",
           "-o", str(out_bin), str(src), str(static_lib)]
    for inc in inc_dirs:
        cmd.extend(["-I", str(inc)])
    cmd.extend(_platform_link_args())
    if verbose >= 1:
        print(f"Compiling {driver.source} -> {out_bin.name}")
    subprocess.check_call(cmd)
    return out_bin


# ---------------------------------------------------------------------------
# Scenario discovery
# ---------------------------------------------------------------------------

def _load_scenarios(root_dir: Path, area: str) -> list[tuple[str, CApiScenario]]:
    module_name = f"tests.c_api_tests.{area}"
    try:
        module = importlib.import_module(module_name)
    except ModuleNotFoundError:
        return []
    scenarios: list[tuple[str, CApiScenario]] = []
    for _, cls in inspect.getmembers(module, inspect.isclass):
        if cls is CApiTestSuite or not issubclass(cls, CApiTestSuite):
            continue
        if cls.__module__ != module_name:
            continue
        scenarios.extend(cls().fetch())
    return scenarios


# ---------------------------------------------------------------------------
# Execution
# ---------------------------------------------------------------------------

def _run_scenario(binary: Path, scenario: CApiScenario) -> tuple[bool, str]:
    proc = subprocess.run(
        [str(binary)], input=scenario.input.encode("utf-8"),
        capture_output=True, timeout=30,
    )
    actual = proc.stdout.decode("utf-8", errors="replace")
    ok = proc.returncode == 0 and actual == scenario.expected
    return ok, actual


def run(ctx: SuiteContext) -> int:
    root_dir = ctx.root_dir
    static_lib = _ensure_static_lib(root_dir, ctx.verbose)

    filter_re = re.compile(ctx.filter_pattern) if ctx.filter_pattern else None

    with tempfile.TemporaryDirectory(prefix="syntaqlite_c_api_") as tmp:
        out_dir = Path(tmp)
        results: list[tuple[str, bool]] = []
        failed: list[str] = []

        for driver in _DRIVERS:
            scenarios = _load_scenarios(root_dir, driver.area)
            if filter_re is not None:
                scenarios = [
                    (name, s) for name, s in scenarios
                    if filter_re.search(f"{driver.area}.{name}")
                ]
            if not scenarios:
                if ctx.verbose >= 1:
                    print(f"No scenarios for driver '{driver.area}', skipping.")
                continue
            binary = _compile_driver(root_dir, driver, static_lib, out_dir, ctx.verbose)

            for name, scenario in scenarios:
                full = f"{driver.area}.{name}"
                start = time.time()
                ok, actual = _run_scenario(binary, scenario)
                elapsed_ms = int((time.time() - start) * 1000)
                results.append((full, ok))
                if ok:
                    if ctx.verbose >= 1:
                        tag = colorize("[       OK ]", Colors.GREEN)
                        print(f"{tag} {full} ({elapsed_ms} ms)")
                else:
                    tag = colorize("[  FAILED  ]", Colors.RED)
                    print(f"{tag} {full} ({elapsed_ms} ms)")
                    for line in format_diff(scenario.expected, actual):
                        print(line)
                    failed.append(full)

        passed = sum(1 for _, ok in results if ok)
        if passed:
            print(colorize("[  PASSED  ]", Colors.GREEN) + f" {passed} tests.")
        if failed:
            print(colorize("[  FAILED  ]", Colors.RED) + f" {len(failed)} tests, listed below:")
            for name in failed:
                print(f"  {name}")
            return 1
        return 0
