# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Test execution logic."""

import subprocess
import textwrap
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
    actual: str = ""
    expected: str = ""
    error: str = ""
    sql: str = ""


def execute_test(
    binary: Path,
    name: str,
    blueprint: AstTestBlueprint,
    timeout: Optional[float] = 30.0
) -> TestResult:
    """Execute a single test.

    Runs the ast_test binary with the SQL input and compares
    the output to the expected AST.

    Args:
        binary: Path to the ast_test executable.
        name: Test name for reporting.
        blueprint: The test definition.
        timeout: Maximum execution time in seconds.

    Returns:
        TestResult with pass/fail status and details.
    """
    try:
        proc = subprocess.run(
            [str(binary)],
            input=blueprint.sql,
            capture_output=True,
            text=True,
            timeout=timeout
        )
    except subprocess.TimeoutExpired:
        return TestResult(
            name=name,
            passed=False,
            error=f"Test timed out after {timeout}s",
            sql=blueprint.sql
        )
    except FileNotFoundError:
        return TestResult(
            name=name,
            passed=False,
            error=f"Binary not found: {binary}",
            sql=blueprint.sql
        )

    if proc.returncode != 0:
        return TestResult(
            name=name,
            passed=False,
            error=proc.stderr.strip() if proc.stderr else f"Exit code: {proc.returncode}",
            sql=blueprint.sql
        )

    actual = normalize_output(proc.stdout)
    expected = normalize_output(blueprint.out)

    return TestResult(
        name=name,
        passed=(actual == expected),
        actual=actual,
        expected=expected,
        sql=blueprint.sql
    )
