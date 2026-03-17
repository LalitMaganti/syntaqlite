"""syntaqlite — SQLite SQL tools."""

import os
import stat
import subprocess
import sys

__version__ = "0.2.3"

# Library API (requires _syntaqlite C extension).
try:
    from ._syntaqlite import FormatError, format_sql, parse, tokenize
    from ._syntaqlite import validate as _validate_raw
except ImportError:
    _validate_raw = None


# ── Result types ─────────────────────────────────────────────────────────────


class Diagnostic:
    """A single diagnostic from validation."""

    __slots__ = ("severity", "message", "start_offset", "end_offset")

    def __init__(self, d: dict):
        self.severity: str = d["severity"]
        self.message: str = d["message"]
        self.start_offset: int = d["start_offset"]
        self.end_offset: int = d["end_offset"]

    def __repr__(self):
        return f"Diagnostic({self.severity}: {self.message!r})"


class ColumnOrigin:
    """The physical table and column a result column traces back to."""

    __slots__ = ("table", "column")

    def __init__(self, d: dict):
        self.table: str = d["table"]
        self.column: str = d["column"]

    def __repr__(self):
        return f"{self.table}.{self.column}"


class ColumnLineage:
    """Lineage information for a single result column."""

    __slots__ = ("name", "index", "origin")

    def __init__(self, d: dict):
        self.name: str = d["name"]
        self.index: int = d["index"]
        o = d["origin"]
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
    """Column lineage for a SELECT statement."""

    __slots__ = ("complete", "columns", "relations", "tables")

    def __init__(self, d: dict):
        self.complete: bool = d["complete"]
        self.columns: list[ColumnLineage] = [ColumnLineage(c) for c in d["columns"]]
        self.relations: list[RelationAccess] = [RelationAccess(r) for r in d["relations"]]
        self.tables: list[str] = d["tables"]

    def __repr__(self):
        status = "complete" if self.complete else "partial"
        return f"Lineage({status}, {len(self.columns)} columns)"


class Table:
    """A table definition for schema registration."""

    __slots__ = ("name", "columns")

    def __init__(self, name: str, columns: list[str] | None = None):
        self.name = name
        self.columns = columns

    def _to_dict(self) -> dict:
        return {"name": self.name, "columns": self.columns}

    def __repr__(self):
        if self.columns:
            return f"Table({self.name!r}, {self.columns!r})"
        return f"Table({self.name!r})"


class View:
    """A view definition for schema registration."""

    __slots__ = ("name", "columns")

    def __init__(self, name: str, columns: list[str] | None = None):
        self.name = name
        self.columns = columns

    def _to_dict(self) -> dict:
        return {"name": self.name, "columns": self.columns}

    def __repr__(self):
        if self.columns:
            return f"View({self.name!r}, {self.columns!r})"
        return f"View({self.name!r})"


class ValidationResult:
    """Result of validate() — diagnostics and optional lineage."""

    __slots__ = ("diagnostics", "lineage")

    def __init__(self, d: dict):
        self.diagnostics: list[Diagnostic] = [Diagnostic(x) for x in d["diagnostics"]]
        lin = d["lineage"]
        self.lineage: Lineage | None = Lineage(lin) if lin else None

    def __repr__(self):
        parts = [f"{len(self.diagnostics)} diagnostics"]
        if self.lineage:
            parts.append(str(self.lineage))
        return f"ValidationResult({', '.join(parts)})"


def validate(
    sql: str,
    *,
    tables: list[Table] | None = None,
    views: list[View] | None = None,
    schema_ddl: str | None = None,
    render: bool = False,
):
    """Validate SQL against an optional schema.

    Args:
        sql: SQL to validate.
        tables: Schema tables.
        views: Schema views.
        schema_ddl: DDL to parse as schema (CREATE TABLE/VIEW statements).
        render: If True, return rendered diagnostics string instead.

    Returns:
        ValidationResult (or str when render=True).
    """
    raw_tables = [t._to_dict() for t in tables] if tables else None
    raw_views = [v._to_dict() for v in views] if views else None
    raw = _validate_raw(
        sql,
        tables=raw_tables,
        views=raw_views,
        schema_ddl=schema_ddl,
        render=render,
    )
    if render:
        return raw
    return ValidationResult(raw)


def get_binary_path():
    """Return the path to the bundled syntaqlite binary."""
    binary = os.path.join(os.path.dirname(__file__), "bin", "syntaqlite")
    if sys.platform == "win32":
        binary += ".exe"

    # Ensure binary is executable on Unix (wheel extraction strips permissions)
    if sys.platform != "win32":
        current_mode = os.stat(binary).st_mode
        if not (current_mode & stat.S_IXUSR):
            os.chmod(
                binary,
                current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH,
            )

    return binary


def main():
    """Execute the bundled binary."""
    binary = get_binary_path()

    if sys.platform == "win32":
        sys.exit(subprocess.call([binary] + sys.argv[1:]))
    else:
        os.execvp(binary, [binary] + sys.argv[1:])
