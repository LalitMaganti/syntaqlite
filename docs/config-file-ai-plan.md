# Project Configuration File Plan

Status: Phases 1–3 complete
Last updated: 2026-03-15

## Goal

Add a `syntaqlite.toml` project configuration file that serves as the single,
editor-agnostic source of truth for syntaqlite settings. Today, configuration is
fragmented: formatting options are CLI-only flags, schema paths are VS Code-only
settings, and the LSP hardcodes `FormatConfig::default()`. A project config file
unifies all of these and lets syntaqlite work in Claude Code, Neovim, Helix,
and Zed without bespoke configuration for each editor.

## Why now

The immediate trigger is the Claude Code plugin. The VS Code extension has a
settings UI for `syntaqlite.schemaPath` and `syntaqlite.schemas` (glob-based
schema routing). Claude Code plugins have no equivalent mechanism: there's no
runtime UI for user settings. Without a config file, Claude Code users have no
way to tell the LSP which schema to use.

But the problem is broader than Claude Code. The VS Code extension is the only
client that can configure the LSP today. Every other LSP client (Neovim via
nvim-lspconfig, Helix, Zed, Emacs) would need to reimplement the schema
resolution logic in their own plugin language. A config file that the LSP reads
directly eliminates this entirely.

## Design Principles

1. **Server-side**: the LSP reads the config file itself; editors don't need to
   understand it or translate it into `initializationOptions`
2. **Walk up**: search from the file being processed up to the filesystem root,
   stop at the first `syntaqlite.toml` found (same as rustfmt, Ruff, Prettier)
3. **CLI flag override**: CLI args always win over config file values
4. **Config file is the single source of truth**: editor extensions do not
   duplicate tool settings. Following rustfmt, Ruff, Prettier, and Pyright:
   editors only have settings for editor-specific concerns (binary path,
   enable/disable), not tool behavior. The VS Code extension's `schemaPath`,
   `schemas`, and any future formatting settings are removed in favor of
   `syntaqlite.toml`.

## Precedent audit

### Config file conventions

| Tool | Config file | Format | Discovery |
|------|-------------|--------|-----------|
| rustfmt | `rustfmt.toml` | TOML | Walk up |
| Ruff | `ruff.toml` | TOML | Walk up |
| Prettier | `.prettierrc` | JSON/YAML/TOML | Walk up |
| SQLFluff | `.sqlfluff` | INI | Walk up |
| Pyright | `pyrightconfig.json` | JSON | Project root |
| gopls | None |: | Editor settings only |

### Editor extension behavior

Every major tool follows the same pattern: the editor extension does NOT
duplicate tool settings. The project config file is authoritative.

| Tool | VS Code extension settings | Project config |
|------|---------------------------|----------------|
| rustfmt | None: rust-analyzer reads `rustfmt.toml` | `rustfmt.toml` |
| Ruff | Extension behavior only (enable/disable) | `ruff.toml` / `pyproject.toml` |
| Prettier | Fallback only when no project config | `.prettierrc` |
| Pyright | Defers to project config | `pyrightconfig.json` |
| ESLint | Defers to project config | `eslint.config.js` |

We follow both conventions: `syntaqlite.toml` for tool config, and the VS Code
extension only keeps `syntaqlite.serverPath` (binary location: genuinely
editor-specific).

## File format

```toml
# Schema DDL files for validation and completions.
# Each entry maps a glob pattern to schema file(s).
# SQL files matching the glob get validated against those schemas.
[schemas]
"src/**/*.sql" = ["schema/main.sql", "schema/views.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []  # no schema validation for migrations

# Default schema for SQL files that don't match any glob above.
# Optional: if omitted, unmatched files get no schema.
# schema = "schema.sql"

# Formatting options (all optional, shown with defaults).
[format]
line-width = 80
indent-width = 2
keyword-case = "upper"    # "upper" | "lower"
semicolons = true
```

### Schema resolution

The `[schemas]` section is a dictionary of `glob → [schema files]`. The glob is
matched against the path of the SQL file relative to the directory containing
`syntaqlite.toml`.

Resolution order (first match wins):
1. `[schemas]` glob entries: checked in order, first matching glob wins
2. `schema` top-level key: fallback for unmatched files
3. No schema: syntax-only validation

Schema file paths are relative to the directory containing `syntaqlite.toml`.

### Why a dictionary of globs

Real projects have multiple schemas. A web app might have the main application
schema, a separate analytics schema, and test fixture tables. Migration files
shouldn't be validated against any schema because they *define* the schema.
Glob-based routing handles all of these cases:

```toml
[schemas]
"src/analytics/**/*.sql" = ["schema/analytics.sql"]
"src/**/*.sql" = ["schema/app.sql"]
"tests/**/*.sql" = ["schema/app.sql", "schema/test_fixtures.sql"]
"migrations/**/*.sql" = []
```

## Where config is read

### 1. LSP server (primary consumer)

**Current state:** The LSP receives schema paths via `initializationOptions` and
`workspace/didChangeConfiguration`, both sent by the VS Code extension. Formatting
uses hardcoded `FormatConfig::default()`.

**After:** On startup, the LSP discovers `syntaqlite.toml` by walking up from the
workspace root. It reads schema mappings and format config. When a document is
opened, the LSP resolves which schema applies using the glob patterns. Format
requests use the config file's formatting options.

The LSP should also watch for `syntaqlite.toml` changes (via
`workspace/didChangeWatchedFiles` or polling) and reload.

The LSP no longer accepts schema paths from `initializationOptions` or
`workspace/didChangeConfiguration`. The config file is the only source.

**Files changed:**
- `syntaqlite/src/lsp/server.rs`: config discovery, schema resolution per
  document, format config from file, remove `load_schema_from_settings()` and
  `load_schema_from_options()`
- `syntaqlite/src/lsp/host.rs`: store `FormatConfig` (currently missing), store
  per-glob schema catalogs

### 2. CLI `fmt` command

**Current state:** Formatting options come exclusively from CLI flags. No config
file is read.

**After:** Before processing, the CLI discovers `syntaqlite.toml` by walking up
from the current directory (or from the input file's directory). Config file
values provide defaults; CLI flags override them.

```bash
# Uses config file defaults
syntaqlite fmt query.sql

# CLI flag overrides config file
syntaqlite fmt -w 120 query.sql
```

**Files changed:**
- `syntaqlite-cli/src/runtime.rs`: load config, merge with CLI args in
  `cmd_format()`

### 3. CLI `validate` command

**Current state:** Schema files passed via `--schema` CLI flag (repeatable,
supports globs).

**After:** If no `--schema` flag is given, the CLI discovers `syntaqlite.toml`
and resolves schemas for each input file using the glob patterns. If `--schema`
is given, it takes precedence over the config file (explicit is better than
implicit).

```bash
# Uses config file schemas
syntaqlite validate src/query.sql

# CLI flag overrides config file
syntaqlite validate --schema other.sql src/query.sql
```

**Files changed:**
- `syntaqlite-cli/src/runtime.rs`: load config, resolve schemas in
  `cmd_validate()`

### 4. CLI `fmt --check` (CI usage)

**Current state:** `fmt --check` uses CLI flags for formatting options.

**After:** Reads `syntaqlite.toml` so CI checks match developer formatting
without requiring flags:

```bash
# Before: CI script must duplicate formatting options
syntaqlite fmt --check -w 120 -k lower "**/*.sql"

# After: CI reads syntaqlite.toml automatically
syntaqlite fmt --check "**/*.sql"
```

No additional files changed beyond the `cmd_format()` changes above.

### 5. VS Code extension (simplified)

**Current state:** Schema configuration via `syntaqlite.schemaPath` and
`syntaqlite.schemas` settings, resolved client-side with `minimatch`, sent to the
LSP via `initializationOptions` and `workspace/didChangeConfiguration`. The
extension has ~100 lines of schema resolution logic.

**After:** All schema and formatting configuration moves to `syntaqlite.toml`,
read server-side by the LSP. The VS Code extension is stripped down to:

- Start the LSP server (binary resolution via `syntaqlite.serverPath`)
- Register commands (`restartServer`, `formatDocument`)
- Status bar item (can show config file status instead of schema path)

The following are **deleted** from the extension:

- `syntaqlite.schemaPath` setting and its resolution logic
- `syntaqlite.schemas` setting and glob matching (`minimatch` dependency removed)
- `resolveSchemaForUri()` function
- `sendSchemaIfChanged()` function
- `workspace/didChangeConfiguration` schema notification plumbing
- `initializationOptions.schemaPath`

The only remaining VS Code setting is `syntaqlite.serverPath` (where to find the
binary: genuinely editor-specific, not tool behavior).

**Files changed:**
- `integrations/vscode/src/extension.ts`: delete schema resolution, simplify
  activation
- `integrations/vscode/package.json`: remove `schemaPath`, `schemas` settings,
  remove `minimatch` dependency

### 6. Claude Code plugin

**Current state:** The LSP config in `plugin.json` has no way to specify schemas.

**After:** Works automatically: the LSP discovers `syntaqlite.toml` in the
project. No changes needed to the Claude Code plugin itself.

**Files changed:** None. This is the whole point.

### 7. MCP server

**Current state:** The `validate_sql` and `format_sql` tools take inline SQL and
use hardcoded defaults. No schema or config support.

**After:** The MCP server could optionally discover `syntaqlite.toml` from the
working directory to use project-specific formatting options and schemas. This is
a stretch goal: the MCP server processes inline SQL snippets, not files, so
glob-based schema routing doesn't apply naturally. The format config section is
useful though.

**Files changed:** `syntaqlite-cli/src/mcp.rs`: optional, low priority.

## Implementation

### New crate dependency

Add `toml = "0.8"` to `syntaqlite-cli/Cargo.toml`. The TOML parsing lives in
the CLI crate, not the core library: the core library doesn't need to know about
config files.

### New module: `syntaqlite-cli/src/config.rs`

```rust
use std::path::{Path, PathBuf};
use serde::Deserialize;

#[derive(Debug, Default, Deserialize)]
pub struct ProjectConfig {
    /// Default schema for files not matching any glob in [schemas].
    pub schema: Option<Vec<String>>,

    /// Glob → schema file mapping.
    #[serde(default)]
    pub schemas: IndexMap<String, Vec<String>>,

    /// Formatting options.
    #[serde(default)]
    pub format: FormatOptions,
}

#[derive(Debug, Default, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub struct FormatOptions {
    pub line_width: Option<usize>,
    pub indent_width: Option<usize>,
    pub keyword_case: Option<String>,
    pub semicolons: Option<bool>,
}

/// Walk up from `start` looking for `syntaqlite.toml`.
/// Returns (config, directory containing the config file).
pub fn discover(start: &Path) -> Option<(ProjectConfig, PathBuf)> { ... }
```

Key details:
- Uses `indexmap::IndexMap` (not `HashMap`) for schemas so glob order is
  preserved (first match wins)
- All format fields are `Option` so we can distinguish "not set" from "set to
  default": unset fields fall back to CLI args or hardcoded defaults
- `discover()` returns the config dir so relative schema paths can be resolved

### Config discovery function

```rust
pub fn discover(start: &Path) -> Option<(ProjectConfig, PathBuf)> {
    let mut dir = start;
    loop {
        let candidate = dir.join("syntaqlite.toml");
        if candidate.is_file() {
            let contents = std::fs::read_to_string(&candidate).ok()?;
            let config: ProjectConfig = toml::from_str(&contents).ok()?;
            return Some((config, dir.to_path_buf()));
        }
        dir = dir.parent()?;
    }
}
```

### Schema resolution function

```rust
/// Given a SQL file path and a config, resolve which schema files apply.
pub fn resolve_schemas(
    sql_path: &Path,
    config: &ProjectConfig,
    config_dir: &Path,
) -> Vec<PathBuf> {
    let relative = sql_path.strip_prefix(config_dir).unwrap_or(sql_path);
    let relative_str = relative.to_string_lossy();

    // Check [schemas] globs in order
    for (glob_pattern, schema_files) in &config.schemas {
        if glob_match(glob_pattern, &relative_str) {
            return schema_files.iter()
                .map(|s| config_dir.join(s))
                .collect();
        }
    }

    // Fall back to top-level schema key
    if let Some(schema) = &config.schema {
        return schema.iter()
            .map(|s| config_dir.join(s))
            .collect();
    }

    vec![]
}
```

### Merging with CLI args

```rust
/// Build FormatConfig from config file + CLI overrides.
pub fn build_format_config(
    file_config: &FormatOptions,
    cli_line_width: Option<usize>,
    cli_indent_width: Option<usize>,
    cli_keyword_case: Option<KeywordCasing>,
    cli_semicolons: Option<bool>,
) -> FormatConfig {
    FormatConfig::default()
        .with_line_width(cli_line_width
            .or(file_config.line_width)
            .unwrap_or(80))
        .with_indent_width(cli_indent_width
            .or(file_config.indent_width)
            .unwrap_or(2))
        // ... etc
}
```

### LSP integration

The LSP server changes are the most involved:

1. **On startup:** discover `syntaqlite.toml` from workspace root, parse it,
   store in `LspHost`
2. **On document open/change:** resolve schemas for that document's URI using the
   glob patterns, load the appropriate schema catalog
3. **On format request:** use the config file's format options
4. **On file watch:** re-read `syntaqlite.toml` when it changes
5. **Delete:** `load_schema_from_settings()`, `load_schema_from_options()`, and
   all `initializationOptions.schemaPath` / `didChangeConfiguration` schema
   handling: editors no longer send schema config

The main architectural question is schema catalog caching. If five SQL files all
map to the same `schema/main.sql`, we should parse that schema file once, not
five times. The LSP should maintain a map of `schema file set → Catalog` and
reuse catalogs across documents.

## Phases

### Phase 1: Config file + CLI (MVP)

- Add `syntaqlite-cli/src/config.rs` with discovery, parsing, schema resolution
- Wire into `cmd_format()` and `cmd_validate()`
- Add `toml` + `indexmap` dependencies
- Tests: discovery walk-up, glob matching, CLI override precedence

### Phase 2: LSP reads config file

- LSP discovers `syntaqlite.toml` on startup
- Schema resolution per document (glob matching)
- Format config from file (currently hardcoded)
- Schema catalog caching (shared catalogs across documents)
- File watching for config changes
- Remove `load_schema_from_settings()` and `load_schema_from_options()` from
  LSP server

### Phase 3: Simplify VS Code extension

- Delete `syntaqlite.schemaPath` and `syntaqlite.schemas` settings from
  `package.json`
- Delete `resolveSchemaForUri()`, `sendSchemaIfChanged()`, and all
  `didChangeConfiguration` schema plumbing from `extension.ts`
- Remove `minimatch` dependency
- Keep only `syntaqlite.serverPath` setting
- Update status bar to show config file status instead of schema path

### Phase 4: Documentation

- Document `syntaqlite.toml` format in CLI reference
- Add getting-started guide for config file
- Update VS Code, Claude Code, and other editor docs
- Add `syntaqlite init` command to generate a starter config file (stretch)

## Open questions

1. **Should `syntaqlite.toml` support validation config?** (`strict_schema`,
   `suggestion_threshold`): probably yes, but low priority since these are
   rarely changed from defaults.

2. **Should the config file support per-file format overrides?** E.g. different
   line widths for migration files vs application SQL. Probably not in v1: keep
   it simple.

3. **Should `syntaqlite init` exist?** A command that generates a starter
   `syntaqlite.toml` would be nice for discoverability but isn't essential.

4. **Glob library:** Use `glob` crate (already a dependency for CLI path
   expansion) or `globset` (faster, used by ripgrep)? Probably `glob` for
   consistency.
