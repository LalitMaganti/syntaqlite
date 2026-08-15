+++
title = "Formatting options"
description = "All formatting configuration options with examples."
weight = 3
+++

# Formatting options

syntaqlite's formatter has four configuration options. All have sensible
defaults; you can use `syntaqlite fmt` with no flags and get well-formatted
SQL. Every option is available across the CLI, Rust, C, and JavaScript APIs.

Options can also be set in
[`syntaqlite.toml`](@/reference/config-file.md). CLI flags override config
file values.

## Options

| Option | CLI flag | Config file key | Default | Description |
|--------|----------|-----------------|---------|-------------|
| Line width | `-w, --line-width <N>` | `line-width` | `80` | Target maximum line width (characters) |
| Indent width | `-t, --indent-width <N>` | `indent-width` | `2` | Spaces per indentation level |
| Keyword case | `-k, --keyword-case <CASE>` | `keyword-case` | `upper` | `upper` or `lower` |
| Semicolons | `--semicolons <BOOL>` | `semicolons` | `true` | Append `;` after each statement |

## Line width

Controls when the formatter breaks a statement across multiple lines. The
formatter tries to fit as much as possible within this width, breaking at
natural points (before clauses, between list items) when the line would be too
long.

```bash
# Narrow: breaks more aggressively
echo "SELECT id, name, email, created_at FROM users WHERE active = 1" \
  | syntaqlite fmt -w 40
```

```sql
SELECT id, name, email, created_at
FROM users
WHERE
  active = 1;
```

```bash
# Wide: keeps more on one line
echo "SELECT id, name, email, created_at FROM users WHERE active = 1" \
  | syntaqlite fmt -w 120
```

```sql
SELECT id, name, email, created_at FROM users WHERE active = 1;
```

The line width is a target, not a hard limit. The formatter won't break a
single long identifier or string literal to stay within the width.

## Indent width

The number of spaces used for each indentation level (e.g., continuation of
`WHERE` conditions, subqueries, column lists that break across lines).

```bash
echo "SELECT id, name FROM users WHERE active = 1 AND role = 'admin'" \
  | syntaqlite fmt -t 4
```

## Keyword case

Controls whether SQL keywords are uppercased or lowercased. Identifiers,
string literals, and other non-keyword tokens are never modified.

```bash
echo "Select Id From Users Where Active = true" | syntaqlite fmt -k upper
```

```sql
SELECT Id FROM Users WHERE Active = true;
```

```bash
echo "Select Id From Users Where Active = true" | syntaqlite fmt -k lower
```

```sql
select Id from Users where Active = true;
```

## Semicolons

Controls whether a semicolon is appended after each statement.

```bash
echo "SELECT 1" | syntaqlite fmt --semicolons=true
```

```sql
SELECT 1;
```

```bash
echo "SELECT 1" | syntaqlite fmt --semicolons=false
```

```sql
SELECT 1
```

## API usage

These options are available in all embedding APIs:

- [libsyntaqlite Rust API](@/reference/rust-api.md): `FormatConfig` builder pattern
- [libsyntaqlite C API](@/reference/c-api.md): `SyntaqliteFormatConfig` struct
