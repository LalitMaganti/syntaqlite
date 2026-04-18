# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CLI introspection suite: tokenize."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

from python.dev.integration_tests.suite import SuiteContext

NAME = "introspect"
DESCRIPTION = "CLI introspection commands (tokenize + dialect codegen group)"

_GREEN = "\033[32m"
_RED = "\033[31m"
_RESET = "\033[0m"


def _pass(name: str) -> None:
    print(f"  {_GREEN}PASS{_RESET}  {name}")


def _fail(name: str, detail: str) -> None:
    print(f"  {_RED}FAIL{_RESET}  {name}: {detail}")


def _run(
    binary: Path,
    *args: str,
    stdin: str | None = None,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(binary), *args],
        input=stdin,
        capture_output=True,
        text=True,
    )


# ── tokenize ─────────────────────────────────────────────────────────────


def _test_tokenize_text_output(ctx: SuiteContext) -> bool:
    """`tokenize` default (text) emits one line per token."""
    result = _run(ctx.binary, "tokenize", "-e", "SELECT 1;")
    if result.returncode != 0:
        _fail("tokenize_text_output", f"exit {result.returncode}: {result.stderr}")
        return False
    lines = [l for l in result.stdout.splitlines() if l.strip()]
    if not lines:
        _fail("tokenize_text_output", f"no output: {result.stdout!r}")
        return False
    joined = "\n".join(lines)
    if "SELECT" not in joined:
        _fail("tokenize_text_output", f"expected SELECT token, got: {joined}")
        return False
    _pass("tokenize_text_output")
    return True


def _test_tokenize_json_output(ctx: SuiteContext) -> bool:
    """`tokenize -o json` emits ndjson with text/offset/length/type fields."""
    result = _run(ctx.binary, "tokenize", "-o", "json", "-e", "SELECT 1;")
    if result.returncode != 0:
        _fail("tokenize_json_output", f"exit {result.returncode}: {result.stderr}")
        return False
    records = [
        json.loads(l) for l in result.stdout.splitlines() if l.strip()
    ]
    if not records:
        _fail("tokenize_json_output", "no records")
        return False
    for field in ("kind", "schema_version", "text", "offset", "length", "type"):
        if field not in records[0]:
            _fail("tokenize_json_output", f"missing {field!r} in {records[0]}")
            return False
    if records[0]["kind"] != "token":
        _fail("tokenize_json_output", f"wrong kind: {records[0]}")
        return False
    select = next((r for r in records if r["text"] == "SELECT"), None)
    if select is None:
        _fail("tokenize_json_output", f"no SELECT token in {records}")
        return False
    _pass("tokenize_json_output")
    return True


def _test_tokenize_stdin(ctx: SuiteContext) -> bool:
    """`tokenize` reads stdin when no files / -e given."""
    result = _run(ctx.binary, "tokenize", "-o", "json", stdin="SELECT 1;\n")
    if result.returncode != 0:
        _fail("tokenize_stdin", f"exit {result.returncode}: {result.stderr}")
        return False
    records = [json.loads(l) for l in result.stdout.splitlines() if l.strip()]
    if not any(r["text"] == "SELECT" for r in records):
        _fail("tokenize_stdin", f"no SELECT in stdin tokens: {records}")
        return False
    _pass("tokenize_stdin")
    return True


# ── dialect codegen group refactor ───────────────────────────────────────


def _test_dialect_generate_still_works(ctx: SuiteContext) -> bool:
    """After the refactor, `dialect generate --name X` still produces files."""
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        result = _run(
            ctx.binary, "dialect", "generate",
            "--name", "sqlite",
            "--output-dir", tmp,
            "--output-type", "runtime-only",
        )
        if result.returncode != 0:
            _fail("dialect_generate_still_works",
                  f"exit {result.returncode}: {result.stderr}")
            return False
        produced = list(Path(tmp).iterdir())
        if not produced:
            _fail("dialect_generate_still_works",
                  "expected files in output dir, found none")
            return False
    _pass("dialect_generate_still_works")
    return True


# ── Suite entry point ────────────────────────────────────────────────────


def run(ctx: SuiteContext) -> int:
    tests = [
        _test_tokenize_text_output,
        _test_tokenize_json_output,
        _test_tokenize_stdin,
        _test_dialect_generate_still_works,
    ]
    results = [t(ctx) for t in tests]
    passed = sum(results)
    total = len(results)
    print(f"\n  {passed}/{total} introspect tests passed.")
    return 0 if all(results) else 1
