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
        idempotent: Whether this blueprint round-trips through the
                formatter without AST-shape changes. Defaults to True.
                Set to False for tests that intentionally exercise
                formatter behaviour that changes shape on re-parse —
                e.g. the formatter deliberately adds precedence-
                clarifying parens around a sub-expression that had
                none in the source, so `parse(fmt(sql)) != parse(sql)`
                on purpose. The `sql-idempotency` suite skips these.
    """
    sql: str
    out: str
    cflags: Optional[List[str]] = None
    version: Optional[str] = None
    line_width: Optional[int] = None
    indent_width: Optional[int] = None
    strict_schema: bool = False
    idempotent: bool = True


@dataclass
class LspDiffTestBlueprint:
    """Defines a single LSP diff test.

    The query is driven against the real `syntaqlite lsp` server over
    JSON-RPC. The response is rendered to deterministic text and compared
    against `out`.

    Attributes:
        sql: SQL opened as the document. Must contain exactly one cursor
             marker `<|>` indicating where the LSP query runs. The marker
             is stripped before the document is sent. Any schema DDL that
             the test needs can be included inline in this SQL (CREATE
             TABLE, CREATE VIEW, WITH binding, etc.) — the LSP host
             analyzes it as part of the document.
        op: One of "hover", "definition", "completion", "references",
            "prepare-rename", "rename", "diagnostics".
        out: Expected rendered output.
        new_name: New symbol name (required for op="rename").
        include_declaration: When true (default), find-references includes
            the symbol's definition site.
    """
    sql: str
    op: str
    out: str
    new_name: Optional[str] = None
    include_declaration: bool = True


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

    def fetch(self) -> List[Tuple[str, object]]:
        """Discover and return all test methods.

        Returns:
            List of (test_name, blueprint) tuples. The blueprint is any
            recognized `*TestBlueprint` dataclass; the test runner dispatches
            on type.
        """
        tests = []
        for name in sorted(dir(self)):
            if name.startswith('test_'):
                method = getattr(self, name)
                if callable(method):
                    blueprint = method()
                    if isinstance(blueprint, (DiffTestBlueprint, LspDiffTestBlueprint)):
                        # Format: ClassName.method_name (without test_ prefix)
                        test_name = f"{self.__class__.__name__}.{name[5:]}"
                        tests.append((test_name, blueprint))
        return tests
