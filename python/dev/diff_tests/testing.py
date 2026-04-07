# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Core testing classes for AST diff tests."""

from dataclasses import dataclass
from typing import List, Optional, Tuple


@dataclass
class DiffTestBlueprint:
    """Defines a single AST diff test.

    Attributes:
        sql: The SQL input to parse.
        out: The expected AST output (as formatted text).
        cflags: Optional list of compile-time flags to enable (e.g.
                ["SQLITE_ENABLE_ORDERED_SET_AGGREGATES"]).
        version: Optional SQLite version to emulate (e.g. "3.47.0").
    """
    sql: str
    out: str
    cflags: Optional[List[str]] = None
    version: Optional[str] = None
    line_width: Optional[int] = None
    indent_width: Optional[int] = None


class TestSuite:
    """Base class for test suites.

    Subclass this and add methods prefixed with `test_` that return
    DiffTestBlueprint instances. The fetch() method will automatically
    discover and collect all test methods.

    Example:
        class SelectTests(TestSuite):
            def test_simple(self):
                return DiffTestBlueprint(
                    sql="SELECT 1",
                    out="SelectStmt\\n  ..."
                )
    """

    def fetch(self) -> List[Tuple[str, DiffTestBlueprint]]:
        """Discover and return all test methods.

        Returns:
            List of (test_name, blueprint) tuples.
            Test names are formatted as "ClassName#method_name" where
            method_name has the "test_" prefix stripped.
        """
        tests = []
        for name in sorted(dir(self)):
            if name.startswith('test_'):
                method = getattr(self, name)
                if callable(method):
                    blueprint = method()
                    if isinstance(blueprint, DiffTestBlueprint):
                        # Format: ClassName.method_name (without test_ prefix)
                        test_name = f"{self.__class__.__name__}.{name[5:]}"
                        tests.append((test_name, blueprint))
        return tests
