# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""C API integration test suite.

Compiles small C drivers against the syntaqlite static library and runs
each scenario through the matching driver's stdin, diffing stdout
against the scenario's expected output.

Scenarios are defined in Python under `tests/c_api_tests/<area>.py` as
classes deriving from `CApiTestSuite`; the module filename selects the
driver area (e.g. `formatter.py` → `formatter_driver.c`).

Sanitizer mode: when `SYNTAQLITE_CAPI_SANITIZE=1`, drivers compile and
link with `-fsanitize=address,undefined`. Point at a sanitizer-built
libsyntaqlite.a via `SYNTAQLITE_STATIC_LIB=<path>`; otherwise the stock
debug lib is used (catches C-side bugs only). Stderr from failing
scenarios is printed so sanitizer reports are visible.
"""

from __future__ import annotations

import importlib
import inspect
import os
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

_DEFAULT_STATIC_LIB_REL = "target/debug/libsyntaqlite.a"


def _env_truthy(name: str) -> bool:
    return os.environ.get(name, "").lower() in ("1", "true", "yes", "on")


def _sanitize_enabled() -> bool:
    return _env_truthy("SYNTAQLITE_CAPI_SANITIZE")


def _resolve_static_lib(root_dir: Path) -> Path:
    override = os.environ.get("SYNTAQLITE_STATIC_LIB")
    if override:
        return Path(override)
    return root_dir / _DEFAULT_STATIC_LIB_REL


def _ensure_static_lib(root_dir: Path, verbose: int) -> Path:
    lib = _resolve_static_lib(root_dir)
    if lib.exists():
        return lib
    if os.environ.get("SYNTAQLITE_STATIC_LIB"):
        raise RuntimeError(f"SYNTAQLITE_STATIC_LIB set but not found: {lib}")
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


def _sanitize_flags() -> list[str]:
    return ["-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-sanitize-recover=undefined"]


def _sanitizer_env() -> dict[str, str]:
    env = dict(os.environ)
    # Rust's global allocator holds process-lifetime state that LSan would
    # flag. Disable leak detection unless the caller explicitly opts in.
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0:abort_on_error=1:halt_on_error=1")
    env.setdefault("UBSAN_OPTIONS", "print_stacktrace=1:halt_on_error=1")
    return env


def _compile_driver(
    root_dir: Path, driver: Driver, static_lib: Path, out_dir: Path,
    verbose: int, sanitize: bool,
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
    if sanitize:
        cmd.extend(_sanitize_flags())
    cmd.extend(_platform_link_args())
    if verbose >= 1:
        print(f"Compiling {driver.source} -> {out_bin.name}{' [sanitize]' if sanitize else ''}")
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

def _run_scenario(
    binary: Path, scenario: CApiScenario, env: dict[str, str] | None,
) -> tuple[bool, str, str]:
    proc = subprocess.run(
        [str(binary)], input=scenario.input.encode("utf-8"),
        capture_output=True, timeout=30, env=env,
    )
    actual = proc.stdout.decode("utf-8", errors="replace")
    stderr = proc.stderr.decode("utf-8", errors="replace")
    ok = proc.returncode == 0 and actual == scenario.expected
    return ok, actual, stderr


def run(ctx: SuiteContext) -> int:
    root_dir = ctx.root_dir
    sanitize = _sanitize_enabled()
    static_lib = _ensure_static_lib(root_dir, ctx.verbose)

    if sanitize and ctx.verbose >= 0:
        print(f"Sanitize mode: ASan+UBSan on C drivers, lib={static_lib}")

    filter_re = re.compile(ctx.filter_pattern) if ctx.filter_pattern else None
    child_env = _sanitizer_env() if sanitize else None

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
            binary = _compile_driver(root_dir, driver, static_lib, out_dir,
                                      ctx.verbose, sanitize)

            for name, scenario in scenarios:
                full = f"{driver.area}.{name}"
                start = time.time()
                ok, actual, stderr = _run_scenario(binary, scenario, child_env)
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
                    if stderr:
                        print("--- stderr ---")
                        print(stderr.rstrip())
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
