+++
title = "Python API reference"
description = "Functions, parameters, and return types for the syntaqlite Python library."
weight = 6
+++

# Python API reference

The Python library is a thin JSON-RPC client for the bundled `syntaqlite`
CLI binary. On first use it spawns `syntaqlite serve` as a long-lived
subprocess and multiplexes all calls over stdio. Requires Python 3.10+;
wheels are published for macOS (arm64, x86_64), Linux (x86_64, aarch64),
and Windows (x86_64).

```python
import syntaqlite
```

## `syntaqlite.format_sql`

Format SQL with configurable options.

```python
syntaqlite.format_sql(sql, *, line_width=80, indent_width=2, keyword_case="upper", semicolons=True)
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | `str` | — | SQL to format |
| `line_width` | `int` | `80` | Maximum line width |
| `indent_width` | `int` | `2` | Spaces per indent level |
| `keyword_case` | `str` | `"upper"` | `"upper"` or `"lower"` |
| `semicolons` | `bool` | `True` | Append semicolons to statements |

**Returns:** `str`. The formatted SQL.

**Raises:** `syntaqlite.FormatError` on parse error (the original SQL is syntactically invalid).

```python
>>> syntaqlite.format_sql("select 1")
'SELECT 1;\n'
>>> syntaqlite.format_sql("select 1", keyword_case="lower", semicolons=False)
'select 1\n'
```

## `syntaqlite.parse`

Parse SQL into a list of typed AST nodes.

```python
syntaqlite.parse(sql)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `sql` | `str` | SQL to parse (may contain multiple statements) |

**Returns:** `list`. One typed AST node per statement. Each node is a
`__slots__` class (e.g. `SelectStmt`, `CreateTableStmt`) with typed attributes:

```python
>>> stmt = syntaqlite.parse("SELECT 1 + 2 FROM foo")[0]
>>> type(stmt).__name__
'SelectStmt'
>>> stmt.columns[0].expr
BinaryExpr(...)
>>> stmt.from_clause
TableRef(...)
>>> stmt.where_clause is None
True
```

Enum and flag fields are wrapped as `IntEnum`/`IntFlag` from `syntaqlite.enums`:

```python
>>> from syntaqlite.enums import BinaryOp
>>> BinaryOp(stmt.columns[0].expr.op).name
'PLUS'
```

Node classes support `isinstance` checks:

```python
>>> from syntaqlite.nodes import SelectStmt, BinaryExpr
>>> isinstance(stmt, SelectStmt)
True
```

On parse error, the entry is a plain dict (not wrapped):

| Key | Type | Description |
|-----|------|-------------|
| `type` | `str` | `"Error"` |
| `message` | `str` | Error message |
| `offset` | `int` | Byte offset of the error |
| `length` | `int` | Length of the error span |

The parser recovers from errors and continues parsing subsequent statements.

### Raw dict access

For performance-sensitive code, use `syntaqlite.parse_raw()` to skip the
wrapping and get plain dicts:

```python
>>> syntaqlite.parse_raw("SELECT 1")[0]["type"]
'SelectStmt'
```

## `syntaqlite.validate`

Validate SQL against an optional schema.

```python
syntaqlite.validate(sql, *, tables=None, views=None, schema_ddl=None, render=False)
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | `str` | — | SQL to validate |
| `tables` | `list[Table] \| None` | `None` | Schema tables |
| `views` | `list[View] \| None` | `None` | Schema views |
| `schema_ddl` | `str \| None` | `None` | DDL to parse as schema (CREATE TABLE/VIEW) |
| `render` | `bool` | `False` | Return rendered diagnostics string instead |

Schema can be provided three ways (or combined):

```python
# Explicit tables and views
syntaqlite.validate(sql,
    tables=[syntaqlite.Table(name="users", columns=["id", "name"])],
    views=[syntaqlite.View(name="active", columns=["id"])],
)

# From DDL
syntaqlite.validate(sql,
    schema_ddl="CREATE TABLE users(id, name); CREATE VIEW active AS SELECT id FROM users;",
)
```

`Table` and `View` accept `name` (required) and `columns` (optional; omit to
accept any column reference).

**Returns (render=False):** `ValidationResult` with attributes:

| Attribute | Type | Description |
|-----------|------|-------------|
| `diagnostics` | `list[Diagnostic]` | Parse and semantic diagnostics |
| `lineage` | `Lineage \| None` | Column lineage for SELECT statements, `None` for non-queries |

**Returns (render=True):** `str`. Human-readable diagnostics with source
context, similar to CLI output.

```python
>>> r = syntaqlite.validate("SELECT id, name FROM users",
...     tables=[syntaqlite.Table(name="users", columns=["id", "name"])])
>>> r.diagnostics
[]
>>> r.lineage.complete
True
>>> for col in r.lineage.columns:
...     print(f"{col.name} <- {col.origin}")
id <- users.id
name <- users.name
>>> r.lineage.physical_tables
['users']
```

### Result types

**`Diagnostic`** — a single diagnostic:

| Attribute | Type | Description |
|-----------|------|-------------|
| `severity` | `str` | `"error"`, `"warning"`, `"info"`, or `"hint"` |
| `message` | `str` | Diagnostic message |
| `start_offset` | `int` | Byte offset of the start of the span |
| `end_offset` | `int` | Byte offset of the end of the span |

**`Lineage`** — column lineage for a SELECT statement:

| Attribute | Type | Description |
|-----------|------|-------------|
| `complete` | `bool` | `True` if all sources fully resolved |
| `columns` | `list[ColumnLineage]` | Per-column lineage |
| `relations` | `list[RelationAccess]` | Catalog relations referenced in FROM |
| `physical_tables` | `list[str]` | Physical table names accessed after CTE/view expansion |
| `unexpanded_views` | `list[str]` | View names whose bodies weren't available for expansion (non-empty means `complete=False`) |

**`ColumnLineage`** — lineage for a single result column:

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | `str` | Output column name (alias or inferred) |
| `index` | `int` | Zero-based position in the result column list |
| `origin` | `ColumnOrigin \| None` | Origin, or `None` for expressions |

**`ColumnOrigin`** — physical table and column:

| Attribute | Type | Description |
|-----------|------|-------------|
| `table` | `str` | Table name |
| `column` | `str` | Column name |

**`RelationAccess`** — a catalog relation in FROM:

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | `str` | Relation name |
| `kind` | `str` | `"table"` or `"view"` |

## `syntaqlite.tokenize`

Tokenize SQL into a list of token dicts.

```python
syntaqlite.tokenize(sql)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `sql` | `str` | SQL to tokenize |

**Returns:** `list[dict]`. One entry per token:

| Key | Type | Description |
|-----|------|-------------|
| `text` | `str` | Token text |
| `offset` | `int` | Byte offset in the source |
| `length` | `int` | Length of the token in bytes |
| `type` | `int` | Internal token type ID |

```python
>>> syntaqlite.tokenize("SELECT 1")
[{'text': 'SELECT', 'offset': 0, 'length': 6, 'type': ...},
 {'text': '1', 'offset': 7, 'length': 1, 'type': ...}]
```

## `syntaqlite.FormatError`

Exception raised by `syntaqlite.format_sql` when the input SQL cannot be parsed.

```python
try:
    syntaqlite.format_sql("SELECT FROM")
except syntaqlite.FormatError as e:
    print(e)  # syntax error near 'FROM'
```

Inherits from `Exception`.
