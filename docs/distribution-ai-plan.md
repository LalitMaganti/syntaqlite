# Distribution & Language Bindings Plan

## Problem

syntaqlite has three distinct audiences with different needs:

1. **Embedders**: want to parse/tokenize SQL in C/C++/Zig with zero dependencies
2. **Devtool authors**: want formatting, validation, completions in any language
3. **Dialect authors**: want to define new SQL dialects on top of the core engine

Today we have a Rust crate and a hand-written WASM layer. We need a coherent
distribution story that covers C, C++, Go, Zig, TypeScript, and Python without
creating an unsustainable maintenance burden.

## Prior Art

### SQLite (the model we're closest to)

- `sqlite-amalgamation-*.zip` → `sqlite3.c`, `sqlite3.h`, `sqlite3ext.h`
- `sqlite-autoconf-*.tar.gz` → Autotools build for `libsqlite3.so/.a`
- Three tiers (embed, link, extend) from two headers and one .c file
- Extension authors include `sqlite3ext.h`, receive function pointers at runtime

### DuckDB

- Prefix-namespaced release artifacts: `duckdb_cli-*`, `libduckdb-*`, `static-libs-*`
- Two-header split: `duckdb.h` (users) / `duckdb_extension.h` (extenders)
- Language bindings shipped via ecosystem package managers (PyPI, npm, Maven),
  not GitHub releases
- Extension template repo with C API that receives function pointers, no
  link-time dependency on DuckDB

### Hugo

- Edition as filename infix: `hugo_extended_withdeploy_0.157.0_linux-amd64.tar.gz`
- Self-documenting filenames, no matrix picker needed
- No extension API: composition via themes/modules, not C plugins

### Tree-sitter (cautionary tale)

- No dedicated embed artifact: C users must clone the repo
- NxM packaging problem: every grammar × every language ecosystem
- Grammar authoring requires Node.js regardless of target language
- Community-acknowledged "packaging mess"

### Key Lessons

1. SQLite's amalgamation insight: `gcc syntaqlite.c app.c -o app` is worth the
   generator complexity
2. DuckDB's prefix namespacing (`duckdb_cli-`, `libduckdb-`) is unambiguous
3. The extension header split (`sqlite3ext.h` / `duckdb_extension.h`) is a
   proven pattern: both SQLite and DuckDB converged on it independently
4. Tree-sitter's NxM grammar×language matrix is exactly what we want to avoid

## Architecture

### The Two Layers

syntaqlite's internal structure is:

```
┌─────────────────────────────────────────────────────┐
│  High-level API (Rust, exposed via C ABI)           │
│  format, validate, complete, semantic tokens, LSP   │
│  (Rust: Formatter, AnalysisHost, etc.)              │
├─────────────────────────────────────────────────────┤
│  Low-level C core                                   │
│  parse, tokenize, AST arena, node access            │
│  (C: syntaqlite_parser_new, syntaqlite_parse, ...)  │
└─────────────────────────────────────────────────────┘
```

These are two different products at two different abstraction levels:

- **The C core** is a parser engine. It gives you a parse tree. Self-contained,
  no dependencies, compiles anywhere C compiles.
- **The Rust layer** adds intelligence: formatting (bytecode interpreter),
  validation (schema checks, fuzzy suggestions), completions, semantic tokens.

The C→Rust→C path is not circular. The low-level C core is a parser state
machine. The high-level C API is a developer tool interface. They share a
language at the boundary but are completely different abstraction levels: like
how SQLite's parser is C and `sqlite3_exec()` is also C, but they serve
different purposes.

### What the high-level API includes

The high-level library (`libsyntaqlite`) **re-exports the parser/tokenizer API
alongside** the format/validate/complete functions. It is a superset, not a
separate world. Users who want low-level AST walking alongside formatting get
it from one library.

Keeping the parser in the library avoids introducing a separate parser dependency.
Refusing to expose it just creates frustration for users who want both levels.

The amalgamation (`syntaqlite_parser.h` + `syntaqlite_parser.c`) is the
**embedded/no-Rust** alternative for people who only need parsing and want zero
dependencies. Same parser API, different packaging:

|                              | `syntaqlite.h` + `libsyntaqlite` | `syntaqlite_parser.h` + `syntaqlite_parser.c` |
|------------------------------|----------------------------------|------------------------------------------------|
| Parse / tokenize             | Yes                              | Yes                                            |
| Format / validate / complete | Yes                              | No                                             |
| Dependency                   | Prebuilt library                 | None (compile from source)                     |
| Audience                     | Devtool authors                  | Embedders                                      |

The parser API surface is identical in both. The difference is what else comes
with it and how you compile it.

### Crate structure

Two workspace crates:

```
syntaqlite-sys/           ← all C code lives here
├── csrc/
│   ├── parser.c                 ← core engine
│   ├── tokenizer.c              ← core tokenizer
│   ├── token_wrapped.c          ← token wrapper
│   ├── dialect_dispatch.h       ← macro dispatch
│   └── sqlite/                  ← SQLite dialect C (generated)
│       ├── dialect.c
│       ├── dialect_builder.h
│       ├── dialect_meta.h
│       ├── dialect_fmt.h
│       ├── dialect_tokens.h
│       ├── sqlite_parse.c
│       ├── sqlite_tokenize.c
│       └── sqlite_keyword.c
├── include/
│   ├── syntaqlite/              ← core public headers
│   │   ├── parser.h
│   │   ├── tokenizer.h
│   │   ├── dialect.h
│   │   ├── types.h
│   │   └── config.h
│   ├── syntaqlite_dialect/          ← dialect extension SPI
│   │   ├── arena.h
│   │   ├── ast_builder.h
│   │   └── vec.h
│   └── syntaqlite_sqlite/       ← SQLite-specific headers
│       ├── sqlite.h
│       ├── sqlite_node.h
│       └── sqlite_tokens.h
└── build.rs                     ← compiles csrc/ via cc crate

syntaqlite/                      ← all Rust code + C API + WASM
├── src/
│   ├── parser/                  ← parser FFI, session, nodes
│   ├── dialect/                 ← Dialect<'d> handle
│   ├── fmt/                     ← formatter (feature="fmt")
│   ├── lsp/                     ← LSP host (feature="lsp")
│   ├── sqlite/                  ← SQLite dialect (feature="sqlite")
│   ├── validation/              ← semantic validation (feature="validation")
│   └── capi/                    ← C API exports (feature="capi")
│       └── mod.rs               ← extern "C" fns: engine_new, format, etc.
├── Cargo.toml
│   [lib]
│   crate-type = ["rlib", "cdylib", "staticlib"]
│   [features]
│   default = ["sqlite", "fmt", "validation"]
│   capi = ["fmt", "lsp", "validation"]
│   ...
└── include/
    └── syntaqlite.h             ← hand-written high-level C API header
```

Key design choices:
- **All C in one crate, all Rust in another.** Clean separation. One-way
  dependency: `syntaqlite` → `syntaqlite-sys`.
- **C API is a feature flag, not a separate crate.** `syntaqlite` with
  `--features capi` produces the `cdylib`/`staticlib` with `extern "C"` exports.
  Without `capi`, it's a normal Rust library.
- **Multiple crate types coexist.** `rlib` + `cdylib` + `staticlib` in the same
  crate. `cargo check`/`clippy` only type-check (no link overhead). `cargo build`
  produces all three but only the relevant one gets used by consumers.
- **External dialect crates** depend on `syntaqlite-sys` (for C headers
  and `syntaqlite_dialect.h`) and on `syntaqlite` (for Rust types). They compile
  their own generated dialect C in their own `build.rs`.

## The Three Use Cases

### Use Case 1: Embed a SQL parser (C amalgamation)

> "I want to parse SQLite SQL in my C/C++/Zig project with zero dependencies."

**What the user gets:**

```
syntaqlite-amalgamation-{version}.zip
├── syntaqlite_parser.h       ← parser/tokenizer API
├── syntaqlite_parser.c       ← core engine + SQLite dialect, single TU
└── USAGE.md
```

**How they use it:**

```c
#include "syntaqlite_parser.h"

// parse, walk AST, access tokens: all from two files
```

```sh
gcc -c syntaqlite_parser.c -o syntaqlite_parser.o
gcc myapp.c syntaqlite_parser.o -o myapp
```

Same model as `sqlite3.c` + `sqlite3.h`. Two files, zero decisions.

### Use Case 2: Devtools (high-level library)

> "I want SQL formatting, validation, and completions in Go/Python/C++/etc."

**What the user gets depends on language:**

| Language   | Install                                   | What ships                               |
|------------|-------------------------------------------|------------------------------------------|
| Rust       | `cargo add syntaqlite`                    | Native crate (already exists)            |
| C / C++    | download `libsyntaqlite-{platform}.zip`   | `syntaqlite.h` + `.so/.dylib/.a`         |
| Zig        | download same zip                         | `@cImport("syntaqlite.h")` + link `.a`   |
| Go         | `go get ...syntaqlite`                    | cgo wrapper over `syntaqlite.h` + `.a`   |
| Python     | `pip install syntaqlite`                  | cffi wrapper, bundled `.so`              |
| TypeScript | `npm install syntaqlite`                  | WASM bundle (already exists)             |

**The high-level C API (`syntaqlite.h`):**

```c
// ── High-level engine API ─────────────────────────────────────────────
//
// Opaque engine handle: owns parser, formatter, validator internally.

typedef struct SyntaqliteEngine SyntaqliteEngine;

typedef struct {
    unsigned int line_width;    // 0 = default (80)
    unsigned int keyword_case;  // 0=preserve, 1=UPPER, 2=lower
    unsigned int semicolons;    // 0=off, 1=on
} SyntaqliteFormatConfig;

// Lifecycle
SyntaqliteEngine* syntaqlite_engine_new(void);
void              syntaqlite_engine_free(SyntaqliteEngine* e);

// Core operations: return malloc'd strings, free with syntaqlite_string_free
char* syntaqlite_format(SyntaqliteEngine* e, const char* sql, size_t len,
                        const SyntaqliteFormatConfig* config);
char* syntaqlite_ast_json(SyntaqliteEngine* e, const char* sql, size_t len);
char* syntaqlite_diagnostics(SyntaqliteEngine* e, const char* sql, size_t len);
char* syntaqlite_completions(SyntaqliteEngine* e, const char* sql, size_t len,
                             unsigned int offset);
char* syntaqlite_semantic_tokens(SyntaqliteEngine* e, const char* sql,
                                 size_t len);
void  syntaqlite_string_free(char* s);

// Configuration
int syntaqlite_set_version(SyntaqliteEngine* e, const char* version);
int syntaqlite_set_schema_ddl(SyntaqliteEngine* e, const char* ddl, size_t len);
int syntaqlite_set_schema_json(SyntaqliteEngine* e, const char* json,
                               size_t len);
int syntaqlite_clear_schema(SyntaqliteEngine* e);
int syntaqlite_set_cflag(SyntaqliteEngine* e, const char* name);
int syntaqlite_clear_cflag(SyntaqliteEngine* e, const char* name);

// ── Low-level parser/tokenizer API ────────────────────────────────────
//
// Also available via the amalgamation (syntaqlite_parser.h) for
// zero-dependency embedding. Same API surface in both.

typedef struct SyntaqliteParser SyntaqliteParser;
// ... (parser_new, parse, walk nodes, tokens, etc.)
```

~20 functions total. The high-level functions are string in, JSON/string out.
The low-level parser functions are the same as the amalgamation header.

### Use Case 3: Dialect authoring (extension API)

> "I have a SQLite-based database with custom syntax. I want syntaqlite to
> understand my dialect."

Dialects always live out-of-tree. A dialect crate needs:

1. `syntaqlite_dialect.h`: the dialect SPI contract (what tables/functions a
   dialect must provide). This is a small header from `syntaqlite-sys`
   and could even be auto-generated into the dialect crate by a build
   dependency.
2. `.synq` files describing the dialect's AST nodes, enums, flags, fmt
3. Run `syntaqlite codegen` to produce dialect C code
4. Compile the generated C alongside the core parser

For Rust dialect crates:
```toml
[build-dependencies]
syntaqlite-sys = "1"   # for include/ headers and ext SPI

[dependencies]
syntaqlite = "1"              # for Rust types
```

For C-only dialect users:
```
syntaqlite-amalgamation-dialect-{version}.zip
├── syntaqlite_parser.h       ← parser/tokenizer API
├── syntaqlite_parser.c       ← core engine (WITHOUT SQLite baked in)
├── syntaqlite_dialect.h          ← dialect SPI
└── USAGE.md
```

Then: `gcc syntaqlite_parser.c syntaqlite_mydialect.c myapp.c -o myapp`

## Naming Conventions

### File naming

The high-level library gets the "prime" unsuffixed name (`syntaqlite.h`).
The amalgamation gets an explicit suffix (`syntaqlite_parser.h`). Rationale:
most users want the high-level thing; the amalgamation is the specialized path.

| File                   | Purpose                           | Audience                        |
|------------------------|-----------------------------------|---------------------------------|
| `syntaqlite.h`         | High-level API (superset)         | Devtool authors (all languages) |
| `syntaqlite_parser.h`  | Low-level parser/tokenizer API    | Embedders, dialect authors      |
| `syntaqlite_parser.c`  | Core engine amalgamation          | Embedders, dialect authors      |
| `syntaqlite_dialect.h`     | Dialect extension SPI             | Dialect authors only            |

### Release artifact naming

Following DuckDB's prefix pattern + Hugo's platform convention:

```
# Amalgamation (embed story): parser only, zero dependencies
syntaqlite-amalgamation-{version}.zip

# Amalgamation for dialect authors: core without SQLite
syntaqlite-amalgamation-dialect-{version}.zip

# High-level library (devtools story): format, validate, complete, parse
libsyntaqlite-{version}-linux-x64.tar.gz
libsyntaqlite-{version}-linux-arm64.tar.gz
libsyntaqlite-{version}-macos-arm64.tar.gz
libsyntaqlite-{version}-macos-x64.tar.gz
libsyntaqlite-{version}-windows-x64.zip

# CLI tool
syntaqlite-cli-{version}-linux-x64.tar.gz
syntaqlite-cli-{version}-macos-arm64.tar.gz
...

# WASM (web/Node)
syntaqlite-wasm-{version}.tar.gz
```

The version token is semver (`1.0.0`), not SQLite's integer encoding, since
we're already using semver for the Rust crate.

## Symbol Coexistence: Library + Amalgamation in the Same Binary

### The problem

In a large codebase, it is possible for `libsyntaqlite.a` (the high-level
library) and `syntaqlite_parser.c` (the amalgamation) to end up linked into the
same binary: e.g. one team vendors the amalgamation for parsing, another
depends on the library for formatting, and a shared test binary pulls in both.

The library contains a copy of the C parser internally (compiled via
`syntaqlite-sys`'s `build.rs` using the `cc` crate). If both copies
export the same symbols, you get duplicate symbol errors at link time. Worse, if
they're different versions, you get silent ABI mismatches and mysterious crashes.

### Background: how `.a` linking works

Static archives (`.a`) use lazy linking: the linker only extracts `.o` members
when it needs an undefined symbol. If the amalgamation is linked first and
already satisfies the parser symbols, the library's `.o` members *could* be
skipped. But this depends on link order (not controllable in large build systems
like Bazel/Buck) and `.o` granularity (if a `.o` contains both a duplicate
symbol and a unique one the linker needs, it must extract the whole `.o`,
causing a conflict). This is too fragile to rely on.

### Solution: three modes with explicit control

#### Default: fail fast (no flags)

Both the library and the amalgamation define a sentinel symbol:

```c
// In syntaqlite-sys's internal parser compilation
const char syntaqlite_parser_sentinel_ __attribute__((used)) = 1;

// In syntaqlite_parser.c (the amalgamation)
const char syntaqlite_parser_sentinel_ __attribute__((used)) = 1;
```

If both are linked → duplicate symbol error. Immediate, obvious, and the symbol
name `syntaqlite_parser_sentinel_` tells the developer exactly what happened.
Documentation says: "You're linking both the library and the amalgamation. Pick
one, or see the coexistence flags below."

Failing by default prevents silent coexistence with potential version mismatches
is far worse than a loud build failure.

#### Mode 2: `-DSYNTAQLITE_ALLOW_DUPLICATE_PARSER`: tolerate coexistence

The amalgamation guards its sentinel:

```c
#ifndef SYNTAQLITE_ALLOW_DUPLICATE_PARSER
const char syntaqlite_parser_sentinel_ __attribute__((used)) = 1;
#endif
```

The user compiles their amalgamation with `-DSYNTAQLITE_ALLOW_DUPLICATE_PARSER`.
The sentinel disappears, no link error. The binary contains two copies of the
parser (~200KB extra). The library's internal copy is built with
`-fvisibility=hidden` so its parser symbols don't conflict with the
amalgamation's visible ones. The library's high-level API uses
`syntaqlite_engine_*` prefixed names for parser access, which are different from
the amalgamation's `syntaqlite_parser_*` names, avoiding a symbol collision.

This mode is for: "I know both are present, I accept the binary size cost, just
make it link."

#### Mode 3: `no-bundled-parser`: library uses external parser

A Cargo feature on `syntaqlite-sys` that tells its `build.rs` to skip
compiling the C parser entirely. The Rust code still declares `extern "C"` FFI
imports for the parser functions but provides no definitions, so they become
undefined symbols that the linker must resolve from elsewhere.

The user links `libsyntaqlite.a` (no parser inside) + `syntaqlite_parser.c`
(provides the parser). One copy of everything, zero duplication.

In `syntaqlite-sys/Cargo.toml`:

```toml
[features]
no-bundled-parser = []
```

In `syntaqlite-sys/build.rs`:

```rust
if env::var("CARGO_FEATURE_NO_BUNDLED_PARSER").is_err() {
    let mut engine_build = cc::Build::new();
    engine_build
        .file(csrc.join("tokenizer.c"))
        .file(csrc.join("parser.c"))
        .file(csrc.join("token_wrapped.c"))
        .flag("-fvisibility=hidden")
        .compile("syntaqlite_engine");
}
```

This mode is for: power users in large monorepos who want to control exactly
one copy of the parser and are willing to manage the linkage themselves.

### Summary

| Mode                   | Flag                                              | Binary size  | Link behavior                    | Who uses it                          |
|------------------------|---------------------------------------------------|--------------|----------------------------------|--------------------------------------|
| Default                | (none)                                            | N/A          | Duplicate symbol error           | Catches accidental coexistence       |
| Allow duplicate        | `-DSYNTAQLITE_ALLOW_DUPLICATE_PARSER` on amalg.   | 2x parser    | Both copies link, hidden+visible | "I know, just make it work"          |
| External parser        | `no-bundled-parser` feature on parser-sys crate   | 1x parser    | Library uses amalgamation's copy | Monorepo power users, zero waste     |

### Parser API namespacing

To support coexistence cleanly, the library and amalgamation use different
symbol prefixes for parser access:

- **Amalgamation** (`syntaqlite_parser.h`): standalone functions like
  `syntaqlite_parser_new()`, `syntaqlite_parser_parse()`, etc.
- **Library** (`syntaqlite.h`): engine-mediated functions like
  `syntaqlite_engine_parse()`, `syntaqlite_engine_walk_nodes()`, etc.

Same capabilities, different names. Both can exist in the same binary without
conflict because the symbol sets are disjoint.

## Language Binding Maintenance

### Tier 1: Zero extra work (C header IS the binding)

| Language | How                                              | Maintenance                                  |
|----------|--------------------------------------------------|----------------------------------------------|
| C        | `syntaqlite.h` or `syntaqlite_parser.h` directly | None: headers are the source of truth       |
| C++      | Same headers (C linkage compatible), optional RAII wrapper | ~50 lines                           |
| Zig      | `@cImport("syntaqlite.h")`, link `.a`            | None                                         |

### Tier 2: Thin wrapper (~100-150 lines each)

| Language | How                                              | Maintenance                                  |
|----------|--------------------------------------------------|----------------------------------------------|
| Go       | cgo package wrapping `syntaqlite.h`              | Low: API is ~15 functions                   |
| Python   | cffi loading `.so` + Pythonic class wrapper       | Low                                          |

### Tier 3: Already exists / different path

| Language   | How                                            | Maintenance                                  |
|------------|------------------------------------------------|----------------------------------------------|
| Rust       | Native crate                                   | Normal Rust development                      |
| TypeScript | WASM (Emscripten, already built)               | Already maintained                           |

**Total new code for all 7 languages: ~400 lines** beyond the C API module
(~200 lines) that implements `syntaqlite.h`.

## Resolved Decisions

### 1. Amalgamation: one file or two?

**Decision:** Two variants, but each is self-contained:

- **Default amalgamation:** `syntaqlite_parser.c` with SQLite baked in. One file.
- **Dialect amalgamation:** `syntaqlite_parser.c` without SQLite. For dialect
  authors who provide their own dialect via codegen.

There is no separate `syntaqlite_sqlite.c` that you compile alongside the core.
Either you get the full thing or the bare core.

### 2. Can the amalgamation and library coexist?

**Decision:** Three explicit modes. Default: fail fast with a sentinel symbol
that causes a duplicate definition error if both are linked: catches accidents.
`-DSYNTAQLITE_ALLOW_DUPLICATE_PARSER`: amalgamation drops its sentinel, both
link with ~200KB duplication, hidden visibility prevents symbol conflicts.
`no-bundled-parser` Cargo feature: library ships without the C parser, uses the
amalgamation's copy at link time, zero duplication. See "Symbol Coexistence"
section for full details.

### 3. Which product gets the unsuffixed "prime" name?

**Decision:** The high-level library gets `syntaqlite.h`. The amalgamation gets
`syntaqlite_parser.h`. Rationale: most users want the high-level API. The
amalgamation is the specialized zero-dependency path. This follows DuckDB's
model (`duckdb.h` = high-level) rather than SQLite's (`sqlite3.h` = everything).

### 4. Does `syntaqlite.h` re-export the parser API?

**Decision:** Yes, but through engine-mediated functions with
`syntaqlite_engine_*` prefix. This provides the full parser/tokenizer
capability without creating symbol conflicts with the amalgamation.

### 5. Headers are hand-written

**Decision:** All public headers (`syntaqlite.h`, `syntaqlite_parser.h`,
`syntaqlite_dialect.h`) are hand-written and treated as stable API contracts. No
cbindgen or other generation. This gives full control over naming, layout, and
documentation in the headers.

### 6. Language wrappers live in-repo

**Decision:** Go, Python, and other language wrappers live in the main repo.
Easier to keep in sync, version together, test in CI. Can extract later if
needed.

### 7. TypeScript uses WASM only

**Decision:** The npm package ships WASM (Emscripten, already built). No native
Node.js addon. WASM is fast enough and avoids platform-specific binaries.

### 8. All artifacts release together

**Decision:** Amalgamation, library, CLI, WASM, and language packages all share
the same version number and release together. No independent versioning.

### 9. `syntaqlite_dialect.h` has no cross-version compatibility

**Decision:** Dialect code must be compiled against the exact same version of
`syntaqlite_parser.c` it will link with. No ABI stability promise across
releases. When syntaqlite updates, dialect authors re-run codegen and recompile.

This is acceptable because:
- Dialect code is generated by `syntaqlite codegen`, not hand-written
- Re-running codegen is trivial
- An unstable ext ABI lets us evolve the dialect interface freely

### 10. Crate structure: two crates

**Decision:** Two workspace crates with a one-way dependency:

- `syntaqlite-sys`: all C code (core engine + SQLite dialect)
- `syntaqlite`: all Rust code + C API (feature-gated) + WASM target

The C API (`capi` feature) and WASM target both live in `syntaqlite` rather than
separate crates. `syntaqlite` uses multiple crate types
(`rlib` + `cdylib` + `staticlib`). `cargo check`/`clippy` has zero overhead from
the extra crate types (no linking). Downstream Rust crates that depend on
`syntaqlite` only build the `rlib`.

### 11. Dialects are always out-of-tree

**Decision:** Dialect crates live in their own repos. They depend on
`syntaqlite-sys` for C headers/ext SPI and `syntaqlite` for Rust types.
The `syntaqlite_dialect.h` header is small and could even be auto-generated into
dialect crates by a build dependency if needed.

## Open Questions

### 1. Version mismatch detection in `no-bundled-parser` mode

In mode 3 (external parser), the library and amalgamation must be the same
version. If they're not, you get silent ABI mismatches: the Rust code expects
one struct layout, the C parser provides another.

The sentinel symbol from mode 1 catches the *presence* of both, but in mode 3
we explicitly want both. We need to catch *version mismatch* instead.

**Decision: both link-time and runtime checks.**

**Link-time:** version-encoded symbol name:

```c
// Both the library and amalgamation define:
const char syntaqlite_version_1_2_3_ __attribute__((used)) = 1;
```

If versions differ, you get a *missing* symbol error (the library wants
`syntaqlite_version_1_2_3_` but the amalgamation provides
`syntaqlite_version_1_1_0_`). Ugly error message, but it fails at link time.

**Runtime:** version check at init:

```c
// Amalgamation exports:
int syntaqlite_parser_version(void) { return 10203; } // 1.2.3

// Library checks at engine_new():
assert(syntaqlite_parser_version() == SYNTAQLITE_EXPECTED_VERSION);
```

Clean error message as safety net for cases where link-time detection slips
through (dynamic linking, dlopen).

**Open sub-question:** how to reliably generate the versioned symbol, especially
for catching drift from code built off `main` (not just releases). Needs more
thought: could embed git SHA or build timestamp, but that may be too strict
for development workflows.
