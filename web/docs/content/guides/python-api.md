+++
title = "Using libsyntaqlite from Python"
description = "Formatter options, schemas built from objects, diagnostics, lineage, the typed AST, and tokens with the syntaqlite package."
weight = 2
+++

# Using libsyntaqlite from Python

This guide picks up where the
[libsyntaqlite for Python tutorial](@/getting-started/python.md) leaves off.
It covers formatter options, building a schema from objects, reading
diagnostics, column lineage, walking the typed AST, parse errors, tokens, and
running in the browser with Pyodide.

## Customize formatting

`format_sql` takes keyword arguments for the line width, indentation, keyword
case, and trailing semicolons:

```python
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    sql = "SELECT id, name, email FROM users WHERE active = 1 AND role = 'admin'"
    print(sq.format_sql(sql, line_width=40, indent_width=4, keyword_case="lower"), end="")

    try:
        sq.format_sql("SELECT FROM users")
    except syntaqlite.FormatError as e:
        print(f"error: {e}")
```

```text
select id, name, email
from users
where
    active = 1
    and role = 'admin';
error: syntax error near 'FROM'
```

SQL that does not parse raises `FormatError`. See
[Formatting options](@/reference/formatting-options.md) for what each setting
does.

## Build a schema from objects

The tutorial loads the schema from `CREATE TABLE` statements. If your program
already knows its tables, for example from a database's catalog, build the
schema from `Table`, `Column`, and `View` objects instead. Mark generated and
hidden columns with `ColumnKind`. A table created without columns accepts any
column name:

```python
import syntaqlite
from syntaqlite import Column, ColumnKind, Schema, Table, View

schema = Schema(
    tables=[
        Table("users", ["id", "name", Column("name_lower", ColumnKind.GENERATED)]),
        # Columns unknown: any column of this table is accepted.
        Table("events"),
    ],
    views=[View("user_names", ["name"])],
)

sql = "SELECT name_lower FROM users; SELECT anything FROM events; SELECT nmae FROM user_names"
with syntaqlite.Syntaqlite() as sq:
    for d in sq.analyze(sql, schema).diagnostics:
        print(f"{d.severity} at bytes {d.start_offset}-{d.end_offset}: {d.message} ({d.code.name})")
```

```text
warning at bytes 66-70: unknown column 'nmae' (UNKNOWN_COLUMN)
```

Unknown tables and columns are reported as warnings.

## Read diagnostics

By default `analyze` returns an `Analysis` whose `diagnostics` are objects, as
in the example above. Each has a severity, a message, a `code` saying which
check produced it, and the byte range it applies to, measured from the start
of the SQL you passed in. Use them to show problems in your own interface, and
`AnalysisOutput.TEXT` when you want the terminal-style report from the tutorial.

## Column lineage

For a query, `result.lineage` says which table and column each result column
comes from, which tables and views the query names, and which tables it reads
once views are expanded:

```python
import syntaqlite
from syntaqlite import Schema, Table

schema = Schema(tables=[
    Table("users", ["id", "name"]),
    Table("posts", ["id", "user_id", "body"]),
])

with syntaqlite.Syntaqlite() as sq:
    result = sq.analyze(
        "SELECT u.name, p.body FROM users u JOIN posts p ON u.id = p.user_id", schema
    )
    for col in result.lineage.columns:
        print(f"column {col.name} <- {col.origin}")
    for rel in result.lineage.relations:
        print(f"relation: {rel.name} ({rel.kind})")
    for table in result.lineage.physical_tables:
        print(f"table: {table}")
```

```text
column name <- users.name
column body <- posts.body
relation: posts (table)
relation: users (table)
table: posts
table: users
```

`result.lineage` is `None` for statements other than queries.

## Walk the AST

Nodes are Python classes, so a visitor can use `isinstance`. This counts the
additions and subtractions in a query:

```python
import syntaqlite
from syntaqlite.enums import BinaryOp
from syntaqlite.nodes import BinaryExpr


def count_ops(node, ops):
    """Counts the binary operators in `ops` in an expression tree."""
    if not isinstance(node, BinaryExpr):
        return 0
    return (node.op in ops) + count_ops(node.left, ops) + count_ops(node.right, ops)


with syntaqlite.Syntaqlite() as sq:
    stmt = sq.parse("SELECT a + b - c, d - e + f + g FROM t WHERE x - 1 > y + 2")[0]
    exprs = [col.expr for col in stmt.columns] + [stmt.where_clause]
    adds = sum(count_ops(e, {BinaryOp.PLUS}) for e in exprs)
    subs = sum(count_ops(e, {BinaryOp.MINUS}) for e in exprs)
    print(f"additions: {adds}, subtractions: {subs}")
```

```text
additions: 4, subtractions: 3
```

The node classes are in `syntaqlite.nodes` and their enums in
`syntaqlite.enums`. The [AST reference](@/reference/ast-reference.md) lists
them all.

## Parse errors

`parse` returns the statements that parsed and leaves out any that didn't. To
find out what went wrong, run the same SQL through `analyze`, which reports
syntax errors as diagnostics:

```python
import syntaqlite

sql = "SELECT FROM; SELECT 1"
with syntaqlite.Syntaqlite() as sq:
    print([type(s).__name__ for s in sq.parse(sql)])
    for d in sq.analyze(sql).diagnostics:
        print(f"{d.severity} at bytes {d.start_offset}-{d.end_offset}: {d.message}")
```

```text
['SelectStmt']
error at bytes 7-11: syntax error near 'FROM'
```

## Tokens

`tokenize` returns every token, including whitespace, with its byte offset and
a category:

```python
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    for tok in sq.tokenize("SELECT max(x) FROM t"):
        print(f"{tok['offset']:>2} {tok['text']!r:8} {tok['category']}")
```

```text
 0 'SELECT' keyword
 6 ' '      other
 7 'max'    identifier
10 '('      punctuation
11 'x'      identifier
12 ')'      punctuation
13 ' '      other
14 'FROM'   keyword
18 ' '      other
19 't'      identifier
```

## Run in the browser with Pyodide

syntaqlite also publishes a wheel for [Pyodide](https://pyodide.org/), the
Python runtime for the browser. Run `pip install syntaqlite` inside Pyodide
and use the same `Syntaqlite` API.

## Next steps

- [libsyntaqlite Python API reference](@/reference/python-api.md): every class
  and method
- [CLI reference](@/reference/cli.md): the `syntaqlite` command the package
  installs
