+++
title = "Formatting philosophy"
description = "How the formatter decides where to break lines and why."
weight = 2
+++

# Formatting philosophy

syntaqlite's formatter is deterministic and opinionated: the same SQL always
produces the same output regardless of how it was originally written. Its
layout decisions follow the document model described below.

## The algorithm: Wadler-style pretty-printing

The formatter uses a
[Wadler-Lindig document algebra](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/fmt/doc.rs),
the same approach used by rustfmt and Prettier. The core idea is simple:

1. Parse the SQL into an AST
2. Convert the AST into a *document*, a tree of layout instructions
3. Render the document, fitting as much as possible on each line

The document tree is built from a small set of primitives
(defined in [`doc.rs`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/fmt/doc.rs)):

| Primitive | Flat mode (fits on line) | Break mode (doesn't fit) |
|-----------|--------------------------|--------------------------|
| `Group`   | Try to render child flat | Render child with breaks |
| `Line`    | Space                    | Newline + indent         |
| `SoftLine`| Nothing                  | Newline + indent         |
| `Nest`    | (no effect)              | Increase indent level    |
| `Keyword` | Keyword text (cased)     | Keyword text (cased)     |
| `Text`    | Source text (as-is)      | Source text (as-is)      |

A `Group` controls whether its contents stay on one line. If the group fits
within the configured line width, the renderer keeps it flat. Otherwise, the
group *breaks* and its `Line` and `SoftLine` nodes become newlines.

This means the formatter doesn't have hard-coded rules about "always break
after FROM" or "always inline short WHERE clauses". Instead, it tries to keep
things compact and breaks when the line would be too long.

## How formatting rules are defined

Each AST node type has formatting rules defined in a
[`.synq` grammar file](https://github.com/LalitMaganti/syntaqlite/tree/main/syntaqlite-buildtools/parser-nodes).
These rules use a declarative DSL that compiles to bytecode at build time.

For example, the following is a simplified formatting rule for `INSERT`
statements (from
[`dml.synq`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite-buildtools/parser-nodes/dml.synq)):

```
fmt {
  group {
    "INSERT"
    " INTO " child(table)
    if_set(columns) {
      group { "(" nest { softline child(columns) } softline ")" }
    }
    if_set(source) { line child(source) }
    if_set(returning) { line "RETURNING " child(returning) }
  }
}
```

The `group { ... }` wrapping means: try to fit the entire INSERT on one line.
If it's too long, break at each `line` point (before the source clause, before
RETURNING). The column list has its own nested group, so it breaks
independently if the column list alone is too long.

The bytecode interpreter
([`interpret.rs`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/fmt/interpret.rs))
walks the AST and executes these rules to build the document tree, which is
then rendered.

## Keyword casing

Keywords are always identified by the parser (they come from SQLite's own
keyword table). The formatter applies the configured casing at render time. `Text` nodes
(identifiers, literals, table names) are never modified.

With default settings (`upper`), `select 1` becomes `SELECT 1;`. With
`keyword-case = "lower"`, `SELECT 1` becomes `select 1;`.

## Comment preservation

Comments are tracked separately from the AST. During formatting, the
[comment handler](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/fmt/comment.rs)
reattaches them at the appropriate positions:

- **Trailing comments** (same line) are placed at the end of the formatted line
  using a `LineSuffix` doc node
- **Leading comments** (own line) are placed before the next statement or
  clause, preserving blank line separation

The formatter preserves all comments. It never drops or relocates them to a
different statement.

## Semicolons

By default, the formatter appends a semicolon after every statement. This can
be disabled with `--semicolons=false` (CLI) or `.with_semicolons(false)` (Rust
API).

## Canonical spellings

SQLite accepts several pairs of spellings that mean exactly the same thing to
its parser. The formatter picks one spelling for each pair, so the same query
always formats the same way regardless of which spelling you wrote.

Optional keywords that carry no meaning are dropped:

| you write | you get |
|---|---|
| `CREATE TRIGGER ... FOR EACH ROW` | `CREATE TRIGGER ...` |
| `BEGIN TRANSACTION`, `COMMIT TRANSACTION` | `BEGIN`, `COMMIT` |
| `ATTACH DATABASE`, `DETACH DATABASE` | `ATTACH`, `DETACH` |
| `a AS (expr) VIRTUAL` | `a AS (expr)` |
| `ORDER BY a ASC`, `CREATE INDEX i ON t(a ASC)` | `ORDER BY a`, `CREATE INDEX i ON t(a)` |

Optional keywords are added where they make the statement read unambiguously:

| you write | you get |
|---|---|
| `CREATE TRIGGER tr INSERT ON t` | `CREATE TRIGGER tr BEFORE INSERT ON t` |
| `ALTER TABLE t RENAME a TO b` | `ALTER TABLE t RENAME COLUMN a TO b` |
| `RELEASE sp`, `ROLLBACK TO sp` | `RELEASE SAVEPOINT sp`, `ROLLBACK TO SAVEPOINT sp` |

And where SQLite has two names for one thing, the shorter is used:

| you write | you get |
|---|---|
| `END`, `END TRANSACTION` | `COMMIT` |
| `a <> b` | `a != b` |
| `CREATE TEMPORARY TABLE` | `CREATE TEMP TABLE` |
| `INSERT OR REPLACE INTO t` | `REPLACE INTO t` |

Every one of these is semantically inert: the two spellings compile to identical
bytecode, and where the keyword is added it matches the default SQLite would
have applied anyway. A trigger with no timing keyword really is a `BEFORE`
trigger, and an index column with no sort order really is `ASC`.

> **Note:** SQLite stores the original text of `CREATE` statements in
> `sqlite_master.sql`. Reformatting a schema therefore changes what a later
> `SELECT sql FROM sqlite_master` returns, in the same way that changing
> whitespace or keyword casing does. The schema itself is unaffected.

## What the formatter does *not* do

The formatter pretty-prints the AST as-is. It does not:

- Rewrite queries (e.g., converting implicit joins to explicit `JOIN`)
- Reorder clauses
- Normalize expressions (e.g., `a = 1` vs `1 = a`) beyond the
  [canonical spellings](#canonical-spellings) above
- Add or remove aliases
- Change quoting style on identifiers

If the SQL parses, the formatted output is semantically identical.
