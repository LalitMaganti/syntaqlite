# syntaqlite: SQLite language server and formatter for Claude Code

SQLite SQL language support for [Claude Code](https://claude.ai/code), with
schema-aware diagnostics, formatting, completions, navigation, and semantic
highlighting.

## Features

- **LSP integration:** automatically starts the syntaqlite language server for
  `.sql` files, providing diagnostics, formatting, completions, and semantic
  highlighting.
- **`/syntaqlite:format` skill:** format SQL files with configurable options.
- **`/syntaqlite:parse` skill:** inspect SQL parse trees and ASTs.
- **`/syntaqlite:validate` skill:** check SQL for errors against a schema.

## Prerequisites

The `syntaqlite` binary must be on your `PATH`. Install via any of:

```bash
# Homebrew
brew install LalitMaganti/tap/syntaqlite

# Cargo
cargo install syntaqlite-cli

# pip
pip install syntaqlite

# Download script
curl -fsSL https://syntaqlite.com/install.sh | sh
```

## Installation

From the GitHub marketplace:

```bash
claude plugin marketplace add lalitmaganti-plugins
claude plugin install syntaqlite@lalitmaganti-plugins
```

## Configuration

Create a `syntaqlite.toml` in your project root to configure schemas and
formatting. The LSP reads it automatically; no plugin settings are needed.

```toml
[schemas]
"src/**/*.sql" = ["schema/main.sql"]

[format]
line-width = 100
```

See the [main README](../../README.md#project-configuration) for the full format.

## Usage

Once installed, the LSP server starts automatically when you open `.sql` files.
Use the skills for CLI operations:

```
/syntaqlite:format query.sql
/syntaqlite:validate --schema schema.sql query.sql
/syntaqlite:parse query.sql
```

## License

Apache 2.0. See [LICENSE](../../LICENSE) for details.
