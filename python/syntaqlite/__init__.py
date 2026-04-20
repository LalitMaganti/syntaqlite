"""syntaqlite — SQLite SQL tools (CLI+RPC Python client).

Spawns the bundled `syntaqlite serve` subprocess once and speaks a
line-delimited JSON protocol over stdio. See
`syntaqlite-cli/src/commands/serve.rs` for the wire spec.
"""

from __future__ import annotations

import atexit
import json
import os
import stat
import subprocess
import sys
import threading
from enum import IntEnum
from typing import Any

from .nodes import _wrap

__version__ = "0.4.2"


# ── Binary discovery ──────────────────────────────────────────────────────────


def get_binary_path() -> str:
    """Return the path to the bundled syntaqlite binary.

    Resolution order:
    1. ``SYNTAQLITE_BIN`` environment variable (used by dev/CI).
    2. The binary bundled inside the wheel under ``syntaqlite/bin/``.
    """
    override = os.environ.get("SYNTAQLITE_BIN")
    if override:
        return override

    binary = os.path.join(os.path.dirname(__file__), "bin", "syntaqlite")
    if sys.platform == "win32":
        binary += ".exe"

    if sys.platform != "win32" and os.path.exists(binary):
        current_mode = os.stat(binary).st_mode
        if not (current_mode & stat.S_IXUSR):
            os.chmod(binary, current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    return binary


# ── RPC client ────────────────────────────────────────────────────────────────


class _ServeError(RuntimeError):
    """Raised when the serve subprocess returns `{"ok": false, ...}`."""


class _ServeClient:
    """Synchronous JSON-RPC client for the `syntaqlite serve` protocol.

    Owns a long-lived subprocess and a lock so the same client is safe to
    share across threads. Construct additional clients for parallelism.
    """

    def __init__(self, binary: str, extra_args: list[str] | None = None):
        argv = [binary, "--no-config"] + (extra_args or []) + ["serve"]
        self._proc = subprocess.Popen(
            argv,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=0,
        )
        self._stdin = self._proc.stdin
        self._stdout = self._proc.stdout
        self._lock = threading.Lock()
        self._closed = False
        self._wait_ready()
        atexit.register(self.close)

    def _wait_ready(self) -> None:
        line = self._stdout.readline()
        if line.strip() != b"READY":
            stderr = self._proc.stderr.read().decode("utf-8", errors="replace")
            raise RuntimeError(
                f"syntaqlite serve failed to start: expected 'READY', got {line!r}.\n{stderr}"
            )

    def call(self, op: str, **params: Any) -> Any:
        """Send `{"op": op, **params}` and return `result` on success."""
        req = {"op": op, **params}
        payload = (json.dumps(req) + "\n").encode("utf-8")
        with self._lock:
            if self._closed:
                raise _ServeError("serve client is closed")
            self._stdin.write(payload)
            self._stdin.flush()
            resp_bytes = self._stdout.readline()
        if not resp_bytes:
            raise _ServeError("serve subprocess closed stdout unexpectedly")
        resp = json.loads(resp_bytes)
        if not resp.get("ok"):
            raise _ServeError(resp.get("error", "unknown error"))
        return resp["result"]

    def close(self) -> None:
        with self._lock:
            if self._closed:
                return
            self._closed = True
            try:
                self._stdin.write(b'{"op":"quit"}\n')
                self._stdin.flush()
            except (BrokenPipeError, OSError):
                pass
            try:
                self._stdin.close()
            except OSError:
                pass
            try:
                self._stdout.close()
            except OSError:
                pass
        try:
            self._proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self._proc.kill()


_default_client: _ServeClient | None = None
_default_lock = threading.Lock()


def _get_default_client() -> _ServeClient:
    global _default_client
    if _default_client is None:
        with _default_lock:
            if _default_client is None:
                _default_client = _ServeClient(get_binary_path())
    return _default_client


# ── Public API ────────────────────────────────────────────────────────────────


class FormatError(Exception):
    """Raised by :func:`format_sql` when the input cannot be parsed."""


def parse(sql: str, *, dialect: Dialect | None = None) -> list:
    """Parse SQL into typed AST nodes."""
    client = dialect._client if dialect else _get_default_client()
    resp = client.call("parse", sql=sql)
    return [_wrap(d) for d in resp["statements"]]


def parse_raw(sql: str, *, dialect: Dialect | None = None) -> list:
    """Parse SQL into plain JSON-shaped dicts (no typed wrapping)."""
    client = dialect._client if dialect else _get_default_client()
    return client.call("parse", sql=sql)["statements"]


def format_sql(
    sql: str,
    *,
    dialect: Dialect | None = None,
    line_width: int = 80,
    indent_width: int = 2,
    keyword_case: str = "upper",
    semicolons: bool = True,
) -> str:
    """Format SQL with configurable options.

    Raises:
        FormatError: if the input fails to parse.
    """
    client = dialect._client if dialect else _get_default_client()
    try:
        resp = client.call(
            "format",
            sql=sql,
            line_width=line_width,
            indent_width=indent_width,
            keyword_case=keyword_case,
            semicolons=semicolons,
        )
    except _ServeError as e:
        raise FormatError(str(e)) from None
    return resp["formatted"]


def tokenize(sql: str, *, dialect: Dialect | None = None) -> list[dict]:
    """Tokenize SQL into a list of token dicts.

    Each dict has: text (str), offset (int), length (int), type (int), category (str).
    """
    client = dialect._client if dialect else _get_default_client()
    return client.call("tokenize", sql=sql)["tokens"]


# ── Dialect ───────────────────────────────────────────────────────────────────


class Dialect:
    """A loaded dialect extension.

    The dialect is loaded by spawning a fresh `syntaqlite serve` with the
    appropriate ``--dialect`` / ``--dialect-name`` flags. The subprocess
    lives for as long as the :class:`Dialect` object.

    Args:
        path: Path to a shared library (.so/.dylib/.dll) containing the dialect.
        name: Dialect name. Resolves the ``syntaqlite_{name}_grammar`` symbol.
              If None, resolves ``syntaqlite_grammar``.
    """

    __slots__ = ("_client", "_path", "_name")

    def __init__(self, path: str, name: str | None = None):
        self._path = path
        self._name = name
        extra = ["--dialect", path]
        if name is not None:
            extra += ["--dialect-name", name]
        self._client = _ServeClient(get_binary_path(), extra_args=extra)

    def close(self) -> None:
        self._client.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass


# ── Result types ─────────────────────────────────────────────────────────────


class DiagnosticCode(IntEnum):
    """Machine-readable kind for a :class:`Diagnostic`.

    Mirrors the integer codes produced by ``syntaqlite serve``.
    """

    PARSE_ERROR = 0
    UNKNOWN_TABLE = 1
    UNKNOWN_COLUMN = 2
    UNKNOWN_FUNCTION = 3
    UNKNOWN_MODULE = 4
    FUNCTION_ARITY = 5
    CTE_COLUMN_COUNT_MISMATCH = 6


class Diagnostic:
    """A single diagnostic from validation."""

    __slots__ = ("severity", "message", "start_offset", "end_offset", "code")

    def __init__(self, d: dict):
        self.severity: str = d["severity"]
        self.message: str = d["message"]
        self.start_offset: int = d["start_offset"]
        self.end_offset: int = d["end_offset"]
        self.code: DiagnosticCode = DiagnosticCode(d["code"])

    def __repr__(self):
        return f"Diagnostic({self.severity}: {self.message!r})"


class DefinedRelation:
    """A relation defined by a DDL statement (CREATE TABLE / CREATE VIEW)."""

    __slots__ = ("name", "is_view")

    def __init__(self, d: dict):
        self.name: str = d["name"]
        self.is_view: bool = d["is_view"]

    def __repr__(self):
        kind = "view" if self.is_view else "table"
        return f"DefinedRelation({self.name!r}, {kind})"


class Statement:
    """Per-statement analysis result."""

    __slots__ = ("diagnostics", "defined_relations", "source")

    def __init__(self, d: dict):
        self.diagnostics: list[Diagnostic] = [Diagnostic(x) for x in d.get("diagnostics", [])]
        self.defined_relations: list[DefinedRelation] = [
            DefinedRelation(x) for x in d.get("defined_relations", [])
        ]
        self.source: str = d.get("source", "")

    def __repr__(self):
        parts = [f"{len(self.diagnostics)} diagnostics"]
        if self.defined_relations:
            parts.append(f"{len(self.defined_relations)} defined relations")
        return f"Statement({', '.join(parts)})"


class ValidationResult:
    """Result of validate() — aggregated diagnostics plus per-statement data."""

    __slots__ = ("diagnostics", "statements")

    def __init__(self, d: dict):
        self.diagnostics: list[Diagnostic] = [Diagnostic(x) for x in d.get("diagnostics", [])]
        self.statements: list[Statement] = [Statement(s) for s in d.get("statements", [])]

    def __repr__(self):
        return f"ValidationResult({len(self.diagnostics)} diagnostics, {len(self.statements)} statements)"


class Table:
    """A table definition for schema registration."""

    __slots__ = ("name", "columns")

    def __init__(self, name: str, columns: list[str] | None = None):
        self.name = name
        self.columns = columns

    def _to_dict(self) -> dict:
        return {"name": self.name, "columns": self.columns}

    def __repr__(self):
        return f"Table({self.name!r}, {self.columns!r})" if self.columns else f"Table({self.name!r})"


class View:
    """A view definition for schema registration."""

    __slots__ = ("name", "columns")

    def __init__(self, name: str, columns: list[str] | None = None):
        self.name = name
        self.columns = columns

    def _to_dict(self) -> dict:
        return {"name": self.name, "columns": self.columns}

    def __repr__(self):
        return f"View({self.name!r}, {self.columns!r})" if self.columns else f"View({self.name!r})"


def validate(
    sql: str,
    *,
    tables: list[Table] | None = None,
    views: list[View] | None = None,
    schema_ddl: str | None = None,
    render: bool = False,
    dialect: Dialect | None = None,
) -> ValidationResult | str:
    """Validate SQL against an optional schema.

    Args:
        sql: SQL to validate.
        tables: Schema tables.
        views: Schema views.
        schema_ddl: DDL to parse as schema (CREATE TABLE/VIEW statements).
        render: If True, return rendered diagnostics string instead.
        dialect: Loaded dialect (default: SQLite).
    """
    params: dict[str, Any] = {"sql": sql, "render": render}
    if tables is not None:
        params["tables"] = [t._to_dict() for t in tables]
    if views is not None:
        params["views"] = [v._to_dict() for v in views]
    if schema_ddl is not None:
        params["schema_ddl"] = schema_ddl

    client = dialect._client if dialect else _get_default_client()
    resp = client.call("validate", **params)
    if render:
        return resp["rendered"]
    return ValidationResult(resp)


# ── Bundled CLI dispatcher (used by console_scripts entry point) ─────────────


def main():
    """Execute the bundled binary — the `syntaqlite` console script."""
    binary = get_binary_path()
    if sys.platform == "win32":
        sys.exit(subprocess.call([binary] + sys.argv[1:]))
    else:
        os.execvp(binary, [binary] + sys.argv[1:])
