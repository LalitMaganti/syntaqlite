# syntaqlite

Parse, format, and validate SQLite SQL from Python using SQLite's own grammar and tokenizer. No approximations: if SQLite accepts it, syntaqlite parses it.

**[Docs](https://docs.syntaqlite.com)** · **[Playground](https://playground.syntaqlite.com)** · **[GitHub](https://github.com/LalitMaganti/syntaqlite)**

```bash
pip install syntaqlite
```

Requires Python 3.10+. Wheels for Linux (x86_64, aarch64), macOS (x86_64, arm64), and Windows (x86_64).

## Library

Everything goes through a `Syntaqlite` instance. Create one and reuse it across many calls:

```python
import syntaqlite

with syntaqlite.Syntaqlite() as sq:
    print(sq.format_sql("select 1"))
```

### Formatting

```python
sql = "select u.id, u.name, p.title from users u join posts p on u.id = p.user_id where u.active = 1 and p.published = true order by p.created_at desc limit 10"
print(sq.format_sql(sql))
```
```
SELECT u.id, u.name, p.title
FROM users AS u
JOIN posts AS p ON u.id = p.user_id
WHERE
  u.active = 1
  AND p.published = true
ORDER BY
  p.created_at DESC
LIMIT 10;
```

Raises `syntaqlite.FormatError` on invalid input.

### Parsing

`sq.parse()` returns a full AST as typed Python objects, one per statement:

```python
stmts = sq.parse("SELECT 1 + 2 FROM foo")
stmt = stmts[0]  # SelectStmt

print(type(stmt).__name__)       # SelectStmt
print(stmt.columns[0].expr)      # BinaryExpr(...)
print(stmt.from_clause)          # TableRef(...)
print(stmt.where_clause)         # None
```

Every node type is a `__slots__` class with typed attributes, so you get IDE
autocomplete and `isinstance` checks:

```python
from syntaqlite.nodes import SelectStmt, BinaryExpr

assert isinstance(stmt, SelectStmt)
assert isinstance(stmt.columns[0].expr, BinaryExpr)
```

Enum and flag fields are `IntEnum`/`IntFlag` from `syntaqlite.enums`:

```python
from syntaqlite.enums import BinaryOp

expr = stmt.columns[0].expr
print(expr.op)  # BinaryOp.PLUS
```

For performance-sensitive code, use `sq.parse_raw()` to get plain dicts
instead of typed objects.

### Tokenizing

```python
for tok in sq.tokenize("SELECT 1 + 2"):
    print(tok["text"], tok["category"])
```
```
SELECT keyword
  other
1 number
  other
+ operator
  other
2 number
```

Each token is a dict with `text`, `offset`, `length`, `type`, and `category` fields.

### Validation

Check SQL against a schema without touching a database. Catches unknown tables, columns, functions, CTE column mismatches, and more.

```python
schema = syntaqlite.Schema(
    tables=[syntaqlite.Table("users", columns=["id", "name", "email"])],
)
result = sq.validate("SELECT nme FROM users", schema)
for d in result.diagnostics:
    print(f"{d.severity}: {d.message}")
```
```
error: unknown column 'nme'
```

Pass `render=True` to get formatted diagnostics with source locations and suggestions:

```python
print(sq.validate("SELECT nme FROM users", schema, render=True))
```
```
error: unknown column 'nme'
 --> <input>:1:8
  |
1 | SELECT nme FROM users
  |        ^~~
  = help: did you mean 'name'?
```

`Schema` also accepts raw DDL:

```python
schema = syntaqlite.Schema(ddl="CREATE TABLE orders (id INTEGER, total REAL);")
result = sq.validate("SELECT * FROM orders", schema)
```

#### Column lineage

For query-bearing statements, the result includes column lineage:

```python
schema = syntaqlite.Schema(
    tables=[syntaqlite.Table("users", columns=["id", "name", "email"])],
)
result = sq.validate("SELECT id, name FROM users", schema)
for col in result.lineage.columns:
    print(f"{col.name} <- {col.origin}")
```
```
id <- users.id
name <- users.name
```

## CLI

The pip package also bundles the `syntaqlite` binary:

```bash
syntaqlite fmt -e "select 1, 2, 3"
syntaqlite validate query.sql
syntaqlite parse -e "SELECT * FROM users"
```

The CLI supports pinning to a specific SQLite version or enabling compile-time flags to match your target environment:

```bash
syntaqlite --sqlite-version 3.32.0 validate query.sql
syntaqlite --sqlite-cflag SQLITE_ENABLE_MATH_FUNCTIONS validate query.sql
```

See the [CLI reference](https://docs.syntaqlite.com/main/reference/cli/) for all commands and flags.

## License

Apache 2.0. SQLite components are public domain under the [SQLite blessing](https://www.sqlite.org/copyright.html).
