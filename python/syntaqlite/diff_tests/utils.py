# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Utilities for test output and formatting."""

import difflib
import sys
from typing import List


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[91m'
    GREEN = '\033[92m'
    RESET = '\033[0m'


def supports_color() -> bool:
    """Check if the terminal supports color output."""
    if not hasattr(sys.stdout, 'isatty'):
        return False
    if not sys.stdout.isatty():
        return False
    return True


def colorize(text: str, color: str) -> str:
    """Apply color to text if terminal supports it."""
    if supports_color():
        return f"{color}{text}{Colors.RESET}"
    return text


def format_diff(expected: str, actual: str) -> List[str]:
    """Generate a unified diff between expected and actual output."""
    expected_lines = expected.splitlines(keepends=True)
    actual_lines = actual.splitlines(keepends=True)

    diff = list(difflib.unified_diff(
        expected_lines,
        actual_lines,
        fromfile='expected',
        tofile='actual',
        lineterm=''
    ))

    colored_diff = []
    for line in diff:
        if line.startswith('-') and not line.startswith('---'):
            colored_diff.append(colorize(line, Colors.RED))
        elif line.startswith('+') and not line.startswith('+++'):
            colored_diff.append(colorize(line, Colors.GREEN))
        else:
            colored_diff.append(line)

    return colored_diff
