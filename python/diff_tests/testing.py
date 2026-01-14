# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Core testing classes for AST diff tests."""

from dataclasses import dataclass
from typing import List, Tuple


@dataclass
class AstTestBlueprint:
    """Defines a single AST diff test.

    Attributes:
        sql: The SQL input to parse.
        out: The expected AST output (as formatted text).
    """
    sql: str
    out: str


class TestSuite:
    """Base class for test suites.

    Subclass this and add methods prefixed with `test_` that return
    AstTestBlueprint instances. The fetch() method will automatically
    discover and collect all test methods.

    Example:
        class SelectTests(TestSuite):
            def test_simple(self):
                return AstTestBlueprint(
                    sql="SELECT 1",
                    out="SelectStmt\\n  ..."
                )
    """

    def fetch(self) -> List[Tuple[str, AstTestBlueprint]]:
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
                    if isinstance(blueprint, AstTestBlueprint):
                        # Format: ClassName.method_name (without test_ prefix)
                        test_name = f"{self.__class__.__name__}.{name[5:]}"
                        tests.append((test_name, blueprint))
        return tests
