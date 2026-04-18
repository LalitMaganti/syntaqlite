+++
title = "Custom dialects"
description = "Define and load a grammar extension for non-standard SQLite syntax."
weight = 10
+++

# Custom dialects

> **Experimental: work in progress.** The dialect API is under active
> development and may change significantly between releases. The `.synq` format,
> codegen output, and shared library ABI are not yet stable.

syntaqlite's grammar is extensible. If you have a SQL dialect that adds syntax
on top of SQLite (custom statements, additional functions, new clauses), you
can define a dialect that syntaqlite will use for parsing, formatting, and
validation.

## Defining a dialect

A dialect is a shared library loaded at runtime with `--dialect /path/to/lib.so`.
It extends SQLite's parser, formatter, and validator with custom rules.

Dialects are defined using `.synq` files, the same grammar definition language
syntaqlite uses internally. A `.synq` file declares nodes, enums, formatting
rules, and semantic annotations.

Here's a minimal example from the Perfetto dialect, which adds a
`CREATE PERFETTO MACRO` statement:

```synq
node CreatePerfettoMacroStmt {
  name: index SqliteIdent
  args: index PerfettoMacroArgList
  returns: inline PerfettoMacroReturns
  body: index Expr

  fmt {
    group {
      "CREATE PERFETTO MACRO" line
      child(name)
      "(" nest { softline child(args) } softline ")"
      line "RETURNS" " " child(returns)
      line "AS" nest { line child(body) }
    }
  }
}
```

## Building a dialect

Use the `syntaqlite dialect generate` command to generate C sources and Rust
bindings from your `.synq` definitions:

```bash
syntaqlite dialect generate --name mydialect --nodes-dir path/to/nodes --output-dir generated/
```

Then compile the generated sources into a shared library. See the
[Perfetto dialect](https://github.com/LalitMaganti/syntaqlite/tree/main/dialects/perfetto)
in the repository for a complete working example.

## Dialect naming

By default, syntaqlite looks for a symbol called `syntaqlite_grammar` in the
shared library. If your dialect has a specific name, use `--dialect-name`:

```bash
syntaqlite fmt --dialect /path/to/lib.so --dialect-name perfetto query.sql
```

This looks for the symbol `syntaqlite_perfetto_grammar`.
