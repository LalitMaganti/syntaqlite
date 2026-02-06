# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Test execution logic."""

import subprocess
import textwrap
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from python.syntaqlite.diff_tests.testing import AstTestBlueprint


def normalize_output(text: str) -> str:
    """Normalize output for comparison.

    Strips leading/trailing whitespace and dedents the text so that
    expected output can be indented naturally in Python source.
    """
    return textwrap.dedent(text).strip()


@dataclass
class TestResult:
    """Result of a single test execution."""
    name: str
    passed: bool
    elapsed_ms: int = 0
    actual: str = ""
    expected: str = ""
    error: str = ""
    sql: str = ""


def execute_test(
    binary: Path,
    name: str,
    blueprint: AstTestBlueprint,
    timeout: Optional[float] = 30.0,
    subcommand: Optional[str] = None
) -> TestResult:
    """Execute a single test.

    Runs the binary with the SQL input and compares
    the output to the expected result.

    Args:
        binary: Path to the executable.
        name: Test name for reporting.
        blueprint: The test definition.
        timeout: Maximum execution time in seconds.
        subcommand: Optional subcommand to pass (e.g., 'ast', 'fmt').

    Returns:
        TestResult with pass/fail status and details.
    """
    cmd = [str(binary)]
    if subcommand:
        cmd.append(subcommand)
    t0 = time.monotonic()
    try:
        proc = subprocess.run(
            cmd,
            input=blueprint.sql,
            capture_output=True,
            text=True,
            timeout=timeout
        )
    except subprocess.TimeoutExpired:
        elapsed_ms = int((time.monotonic() - t0) * 1000)
        return TestResult(
            name=name,
            passed=False,
            elapsed_ms=elapsed_ms,
            error=f"Test timed out after {timeout}s",
            sql=blueprint.sql
        )
    except FileNotFoundError:
        elapsed_ms = int((time.monotonic() - t0) * 1000)
        return TestResult(
            name=name,
            passed=False,
            elapsed_ms=elapsed_ms,
            error=f"Binary not found: {binary}",
            sql=blueprint.sql
        )
    elapsed_ms = int((time.monotonic() - t0) * 1000)

    if proc.returncode != 0:
        return TestResult(
            name=name,
            passed=False,
            elapsed_ms=elapsed_ms,
            error=proc.stderr.strip() if proc.stderr else f"Exit code: {proc.returncode}",
            sql=blueprint.sql
        )

    actual = normalize_output(proc.stdout)
    expected = normalize_output(blueprint.out)

    return TestResult(
        name=name,
        passed=(actual == expected),
        elapsed_ms=elapsed_ms,
        actual=actual,
        expected=expected,
        sql=blueprint.sql
    )
