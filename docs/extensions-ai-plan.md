# Extension / Dialect Support Plan

## Overview

syntaqlite needs to support SQLite dialects — databases that use SQLite as a base and add extensions (libSQL, Perfetto, rqlite, etc.). This document describes the architecture for making the parser, formatter, linter, and LSP work with arbitrary grammar extensions while keeping the core runtime grammar-agnostic.

## Design Principles

1. **Runtime is generic.** The parser engine, formatter interpreter, lint engine, and LSP framework never know about specific node types or tokens. They work with opaque tags and descriptor tables.
2. **Dialects are data.** A dialect provides parser tables, token/node definitions, keyword hashes, formatter bytecode, and dialect-specific logic (lint rules, etc.). The runtime consumes this data.
3. **Codegen produces only what's dialect-specific.** The runtime is compiled once, shared by all dialects. Codegen generates the minimal set of artifacts a dialect needs.
4. **C-first with Rust bindings.** The parser layer works for pure C projects. Rust adds type safety and ergonomic APIs on top.
5. **Out-of-tree dialects are first-class.** A third-party dialect author can build their own crate without forking syntaqlite.

## Crate Structure

```
syntaqlite-runtime        # Grammar-agnostic engines
syntaqlite                # SQLite dialect (generated + hand-written)
syntaqlite-codegen        # Generic dialect code generation library
syntaqlite-codegen-sqlite # SQLite extraction + orchestration library
syntaqlite-cli            # lib + bin: reusable CLI framework + default binary
```

### syntaqlite-runtime

The runtime crate contains all grammar-agnostic machinery. It works with opaque `u32` tags and descriptor tables — never names a specific node type or token.

**C code (compiled via `build.rs`):**

- `arena.c` — arena allocator for AST nodes
- `parser_engine.c` — Lemon push-parser loop (derived from `lempar.c` template)
- `tokenizer.c` — tokenizer framework (scan loop, trivia collection, char class logic)

**Rust code:**

- `parser.rs` — generic parser driver, feeds tokens into the C engine
- `node.rs` — `FieldDescriptor`, `FieldVal`, raw node access by tag + offset
- `fmt/interpreter.rs` — bytecode interpreter (generic, dialect provides bytecode blob)
- `fmt/doc.rs` — Doc IR and renderer (Wadler-Lindig style)
- `lint/engine.rs` — rule runner, diagnostic infrastructure
- `lsp/server.rs` — LSP protocol handling, workspace management

**Rust API (data-pointer parameterized, no generics/traits):**

```rust
// Parse
pub fn parse(source: &str, tables: &ParserTables, keywords: &KeywordHash) -> ParseTree;

// Raw node access
impl ParseTree {
    pub fn node_tag(&self, id: NodeId) -> u32;
    pub fn node_fields(&self, id: NodeId, registry: &NodeRegistry) -> Fields;
}

// Format
pub fn format(tree: &ParseTree, bytecode: &[u8], config: &FmtConfig) -> String;
```

**Features:**

```toml
[features]
default = ["parser", "fmt", "lint", "lsp"]
parser = []
fmt = ["parser"]
lint = ["parser"]
lsp = ["parser", "fmt", "lint"]
```

### syntaqlite

The SQLite dialect crate. Depends on `syntaqlite-runtime`. Provides both the generated dialect data and a high-level typed API.

**Generated C code (via `syntaqlite-codegen`):**

- `sqlite_parse_tables.c` — Lemon parser state machine (`yy_action[]`, `yy_lookahead[]`, etc.)
- `sqlite_reduce.c` — reduce function (switch statement calling builder functions)
- `sqlite_keyword.c` — keyword hash tables
- `node.h` — node struct typedefs
- `ast_builder.h` — inline builder functions (one per node type)

**Generated Rust code (via `syntaqlite-codegen`):**

- `tokens.rs` — `TokenType` enum
- `nodes.rs` — node structs, `NodeTag` enum, `FieldDescriptor` arrays per node type
- `fmt.bin` — formatter bytecode blob

**Hand-written Rust code:**

- `lib.rs` — high-level API wrapping the runtime with SQLite-specific data
- `lint_rules.rs` — SQLite-specific lint rules (uses generated node types)

**High-level API example:**

```rust
// Consumers get ergonomic, typed access:
let parser = syntaqlite::Parser::new();
let session = parser.parse("SELECT 1");

// Generated typed node access:
impl SelectStmt {
    pub fn columns(&self, tree: &Parser) -> NodeId { ... }
    pub fn where_clause(&self, tree: &Parser) -> Option<NodeId> { ... }
}
```

**Features (mirror the runtime):**

```toml
[features]
default = ["parser", "fmt", "lint"]
parser = ["syntaqlite-runtime/parser"]
fmt = ["parser", "syntaqlite-runtime/fmt"]
lint = ["parser", "syntaqlite-runtime/lint"]
```

### syntaqlite-codegen + syntaqlite-codegen-sqlite

`syntaqlite-codegen` is the generic dialect code generation library: `.synq` parsing, AST model, C/Rust code generation. It has no SQLite-specific knowledge.

`syntaqlite-codegen-sqlite` handles SQLite-specific extraction and orchestration: tokenizer/keyword extraction from SQLite C sources, lemon/mkkeyword tools, and the pipeline that wires SQLite extraction into generic dialect codegen.

For extension dialects, codegen merges the base SQLite grammar with extension grammar files and generates a complete set of artifacts (not a delta — Lemon produces monolithic tables).

### syntaqlite-cli

A library + binary crate. The library provides a reusable CLI framework (subcommands for parse, format, lint, etc.) parameterized by dialect. The binary is the default SQLite CLI.

Dialect authors can build their own CLI by depending on the library:

```rust
// In a custom dialect's CLI:
fn main() {
    syntaqlite_cli::run::<MyDialect>();
}
```

Or codegen can scaffold a complete CLI crate as part of dialect generation.

## Dependency Graph

```
End user / application
  └─ syntaqlite (or custom dialect crate)
       └─ syntaqlite-runtime (activated transitively via features)

Dialect author
  └─ syntaqlite-codegen-sqlite (build-time tool)
       └─ syntaqlite-codegen (generic dialect codegen library)
       → generates dialect crate that depends on syntaqlite-runtime
```

For the default SQLite case:

```
syntaqlite-cli
  └─ syntaqlite (features = ["parser", "fmt", "lint"])
       └─ syntaqlite-runtime (features = ["parser", "fmt", "lint"])
```

For a custom dialect:

```
libsql-cli
  └─ libsql-syntax (features = ["parser", "fmt"])
       └─ syntaqlite-runtime (features = ["parser", "fmt"])
```

Note: `libsql-syntax` depends on `syntaqlite-runtime`, NOT on `syntaqlite`. Each dialect is independent.

## Extension Grammar System

### What an extension provides

An extension is a directory mirroring the base structure:

```
my-dialect/
  actions/
    my_new_stmts.y          # Lemon grammar rules with AST-building actions
  nodes/
    my_new_stmts.synq        # Node definitions + formatter DSL
```

### Grammar merge strategy

Extension grammar files are merged with the base SQLite grammar:

1. Concatenate base `.y` files (from `syntaqlite-syntax/parser-actions/`)
2. Append extension `.y` files after base grammar
3. Extension `%token` declarations are placed after base rules to ensure base tokens get IDs first
4. Run Lemon on the merged grammar → monolithic parser tables + reduce function

**Token ID stability:** Base token IDs are stable regardless of extensions because base terminals appear first in the grammar and get IDs by order of first appearance. A test verifies this property (compare base-only vs. merged token IDs).

**Modifying existing rules:** Extensions can add new alternatives to existing nonterminals (e.g., `expr ::= expr ARROW expr`). This works naturally — Lemon just gets more rules for that nonterminal. The parser tables are regenerated from scratch, and the new node type flows through the existing nonterminal.

### What codegen produces

Given base grammar + extension grammar + extension nodes, codegen generates:

**C outputs:**

- Parser tables (complete, not a delta — includes base + extension)
- Reduce function (complete — handles all rules)
- Keyword hash (regenerated with base + extension keywords)
- Node structs + builders (base + extension nodes)

**Rust outputs:**

- `TokenType` enum (base + extension tokens)
- `NodeTag` enum (base + extension tags)
- Node structs + `FieldDescriptor` arrays (base + extension)
- Formatter bytecode (base + extension formatting rules)

### Extension levels

1. **Keywords only:** New `%token` declarations, no grammar rules. Regenerates keyword hash. Useful for dialects that want new keywords recognized by the tokenizer without changing the parser.

2. **Keywords + syntax:** New tokens + grammar rules referencing base nonterminals. Generates full parser tables + reduce function. Example: Perfetto adds `CREATE PERFETTO FUNCTION ...` as new `cmd` rules.

3. **Full AST support:** Levels 1-2 + custom node definitions (`.synq` files) with formatter DSL. Generates typed node access, formatter bytecode, etc.

## Pure C Distribution

### Amalgamation

For pure C consumers, codegen produces a single-file amalgamation:

```bash
syntaqlite amalgamate \
  --output syntaqlite.c syntaqlite.h
```

The amalgamation includes both runtime engine code and dialect-specific tables:

```c
// === syntaqlite.c ===

// --- Runtime engine (from syntaqlite-runtime) ---
// arena, parser engine, tokenizer framework

// --- Dialect tables (swappable via ifdef) ---
#ifdef SYNTAQLITE_DIALECT
#include SYNTAQLITE_DIALECT
#else
// Default: SQLite tables, reduce function, keyword hash, node definitions
#endif
```

### Using a custom dialect in C

1. Run codegen with extension grammar → produces `my_dialect.h`
2. Compile: `cc -c syntaqlite.c -DSYNTAQLITE_DIALECT=\"my_dialect.h\"`

The dialect header is a **complete replacement** of the dialect section (not a delta). This is necessary because Lemon produces monolithic parser tables — adding a grammar rule changes the entire state machine.

### C API

The C API mirrors the runtime's data-pointer approach:

```c
// Runtime (grammar-agnostic):
typedef struct SynqParseCtx SynqParseCtx;
SynqParseCtx* synq_parse_new(const SynqParserConfig* config);
void synq_parse_feed(SynqParseCtx* ctx, SynqToken token);
uint32_t synq_parse_finish(SynqParseCtx* ctx);
void synq_parse_free(SynqParseCtx* ctx);
uint32_t synq_node_tag(SynqParseCtx* ctx, uint32_t node_id);

// Dialect-specific (generated, in the ifdef-able section):
// Node struct typedefs, builder functions, keyword tables
```

## Building a Custom Dialect

### For Rust consumers

```bash
# 1. Generate dialect crate
syntaqlite codegen \
  --base-grammar third_party/sqlite/parse.y \
  --base-actions syntaqlite-syntax/parser-actions/ \
  --base-nodes syntaqlite-syntax/parser-nodes/ \
  --ext-actions my-dialect/actions/ \
  --ext-nodes my-dialect/nodes/ \
  --output-crate my-dialect-syntax/

# 2. Build
cd my-dialect-syntax && cargo build
```

The generated crate depends on `syntaqlite-runtime` and provides the same API shape as `syntaqlite` but with the extended grammar.

### For C consumers

```bash
# 1. Generate dialect header
syntaqlite amalgamate \
  --ext-actions my-dialect/actions/ \
  --ext-nodes my-dialect/nodes/ \
  --output-dialect my_dialect.h

# 2. Compile with the base amalgamation
cc -c syntaqlite.c -DSYNTAQLITE_DIALECT=\"my_dialect.h\"
```

### Example: Perfetto extension

```
perfetto-syntax/
  actions/
    perfetto_stmts.y          # CREATE PERFETTO FUNCTION, INCLUDE PERFETTO MODULE, etc.
  nodes/
    perfetto_stmts.synq       # PerfettoFunctionStmt, PerfettoTableStmt, etc.
```

Grammar file (`perfetto_stmts.y`):

```lemon
%token PERFETTO MACRO INCLUDE MODULE RETURNS FUNCTION DELEGATES.
%fallback ID FUNCTION MODULE PERFETTO.

cmd(A) ::= CREATE or_replace(R) PERFETTO FUNCTION ID(N) LP ... { ... }
cmd(A) ::= INCLUDE PERFETTO MODULE ID(M) DOT ID(N). { ... }
```

Node definition (`perfetto_stmts.synq`):

```
node PerfettoFunctionStmt {
    func_name: inline TextSpan
    body: index Stmt
    is_replace: inline Bool
    fmt {
        Text("CREATE")
        IfSet(is_replace) { Text("OR REPLACE") }
        Text("PERFETTO FUNCTION")
        Span(func_name)
        ...
    }
}
```

## Migration Path

The current codebase has everything in a handful of crates. The migration to the runtime/dialect split is:

1. **Extract the runtime.** Move grammar-agnostic C code (arena, parser engine, tokenizer framework) and Rust code (generic parser driver, bytecode interpreter, doc renderer) into `syntaqlite-runtime`.

2. **Reshape `syntaqlite` as a dialect crate.** The remaining code — generated parser tables, token/node definitions, keyword hash, formatter bytecode — becomes the SQLite dialect crate. Add the high-level typed API.

3. **Update codegen.** Teach `syntaqlite-codegen`/`syntaqlite-codegen-sqlite` to accept extension grammar/node directories and produce merged output. Add amalgamation generation.

4. **Validate with Perfetto.** The existing Perfetto test extension (in `tests/extensions/`) is the first real dialect to build against the new system.

5. **CLI as lib + bin.** Extract the CLI framework into a library so dialect authors can reuse it.

## Open Questions

- **Tokenizer boundary.** How much of the tokenizer is runtime vs. dialect-specific? The scan loop and trivia collection are generic, but `getToken()` itself has SQLite-specific rules (string literals, hex integers, etc.). Dialects might need custom tokenizer behavior (e.g., `#` comments). Need to define the extension point.

- **Lint rule distribution.** Some lint rules are generic (e.g., "unused alias"), some are dialect-specific (e.g., "SQLite < 3.39 doesn't support RIGHT JOIN"). Where do generic rules live — in the runtime or in `syntaqlite`?

- **Node ID allocation convention.** Codegen assigns node tags sequentially across base + extension. Should we guarantee base tags are always 1–N and extension tags start at N+1? This would let code distinguish base vs. extension nodes without knowing the specific dialect.

- **Shared library / plugin story.** Currently out of scope. The Rust-native answer is "make your own crate." A future plugin system (CLI loads `.so` at runtime) could be added later but isn't necessary for the initial implementation.
