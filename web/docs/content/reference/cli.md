+++
title = "CLI reference"
description = "All flags and options for every subcommand."
weight = 1
+++

# CLI reference

## syntaqlite fmt

Format SQL files.

```bash
syntaqlite fmt [OPTIONS] [FILES...]
```

Reads from stdin if no files are given. When stdin is a terminal, a hint is
printed to stderr. Formatting defaults can be set in
[`syntaqlite.toml`](@/reference/config-file.md). CLI flags override config
file values.

| Option | Default | Description |
|--------|---------|-------------|
| `-e, --expression <SQL>` | | SQL to format directly (instead of files or stdin) |
| `-w, --line-width <N>` | `80` | Target maximum line width |
| `-t, --indent-width <N>` | `2` | Spaces per indentation level |
| `-k, --keyword-case <CASE>` | `upper` | Keyword casing: `upper` or `lower` |
| `--semicolons <BOOL>` | `true` | Append semicolons after statements |
| `-i, --in-place` | | Write formatted output back to files |
| `--check` | | Check if files are formatted (exit 1 if not, conflicts with `-i`) |
| `-o, --output <FORMAT>` | `formatted` | Output mode: `formatted`, `bytecode`, or `doc-tree` |
| `--experimental-lang <LANG>` | | Host language for embedded SQL: `python`, `typescript`, or `shell`. sqlite3 shell scripts are auto-detected when omitted — dot-commands are preserved and only the SQL is reformatted |
| `--dialect <PATH>` | | Path to custom dialect shared library |
| `--dialect-name <NAME>` | | Symbol name in dialect library |
| `--sqlite-version <VER>` | `latest` | Target SQLite version (e.g., `3.47.0`) |
| `--sqlite-cflag <FLAG>` | | Enable a compile-time flag (repeatable) |

Output modes:
- `formatted` — formatted SQL (default)
- `bytecode` — dump raw interpreter bytecode for each statement (maintainer)
- `doc-tree` — dump the Wadler-Lindig document tree after interpretation (maintainer)

Exit codes:
- `0` — success (or all files already formatted with `--check`)
- `1` — parse error (or files would be reformatted with `--check`)

## syntaqlite analyze

Analyze SQL against schema.

```bash
syntaqlite analyze [OPTIONS] [FILES...]
```

| Option | Default | Description |
|--------|---------|-------------|
| `-e, --expression <SQL>` | | SQL to validate directly (instead of files or stdin) |
| `--schema <FILE>` | | Schema DDL file(s) to load (repeatable, supports globs) |
| `-A, --allow <CHECK>` | | Suppress a check category (repeatable) |
| `-W, --warn <CHECK>` | | Warn on a check category (repeatable) |
| `-D, --deny <CHECK>` | | Error on a check category (repeatable) |
| `--experimental-lang <LANG>` | | Host language for embedded SQL: `python`, `typescript`, or `shell`. sqlite3 shell scripts (`.read` dot-commands) are auto-detected when omitted — see [Embedded SQL](@/guides/embedded-sql.md) |
| `--dialect <PATH>` | | Path to custom dialect shared library |
| `--dialect-name <NAME>` | | Symbol name in dialect library |
| `--sqlite-version <VER>` | `latest` | Target SQLite version |
| `--sqlite-cflag <FLAG>` | | Enable a compile-time flag (repeatable) |

Check categories: `parse-errors`, `unknown-table`, `unknown-column`,
`unknown-function`, `function-arity`, `cte-columns`. Groups: `schema` (all 4
schema checks), `all`.

When `--schema` is provided, the validator loads `CREATE TABLE` / `CREATE VIEW`
statements from the schema files and checks the remaining input files against
that schema. When `--schema` is omitted and a
[`syntaqlite.toml`](@/reference/config-file.md) exists, schemas are resolved
from the config file's glob patterns. Without either, inline DDL in the input
is used instead. Diagnostics are printed to stderr in rustc-style format.

Exit codes:
- `0` — no errors (warnings may still be printed)
- `1` — one or more error-level diagnostics

When a schema is provided (`--schema` or `syntaqlite.toml`), schema checks
default to `deny` (errors). Without a schema, they default to `warn`. Explicit
`-A`/`-W`/`-D` flags or `[checks]` in `syntaqlite.toml` override these
defaults.

## syntaqlite parse

Parse SQL and report results.

```bash
syntaqlite parse [OPTIONS] [FILES...]
```

Reads from stdin if no files are given. When stdin is a terminal, a hint is
printed to stderr.

| Option | Default | Description |
|--------|---------|-------------|
| `-e, --expression <SQL>` | | SQL to parse directly (instead of files or stdin) |
| `-o, --output <FORMAT>` | `text` | Output format: `text`, `json`, or `summary` |

Output formats:
- `text` — print the AST as human-readable text (default)
- `json` — print the AST as JSON
- `summary` — print statement/error counts (compact, for benchmarks) (maintainer)

When `text` output is used with multiple files, each is prefixed with
`==> filename <==`.

Exit codes:
- `0` — parsed successfully
- `1` — parse error

## syntaqlite lineage

Extract column and table lineage from SQL.

```bash
syntaqlite lineage [OPTIONS] [FILES...]
syntaqlite lineage tables [OPTIONS] [FILES...]
syntaqlite lineage columns [OPTIONS] [FILES...]
```

Reads from stdin if no files are given. For each statement, emits a
lineage record listing the output columns, their origin table/column
(when the column is an untransformed passthrough), the catalog
relations referenced in FROM, and the physical tables accessed after
CTE/view expansion. Statements that fail parsing or validation emit an
error record instead and cause a non-zero exit.

| Option | Default | Description |
|--------|---------|-------------|
| `-e, --expression <SQL>` | | SQL to analyze directly (instead of files or stdin) |
| `--schema <FILE>` | | Schema DDL file(s) to load (repeatable, supports globs) |
| `-o, --output <FORMAT>` | `json` | Output format: `json` or `text` |
| `--dialect <PATH>` | | Path to custom dialect shared library |
| `--dialect-name <NAME>` | | Symbol name in dialect library |
| `--sqlite-version <VER>` | `latest` | Target SQLite version |
| `--sqlite-cflag <FLAG>` | | Enable a compile-time flag (repeatable) |

Output formats:
- `json` — newline-delimited JSON (NDJSON), one record per statement or
  error. Each record includes `schema_version` (currently `0`, pre-stable),
  `file`, `statement_index`, `status` (`complete` or `partial`),
  `partial_reasons`, `target`, `columns`, `relations`, and
  `physical_tables`. Suitable for piping into lineage tooling.
- `text` — human-readable indented format, one record per statement or error.

Scope subcommands project the output to a subset:
- `tables` — emit only relations and physical tables (drops `columns`)
- `columns` — emit only column lineage (drops `relations` and `physical_tables`)

A column's `origin` is populated only when the column is an untransformed
passthrough (direct reference or pure rename alias). Expressions, casts,
aggregates, window functions, and set-operation (`UNION`, etc.) columns
have `origin: null`. Views and CTEs are transparent (expanded into
`physical_tables`); real tables and temp tables are opaque boundaries.
When a view body is not available for expansion, the record's `status`
becomes `partial` and `partial_reasons` includes an `unexpanded_view`
entry with the view name.

Exit codes:
- `0` — all statements produced a lineage record
- `1` — one or more error records were emitted

## syntaqlite lsp

Start the language server on stdio. On startup, the server discovers
[`syntaqlite.toml`](@/reference/config-file.md) from the current directory and
uses it for schema loading and formatting defaults.

```bash
syntaqlite lsp [OPTIONS]
```

| Option | Default | Description |
|--------|---------|-------------|
| `--dialect <PATH>` | | Path to custom dialect shared library |
| `--dialect-name <NAME>` | | Symbol name in dialect library |
| `--sqlite-version <VER>` | `latest` | Target SQLite version |
| `--sqlite-cflag <FLAG>` | | Enable a compile-time flag (repeatable) |

Supports:
- `textDocument/publishDiagnostics`
- `textDocument/formatting`
- `textDocument/completion`
- `textDocument/semanticTokens/full`

sqlite3 shell scripts are auto-detected: `.read` and other dot-commands are not
reported as syntax errors, and every feature above operates on the SQL between
them. See [Embedded SQL](@/guides/embedded-sql.md).

## Global options

These options are available on all subcommands:

| Option | Description |
|--------|-------------|
| `--dialect <PATH>` | Load a custom dialect from a shared library |
| `--dialect-name <NAME>` | Symbol name for the dialect (default: `syntaqlite_dialect_template`, with name: `syntaqlite_<NAME>_dialect_template`) |
| `--sqlite-version <VER>` | Emulate a specific SQLite version |
| `--sqlite-cflag <FLAG>` | Enable a SQLite compile-time flag (can be specified multiple times) |
