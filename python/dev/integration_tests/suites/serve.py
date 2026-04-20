# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""`syntaqlite serve json` RPC protocol suite.

Covers the raw JSON line protocol: READY handshake, framing, op dispatch,
and error frames. Python client-side coverage lives in the `python-api`
suite.
"""

from __future__ import annotations

import json
import subprocess

from python.dev.integration_tests.suite import SuiteContext

NAME = "serve"
DESCRIPTION = "syntaqlite serve JSON RPC protocol tests"

_GREEN = "\033[32m"
_RED = "\033[31m"
_RESET = "\033[0m"


def _pass(name: str) -> None:
    print(f"  {_GREEN}PASS{_RESET}  {name}")


def _fail(name: str, detail: str) -> None:
    print(f"  {_RED}FAIL{_RESET}  {name}: {detail}")


# ── Raw protocol ─────────────────────────────────────────────────────────────


def _spawn(ctx: SuiteContext) -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        [str(ctx.binary), "--no-config", "serve", "json"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        bufsize=0,
    )


def _read_ready(proc: subprocess.Popen[bytes]) -> bool:
    return proc.stdout.readline().strip() == b"READY"


def _call(proc: subprocess.Popen[bytes], req: dict) -> dict:
    proc.stdin.write((json.dumps(req) + "\n").encode())
    proc.stdin.flush()
    return json.loads(proc.stdout.readline())


def _test_ready_then_quit(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        if not _read_ready(proc):
            _fail("ready_then_quit", "expected READY on startup")
            return False
        proc.stdin.write(b'{"op":"quit"}\n')
        proc.stdin.flush()
        proc.wait(timeout=5)
        if proc.returncode != 0:
            _fail("ready_then_quit", f"quit exit {proc.returncode}")
            return False
        _pass("ready_then_quit")
        return True
    finally:
        if proc.poll() is None:
            proc.kill()


def _test_parse_returns_ast(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        resp = _call(proc, {"op": "parse", "sql": "SELECT 1"})
        if not resp.get("ok"):
            _fail("parse_returns_ast", f"err: {resp}")
            return False
        stmts = resp["result"]["statements"]
        if len(stmts) != 1 or stmts[0].get("type") != "SelectStmt":
            _fail("parse_returns_ast", f"unexpected: {stmts}")
            return False
        _pass("parse_returns_ast")
        return True
    finally:
        proc.kill()


def _test_format_uppercases(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        resp = _call(proc, {"op": "format", "sql": "select 1", "keyword_case": "upper"})
        if not resp.get("ok") or "SELECT" not in resp["result"]["formatted"]:
            _fail("format_uppercases", f"got: {resp}")
            return False
        _pass("format_uppercases")
        return True
    finally:
        proc.kill()


def _test_tokenize_returns_tokens(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        resp = _call(proc, {"op": "tokenize", "sql": "SELECT"})
        if not resp.get("ok"):
            _fail("tokenize_returns_tokens", f"err: {resp}")
            return False
        tokens = resp["result"]["tokens"]
        if len(tokens) != 1 or tokens[0]["category"] != "keyword":
            _fail("tokenize_returns_tokens", f"unexpected: {tokens}")
            return False
        _pass("tokenize_returns_tokens")
        return True
    finally:
        proc.kill()


def _test_validate_flags_unknown_table(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        resp = _call(proc, {"op": "analyze", "sql": "select * from does_not_exist"})
        if not resp.get("ok"):
            _fail("validate_flags_unknown_table", f"err: {resp}")
            return False
        diags = resp["result"]["diagnostics"]
        if not any("does_not_exist" in d["message"] for d in diags):
            _fail("validate_flags_unknown_table", f"no matching diag: {diags}")
            return False
        _pass("validate_flags_unknown_table")
        return True
    finally:
        proc.kill()


def _test_validate_uses_provided_tables(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        resp = _call(
            proc,
            {
                "op": "analyze",
                "sql": "select id from users",
                "tables": [{"name": "users", "columns": ["id", "name"]}],
            },
        )
        diags = resp["result"]["diagnostics"]
        if diags:
            _fail("validate_uses_provided_tables", f"unexpected diags: {diags}")
            return False
        _pass("validate_uses_provided_tables")
        return True
    finally:
        proc.kill()


def _test_bad_op_returns_err(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        resp = _call(proc, {"op": "bogus"})
        if resp.get("ok") or "unknown op" not in resp.get("error", ""):
            _fail("bad_op_returns_err", f"expected unknown-op err: {resp}")
            return False
        _pass("bad_op_returns_err")
        return True
    finally:
        proc.kill()


def _test_multi_requests_one_process(ctx: SuiteContext) -> bool:
    proc = _spawn(ctx)
    try:
        _read_ready(proc)
        for sql in ["select 1", "select 2", "select 3"]:
            resp = _call(proc, {"op": "format", "sql": sql})
            if not resp.get("ok") or "SELECT" not in resp["result"]["formatted"]:
                _fail("multi_requests_one_process", f"sql={sql!r}: {resp}")
                return False
        _pass("multi_requests_one_process")
        return True
    finally:
        proc.kill()


# ── Entry point ──────────────────────────────────────────────────────────────


_TESTS = [
    _test_ready_then_quit,
    _test_parse_returns_ast,
    _test_format_uppercases,
    _test_tokenize_returns_tokens,
    _test_validate_flags_unknown_table,
    _test_validate_uses_provided_tables,
    _test_bad_op_returns_err,
    _test_multi_requests_one_process,
]


def run(ctx: SuiteContext) -> int:
    passed = 0
    for t in _TESTS:
        if t(ctx):
            passed += 1
    total = len(_TESTS)
    print(f"\n  {passed}/{total} serve tests passed.")
    return 0 if passed == total else 1
