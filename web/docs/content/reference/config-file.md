+++
title = "Config file"
description = "syntaqlite.toml format: schemas, formatting, and discovery."
weight = 2
+++

# Config file

Project configuration lives in `syntaqlite.toml`. The CLI, LSP server, and all
editor integrations read it.

## Discovery

syntaqlite walks up from the current working directory and uses the first
`syntaqlite.toml` it finds. Use `--config <path>` to point at a specific file.

## File format

```toml
# Default schema for SQL files that don't match any glob in [schemas].
# Optional. If omitted, unmatched files get no schema.
# schema = ["schema.sql"]

# SQLite version to emulate (e.g. "3.47.0", "latest").
# sqlite-version = "3.47.0"

# SQLite compile-time flags to enable.
# sqlite-cflags = ["SQLITE_ENABLE_MATH_FUNCTIONS", "SQLITE_ENABLE_FTS5"]

# Schema DDL files for validation and completions.
# Each entry maps a glob pattern to schema file(s).
[schemas]
"src/**/*.sql" = ["schema/main.sql", "schema/views.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []  # no schema validation

# Formatting options (all optional, shown with defaults).
[format]
line-width = 80
indent-width = 2
keyword-case = "upper"    # "upper" | "lower"
semicolons = true

# Per-category check levels (all optional, shown with defaults).
[checks]
parse-errors = "deny"       # "allow" | "warn" | "deny"
unknown-table = "warn"
unknown-column = "warn"
unknown-function = "warn"
function-arity = "warn"
cte-columns = "deny"
# schema = "deny"           # shorthand for all 4 schema checks
```

## `schema`

Top-level key. Default schemas applied to SQL files that don't match any
`[schemas]` glob.

- Type: array of strings (file paths relative to the config file directory)
- Optional

## `[schemas]`

Maps glob patterns to schema DDL file(s). Globs are matched against the SQL
file's path relative to the directory containing `syntaqlite.toml`. Entries are
checked in file order; first match wins.

- Type: table of `"glob pattern" = ["schema_file.sql", ...]`
- Optional

Schema resolution order:

1. `[schemas]` glob entries (first matching glob wins)
2. `schema` top-level key (fallback for unmatched files)
3. No schema (syntax-only checks)

Schema files contain SQL DDL (`CREATE TABLE`, `CREATE VIEW`, etc.), the same
format as `sqlite3 mydb.db .schema` output.

## `sqlite-version`

SQLite version to emulate. Controls which keywords and functions are recognized
by the parser and semantic analyzer.

- Type: string (e.g. `"3.47.0"`, `"latest"`)
- Optional, defaults to latest
- Equivalent to `--sqlite-version` on the CLI

## `sqlite-cflags`

SQLite compile-time flags to enable. Controls which extensions and optional
features are available during analysis.

- Type: array of strings (e.g. `["SQLITE_ENABLE_MATH_FUNCTIONS", "SQLITE_ENABLE_FTS5"]`)
- Optional, defaults to empty (no extra flags)
- Equivalent to `--sqlite-cflag` on the CLI (repeatable)

Common flags:

| Flag | What it enables |
|------|----------------|
| `SQLITE_ENABLE_MATH_FUNCTIONS` | `sin()`, `cos()`, `log()`, `pow()`, etc. |
| `SQLITE_ENABLE_ORDERED_SET_AGGREGATES` | `percentile_cont()`, `percentile_disc()`, `mode()`, `median()` |
| `SQLITE_ENABLE_JSON1` | `json()`, `json_extract()`, etc. |
| `SQLITE_ENABLE_FTS5` | `fts5()` full-text search |

Functions gated behind a flag you didn't specify will be flagged as unknown.

## `[format]`

Default formatting options. All fields are optional. Omitted fields use
built-in defaults. CLI flags override these values.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `line-width` | integer | `80` | Target maximum line width |
| `indent-width` | integer | `2` | Spaces per indentation level |
| `keyword-case` | string | `"upper"` | `"upper"` or `"lower"` |
| `semicolons` | boolean | `true` | Append semicolons after statements |

See [Formatting options](@/reference/formatting-options.md) for detailed
descriptions of each option.

## `[checks]`

Per-category diagnostic levels. Each field accepts `"allow"` (suppress),
`"warn"`, or `"deny"` (error). Omitted fields use built-in defaults.
CLI flags (`-A`/`-W`/`-D`) override these values.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `parse-errors` | string | `"deny"` | Syntax errors from the parser |
| `unknown-table` | string | `"warn"` | Unresolved table/view references |
| `unknown-column` | string | `"warn"` | Unresolved column references |
| `unknown-function` | string | `"warn"` | Unresolved function names |
| `function-arity` | string | `"warn"` | Wrong number of function arguments |
| `cte-columns` | string | `"deny"` | CTE column count mismatches |
| `schema` | string | — | Shorthand for `unknown-table`, `unknown-column`, `unknown-function`, `function-arity` |
| `all` | string | — | Shorthand for all categories |

When a schema is provided (`--schema` or `syntaqlite.toml`), schema checks
default to `"deny"` instead of `"warn"`. Explicit `[checks]` values override
this.

## Precedence

CLI flags always override config file values. When `--sqlite-version` or
`--sqlite-cflag` is passed on the command line, it takes precedence over
`sqlite-version` and `sqlite-cflags` in the config file. Likewise, `--schema`
overrides `[schemas]` and `schema`.

## Consumers

| Consumer | Behavior |
|----------|----------|
| `syntaqlite fmt` | Discovers config from input file directory or cwd. Format options are defaults; CLI flags override. |
| `syntaqlite analyze` | Discovers config from input file directory or cwd. Schema resolution from config when `--schema` is not given. |
| `syntaqlite lsp` | Discovers config from cwd at startup. Loads schema catalog and format config. |
| VS Code, Claude Code, Neovim, Helix | The LSP reads `syntaqlite.toml` directly. No editor-specific configuration needed. |
