"""syntaqlite — fast, accurate SQLite SQL tools for Python.

The entry point is :class:`Syntaqlite`. Each instance lets you parse,
format, analyze, and tokenize SQL::

    import syntaqlite

    with syntaqlite.Syntaqlite() as sq:
        print(sq.format_sql("select 1"))
        stmts = sq.parse("select * from users")
"""

from __future__ import annotations

import ctypes
import json
import os
import stat
import subprocess
import sys
from enum import IntEnum, StrEnum
from typing import Any

from .nodes import _wrap

__version__ = "0.7.2"


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


def _ffi_lib_filename() -> str:
    """Return the platform-specific filename for the syntaqlite cdylib."""
    if sys.platform == "win32":
        return "syntaqlite.dll"
    if sys.platform == "darwin":
        return "libsyntaqlite.dylib"
    if sys.platform == "emscripten":
        return "libsyntaqlite.so"
    return "libsyntaqlite.so"


def _ffi_lib_path() -> str:
    """Return the path to the syntaqlite cdylib.

    Resolution order:
    1. ``SYNTAQLITE_FFI_LIB`` environment variable (absolute path, used by
       dev/CI and the in-process verify step).
    2. The cdylib bundled inside the wheel under ``syntaqlite/lib/``.
    """
    override = os.environ.get("SYNTAQLITE_FFI_LIB")
    if override:
        return override
    return os.path.join(os.path.dirname(__file__), "lib", _ffi_lib_filename())


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


class AnalysisOutput(StrEnum):
    """Output format for :meth:`Syntaqlite.analyze`.

    - :attr:`STRUCTURED` — returns a :class:`Analysis` with typed
      diagnostics, lineage, and per-statement data. The default.
    - :attr:`TEXT` — returns a rendered string with source locations and
      help suggestions, matching the CLI's output.
    """

    STRUCTURED = "structured"
    TEXT = "text"


class RenderOptions:
    """Options controlling :attr:`AnalysisOutput.TEXT` rendering.

    Only consulted when ``output=AnalysisOutput.TEXT``.
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
    """A single diagnostic from validation.

    ``start_offset`` and ``end_offset`` are document-absolute byte
    offsets into the SQL passed to :meth:`Syntaqlite.analyze` — measured
    from byte 0 of the input, not relative to a statement.
    """

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

    Returned by :attr:`Statement.lineage` and :attr:`Analysis.lineage`.
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


class Analysis:
    """Result of :meth:`Syntaqlite.analyze`.

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
        return f"Analysis({', '.join(parts)})"


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
    """A catalog schema for :meth:`Syntaqlite.analyze`.

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


# ── In-process transport ──────────────────────────────────────────────────────
#
# The transport carries a single UTF-8 JSON request to the syntaqlite core and
# returns a single UTF-8 JSON response envelope ({"ok": ...}). Envelope decoding
# lives in ``Syntaqlite._call``; the transport deals only in raw I/O.


class _InProcessTransport:
    """Transport that calls the syntaqlite cdylib in-process via ctypes.

    This is the sole transport: every call runs in the host process with no
    subprocess. ctypes releases the GIL for the duration of each foreign call,
    so instances on separate threads run in parallel without extra work.
    """

    __slots__ = ("_lib", "_handle")

    def __init__(
        self,
        *,
        dialect_path: str | None,
        dialect_name: str | None,
    ):
        lib_path = _ffi_lib_path()
        try:
            lib = ctypes.CDLL(lib_path)
        except OSError as e:
            raise SyntaqliteError(
                f"failed to load syntaqlite cdylib at {lib_path!r}: {e}"
            ) from e

        # syntaqlite_rpc_create_sqlite() -> *mut SyntaqliteRpc
        lib.syntaqlite_rpc_create_sqlite.argtypes = []
        lib.syntaqlite_rpc_create_sqlite.restype = ctypes.c_void_p
        # syntaqlite_rpc_call(handle, request, request_len, *out_len) -> *mut u8
        lib.syntaqlite_rpc_call.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_uint64,
            ctypes.POINTER(ctypes.c_uint64),
        ]
        lib.syntaqlite_rpc_call.restype = ctypes.c_void_p
        # syntaqlite_rpc_free(ptr, len)
        lib.syntaqlite_rpc_free.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        lib.syntaqlite_rpc_free.restype = None
        # syntaqlite_rpc_destroy(handle)
        lib.syntaqlite_rpc_destroy.argtypes = [ctypes.c_void_p]
        lib.syntaqlite_rpc_destroy.restype = None

        self._lib = lib

        if dialect_path is not None:
            create_dialect = getattr(lib, "syntaqlite_rpc_create_dialect", None)
            if create_dialect is None:
                raise SyntaqliteError(
                    "in-process transport: dialect loading requires the "
                    "syntaqlite cdylib built with the 'dynload' feature"
                )
            # create_dialect(path, path_len, name, name_len) -> *mut SyntaqliteRpc
            create_dialect.argtypes = [
                ctypes.c_char_p,
                ctypes.c_uint64,
                ctypes.c_char_p,
                ctypes.c_uint64,
            ]
            create_dialect.restype = ctypes.c_void_p
            path_bytes = dialect_path.encode("utf-8")
            if dialect_name is not None:
                name_bytes = dialect_name.encode("utf-8")
                handle = create_dialect(
                    path_bytes, len(path_bytes), name_bytes, len(name_bytes)
                )
            else:
                handle = create_dialect(path_bytes, len(path_bytes), None, 0)
            if not handle:
                raise SyntaqliteError(
                    f"failed to load dialect {dialect_path!r} in-process"
                )
        else:
            handle = lib.syntaqlite_rpc_create_sqlite()
            if not handle:
                raise SyntaqliteError("failed to create in-process SQLite session")

        self._handle = handle

    def request(self, req_json: str) -> bytes:
        req_bytes = req_json.encode("utf-8")
        out_len = ctypes.c_uint64(0)
        ptr = self._lib.syntaqlite_rpc_call(
            self._handle, req_bytes, len(req_bytes), ctypes.byref(out_len)
        )
        if not ptr:
            raise SyntaqliteError("syntaqlite_rpc_call returned null")
        try:
            return ctypes.string_at(ptr, out_len.value)
        finally:
            self._lib.syntaqlite_rpc_free(ptr, out_len.value)

    def close(self) -> None:
        handle = self._handle
        if handle:
            self._handle = None
            self._lib.syntaqlite_rpc_destroy(handle)


# ── Syntaqlite client ────────────────────────────────────────────────────────


class Syntaqlite:
    """Parse, format, analyze, and tokenize SQLite SQL.

    A :class:`Syntaqlite` instance loads the syntaqlite core in-process and
    reuses it across many calls, so create one and reuse it. Not intended for
    concurrent use on one instance: for parallelism, create one per thread.

    Use as a context manager or call :meth:`close` when done. By default
    operates on SQLite syntax; pass ``dialect_path`` to load a compiled
    dialect extension.

    Args:
        dialect_path: Optional path to a dialect shared library
            (``.so``/``.dylib``/``.dll``).
        dialect_name: Optional dialect name for symbol lookup. Required
            only when ``dialect_path`` exports more than one dialect.
    """

    __slots__ = ("_transport", "_closed")

    def __init__(
        self,
        *,
        dialect_path: str | None = None,
        dialect_name: str | None = None,
    ):
        self._transport = _InProcessTransport(
            dialect_path=dialect_path,
            dialect_name=dialect_name,
        )
        self._closed = False

    # ── lifecycle ───────────────────────────────────────────────────────────

    def _call(self, op: str, **params: Any) -> Any:
        if self._closed:
            raise SyntaqliteError("Syntaqlite instance is closed")
        req = json.dumps({"op": op, **params})
        resp_bytes = self._transport.request(req)
        resp = json.loads(resp_bytes)
        if not resp.get("ok"):
            raise SyntaqliteError(resp.get("error", "unknown error"))
        return resp["result"]

    def close(self) -> None:
        if self._closed:
            return
        self._closed = True
        self._transport.close()

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

        ``offset`` is a document-absolute byte offset into ``sql`` (byte 0 =
        first byte of input). ``length`` is the token's byte length.
        """
        return self._call("tokenize", sql=sql)["tokens"]

    def analyze(
        self,
        sql: str,
        schema: Schema | None = None,
        *,
        output: AnalysisOutput | str = AnalysisOutput.STRUCTURED,
        render_options: RenderOptions | None = None,
    ) -> Analysis | str:
        """Analyze SQL against an optional schema.

        Args:
            sql: SQL to analyze.
            schema: Catalog schema (tables, views, DDL, optional modules).
            output: :class:`AnalysisOutput` (or its string value) selecting
                the return shape. :attr:`AnalysisOutput.STRUCTURED`
                returns an :class:`Analysis`; :attr:`AnalysisOutput.TEXT`
                returns a rendered diagnostics string.
            render_options: Fine-grained options for
                :attr:`AnalysisOutput.TEXT` (e.g. source label). Ignored
                for other outputs.
        """
        output = AnalysisOutput(output)
        params: dict[str, Any] = {"sql": sql, "output": output.value}
        if schema is not None:
            params.update(schema._to_request())
        if render_options is not None:
            params["render_options"] = render_options._to_request()

        resp = self._call("analyze", **params)
        if output is AnalysisOutput.TEXT:
            return resp["rendered"]
        return Analysis(resp)


# ── Bundled CLI dispatcher (used by console_scripts entry point) ─────────────


def main():
    """Execute the bundled binary — the `syntaqlite` console script."""
    binary = get_binary_path()
    if sys.platform == "win32":
        sys.exit(subprocess.call([binary] + sys.argv[1:]))
    else:
        os.execvp(binary, [binary] + sys.argv[1:])
