# Changelog

## 0.2.16

- Fixed quoted identifiers (`"set"`, `` `set` ``, `[set]`) producing false-positive `unknown-table` and `unknown-column` diagnostics when DDL and query used different quoting styles ([#50](https://github.com/LalitMaganti/syntaqlite/issues/50)).
- Formatter now normalizes backtick and bracket quoting to standard double quotes.

## 0.2.15

- Added Zed editor extension for syntax highlighting and diagnostics.
- Added Zed getting-started documentation.
- Fixed Windows ARM64 Python wheel cross-compilation.
- Fixed amalgamation missing `tokens.h` after embedded-sources refactor.
- Fixed release workflow failing to upload artifacts to GitHub Release.
- Fixed Homebrew tap update using wrong token for cross-repo push.

## 0.2.14

- Skipped (broken release pipeline).

## 0.2.13

- Skipped (broken release pipeline).

## 0.2.12

- Fixed `cargo install syntaqlite-cli` failing due to `syntaqlite-buildtools` referencing files outside its crate directory ([#29](https://github.com/LalitMaganti/syntaqlite/issues/29))

## 0.2.11

- Added Python 3.14 wheel support.
- Fixed PyPI publishing (wheels were not uploaded for 0.2.1–0.2.9).

## 0.2.10

- Added Python 3.14 wheel support.
- Fixed PyPI publishing (wheels were not uploaded for 0.2.1–0.2.9).

## 0.2.9

- No user-visible changes (release pipeline consolidation).

## 0.2.8

- Fixed PyPI wheel smoke tests failing on non-Linux platforms by running each build on its native runner.

## 0.2.7

- **Breaking:** Renamed Python modules to public names: `syntaqlite._enums` → `syntaqlite.enums`, `syntaqlite._nodes` → `syntaqlite.nodes`.
- **Breaking:** `syntaqlite._parse_raw()` is now `syntaqlite.parse_raw()`.

## 0.2.6

- `parse()` now returns typed Python objects (`SelectStmt`, `BinaryExpr`, etc.) with attribute access, IDE autocomplete, and `isinstance` support. Raw dict access is still available via `syntaqlite._parse_raw()`.
- Added `python/.gitignore` and removed tracked build artifacts (`.so`, `.egg-info`).

## 0.2.5

- Fixed Python extension CI for x86_64 macOS by replacing deprecated `macos-13` runner with `macos-26-intel`.

## 0.2.4

- Fixed Windows Python extension linker errors (`__imp_` unresolved symbols) by making static linking the default for `SYNTAQLITE_API`.
- Added `SYNTAQLITE_API` annotations to all C function definitions, fixing MSVC dllimport/dllexport mismatch warnings.

## 0.2.3

- Fixed Windows Python extension build using the correct static library name for MSVC.

## 0.2.2

- Fixed Python extension build failing on Windows due to MSVC not supporting `_Static_assert` in C mode.

## 0.2.1

- Fixed PyPI wheel builds failing across all Python versions (3.10–3.13).

## 0.2.0

### Python library API

The `pip install syntaqlite` package now includes a native C extension with a full library API — previously it only bundled the CLI binary. Four functions are available: `parse()`, `format_sql()`, `validate()`, and `tokenize()`.

- `validate()` returns a `ValidationResult` with `.diagnostics` and `.lineage` attributes.
- Schema can be provided via `Table`, `View` objects or raw DDL with `schema_ddl=`.
- `format_sql()` supports `line_width`, `indent_width`, `keyword_case`, and `semicolons` kwargs.
- `parse()` returns typed AST node dicts; `tokenize()` returns token dicts.

### Column lineage

- New lineage analysis for SELECT statements — traces each result column back to its source table and column, resolving through CTEs, subqueries, and aliases.
- `SemanticModel` gains `lineage()`, `relations_accessed()`, and `tables_accessed()` methods, returning `Complete` or `Partial` results depending on view resolution.
- C API: 7 new lineage accessor functions (`syntaqlite_validator_column_lineage`, `syntaqlite_validator_relations`, `syntaqlite_validator_tables`, etc.).

### Schema registration

- C API: new `syntaqlite_validator_add_views()` for registering views separately from tables.
- C API: new `syntaqlite_validator_load_schema_ddl()` to register schema from DDL strings (CREATE TABLE/VIEW).
- C API: `SyntaqliteTableDef` renamed to `SyntaqliteRelationDef` (used for both tables and views).

### Bug fixes

- Fix stack overflow in lineage resolver for recursive CTEs on Linux.
- Fix formatter macro handling to respect per-dialect macro style settings.

## 0.1.0

Initial release of syntaqlite — a fast, accurate SQL toolkit for SQLite, built from SQLite's own grammar.

### Highlights

- **Formatter** — opinionated SQL formatter with configurable line width, keyword casing, and semicolons. Supports stdin, files, and glob patterns.
- **Parser** — full SQLite SQL parser producing a concrete syntax tree. Handles all SQLite syntax including CTEs, window functions, upsert clauses, and `RETURNING`.
- **Validator** — semantic analysis with diagnostics for unknown tables, columns, and functions. Supports embedded SQL extraction from Python and TypeScript.
- **Language Server (LSP)** — diagnostics, formatting, completions, go-to-definition, document highlights, and semantic tokens over stdio.
- **C API** — prebuilt shared libraries for macOS, Linux, and Windows, plus a source amalgamation for embedding.
- **WASM / JS** — browser-ready builds powering the interactive playground.
- **Dialect extensibility** — load custom grammars as shared libraries at runtime.

### Project configuration

`syntaqlite.toml` is the single, editor-agnostic source of truth for schemas and formatting — it works across VS Code, Claude Code, Neovim, Helix, and the CLI with no additional setup.

```toml
[schemas]
"src/**/*.sql" = ["schema/main.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []

[format]
line-width = 100
keyword-case = "lower"
```

### Install

- CLI binaries for macOS (arm64, x86_64), Linux (arm64, x86_64), and Windows (x86_64, arm64)
- `pip install syntaqlite` — bundled platform-specific binary, includes built-in MCP server (`syntaqlite mcp`)
- `brew install LalitMaganti/tap/syntaqlite`
- `cargo install syntaqlite-cli`
- `mise use github:LalitMaganti/syntaqlite`
- VS Code extension with bundled LSP (VS Code Marketplace + Open VSX)
- Claude Code plugin via Marketplace
- Rust crates: `syntaqlite`, `syntaqlite-cli`, `syntaqlite-syntax`, `syntaqlite-common`
- NPM package: `syntaqlite`
