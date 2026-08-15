+++
title = "Formatting philosophy"
description = "How the formatter decides where to break lines and why."
weight = 2
+++

# Formatting philosophy

syntaqlite's formatter is deterministic and opinionated: the same SQL always
produces the same output regardless of how it was originally written. This page
explains the algorithm and the reasoning behind key formatting decisions.

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

The key is `Group`: the renderer tries to fit the group's contents on a single
line. If it fits within the configured line width, everything stays flat. If
not, the group *breaks*, and `Line`/`SoftLine` nodes become newlines.

This means the formatter doesn't have hard-coded rules about "always break
after FROM" or "always inline short WHERE clauses". Instead, it tries to keep
things compact and breaks when the line would be too long.

## How formatting rules are defined

Each AST node type has formatting rules defined in a
[`.synq` grammar file](https://github.com/LalitMaganti/syntaqlite/tree/main/syntaqlite-buildtools/parser-nodes).
These rules use a declarative DSL that compiles to bytecode at build time.

For example, here's a simplified version of how `INSERT` statements are
formatted (from
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

## What the formatter does *not* do

The formatter pretty-prints the AST as-is. It does not:

- Rewrite queries (e.g., converting implicit joins to explicit `JOIN`)
- Reorder clauses
- Normalize expressions (e.g., `a = 1` vs `1 = a`)
- Add or remove aliases
- Change quoting style on identifiers

If the SQL parses, the formatted output is semantically identical.
