+++
title = "Embedded SQL"
description = "Validate SQL strings embedded in Python or TypeScript source files."
weight = 6
+++

# Embedded SQL

syntaqlite can extract and validate SQL string literals from Python and
TypeScript source files without needing to maintain separate `.sql` files for
validation.

```bash
syntaqlite analyze --experimental-lang python app.py
syntaqlite analyze --experimental-lang typescript db.ts
```

syntaqlite finds SQL strings in the host language, then runs the full
validation pipeline on each fragment: syntax checking, schema validation, and
function/arity checks all work as they do on standalone `.sql` files.

## How extraction works

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
