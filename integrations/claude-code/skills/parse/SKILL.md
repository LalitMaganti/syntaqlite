---
name: parse
description: Parse SQL and inspect the AST using syntaqlite. Use when the user wants to see the parse tree, debug SQL syntax, or understand how a query is structured.
---

# Parse SQL

Parse SQLite SQL and display the abstract syntax tree (AST) using the syntaqlite CLI.

## Usage

```bash
# Print AST from a file
syntaqlite parse query.sql

# Print AST from stdin
echo "SELECT 1 + 2 FROM t" | syntaqlite parse

# Parse an inline expression
syntaqlite parse -e "SELECT 1"

# Output as JSON
syntaqlite parse -o json query.sql
```

## Options

- `-e, --expression <SQL>`: parse an inline SQL expression instead of files
- `-o, --output <FORMAT>`: output format (default: text)

## Output modes

- `text`: Human-readable indented AST tree (default)
- `json`: Machine-readable JSON AST

## Notes

- Use the default `text` output for quick inspection and `json` for programmatic use.
