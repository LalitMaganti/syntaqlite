# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""`syntaqlite serve json` RPC suite.

Covers the raw JSON line protocol (framing, op dispatch, error frames) and
the public Python API that wraps it (parse, format_sql, tokenize, validate,
Dialect).
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

from python.dev.integration_tests.suite import SuiteContext

NAME = "serve"
DESCRIPTION = "syntaqlite serve JSON RPC protocol + Python client tests"

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
        resp = _call(proc, {"op": "validate", "sql": "select * from does_not_exist"})
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
                "op": "validate",
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


# ── Public Python API ────────────────────────────────────────────────────────


def _py_run(ctx: SuiteContext, script: str) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["PYTHONPATH"] = str(ctx.root_dir / "python") + os.pathsep + env.get("PYTHONPATH", "")
    env["SYNTAQLITE_BIN"] = str(ctx.binary)
    return subprocess.run(
        [sys.executable, "-c", script],
        env=env,
        capture_output=True,
        text=True,
    )


def _test_py_parse_wraps_nodes(ctx: SuiteContext) -> bool:
    r = _py_run(
        ctx,
        (
            "import syntaqlite;"
            " sq = syntaqlite.Syntaqlite();"
            " stmts = sq.parse('select 1 as x');"
            " assert len(stmts) == 1, stmts;"
            " assert type(stmts[0]).__name__ == 'SelectStmt', type(stmts[0]).__name__;"
            " sq.close();"
            " print('ok')"
        ),
    )
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail("py_parse_wraps_nodes", f"{r.returncode} {r.stdout!r} {r.stderr!r}")
        return False
    _pass("py_parse_wraps_nodes")
    return True


def _test_py_format_sql(ctx: SuiteContext) -> bool:
    r = _py_run(
        ctx,
        (
            "import syntaqlite;"
            " sq = syntaqlite.Syntaqlite();"
            " out = sq.format_sql('select 1');"
            " assert 'SELECT' in out, out;"
            " sq.close();"
            " print('ok')"
        ),
    )
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail("py_format_sql", f"{r.returncode} {r.stdout!r} {r.stderr!r}")
        return False
    _pass("py_format_sql")
    return True


def _test_py_tokenize(ctx: SuiteContext) -> bool:
    r = _py_run(
        ctx,
        (
            "import syntaqlite;"
            " sq = syntaqlite.Syntaqlite();"
            " toks = sq.tokenize('SELECT');"
            " assert toks and toks[0]['category'] == 'keyword', toks;"
            " sq.close();"
            " print('ok')"
        ),
    )
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail("py_tokenize", f"{r.returncode} {r.stdout!r} {r.stderr!r}")
        return False
    _pass("py_tokenize")
    return True


def _test_py_validate(ctx: SuiteContext) -> bool:
    r = _py_run(
        ctx,
        (
            "import syntaqlite;"
            " sq = syntaqlite.Syntaqlite();"
            " schema = syntaqlite.Schema(tables=[syntaqlite.Table('users', ['id'])]);"
            " result = sq.validate('select x from users', schema);"
            " assert any('x' in d.message for d in result.diagnostics), result.diagnostics;"
            " sq.close();"
            " print('ok')"
        ),
    )
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail("py_validate", f"{r.returncode} {r.stdout!r} {r.stderr!r}")
        return False
    _pass("py_validate")
    return True


def _test_py_schema_modules_passthrough(ctx: SuiteContext) -> bool:
    """Supplying `modules` on `Schema` shouldn't break validation for
    dialects that don't implement imports — it's dialect-specific and
    expected to be ignored by SQLite here."""
    r = _py_run(
        ctx,
        (
            "import syntaqlite;"
            " sq = syntaqlite.Syntaqlite();"
            " schema = syntaqlite.Schema("
            "     tables=[syntaqlite.Table('t', ['x'])],"
            "     modules={'stdlib.helpers': 'CREATE TABLE helper(a INT)'},"
            " );"
            " result = sq.validate('SELECT x FROM t', schema);"
            " assert not result.diagnostics, result.diagnostics;"
            " sq.close();"
            " print('ok')"
        ),
    )
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail(
            "py_schema_modules_passthrough",
            f"{r.returncode} {r.stdout!r} {r.stderr!r}",
        )
        return False
    _pass("py_schema_modules_passthrough")
    return True


_LINEAGE_SCRIPT = """
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    schema = syntaqlite.Schema(
        tables=[syntaqlite.Table("users", ["id", "name", "email"])],
    )
    result = sq.validate("SELECT id, name FROM users", schema)
    assert result.lineage is not None, "expected lineage on SELECT"
    assert result.lineage.complete, result.lineage
    cols = result.lineage.columns
    assert [c.name for c in cols] == ["id", "name"], cols
    assert cols[0].origin.table == "users"
    assert cols[0].origin.column == "id"
    assert result.lineage.physical_tables == ["users"]
    print("ok")
"""


def _test_py_validate_lineage(ctx: SuiteContext) -> bool:
    r = _py_run(ctx, _LINEAGE_SCRIPT)
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail("py_validate_lineage", f"{r.returncode} {r.stdout!r} {r.stderr!r}")
        return False
    _pass("py_validate_lineage")
    return True


_CTX_MGR_SCRIPT = """
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    assert "SELECT" in sq.format_sql("select 1")

try:
    sq.parse("select 1")
except syntaqlite.SyntaqliteError:
    print("ok")
else:
    raise AssertionError("expected SyntaqliteError after close")
"""


def _test_py_context_manager(ctx: SuiteContext) -> bool:
    r = _py_run(ctx, _CTX_MGR_SCRIPT)
    if r.returncode != 0 or "ok" not in r.stdout:
        _fail("py_context_manager", f"{r.returncode} {r.stdout!r} {r.stderr!r}")
        return False
    _pass("py_context_manager")
    return True


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
    _test_py_parse_wraps_nodes,
    _test_py_format_sql,
    _test_py_tokenize,
    _test_py_validate,
    _test_py_validate_lineage,
    _test_py_schema_modules_passthrough,
    _test_py_context_manager,
]


def run(ctx: SuiteContext) -> int:
    passed = 0
    for t in _TESTS:
        if t(ctx):
            passed += 1
    total = len(_TESTS)
    print(f"\n  {passed}/{total} serve tests passed.")
    return 0 if passed == total else 1
