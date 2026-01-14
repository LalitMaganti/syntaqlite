# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Test discovery and loading."""

import re
import sys
from pathlib import Path
from typing import List, Optional, Tuple

from python.diff_tests.testing import AstTestBlueprint


def load_all_tests(
    root_dir: Path,
    filter_pattern: Optional[str] = None
) -> Tuple[List[Tuple[str, AstTestBlueprint]], List[str]]:
    """Load all tests from the test directory.

    Dynamically imports tests/diff_tests/include_index.py and collects
    all registered test suites.

    Args:
        root_dir: The project root directory.
        filter_pattern: Optional regex pattern to filter test names.

    Returns:
        Tuple of (tests_to_run, skipped_test_names).
        tests_to_run is a list of (test_name, blueprint) tuples.
        skipped_test_names is a list of test names that were filtered out.
    """
    # Add root to path so imports work
    if str(root_dir) not in sys.path:
        sys.path.insert(0, str(root_dir))

    # Import the test index
    from tests.diff_tests.include_index import fetch_all_diff_tests

    # Collect all tests from all suites
    all_tests = []
    for suite in fetch_all_diff_tests():
        all_tests.extend(suite.fetch())

    # Filter if pattern provided
    if filter_pattern:
        pattern = re.compile(filter_pattern, re.IGNORECASE)
        tests_to_run = []
        skipped = []
        for name, blueprint in all_tests:
            if pattern.search(name):
                tests_to_run.append((name, blueprint))
            else:
                skipped.append(name)
        return tests_to_run, skipped

    return all_tests, []
