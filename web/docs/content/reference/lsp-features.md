+++
title = "LSP features"
description = "What each feature of the syntaqlite language server covers."
weight = 11
+++

# LSP features

`syntaqlite lsp` runs a language server over stdio. This page lists every
feature the server advertises in its `initialize` response and what each one
covers. For editor setup, see [VS Code](@/getting-started/vscode.md),
[Zed](@/getting-started/zed.md), or
[other editors](@/getting-started/other-editors.md).

The server reads `syntaqlite.toml` from the directory it starts in. Schema
files listed under `schema` and `[schemas]` give it the tables, views and
columns it uses for diagnostics, completion, and hover. Tables and views
created earlier in the same document are also known. See the
[config file reference](@/reference/config-file.md).

## Summary

| Feature | LSP method |
|---------|------------|
| [Diagnostics](#diagnostics) | `textDocument/publishDiagnostics` |
| [Formatting](#formatting) | `textDocument/formatting` |
| [Completion](#completion) | `textDocument/completion` |
| [Hover](#hover) | `textDocument/hover` |
| [Signature help](#signature-help) | `textDocument/signatureHelp` |
| [Go to definition](#go-to-definition) | `textDocument/definition` |
| [Find references](#find-references) | `textDocument/references` |
| [Rename](#rename) | `textDocument/prepareRename`, `textDocument/rename` |
| [Document highlights](#document-highlights) | `textDocument/documentHighlight` |
| [Semantic tokens](#semantic-tokens) | `textDocument/semanticTokens/full`, `textDocument/semanticTokens/range` |
| [Session context](#session-context) | `syntaqlite/setSessionContext` |

The server uses full-document sync and UTF-16 positions.

## Diagnostics

The server publishes diagnostics when a document is opened or changed, and
clears them when it is closed. Every statement is checked, so a syntax error
does not hide problems in later statements.

Diagnostics cover syntax errors and every semantic check listed in the
[config file reference](@/reference/config-file.md#checks), such as unknown
tables and columns.
Unknown names come with a "did you mean" suggestion when a close match exists.

Each check's level comes from the `[checks]` section of `syntaqlite.toml`.
`"deny"` reports an error, `"warn"` reports a warning, and `"allow"` suppresses
the diagnostic. See the [config file reference](@/reference/config-file.md)
for the categories and their defaults.

## Formatting

Formats the whole document with the syntaqlite formatter. Options come from
the `[format]` section of `syntaqlite.toml`. See
[formatting options](@/reference/formatting-options.md).

## Completion

Offers keywords, table and view names, column names, and function names. The
server requests completion after a space, `.`, newline, tab, or `;`.

- Keywords are limited to those the parser accepts at the cursor.
- Table, column and function names are only offered where an identifier is
  valid.
- After `FROM` or `JOIN`, names are limited to tables and views.
- After `t.`, only the columns of `t` are offered.
- Inside a query, columns and table names come from the tables in scope:
  its `FROM` and `JOIN` sources and their aliases, and those of enclosing
  queries. Outside a query, or when a table in scope has unknown columns,
  every table's columns are offered.

Items are filtered by the partially typed word, ignoring case. A word matches
a name if it is a prefix of the name, a prefix of one of its `_`-separated
parts, a substring, or a subsequence. Matches are ranked in that order within
each kind, and the kinds are ordered columns, tables, keywords, then
functions. For example, typing `slice` after `FROM` ranks `slice` first, then
`thread_slice`, then `spline_cache`.

## Hover

Hovering a reference shows:

- **Table or view**: its name and column list.
- **Column**: the table it belongs to and that table's columns, with the
  hovered column in bold.
- **Function**: its category, such as scalar, aggregate or window, and its
  accepted argument counts.

Names that do not resolve show no hover.

## Signature help

Inside a function call, shows the function's accepted argument counts and
highlights the argument under the cursor. Parameters are shown as `arg1`,
`arg2`, and so on. Signature help triggers on `(` and `,`.

## Navigation

Go to definition, find references, rename and document highlights share one
model of which names resolve. They cover:

- Tables, views and columns referenced in queries and DML statements,
  including columns reached through a table alias such as `c.qty` in
  `FROM child c`.
- CTE names and CTE columns, from either the column list or the select
  aliases.
- View columns, from the view's column list when it has one.
- `CREATE INDEX`: the target table, the indexed columns, and the `WHERE`
  clause of a partial index.
- `CREATE TABLE`: columns in `CHECK` constraints, generated column
  expressions, and `PRIMARY KEY`, `UNIQUE` and `FOREIGN KEY` column lists.
- `REFERENCES` clauses: the parent table and parent columns.
- `CREATE TRIGGER`: the target table, its columns through `NEW` and `OLD`,
  and the `UPDATE OF` column list.
- `ALTER TABLE` targets.
- `DROP TABLE` and `DROP VIEW` targets.

Functions are not navigation targets.

### Go to definition

Jumps from a reference to the `CREATE TABLE`, `CREATE VIEW` or CTE that
defines the name, or to the column definition. The definition can be in the
same document or in a schema file listed in `syntaqlite.toml`.

### Find references

Lists every reference to the table, view or column under the cursor across
all open documents. The cursor can be on a reference or on the definition.
The definition itself is included when the client asks for it.

### Rename

Renames a table, view or column at its definition and every reference across
all open documents. Start the rename with the cursor on the definition or on
any reference.

### Document highlights

Highlights the definition and every reference to the name under the cursor
in the current document.

## Semantic tokens

Classifies tokens as keywords, bind parameters, strings, numbers, comments,
identifiers, function names and type names. Operators and punctuation are not
classified. Both full-document and range requests are supported.

## Session context

`syntaqlite/setSessionContext` is a syntaqlite extension request, advertised
under `capabilities.experimental.syntaqlite`. It sets the schema used for
analysis without a config file, which is useful when embedding the server.
The params are one of:

- `{"ddl": "CREATE TABLE ..."}` to build the schema from DDL.
- `{"context": {...}}` to pass a structured schema as JSON, with `tables`,
  `views` and `functions`. A table column is a name, or
  `{"name": "total", "kind": "generated"}` where `kind` is `normal`,
  `generated` or `hidden`.
- `{}` or `null` to clear it.

The result is `{"errors": [...]}`. DDL that only partially parses is still
applied, and its parse errors are listed in `errors`. Diagnostics for every
open document are republished after the change.
