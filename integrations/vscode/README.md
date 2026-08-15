# syntaqlite: SQLite language server and formatter

A SQLite SQL language server and formatter with schema-aware diagnostics, completions,
navigation, and semantic highlighting. The extension bundles syntaqlite, so it works without
a database connection or a separate installation.

## Features

- **Diagnostics:** syntax errors and semantic warnings as you type
- **Formatting:** format SQL documents via `Format Document` or on save
- **Completions:** SQL keywords, functions, tables, and columns
- **Go to Definition:** Ctrl+click on tables, columns, and CTEs to jump to their definition
- **Find References:** find all usages of a symbol across open files
- **Rename:** rename tables, columns, and CTEs across all references
- **Hover:** hover over identifiers to see type and schema info
- **Document Highlights:** highlights all occurrences of a symbol in the current file
- **Signature Help:** parameter hints for SQL functions
- **Semantic highlighting:** context-aware token coloring

## Installation

Install from the VS Code Marketplace. The extension includes the
syntaqlite binary for your platform; no additional setup is required.

## Configuration

Schemas and formatting are configured via `syntaqlite.toml` in your project
root. The LSP reads it automatically. See the
[config file reference](https://docs.syntaqlite.com/main/reference/config-file/)
for the full format.

```toml
[schemas]
"src/**/*.sql" = ["schema/main.sql"]

[format]
line-width = 100
keyword-case = "lower"
```

| VS Code Setting          | Default | Description                                   |
|--------------------------|---------|-----------------------------------------------|
| `syntaqlite.serverPath`  | `""`    | Override path to the syntaqlite binary.       |

## Commands

- **syntaqlite: Restart Language Server:** restart the LSP server
- **syntaqlite: Format Document:** format the active SQL file
- **syntaqlite: Open Config File:** open the project's `syntaqlite.toml`

## Development

```sh
cd integrations/vscode
npm install
npm run compile
```

Build the CLI and point the extension at it via `syntaqlite.serverPath`:

```sh
cargo build --release -p syntaqlite-cli
```

Then set `syntaqlite.serverPath` to the absolute path of `target/release/syntaqlite`
in your VS Code settings.

Press **F5** to launch an Extension Development Host.

### Packaging platform-specific .vsix

```sh
cargo build --release -p syntaqlite-cli
node scripts/package-target.mjs --target darwin-arm64 --binary ../../target/release/syntaqlite
```

Supported targets: `darwin-arm64`, `darwin-x64`, `linux-arm64`, `linux-x64`, `win32-x64`
