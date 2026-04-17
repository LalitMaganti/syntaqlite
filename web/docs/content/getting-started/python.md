+++
title = "Python library"
description = "Use syntaqlite from Python to parse, format, validate, and tokenize SQL."
weight = 6
+++

# Using syntaqlite from Python

This tutorial walks you through using syntaqlite as a Python library. By the end
you'll have a script that formats SQL, validates it against a schema, traces
column lineage, and inspects the AST, all from Python.

## 1. Install

```bash
pip install syntaqlite
```

The pip package includes both the `syntaqlite` CLI binary and a native C
extension that exposes the library API directly to Python. Requires Python 3.10+.

> **Note:** On platforms where the C extension isn't available (e.g. Windows
> arm64), pip still installs the CLI binary. Only the library functions below
> won't be importable.

## 2. Format a query

```python
import syntaqlite

sql = "select id,name,email from users where active=1 order by name"
print(syntaqlite.format_sql(sql))
```

Output:

```sql
SELECT id, name, email FROM users WHERE active = 1 ORDER BY name;
```

Customize formatting with keyword arguments:

```python
print(syntaqlite.format_sql(sql, line_width=40, indent_width=4, keyword_case="lower"))
```

```sql
select id, name, email
from users
where
  active = 1
order by
  name;
```

## 3. Validate against a schema

Pass table definitions to validate column and table references:

```python
import syntaqlite
from syntaqlite import Table

schema = [Table("users", ["id", "name", "email", "active"])]
result = syntaqlite.validate(
    "SELECT nme FROM users WHERE active = 1",
    tables=schema,
)
for d in result.diagnostics:
    print(f"{d.severity}: {d.message}")
```

```text
error: unknown column 'nme'
```

You can also load schema directly from DDL:

```python
result = syntaqlite.validate(
    "SELECT nme FROM users WHERE active = 1",
    schema_ddl="CREATE TABLE users (id INT, name TEXT, email TEXT, active INT)",
)
```

For human-readable output with source locations, use `render=True`:

```python
print(syntaqlite.validate(
    "SELECT nme FROM users WHERE active = 1",
    tables=schema,
    render=True,
))
```

```text
error: unknown column 'nme'
 --> <input>:1:8
  |
1 | SELECT nme FROM users WHERE active = 1
  |        ^~~
  = help: did you mean 'name'?
```

## 3b. Column lineage

When validating a SELECT, the result includes column lineage: which source
table and column each output column traces back to:

```python
result = syntaqlite.validate(
    "SELECT u.name, u.email FROM users u",
    tables=[Table("users", ["id", "name", "email"])],
)
for col in result.lineage.columns:
    print(f"  {col.name} <- {col.origin}")
```

```text
  name <- users.name
  email <- users.email
```

The `lineage` object also lists the catalog relations referenced in FROM
(tables and views as written in the query) and the physical tables
accessed after CTE/view expansion:

```python
for rel in result.lineage.relations:
    print(f"  relation: {rel.name} ({rel.kind})")
for t in result.lineage.physical_tables:
    print(f"  physical table: {t}")
```

```text
  relation: users (table)
  physical table: users
```

## 4. Parse and inspect the AST

`syntaqlite.parse()` returns typed Python objects. Each statement is a class
with named attributes — you get IDE autocomplete and `isinstance` checks instead
of string-keyed dict access:

```python
import syntaqlite

stmt = syntaqlite.parse("SELECT id, name FROM users WHERE active = 1")[0]

print(type(stmt).__name__)       # SelectStmt
print(stmt.from_clause)          # TableRef(...)
print(stmt.where_clause)         # BinaryExpr(...)

for col in stmt.columns:
    print(f"  {type(col.expr).__name__}: {col.expr.column}")
```

```text
SelectStmt
TableRef(...)
BinaryExpr(...)
  ColumnRef: id
  ColumnRef: name
```

### Walking the AST

Because nodes are typed, you can write recursive visitors with `isinstance`.
Here's a function that counts arithmetic operators in a query:

```python
import syntaqlite
from syntaqlite.nodes import BinaryExpr
from syntaqlite.enums import BinaryOp

def count_ops(node, target_ops):
    """Walk an AST node tree and count specific binary operators."""
    count = 0
    if isinstance(node, BinaryExpr):
        if BinaryOp(node.op) in target_ops:
            count += 1
        count += count_ops(node.left, target_ops)
        count += count_ops(node.right, target_ops)
    return count

sql = "SELECT a + b - c, d - e + f + g FROM t WHERE x - 1 > y + 2"
stmt = syntaqlite.parse(sql)[0]

adds = 0
subs = 0
for col in stmt.columns:
    adds += count_ops(col.expr, {BinaryOp.PLUS})
    subs += count_ops(col.expr, {BinaryOp.MINUS})
if stmt.where_clause:
    adds += count_ops(stmt.where_clause, {BinaryOp.PLUS})
    subs += count_ops(stmt.where_clause, {BinaryOp.MINUS})

print(f"additions: {adds}, subtractions: {subs}")
```

```text
additions: 4, subtractions: 3
```

### Error recovery

The parser recovers from errors and continues parsing. Error nodes are returned
alongside valid statements:

```python
from syntaqlite.nodes import Error

stmts = syntaqlite.parse("SELECT FROM; SELECT 1")
for s in stmts:
    if isinstance(s, Error):
        print(f"error: {type(s).__name__}")
    else:
        print(f"ok: {type(s).__name__}")
```

```text
error: Error
ok: SelectStmt
```

## 5. Tokenize

```python
import syntaqlite

for tok in syntaqlite.tokenize("SELECT 1"):
    print(f"  {tok['text']!r:10s}  offset={tok['offset']}  len={tok['length']}")
```

```text
  'SELECT'    offset=0  len=6
  ' '         offset=6  len=1
  '1'         offset=7  len=1
```

## Next steps

- [Python API reference](@/reference/python-api.md) — all functions, parameters,
  and return types
- [CLI reference](@/reference/cli.md) — the `syntaqlite` command installed
  alongside the library
- [Formatting options](@/reference/formatting-options.md) — line width, keyword
  casing, and more
