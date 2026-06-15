+++
title = "Embedded SQL"
description = "Validate sqlite3 shell scripts and SQL strings embedded in Python or TypeScript."
weight = 6
+++

# Embedded SQL

syntaqlite can analyze SQL that doesn't live in a plain `.sql` file: sqlite3
command-line shell scripts (with `.read` and friends) and SQL string literals
inside Python and TypeScript source files. In each case it isolates the SQL,
then runs the full pipeline on it — syntax checking, schema validation, and
function/arity checks all work exactly as they do on standalone SQL.

## sqlite3 shell scripts

Scripts written for the `sqlite3` command-line shell interleave SQL with shell
*dot-commands* (`.read`, `.print`, `.import`, …), `#` comments, and `GO` / `/`
statement terminators. Those lines are not valid SQL, so a plain SQL parser
fails with a syntax error on the first `.` command.

syntaqlite detects such scripts automatically — no `--experimental-lang` flag
required. When a file looks like a sqlite3 script, the non-SQL lines are set
aside and only the SQL between them is parsed, validated, and formatted:

```bash
syntaqlite fmt schema.sql
```
```text
.read tables.sql
.read views.sql

CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);
```

Detection is content-based: a file is treated as a shell script only when it
contains a dot-command (or other shell marker) at the start of a line. A `.`
inside a string literal or mid-statement stays ordinary SQL. This works across
the CLI (`fmt`, `analyze`, `parse`, `lineage`) and in the language server, so
the same files behave identically on the command line and in your editor.

## SQL in Python and TypeScript

syntaqlite also extracts SQL string literals from Python and TypeScript source
files, without needing to maintain separate `.sql` files for validation.

```bash
syntaqlite analyze --experimental-lang python app.py
syntaqlite analyze --experimental-lang typescript db.ts
```

syntaqlite finds SQL strings in the host language, then runs the same
validation pipeline on each fragment, handling interpolation holes
(`{var}`, `${expr}`, `?`).

## How string extraction works

The extractor looks for string literals that contain SQL keywords (`SELECT`,
`INSERT`, `CREATE TABLE`, etc.) and parses them as SQL. Multi-line strings,
f-strings, and template literals are supported:

```python
# Python — all of these are recognized
cursor.execute("SELECT id, name FROM users WHERE active = 1")

query = """
    SELECT u.name, p.title
    FROM users u
    JOIN posts p ON p.user_id = u.id
"""

cursor.execute(f"SELECT * FROM {table_name} WHERE id = ?")
```

```typescript
// TypeScript — template literals work too
const query = `
  SELECT id, name
  FROM users
  WHERE role = 'admin'
`;
```

## Limitations

This feature is experimental. Some patterns are not recognized:

- String concatenation across multiple statements (`query += "..."`)
- SQL built dynamically at runtime
- Complex interpolation where the SQL structure itself is parameterized
- Languages other than Python and TypeScript

## Schema validation

Embedded SQL validation respects `syntaqlite.toml`. If you have a schema
configured, references are checked against it. You can also pass schema
explicitly:

```bash
syntaqlite analyze --experimental-lang python --schema schema.sql app.py
```

## In CI

Add embedded SQL checks alongside your regular SQL validation:

```yaml
- name: Check embedded SQL
  run: |
    syntaqlite analyze --experimental-lang python "src/**/*.py"
    syntaqlite analyze --experimental-lang typescript "src/**/*.ts"
```
