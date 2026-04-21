+++
title = "Custom dialects"
description = "Define and load a grammar extension for non-standard SQLite syntax."
weight = 10
+++

# Custom dialects

> **Experimental.** The dialect API, `.synq` format, codegen output, and
> shared-library ABI are not yet stable and can change between releases.
> Pin your syntaqlite version when you ship a dialect.

If you have SQL that extends SQLite — custom statements, extra functions,
new clauses — you can package it as a dialect that syntaqlite's parser,
formatter, and analyzer will understand. This guide walks an end-to-end
dialect from source files to a loaded shared library, using the in-tree
[Perfetto dialect](https://github.com/LalitMaganti/syntaqlite/tree/main/dialects/perfetto)
as a running example.

## 1. Write the grammar

A dialect is a directory of `.synq` node definitions plus optional `.y`
grammar action files. `.synq` is syntaqlite's grammar DSL: each `node`
declares an AST shape with typed fields, optional semantic annotations,
and a `fmt` block for pretty-printing.

Minimal example (from `dialects/perfetto/nodes/perfetto.synq`):

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

`.y` action files in a parallel `actions/` directory hook the dialect's
rules into the base SQLite grammar; simple dialects that only add functions
or reserved words may not need any.

## 2. Generate C sources

`syntaqlite dialect generate` reads your `.synq` (and optional `.y`) files
and emits a C amalgamation — the runtime plus your dialect, inlined into a
single `.h`/`.c` pair:

```bash
syntaqlite dialect generate \
  --name perfetto \
  --nodes-dir dialects/perfetto/nodes \
  --actions-dir dialects/perfetto/actions \
  --output-dir out \
  --output-type full
```

Produces:

```
out/syntaqlite_perfetto.h
out/syntaqlite_perfetto.c
```

The `--name` flag drives all generated symbol names: the dialect exposes
`syntaqlite_perfetto_dialect_template()`, which is what the loader looks for
at runtime.

Other `--output-type` values split the artifacts differently (`raw` for
flat C files per module, `runtime-only` to skip the dialect body); `full`
is the right default for shipping a self-contained shared library.

## 3. Compile to a shared library

The generated `.c` is pure C, no Rust toolchain required. Build it as a
shared library for the target platform:

```bash
# Linux / macOS
cc -O2 -shared -fPIC -o libperfetto.dylib out/syntaqlite_perfetto.c
```

On Linux the extension is `.so`; on Windows, build a `.dll` with your C
compiler's equivalent of `-shared`.

Confirm the loader symbol is exported:

```bash
nm libperfetto.dylib | grep dialect_template
# T _syntaqlite_perfetto_dialect_template
```

## 4. Use the dialect at runtime

Pass the library and dialect name to any `syntaqlite` subcommand:

```bash
syntaqlite fmt \
  --dialect ./libperfetto.dylib \
  --dialect-name perfetto \
  -e "create perfetto macro m() returns int as 42"
```

```sql
CREATE PERFETTO MACRO m()
RETURNS int
AS 42;
```

The `--dialect-name` matches the `--name` you passed to `dialect generate`.
Omit it to resolve the default `syntaqlite_dialect_template` symbol, which
is what an unnamed dialect exports.

To bake the choice into a project, set the dialect in `syntaqlite.toml`;
see the [config reference](@/reference/config-file.md).

## Alternative: compile the dialect into your binary

Dynamic loading is the fastest path to "works with the stock CLI", but it
adds a runtime dependency and a symbol-resolution step. If you're shipping
your own binary (for example, a `syntaqlite-mydialect` wrapper), compile
the dialect in directly via the Rust `CliApp` trait. See
[`syntaqlite-cli/examples/cli_wrapper.rs`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite-cli/examples/cli_wrapper.rs)
for a complete example.

Trade-offs:

| | Dynamic (`--dialect`) | Static (`CliApp`) |
|---|---|---|
| Distribution | One shared library, stock CLI | Single binary, your own name |
| Startup cost | `dlopen` on every invocation | None |
| ABI stability | Needed — library and CLI must match | Not needed — compiled together |
| Tooling | Works with any `syntaqlite` install | Requires the Rust toolchain to build |

## Next steps

- [Perfetto dialect source](https://github.com/LalitMaganti/syntaqlite/tree/main/dialects/perfetto)
  — complete working example including advanced `.synq` features
- [Architecture: grammar system](@/contributing/architecture.md#grammar-system)
  — how `.synq` flows through codegen into the parser, formatter, and analyzer
