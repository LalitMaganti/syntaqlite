+++
title = "Python API reference"
description = "The Syntaqlite class and its methods, exceptions, and result types."
weight = 6
+++

# Python API reference

The Python package calls the syntaqlite core in-process through a bundled
native library (loaded with `ctypes`), and also ships the `syntaqlite` CLI
binary for the command-line entry point. Requires Python 3.10+. Wheels are
published for macOS (arm64, x86_64), Linux (x86_64, aarch64), and Windows
(x86_64).

```python
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    print(sq.format_sql("select 1"))
    stmts = sq.parse("select * from users")
```

## `syntaqlite.Syntaqlite`

The entry point for all SQL operations. Create one instance and reuse it
across many calls — each instance loads the syntaqlite core in-process on
construction and keeps it ready until `close()` is called.

```python
syntaqlite.Syntaqlite(*, dialect_path=None, dialect_name=None)
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `dialect_path` | `str \| None` | `None` | Path to a dialect shared library (`.so`/`.dylib`/`.dll`). Defaults to SQLite. |
| `dialect_name` | `str \| None` | `None` | Dialect name. Required only when `dialect_path` exports more than one dialect. |

Supports the context-manager protocol, so `with syntaqlite.Syntaqlite() as sq:`
cleans up automatically. Not intended for concurrent use — if you want
parallelism, create one instance per thread.

### `sq.format_sql`

Format SQL with configurable options.

```python
sq.format_sql(sql, *, line_width=80, indent_width=2, keyword_case="upper", semicolons=True)
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | `str` | — | SQL to format |
| `line_width` | `int` | `80` | Maximum line width |
| `indent_width` | `int` | `2` | Spaces per indent level |
| `keyword_case` | `str` | `"upper"` | `"upper"` or `"lower"` |
| `semicolons` | `bool` | `True` | Append semicolons to statements |

**Returns:** `str` — the formatted SQL.

**Raises:** [`FormatError`](#syntaqlite-formaterror) when the input cannot
be parsed.

```python
>>> sq.format_sql("select 1")
'SELECT 1;\n'
>>> sq.format_sql("select 1", keyword_case="lower", semicolons=False)
'select 1\n'
```

### `sq.parse`

Parse SQL into a list of typed AST nodes.

```python
sq.parse(sql)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `sql` | `str` | SQL to parse (may contain multiple statements) |

**Returns:** `list` — one typed AST node per statement. Each node is a
`__slots__` class (e.g. `SelectStmt`, `CreateTableStmt`) with typed
attributes.

```python
>>> stmt = sq.parse("SELECT 1 + 2 FROM foo")[0]
>>> type(stmt).__name__
'SelectStmt'
>>> stmt.columns[0].expr
BinaryExpr(...)
>>> stmt.from_clause
TableRef(...)
>>> stmt.where_clause is None
True
```

Enum and flag fields are `IntEnum`/`IntFlag` from `syntaqlite.enums`:

```python
>>> from syntaqlite.enums import BinaryOp
>>> stmt.columns[0].expr.op
<BinaryOp.PLUS: 0>
```

Node classes support `isinstance` checks:

```python
>>> from syntaqlite.nodes import SelectStmt
>>> isinstance(stmt, SelectStmt)
True
```

The parser recovers from errors and continues; any unparseable fragment
comes through as an `Error` node in the list.

### `sq.parse_raw`

Same as [`parse`](#sq-parse) but returns plain JSON-shaped dicts without
the typed-class wrapping. Use this for performance-sensitive code that
doesn't need attribute-style access.

```python
>>> sq.parse_raw("SELECT 1")[0]["type"]
'SelectStmt'
```

### `sq.analyze`

Analyze SQL against an optional [`Schema`](#syntaqlite-schema).

```python
sq.analyze(sql, schema=None, *, output=AnalysisOutput.STRUCTURED, render_options=None)
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | `str` | — | SQL to analyze |
| `schema` | [`Schema`](#syntaqlite-schema) `\| None` | `None` | Catalog schema to analyze against |
| `output` | [`AnalysisOutput`](#analysisoutput) `\| str` | `STRUCTURED` | Return shape — typed result or rendered string |
| `render_options` | [`RenderOptions`](#renderoptions) `\| None` | `None` | Fine-grained options for text rendering (source label, etc.). Ignored unless `output=TEXT`. |

`Schema` can be built from explicit tables/views, from DDL text, or both:

```python
sq.analyze(
    sql,
    syntaqlite.Schema(
        tables=[syntaqlite.Table("users", ["id", "name"])],
        views=[syntaqlite.View("active", ["id"])],
    ),
)
```

**Returns** (when `output=AnalysisOutput.STRUCTURED`): an [`Analysis`](#analysis).

**Returns** (when `output=AnalysisOutput.TEXT`): `str` with the diagnostics rendered
with source context, matching the CLI output.

```python
>>> schema = syntaqlite.Schema(tables=[syntaqlite.Table("users", ["id", "name"])])
>>> r = sq.analyze("SELECT id FROM users", schema)
>>> r.diagnostics
[]

>>> # Rendered text output with a custom file label
>>> text = sq.analyze(
...     "SELECT nme FROM users", schema,
...     output=syntaqlite.AnalysisOutput.TEXT,
...     render_options=syntaqlite.RenderOptions(source_name="query.sql"),
... )
>>> print(text)
error: unknown column 'nme'
 --> query.sql:1:8
...
```

### `sq.tokenize`

Tokenize SQL into a list of token dicts.

```python
sq.tokenize(sql)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `sql` | `str` | SQL to tokenize |

**Returns:** `list[dict]` — one entry per token:

| Key | Type | Description |
|-----|------|-------------|
| `text` | `str` | Token text |
| `offset` | `int` | Byte offset in the source |
| `length` | `int` | Length of the token in bytes |
| `type` | `int` | Internal token type ID |
| `category` | `str` | `"keyword"`, `"identifier"`, `"string"`, `"number"`, `"operator"`, `"punctuation"`, `"comment"`, `"parameter"`, `"function"`, `"type"`, or `"other"` |

```python
>>> sq.tokenize("SELECT 1")
[{'text': 'SELECT', 'offset': 0, 'length': 6, 'type': 161, 'category': 'keyword'},
 ...]
```

### `sq.close`

Release the worker and stop accepting calls. Called automatically when
the instance is used as a context manager.

```python
sq.close()
```

After `close()`, any method call raises
[`SyntaqliteError`](#syntaqlite-syntaqliteerror).

## Schema types

### `syntaqlite.Schema`

A catalog schema. Everything that contributes to the analyzer's catalog
lives here — pick whichever combination fits your use case.

```python
syntaqlite.Schema(*, tables=None, views=None, ddl=None, modules=None)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `tables` | `list[Table] \| None` | Structured table definitions |
| `views` | `list[View] \| None` | Structured view definitions |
| `ddl` | `str \| None` | Raw DDL (`CREATE TABLE` / `CREATE VIEW`) text, parsed once |
| `modules` | `dict[str, str] \| None` | **Dialect-specific.** Map from dotted module path to SQL source, loaded lazily when the analyzer encounters an import (e.g. Perfetto's `INCLUDE PERFETTO MODULE`). Ignored by dialects without module support. |

### `syntaqlite.Table`

```python
syntaqlite.Table(name, columns=None)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `str` | Table name |
| `columns` | `list[str] \| None` | Column names. When `None`, any column reference is accepted. |

### `syntaqlite.View`

```python
syntaqlite.View(name, columns=None)
```

Same fields as [`Table`](#syntaqlite-table).

## Result types

### `Analysis`

Returned by [`analyze`](#sq-analyze) when `output=AnalysisOutput.STRUCTURED`.

| Attribute | Type | Description |
|-----------|------|-------------|
| `diagnostics` | `list[Diagnostic]` | All diagnostics, aggregated across statements |
| `statements` | `list[Statement]` | Per-statement analysis |
| `lineage` | [`Lineage`](#lineage) `\| None` | Column lineage of the final query-bearing statement; `None` when no statement had a query body |

### `Statement`

Per-statement analysis, available on `Analysis.statements`.

| Attribute | Type | Description |
|-----------|------|-------------|
| `source` | `str` | The SQL source text for this statement |
| `diagnostics` | `list[Diagnostic]` | Diagnostics for this statement |
| `defined_relations` | `list[DefinedRelation]` | Tables/views defined by DDL statements |
| `lineage` | [`Lineage`](#lineage) `\| None` | Column lineage; `None` if this statement has no query body |

### `Lineage`

Column lineage for a query-bearing statement.

| Attribute | Type | Description |
|-----------|------|-------------|
| `complete` | `bool` | `True` if all sources were fully resolved |
| `columns` | `list[ColumnLineage]` | Per-column lineage |
| `relations` | `list[RelationAccess]` | Catalog relations referenced directly in `FROM` |
| `physical_tables` | `list[str]` | Physical table names accessed after CTE/view expansion |
| `unexpanded_views` | `list[str]` | Views whose bodies weren't available for expansion (non-empty implies `complete=False`) |

### `ColumnLineage`

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | `str` | Output column name (alias or inferred) |
| `index` | `int` | Zero-based position in the result column list |
| `origin` | [`ColumnOrigin`](#columnorigin) `\| None` | Origin table/column, or `None` for expressions/literals/aggregates |

### `ColumnOrigin`

| Attribute | Type | Description |
|-----------|------|-------------|
| `table` | `str` | Physical table name |
| `column` | `str` | Column name in that table |

### `RelationAccess`

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | `str` | Relation name as it appears in the catalog |
| `kind` | `str` | `"table"` or `"view"` |

### `Diagnostic`

| Attribute | Type | Description |
|-----------|------|-------------|
| `severity` | `str` | `"error"`, `"warning"`, `"info"`, or `"hint"` |
| `message` | `str` | Diagnostic message |
| `start_offset` | `int` | Byte offset of the start of the span |
| `end_offset` | `int` | Byte offset of the end of the span |
| `code` | [`DiagnosticCode`](#diagnosticcode) | Machine-readable kind |

### `AnalysisOutput`

`StrEnum` selecting the return shape of [`analyze`](#sq-analyze):

| Name | Value | Meaning |
|------|-------|---------|
| `STRUCTURED` | `"structured"` | Return an [`Analysis`](#analysis) (default) |
| `TEXT` | `"text"` | Return a rendered diagnostics string |

### `RenderOptions`

Options that shape [`AnalysisOutput.TEXT`](#analysisoutput) output.

```python
syntaqlite.RenderOptions(*, source_name="")
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `source_name` | `str` | `""` | Source label shown in rendered diagnostics (analogous to a file path). |

### `DiagnosticCode`

`IntEnum` of machine-readable diagnostic kinds:

| Name | Value |
|------|-------|
| `PARSE_ERROR` | `0` |
| `UNKNOWN_TABLE` | `1` |
| `UNKNOWN_COLUMN` | `2` |
| `UNKNOWN_FUNCTION` | `3` |
| `UNKNOWN_MODULE` | `4` |
| `FUNCTION_ARITY` | `5` |
| `CTE_COLUMN_COUNT_MISMATCH` | `6` |

### `DefinedRelation`

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | `str` | Relation name |
| `is_view` | `bool` | `True` for views, `False` for tables |

## Exceptions

### `syntaqlite.FormatError`

Raised by [`format_sql`](#sq-format-sql) when the input SQL cannot be
parsed. Inherits from `Exception`.

```python
try:
    sq.format_sql("SELECT FROM")
except syntaqlite.FormatError as e:
    print(e)
```

### `syntaqlite.SyntaqliteError`

Base class for runtime errors raised by a [`Syntaqlite`](#syntaqlite-syntaqlite)
instance — for example, calls made after [`close`](#sq-close). Inherits
from `RuntimeError`.
