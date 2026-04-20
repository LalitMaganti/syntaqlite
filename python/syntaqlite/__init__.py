"""syntaqlite — fast, accurate SQLite SQL tools for Python.

The entry point is :class:`Syntaqlite`. Each instance lets you parse,
format, validate, and tokenize SQL::

    import syntaqlite

    with syntaqlite.Syntaqlite() as sq:
        print(sq.format_sql("select 1"))
        stmts = sq.parse("select * from users")
"""

from __future__ import annotations

import json
import os
import stat
import subprocess
import sys
from enum import IntEnum, StrEnum
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


# ── Result types ─────────────────────────────────────────────────────────────


class DiagnosticCode(IntEnum):
    """Machine-readable kind for a :class:`Diagnostic`."""

    PARSE_ERROR = 0
    UNKNOWN_TABLE = 1
    UNKNOWN_COLUMN = 2
    UNKNOWN_FUNCTION = 3
    UNKNOWN_MODULE = 4
    FUNCTION_ARITY = 5
    CTE_COLUMN_COUNT_MISMATCH = 6


class ValidateOutput(StrEnum):
    """Output format for :meth:`Syntaqlite.validate`.

    - :attr:`STRUCTURED` — returns a :class:`ValidationResult` with typed
      diagnostics, lineage, and per-statement data. The default.
    - :attr:`TEXT` — returns a rendered string with source locations and
      help suggestions, matching the CLI's output.
    """

    STRUCTURED = "structured"
    TEXT = "text"


class RenderOptions:
    """Options controlling :attr:`ValidateOutput.TEXT` rendering.

    Only consulted when ``output=ValidateOutput.TEXT``.
    """

    __slots__ = ("source_name",)

    def __init__(self, *, source_name: str = ""):
        #: Source label shown in rendered diagnostics (analogous to a file
        #: path). Defaults to an empty string.
        self.source_name: str = source_name

    def _to_request(self) -> dict:
        return {"source_name": self.source_name}

    def __repr__(self):
        return f"RenderOptions(source_name={self.source_name!r})"


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


class ColumnOrigin:
    """The physical table and column a result column traces back to."""

    __slots__ = ("table", "column")

    def __init__(self, d: dict):
        self.table: str = d["table"]
        self.column: str = d["column"]

    def __repr__(self):
        return f"{self.table}.{self.column}"


class ColumnLineage:
    """Lineage for a single result column."""

    __slots__ = ("name", "index", "origin")

    def __init__(self, d: dict):
        self.name: str = d["name"]
        self.index: int = d["index"]
        o = d.get("origin")
        self.origin: ColumnOrigin | None = ColumnOrigin(o) if o else None

    def __repr__(self):
        if self.origin:
            return f"ColumnLineage({self.name} <- {self.origin})"
        return f"ColumnLineage({self.name})"


class RelationAccess:
    """A catalog relation (table or view) referenced in a FROM clause."""

    __slots__ = ("name", "kind")

    def __init__(self, d: dict):
        self.name: str = d["name"]
        self.kind: str = d["kind"]

    def __repr__(self):
        return f"RelationAccess({self.name}, {self.kind})"


class Lineage:
    """Column lineage for a query-bearing statement.

    Returned by :attr:`Statement.lineage` and :attr:`ValidationResult.lineage`.
    `None` for statements that don't contain a query body.
    """

    __slots__ = ("complete", "columns", "relations", "physical_tables", "unexpanded_views")

    def __init__(self, d: dict):
        self.complete: bool = d["complete"]
        self.columns: list[ColumnLineage] = [ColumnLineage(c) for c in d.get("columns", [])]
        self.relations: list[RelationAccess] = [RelationAccess(r) for r in d.get("relations", [])]
        self.physical_tables: list[str] = list(d.get("physical_tables", []))
        self.unexpanded_views: list[str] = list(d.get("unexpanded_views", []))

    def __repr__(self):
        status = "complete" if self.complete else "partial"
        return f"Lineage({status}, {len(self.columns)} columns)"


class Statement:
    """Per-statement analysis result."""

    __slots__ = ("diagnostics", "defined_relations", "lineage", "source")

    def __init__(self, d: dict):
        self.diagnostics: list[Diagnostic] = [Diagnostic(x) for x in d.get("diagnostics", [])]
        self.defined_relations: list[DefinedRelation] = [
            DefinedRelation(x) for x in d.get("defined_relations", [])
        ]
        lin = d.get("lineage")
        self.lineage: Lineage | None = Lineage(lin) if lin else None
        self.source: str = d.get("source", "")

    def __repr__(self):
        parts = [f"{len(self.diagnostics)} diagnostics"]
        if self.lineage:
            parts.append(str(self.lineage))
        if self.defined_relations:
            parts.append(f"{len(self.defined_relations)} defined relations")
        return f"Statement({', '.join(parts)})"


class ValidationResult:
    """Result of :meth:`Syntaqlite.validate`.

    ``lineage`` is the :class:`Lineage` of the final query-bearing statement,
    or ``None`` when no statement had a query body.
    """

    __slots__ = ("diagnostics", "lineage", "statements")

    def __init__(self, d: dict):
        self.diagnostics: list[Diagnostic] = [Diagnostic(x) for x in d.get("diagnostics", [])]
        lin = d.get("lineage")
        self.lineage: Lineage | None = Lineage(lin) if lin else None
        self.statements: list[Statement] = [Statement(s) for s in d.get("statements", [])]

    def __repr__(self):
        parts = [f"{len(self.diagnostics)} diagnostics"]
        if self.lineage:
            parts.append(str(self.lineage))
        parts.append(f"{len(self.statements)} statements")
        return f"ValidationResult({', '.join(parts)})"


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


class Schema:
    """A catalog schema for :meth:`Syntaqlite.validate`.

    Everything that contributes to the validator's catalog lives here:

    - :class:`Table` / :class:`View` entries — structured column lists.
    - ``ddl`` — raw ``CREATE TABLE`` / ``CREATE VIEW`` text, parsed once.
    - ``modules`` — dialect-specific. For dialects that support module
      imports (e.g. Perfetto's ``INCLUDE PERFETTO MODULE``), a map from
      dotted module path to SQL source. Module sources are loaded lazily
      when the analyzer encounters an import. Ignored by dialects without
      module support.
    """

    __slots__ = ("tables", "views", "ddl", "modules")

    def __init__(
        self,
        *,
        tables: list[Table] | None = None,
        views: list[View] | None = None,
        ddl: str | None = None,
        modules: dict[str, str] | None = None,
    ):
        self.tables: list[Table] = list(tables) if tables else []
        self.views: list[View] = list(views) if views else []
        self.ddl: str | None = ddl
        self.modules: dict[str, str] | None = dict(modules) if modules else None

    def _to_request(self) -> dict:
        d: dict[str, Any] = {}
        if self.tables:
            d["tables"] = [t._to_dict() for t in self.tables]
        if self.views:
            d["views"] = [v._to_dict() for v in self.views]
        if self.ddl is not None:
            d["schema_ddl"] = self.ddl
        if self.modules:
            d["modules"] = self.modules
        return d

    def __repr__(self):
        parts = []
        if self.tables:
            parts.append(f"{len(self.tables)} tables")
        if self.views:
            parts.append(f"{len(self.views)} views")
        if self.ddl is not None:
            parts.append("ddl")
        if self.modules:
            parts.append(f"{len(self.modules)} modules")
        return f"Schema({', '.join(parts) if parts else 'empty'})"


class FormatError(Exception):
    """Raised by :meth:`Syntaqlite.format_sql` when the input cannot be parsed."""


class SyntaqliteError(RuntimeError):
    """Base class for runtime errors raised by :class:`Syntaqlite`."""


# ── Syntaqlite client ────────────────────────────────────────────────────────


class Syntaqlite:
    """Parse, format, validate, and tokenize SQLite SQL.

    A :class:`Syntaqlite` instance manages its own long-lived worker, so
    create one and reuse it across many calls. Not intended for concurrent
    use: if you want parallelism, create one instance per thread.

    Use as a context manager or call :meth:`close` when done. By default
    operates on SQLite syntax; pass ``dialect_path`` to load a compiled
    dialect extension.

    Args:
        dialect_path: Optional path to a dialect shared library
            (``.so``/``.dylib``/``.dll``).
        dialect_name: Optional dialect name for symbol lookup. Required
            only when ``dialect_path`` exports more than one dialect.
        binary: Override the path to the ``syntaqlite`` CLI. Defaults to
            the binary shipped with the wheel.
    """

    __slots__ = ("_proc", "_stdin", "_stdout", "_closed")

    def __init__(
        self,
        *,
        dialect_path: str | None = None,
        dialect_name: str | None = None,
        binary: str | None = None,
    ):
        bin_path = binary if binary is not None else get_binary_path()
        argv: list[str] = [bin_path, "--no-config"]
        if dialect_path is not None:
            argv += ["--dialect", dialect_path]
        if dialect_name is not None:
            argv += ["--dialect-name", dialect_name]
        argv += ["serve", "json"]

        self._proc = subprocess.Popen(
            argv,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=0,
        )
        self._stdin = self._proc.stdin
        self._stdout = self._proc.stdout
        self._closed = False
        self._wait_ready()

    # ── subprocess lifecycle ────────────────────────────────────────────────

    def _wait_ready(self) -> None:
        line = self._stdout.readline()
        if line.strip() != b"READY":
            stderr = self._proc.stderr.read().decode("utf-8", errors="replace")
            raise RuntimeError(
                f"syntaqlite serve failed to start: expected 'READY', got {line!r}.\n{stderr}"
            )

    def _call(self, op: str, **params: Any) -> Any:
        if self._closed:
            raise SyntaqliteError("Syntaqlite instance is closed")
        req = {"op": op, **params}
        self._stdin.write((json.dumps(req) + "\n").encode("utf-8"))
        self._stdin.flush()
        resp_bytes = self._stdout.readline()
        if not resp_bytes:
            raise SyntaqliteError("syntaqlite worker exited unexpectedly")
        resp = json.loads(resp_bytes)
        if not resp.get("ok"):
            raise SyntaqliteError(resp.get("error", "unknown error"))
        return resp["result"]

    def close(self) -> None:
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

    def __enter__(self) -> Syntaqlite:
        return self

    def __exit__(self, *_exc: object) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass

    # ── Public API ──────────────────────────────────────────────────────────

    def parse(self, sql: str) -> list:
        """Parse SQL into typed AST nodes."""
        resp = self._call("parse", sql=sql)
        return [_wrap(d) for d in resp["statements"]]

    def parse_raw(self, sql: str) -> list:
        """Parse SQL into JSON-shaped dicts (no typed wrapping)."""
        return self._call("parse", sql=sql)["statements"]

    def format_sql(
        self,
        sql: str,
        *,
        line_width: int = 80,
        indent_width: int = 2,
        keyword_case: str = "upper",
        semicolons: bool = True,
    ) -> str:
        """Format SQL with configurable options.

        Raises:
            FormatError: if the input fails to parse.
        """
        try:
            resp = self._call(
                "format",
                sql=sql,
                line_width=line_width,
                indent_width=indent_width,
                keyword_case=keyword_case,
                semicolons=semicolons,
            )
        except SyntaqliteError as e:
            raise FormatError(str(e)) from None
        return resp["formatted"]

    def tokenize(self, sql: str) -> list[dict]:
        """Tokenize SQL into a list of token dicts.

        Each dict has: text (str), offset (int), length (int), type (int), category (str).
        """
        return self._call("tokenize", sql=sql)["tokens"]

    def validate(
        self,
        sql: str,
        schema: Schema | None = None,
        *,
        output: ValidateOutput | str = ValidateOutput.STRUCTURED,
        render_options: RenderOptions | None = None,
    ) -> ValidationResult | str:
        """Validate SQL against an optional schema.

        Args:
            sql: SQL to validate.
            schema: Catalog schema (tables, views, DDL, optional modules).
            output: :class:`ValidateOutput` (or its string value) selecting
                the return shape. :attr:`ValidateOutput.STRUCTURED`
                returns a :class:`ValidationResult`; :attr:`ValidateOutput.TEXT`
                returns a rendered diagnostics string.
            render_options: Fine-grained options for
                :attr:`ValidateOutput.TEXT` (e.g. source label). Ignored
                for other outputs.
        """
        output = ValidateOutput(output)
        params: dict[str, Any] = {"sql": sql, "output": output.value}
        if schema is not None:
            params.update(schema._to_request())
        if render_options is not None:
            params["render_options"] = render_options._to_request()

        resp = self._call("validate", **params)
        if output is ValidateOutput.TEXT:
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
