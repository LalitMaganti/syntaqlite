# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Test discovery and loading."""

import importlib
import inspect
import re
import sys
from glob import glob
from pathlib import Path
from typing import List, Optional, Tuple

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


def _discover_test_suites(root_dir: Path, test_dir: str = "tests/ast_diff_tests") -> List[TestSuite]:
    """Auto-discover all TestSuite subclasses in the given test directory.

    Scans for .py files (excluding __init__.py) in the test directory,
    sorted by filename for deterministic order.

    Args:
        root_dir: The project root directory.
        test_dir: Relative path to the test directory (default: tests/ast_diff_tests).

    Returns:
        List of TestSuite instances found across all test modules.
    """
    test_base = root_dir / test_dir
    pattern = str(test_base / "*.py")
    suites = []

    for test_file in sorted(glob(pattern)):
        test_path = Path(test_file)
        if test_path.name == "__init__.py":
            continue
        # Convert filesystem path to Python module path:
        #   tests/ast_diff_tests/select.py -> tests.ast_diff_tests.select
        relative = test_path.relative_to(root_dir)
        module_name = str(relative.with_suffix("")).replace("/", ".")

        module = importlib.import_module(module_name)

        for _name, obj in inspect.getmembers(module, inspect.isclass):
            if issubclass(obj, TestSuite) and obj is not TestSuite:
                suites.append(obj())

    return suites


def load_all_tests(
    root_dir: Path,
    filter_pattern: Optional[str] = None,
    test_dir: str = "tests/ast_diff_tests"
) -> List[Tuple[str, AstTestBlueprint]]:
    """Load all tests from the test directory.

    Auto-discovers test suites by scanning the given test directory
    for TestSuite subclasses.

    Args:
        root_dir: The project root directory.
        filter_pattern: Optional regex pattern to filter test names.
        test_dir: Relative path to the test directory (default: tests/ast_diff_tests).

    Returns:
        List of (test_name, blueprint) tuples to run.
    """
    # Add root to path so imports work
    if str(root_dir) not in sys.path:
        sys.path.insert(0, str(root_dir))

    # Collect all tests from all auto-discovered suites
    all_tests = []
    for suite in _discover_test_suites(root_dir, test_dir):
        all_tests.extend(suite.fetch())

    # Filter if pattern provided
    if filter_pattern:
        pattern = re.compile(filter_pattern, re.IGNORECASE)
        return [(name, bp) for name, bp in all_tests if pattern.search(name)]

    return all_tests
