+++
title = "Python library"
description = "Use syntaqlite from Python to parse, format, validate, and tokenize SQL."
weight = 6
+++

# Using syntaqlite from Python

This tutorial walks you through using syntaqlite as a Python library. By the end
you'll have a script that formats SQL, validates it against a schema, and
inspects the AST — all from Python.

## 1. Install

```bash
pip install syntaqlite
```

The pip package calls the syntaqlite core in-process via a bundled native
library and also includes the `syntaqlite` CLI binary. Requires Python 3.10+.

> **Pyodide / browser:** syntaqlite also ships an Emscripten wheel, so it runs
> in [Pyodide](https://pyodide.org/) with no subprocess. `pip install syntaqlite`
> inside Pyodide and use the same `Syntaqlite` API shown below.

## 2. Create a `Syntaqlite` instance

All operations go through a [`Syntaqlite`](@/reference/python-api.md#syntaqlite-syntaqlite)
instance. Create one and reuse it across many calls:

```python
import syntaqlite

sq = syntaqlite.Syntaqlite()
# ... use sq ...
sq.close()
```

Or use the context-manager form to close automatically:

```python
with syntaqlite.Syntaqlite() as sq:
    # ... use sq ...
    pass
```

## 3. Format a query

```python
sql = "select id,name,email from users where active=1 order by name"
print(sq.format_sql(sql))
```

Output:

```sql
SELECT id, name, email FROM users WHERE active = 1 ORDER BY name;
```

Customize formatting with keyword arguments:

```python
print(sq.format_sql(sql, line_width=40, indent_width=4, keyword_case="lower"))
```

```sql
select id, name, email
from users
where
  active = 1
order by
  name;
```

## 4. Analyze against a schema

Build a `Schema` with the tables you want to check against and pass it to
`analyze()`:

```python
from syntaqlite import Schema, Table

schema = Schema(tables=[Table("users", ["id", "name", "email", "active"])])
result = sq.analyze(
    "SELECT nme FROM users WHERE active = 1",
    schema,
)
for d in result.diagnostics:
    print(f"{d.severity}: {d.message}")
```

```text
error: unknown column 'nme'
```

`Schema` also accepts raw DDL, so you can point at an existing schema
file instead of listing columns by hand:

```python
schema = Schema(ddl="CREATE TABLE users (id INT, name TEXT, email TEXT, active INT)")
result = sq.analyze("SELECT nme FROM users WHERE active = 1", schema)
```

For human-readable output with source locations, switch the output
format:

```python
from syntaqlite import RenderOptions, AnalysisOutput

print(sq.analyze(
    "SELECT nme FROM users WHERE active = 1",
    schema,
    output=AnalysisOutput.TEXT,
    render_options=RenderOptions(source_name="query.sql"),
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

### Column lineage

When validating a SELECT, the result includes column lineage — which
source table and column each output column traces back to:

```python
schema = Schema(tables=[Table("users", ["id", "name", "email"])])
result = sq.analyze("SELECT u.name, u.email FROM users u", schema)
for col in result.lineage.columns:
    print(f"  {col.name} <- {col.origin}")
```

```text
  name <- users.name
  email <- users.email
```

The `lineage` object also lists the catalog relations referenced in FROM
and the physical tables accessed after CTE/view expansion:

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

`result.lineage` is `None` when the query had no resolvable body (for
example, a `CREATE TABLE` statement).

## 5. Parse and inspect the AST

`sq.parse()` returns typed Python objects. Each statement is a class with
named attributes — you get IDE autocomplete and `isinstance` checks instead
of string-keyed dict access:

```python
stmt = sq.parse("SELECT id, name FROM users WHERE active = 1")[0]

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
from syntaqlite.nodes import BinaryExpr
from syntaqlite.enums import BinaryOp

def count_ops(node, target_ops):
    """Walk an AST node tree and count specific binary operators."""
    count = 0
    if isinstance(node, BinaryExpr):
        if node.op in target_ops:
            count += 1
        count += count_ops(node.left, target_ops)
        count += count_ops(node.right, target_ops)
    return count

sql = "SELECT a + b - c, d - e + f + g FROM t WHERE x - 1 > y + 2"
stmt = sq.parse(sql)[0]

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

The parser recovers from errors and continues. Error nodes are returned
alongside valid statements:

```python
from syntaqlite.nodes import Error

stmts = sq.parse("SELECT FROM; SELECT 1")
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

## 6. Tokenize

```python
for tok in sq.tokenize("SELECT 1"):
    print(f"  {tok['text']!r:10s}  offset={tok['offset']}  len={tok['length']}  {tok['category']}")
```

```text
  'SELECT'    offset=0  len=6  keyword
  ' '         offset=6  len=1  other
  '1'         offset=7  len=1  number
```

## Next steps

- [Python API reference](@/reference/python-api.md) — the full `Syntaqlite`
  class, parameters, and return types
- [CLI reference](@/reference/cli.md) — the `syntaqlite` command installed
  alongside the library
- [Formatting options](@/reference/formatting-options.md) — line width, keyword
  casing, and more
