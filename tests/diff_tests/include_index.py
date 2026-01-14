# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Registers all test suites for discovery.

Add new test suite imports here as they are created.
"""

from tests.diff_tests.select.tests import SelectBasic


def fetch_all_diff_tests():
    """Return all test suites to run.

    Returns:
        List of TestSuite instances.
    """
    return [
        SelectBasic(),
        # Add more test suites here as they are created:
        # InsertBasic(),
        # UpdateBasic(),
        # DeleteBasic(),
        # ExpressionTests(),
    ]
