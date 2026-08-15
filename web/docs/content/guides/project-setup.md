+++
title = "Project setup"
description = "Configure syntaqlite for a real project: schema, formatting, version pinning, editor, and CI."
weight = 0
+++

# Project setup

A `syntaqlite.toml` in your project root is the single source of configuration
for the entire toolchain. Every tool (the CLI, VS Code extension,
Neovim/Helix via LSP, Claude Code via MCP, and CI) discovers and reads
this file automatically. Configure once, and the whole team gets consistent
formatting, validation, and diagnostics everywhere.

## Schema

Point syntaqlite at your DDL so it can validate table, column, and function
references. The simplest setup uses the top-level `schema` key:

```toml
schema = ["schema.sql"]
```

Schema files use the same format as SQLite's `.schema` output: plain `CREATE
TABLE` and `CREATE VIEW` statements. You can export directly from an existing
database:

```bash
sqlite3 mydb.db .schema > schema.sql
```

> **Tip:** If your project uses migrations, run `.schema` against your
> development database after applying all migrations to get a single up-to-date
> snapshot rather than pointing syntaqlite at individual migration files.

For projects where different directories use different schemas, use glob-based
routing. First match wins; unmatched files fall back to the top-level `schema`
key:

```toml
schema = ["schema/main.sql"]

[schemas]
"src/**/*.sql"   = ["schema/main.sql", "schema/views.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []
```

Setting a glob to `[]` disables schema validation for those files.

### Warnings vs errors

When a schema is provided (via `syntaqlite.toml` or `--schema`), unresolved
table and column references are reported as **errors** and cause a non-zero
exit code. Without a schema, the same issues are reported as **warnings** and
the exit code remains zero. This means `syntaqlite analyze` in CI will only
fail the build when you've explicitly declared your schema.

For one-off checks without a config file, use `--schema`:

```bash
syntaqlite analyze --schema schema.sql query.sql
```

## SQLite version and compile flags

Pin the version and compile-time flags to match your production environment.
This prevents syntaqlite from accepting syntax or functions that won't exist
at runtime:

```toml
sqlite-version = "3.41.0"
sqlite-cflags = [
  "SQLITE_ENABLE_MATH_FUNCTIONS",
  "SQLITE_ENABLE_FTS5",
]
```

Without `sqlite-version`, syntaqlite defaults to `latest`, so all known syntax
is accepted. For example, the `RETURNING` clause was added in SQLite 3.35.0.
If you pin to 3.34.0, syntaqlite will reject it.

Without `sqlite-cflags`, no optional features are enabled, so functions like
`sin()` or `fts5()` will be flagged as unknown.

These settings apply to all commands (`analyze`, `fmt`, and `lsp`), so
the formatter and language server also respect your target environment. CLI
flags (`--sqlite-version`, `--sqlite-cflag`) override the config file when you
need a one-off change.

See the [config file reference](@/reference/config-file.md#sqlite-cflags) for
the full list of supported flags.

## Formatting

```toml
[format]
line-width = 100
indent-width = 4
keyword-case = "upper"
semicolons = true
```

All values are optional and use built-in defaults when omitted. CLI flags such
as `--line-width` can override them for an individual command.

## Check levels

When a schema is provided, schema checks (`unknown-table`, `unknown-column`,
etc.) default to `"deny"`. Override per category with the `[checks]` section:

```toml
[checks]
schema = "deny"              # shorthand for all schema checks
unknown-function = "warn"    # per-category override
function-arity = "allow"
```

Each category accepts `"allow"`, `"warn"`, or `"deny"`. The `schema` and `all`
shorthands set multiple categories at once; per-category keys override them.

## Editor and tool integration

All integrations read the config file, so formatting and validation settings
do not need to be repeated for each tool.

**VS Code:** install
[syntaqlite: SQLite language server and formatter](https://marketplace.visualstudio.com/items?itemName=syntaqlite.syntaqlite).
It discovers `syntaqlite.toml` on startup. See the
[VS Code tutorial](@/getting-started/vscode.md).

**Neovim, Helix, and other LSP clients:** point your LSP config at
`syntaqlite lsp`. See [other editors](@/getting-started/other-editors.md) for
copy-pasteable configs.

**Claude Code, Cursor, Windsurf**: set up the MCP server. See
[MCP server setup](@/guides/mcp.md).

Restart your editor after adding `syntaqlite.toml`. Diagnostics appear inline
with "did you mean?" suggestions for unknown columns and tables.

## Putting it together

A typical production config:

```toml
sqlite-version = "3.41.0"
sqlite-cflags = ["SQLITE_ENABLE_MATH_FUNCTIONS"]

[schemas]
"src/**/*.sql"   = ["schema/main.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]

[format]
line-width = 100
indent-width = 4

[checks]
schema = "deny"
```

Once committed, this configuration applies to the CLI, editors, and CI without
separate settings for each tool.

## Precedence

CLI flags always override config file values. This lets you use the config for
team-wide defaults while still allowing local overrides:

```bash
# Uses config defaults
syntaqlite analyze "**/*.sql"

# Overrides just the version for this run
syntaqlite analyze --sqlite-version 3.46.0 "**/*.sql"
```

## Next steps

- [Config file reference](@/reference/config-file.md): full format
  specification
- [CI integration](@/guides/ci-integration.md): GitHub Actions, pre-push hooks,
  and validation in CI
- [Embedded SQL](@/guides/embedded-sql.md): validate SQL inside Python and
  TypeScript files
- [Using libsyntaqlite from Rust](@/guides/rust-api.md): Rust API for formatting,
  parsing, and analysis
