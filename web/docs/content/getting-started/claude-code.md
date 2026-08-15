+++
title = "Claude Code"
description = "Install the syntaqlite plugin for Claude Code."
weight = 3
+++

# Claude Code

## Install the plugin

Add the marketplace and install the plugin:

```bash
claude plugin marketplace add LalitMaganti/claude-code-plugin
claude plugin install syntaqlite@lalitmaganti-plugins
```

## Plugin features

The plugin combines language-server feedback with CLI commands:

**Language server**: starts automatically for `.sql` files. When Claude writes
or edits SQL, the server feeds back syntax errors, unknown tables/columns, and
function typos. These diagnostics are available to Claude while it edits the
file.

**CLI skills**: Claude can run `syntaqlite fmt`, `syntaqlite analyze`, and
`syntaqlite parse` directly when you ask it to format a query, check a file, or
inspect a parse tree.

## Try it out

Open a project with `.sql` files and ask Claude to edit one. If the SQL has an
error (say a misspelled column name), Claude will notice the diagnostic from
the language server and fix it in the same edit.

You can also ask Claude to format or validate explicitly:

> Format all the SQL files in src/

> Check query.sql for errors against schema.sql

## Add schema validation

Without a schema, the language server validates against an empty catalog: it
catches syntax errors and bad function calls, but not unknown tables or columns.
To enable full validation, set up a schema file. See the
[project setup guide](@/guides/project-setup.md) for instructions.

Once configured, when Claude writes `SELECT nme FROM users`, the language server
flags `nme` as unknown and suggests `name`. Claude sees this and corrects it
automatically.
