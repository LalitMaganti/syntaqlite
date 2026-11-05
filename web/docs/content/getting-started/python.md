+++
title = "libsyntaqlite for Python"
description = "Parse, format, and statically analyze SQLite SQL from Python."
weight = 7
+++

# Using libsyntaqlite from Python

The `syntaqlite` package is libsyntaqlite's Python API. In this tutorial you
install it, format a query, check the query against a schema, and look at its
syntax tree.

## 1. Install

```bash
pip install syntaqlite
```

It needs Python 3.10 or later. The wheel includes the native library, so
there is nothing to compile, and it also installs the `syntaqlite` command.

## 2. Format a query

Create `format.py`:

```python
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    sql = "select id,name,email from users where active=1 order by name"
    print(sq.format_sql(sql), end="")
```

```bash
python format.py
```

```sql
SELECT id, name, email FROM users WHERE active = 1 ORDER BY name;
```

Every call goes through a `Syntaqlite` instance, which holds the native
library's state. The `with` block closes it when you are done. In a longer
program, create one instance and reuse it.

## 3. Validate against a schema

Create `analyze.py`:

```python
import syntaqlite
from syntaqlite import AnalysisOutput, RenderOptions, Schema

schema = Schema(ddl="CREATE TABLE users (id INTEGER, name TEXT, email TEXT, active INTEGER);")

with syntaqlite.Syntaqlite() as sq:
    report = sq.analyze(
        "SELECT id, nme FROM users WHERE active = 1",
        schema,
        output=AnalysisOutput.TEXT,
        render_options=RenderOptions(source_name="query.sql"),
        strict_schema=True,
    )
    print(report, end="")
```

```bash
python analyze.py
```

```text
error: unknown column 'nme'
 --> query.sql:1:12
  |
1 | SELECT id, nme FROM users WHERE active = 1
  |            ^~~
  = help: did you mean 'name'?
```

`strict_schema=True` makes unknown tables and columns errors. Without it, they
are reported as warnings. `AnalysisOutput.TEXT` returns the report as text;
without it, `analyze` returns the diagnostics as objects, as the
[Python guide](@/guides/python-api.md) shows.

## 4. Parse and inspect the AST

Create `parse.py`:

```python
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    stmt = sq.parse("SELECT id, name FROM users WHERE active = 1")[0]

    print(type(stmt).__name__)
    print(type(stmt.from_clause).__name__, stmt.from_clause.table_name)
    for col in stmt.columns:
        print(f"  {type(col.expr).__name__}: {col.expr.column}")
```

```bash
python parse.py
```

```text
SelectStmt
TableRef users
  ColumnRef: id
  ColumnRef: name
```

`parse` returns one typed object per statement, with a class for each kind of
node, so your editor can complete attribute names and you can use
`isinstance` to check what a node is.

## Next steps

- [Using libsyntaqlite from Python](@/guides/python-api.md): formatter options,
  schemas built from objects, lineage, walking the AST, and tokens
- [libsyntaqlite Python API reference](@/reference/python-api.md): every class
  and method
- [CLI reference](@/reference/cli.md): the `syntaqlite` command the package
  installs
