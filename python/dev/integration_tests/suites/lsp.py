# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""LSP server integration test suite.

Spawns `syntaqlite lsp` as a subprocess and communicates via JSON-RPC over
stdio to verify schema loading and diagnostic behaviour.
"""

from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from python.dev.diff_tests.lsp_client import LspClient, spawn_lsp
from python.dev.integration_tests.suite import SuiteContext

NAME = "lsp"
DESCRIPTION = "LSP server integration tests (schema loading, diagnostics)"


def _open_and_get_diagnostics(
    client: LspClient,
    uri: str,
    text: str,
) -> list[dict[str, Any]]:
    """Open a document and collect the resulting diagnostics."""
    client.send_notification("textDocument/didOpen", {
        "textDocument": {
            "uri": uri,
            "languageId": "sql",
            "version": 1,
            "text": text,
        },
    })
    return client.collect_diagnostics()


# ── Test cases ────────────────────────────────────────────────────────────

_GREEN = "\033[32m"
_RED = "\033[31m"
_RESET = "\033[0m"


def _pass(name: str) -> None:
    print(f"  {_GREEN}PASS{_RESET}  {name}")


def _fail(name: str, detail: str) -> None:
    print(f"  {_RED}FAIL{_RESET}  {name}: {detail}")


def _test_schema_from_ddl(ctx: SuiteContext) -> bool:
    """With schemaPath, SELECT from a known table should produce zero diagnostics."""
    schema_file = ctx.root_dir / "tests" / "lsp_tests" / "schema.sql"
    query_text = (ctx.root_dir / "tests" / "lsp_tests" / "query.sql").read_text()

    client = spawn_lsp(ctx.binary, {"schemaPath": str(schema_file)})
    try:
        diags = _open_and_get_diagnostics(client, "file:///test.sql", query_text)
        # Filter to only error-severity diagnostics about unknown tables/columns.
        errors = [d for d in diags if d.get("severity", 0) <= 2]
        if errors:
            _fail("schema_from_ddl", f"expected 0 errors, got {len(errors)}: {errors}")
            return False
        _pass("schema_from_ddl")
        return True
    finally:
        client.shutdown()


def _test_no_schema_warns(ctx: SuiteContext) -> bool:
    """Without schemaPath, SELECT from unknown table should emit a diagnostic."""
    query_text = (ctx.root_dir / "tests" / "lsp_tests" / "query.sql").read_text()

    client = spawn_lsp(ctx.binary)
    try:
        diags = _open_and_get_diagnostics(client, "file:///test.sql", query_text)
        table_diags = [d for d in diags if "users" in d.get("message", "").lower()]
        if not table_diags:
            _fail("no_schema_warns", f"expected 'unknown table' diagnostic, got: {diags}")
            return False
        _pass("no_schema_warns")
        return True
    finally:
        client.shutdown()


def _find_sqlite3(root_dir: Path) -> str | None:
    """Find the hermetic sqlite3 binary from third_party/bin/."""
    import platform as _platform

    sys_name = _platform.system().lower()
    machine = _platform.machine().lower()

    if sys_name == "darwin":
        prefix = "mac"
    elif sys_name == "linux":
        prefix = "linux"
    else:
        return shutil.which("sqlite3")

    arch = "arm64" if machine in ("arm64", "aarch64") else "amd64"
    candidate = root_dir / "third_party" / "bin" / f"{prefix}-{arch}" / "sqlite3"
    if candidate.exists():
        return str(candidate)
    return shutil.which("sqlite3")


def _test_sqlite3_dot_schema(ctx: SuiteContext) -> bool:
    """Create a real SQLite DB, dump .schema, and verify LSP parses it."""
    sqlite3 = _find_sqlite3(ctx.root_dir)
    if sqlite3 is None:
        _fail("sqlite3_dot_schema", "sqlite3 not found (run tools/install-build-deps)")
        return False

    with tempfile.TemporaryDirectory() as tmp:
        db_path = Path(tmp) / "test.db"
        schema_path = Path(tmp) / "schema.sql"

        # Create a database with realistic schema.
        subprocess.run(
            [sqlite3, str(db_path)],
            input=(
                "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, email TEXT UNIQUE);\n"
                "CREATE TABLE orders (id INTEGER PRIMARY KEY, user_id INTEGER REFERENCES users(id), "
                "amount REAL, created_at TEXT DEFAULT CURRENT_TIMESTAMP);\n"
                "CREATE INDEX idx_orders_user ON orders(user_id);\n"
            ),
            check=True,
            capture_output=True,
            text=True,
        )

        # Dump .schema — this is the exact workflow users will follow.
        result = subprocess.run(
            [sqlite3, str(db_path), ".schema"],
            check=True,
            capture_output=True,
            text=True,
        )
        schema_path.write_text(result.stdout)

        # Feed the real .schema output to the LSP.
        client = spawn_lsp(ctx.binary, {"schemaPath": str(schema_path)})
        try:
            # Query both tables using columns from the schema.
            diags = _open_and_get_diagnostics(
                client, "file:///test.sql",
                "SELECT name, email FROM users; SELECT amount, created_at FROM orders;",
            )
            errors = [d for d in diags if d.get("severity", 0) <= 2]
            if errors:
                _fail("sqlite3_dot_schema", f"expected 0 errors, got: {errors}")
                return False
            _pass("sqlite3_dot_schema")
            return True
        finally:
            client.shutdown()


def _test_schema_reload(ctx: SuiteContext) -> bool:
    """Changing schemaPath via didChangeConfiguration should update diagnostics."""
    query_text = (ctx.root_dir / "tests" / "lsp_tests" / "query.sql").read_text()
    schema_file = ctx.root_dir / "tests" / "lsp_tests" / "schema.sql"

    # Start without schema — should warn.
    client = spawn_lsp(ctx.binary)
    try:
        diags = _open_and_get_diagnostics(client, "file:///test.sql", query_text)
        table_diags = [d for d in diags if "users" in d.get("message", "").lower()]
        if not table_diags:
            _fail("schema_reload", f"phase 1: expected 'unknown table' diagnostic, got: {diags}")
            return False

        # Send didChangeConfiguration with the schema path.
        client.send_notification("workspace/didChangeConfiguration", {
            "settings": {"schemaPath": str(schema_file)},
        })

        # Re-open (via didChange) to trigger re-analysis.
        client.send_notification("textDocument/didChange", {
            "textDocument": {"uri": "file:///test.sql", "version": 2},
            "contentChanges": [{"text": query_text}],
        })
        diags = client.collect_diagnostics()
        errors = [d for d in diags if d.get("severity", 0) <= 2]
        if errors:
            _fail("schema_reload", f"phase 2: expected 0 errors after reload, got: {errors}")
            return False

        _pass("schema_reload")
        return True
    finally:
        client.shutdown()


def _test_multi_schema(ctx: SuiteContext) -> bool:
    """Switching schemas via didChangeConfiguration should change diagnostics."""
    with tempfile.TemporaryDirectory() as tmp:
        schema_a = Path(tmp) / "schema_a.sql"
        schema_b = Path(tmp) / "schema_b.sql"
        schema_a.write_text("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);\n")
        schema_b.write_text("CREATE TABLE orders (id INTEGER PRIMARY KEY, amount REAL);\n")

        # Start with schema_a (users table).
        client = spawn_lsp(ctx.binary, {"schemaPath": str(schema_a)})
        try:
            # Query users — should be fine with schema_a.
            diags = _open_and_get_diagnostics(
                client, "file:///test.sql", "SELECT name FROM users;",
            )
            errors = [d for d in diags if d.get("severity", 0) <= 2]
            if errors:
                _fail("multi_schema", f"phase 1: expected 0 errors for users, got: {errors}")
                return False

            # Switch to schema_b (orders table, no users).
            client.send_notification("workspace/didChangeConfiguration", {
                "settings": {"schemaPath": str(schema_b)},
            })

            # Re-send the same doc querying users — should now error.
            client.send_notification("textDocument/didChange", {
                "textDocument": {"uri": "file:///test.sql", "version": 2},
                "contentChanges": [{"text": "SELECT name FROM users;"}],
            })
            diags = client.collect_diagnostics()
            table_diags = [d for d in diags if "users" in d.get("message", "").lower()]
            if not table_diags:
                _fail("multi_schema", f"phase 2: expected 'unknown table' for users, got: {diags}")
                return False

            # Open a new doc querying orders — should be fine with schema_b.
            diags = _open_and_get_diagnostics(
                client, "file:///orders.sql", "SELECT amount FROM orders;",
            )
            errors = [d for d in diags if d.get("severity", 0) <= 2]
            if errors:
                _fail("multi_schema", f"phase 3: expected 0 errors for orders, got: {errors}")
                return False

            _pass("multi_schema")
            return True
        finally:
            client.shutdown()


# ── Suite entry point ─────────────────────────────────────────────────────

def run(ctx: SuiteContext) -> int:
    tests = [
        _test_schema_from_ddl,
        _test_no_schema_warns,
        _test_sqlite3_dot_schema,
        _test_schema_reload,
        _test_multi_schema,
    ]
    results = [t(ctx) for t in tests]
    passed = sum(results)
    total = len(results)
    print(f"\n  {passed}/{total} LSP tests passed.")
    return 0 if all(results) else 1
