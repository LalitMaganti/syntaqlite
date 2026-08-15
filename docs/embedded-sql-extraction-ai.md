# Embedded SQL Extraction: Language Analysis

Status: Research notes
Date: 2026-03-01

## Key Finding

String extraction complexity is concentrated in Python and JS/TS. Most target languages (Go, Rust, Java, C++, Kotlin, Swift) have no interpolation inside string literals or only simple interpolation: making extraction trivial.

## Per-Language Analysis

| Language | SQL string pattern | Interpolation | Holes in string? | Extraction difficulty |
|----------|-------------------|---------------|-------------------|----------------------|
| Python | `f"SELECT ... {x}"` | `{}` with arbitrary exprs | Yes | Medium (brace matching, nested exprs, dict literals, strings inside holes) |
| JS/TS | `` sql`SELECT ... ${x}` `` | `${}` with arbitrary exprs | Yes | Easy if using tsserver; medium otherwise |
| Go | `"SELECT ... " + x` | None | No | Trivial (find string literals) |
| Rust | `format!("SELECT ... {}", x)` / `sqlx::query!("...")` | None in the string | No | Trivial (find string literals) |
| Java | `"SELECT ... " + x` | None (text blocks are literal) | No | Trivial (find string literals) |
| Kotlin | `"SELECT ... $x"` / `"${expr}"` | `$` / `${}` | Yes | Easy (simple brace matching) |
| Swift | `"SELECT ... \(x)"` | `\()` | Yes | Easy (paren matching) |
| C++ | `"SELECT ... "` | None | No | Trivial (find string literals) |

## Extraction Strategy

### Default: server-side extraction in Rust

For most languages, a lightweight Rust string-literal finder (50–100 lines per language) is sufficient. No external parser dependency needed: just match quote characters, handle escapes, and optionally match simple interpolation delimiters.

This covers: Go, Rust, Java, C++, Kotlin, Swift, and Python.

### Exception: JS/TS can use client-side extraction via TS plugin

TypeScript's own language service already has perfect AST knowledge of template literals. Microsoft's `typescript-template-language-service-decorator` library provides:
- AST traversal to find tagged template literals matching configured tag names
- Coordinate mapping between template-local offsets and file-absolute positions
- `${expr}` substitution handling with offset preservation
- Proxying of LanguageService methods

A thin TS plugin (about 50 lines of glue) can send `EmbeddedSqlFragment` data to syntaqlite, avoiding any JS/TS parsing in Rust. The CLI (`syntaqlite validate file.js`) would still need a simple server-side fallback, but tokenizing template literals is straightforward: match `` ` `` and `${`/`}`.

### The unified interface

Regardless of where extraction happens, the contract is the same:

```rust
pub struct EmbeddedFragment { /* private fields */ }

impl EmbeddedFragment {
    pub fn sql_range(&self) -> &Range<usize>;  // byte range of SQL in host file
    pub fn sql_text(&self) -> &str;            // SQL text with holes replaced
    pub fn holes(&self) -> &[Hole];            // interpolation holes
}

pub struct Hole { /* private fields */ }

impl Hole {
    pub fn host_range(&self) -> &Range<usize>; // byte range in host file
    pub fn sql_offset(&self) -> usize;         // offset within sql_text
}
```

It doesn't matter whether this struct was built by Rust code inside the server or by a TS plugin sending it over LSP.

## Rust Library Options for Server-Side Extraction

### For Python f-strings

| Option | Published | License | Notes |
|--------|-----------|---------|-------|
| Hand-rolled (~150 lines) | N/A | N/A | Brace matching with edge cases: nested `{}`, strings inside holes, `{{`/`}}` escapes, triple-quoted strings |
| `tree-sitter` + `tree-sitter-python` | Yes | MIT | CST gives `string` → `interpolation` children with byte ranges. Handles all edge cases including PEP 701 nested f-strings. Compiles C grammar via `cc` |
| `rustpython-parser` | Yes (v0.4) | MIT | Full parser, gives `ExprJoinedStr` / `FormattedValue` AST with source ranges. Frozen pre-PEP 701 (no Python 3.12 nested f-strings). Medium weight (LALRPOP, bigint) |
| Ruff's `ruff_python_parser` | **Not published** | MIT | Best Python parser in Rust, fully implements PEP 701. Would require vendoring from ruff monorepo |

### For JS/TS (CLI fallback / non-editor use)

| Option | Published | License | Notes |
|--------|-----------|---------|-------|
| Hand-rolled (~100 lines) | N/A | N/A | Simple: match `` ` ``, find `${`, track brace depth to `}` |
| `tree-sitter` + `tree-sitter-javascript` | Yes | MIT | CST gives `template_string` → `template_substitution` children. Shares runtime with tree-sitter-python |
| `oxc_parser` | Yes | MIT | Fast, all-Rust, arena-allocated. `TemplateLiteral` AST with interleaved quasis + expressions |
| `swc_ecma_parser` | Yes | Apache-2.0 | Most downloaded, but heavy (macro-heavy AST, globals system) |

### Recommendation

For Python, start with a hand-rolled extractor. F-string brace matching is well-scoped and avoids adding a parser dependency. Fall back to tree-sitter-python later only if edge cases (PEP 701 nested f-strings) prove painful.

For JS/TS, use the TS plugin for editor contexts. For CLI, a hand-rolled template literal tokenizer is straightforward: `${}` hole detection is simpler than Python's `{}` (no ambiguity with dict literals).

Tree-sitter is the insurance policy: if hand-rolled extractors accumulate too many edge cases, swap to tree-sitter for both languages with one shared runtime dependency.

## Formatting Embedded SQL

### The problem

Formatting SQL inside host language strings requires three steps:
1. **Format the SQL**: the existing formatter handles this, with `begin_macro`/`end_macro` emitting holes verbatim
2. **Re-embed**: splice the formatted SQL back into the host string, handling indentation, quoting, and escapes
3. **Coexist**: don't fight the user's existing host language formatter (Black, Ruff, Prettier)

### Re-embedding challenges

**Indentation context.** The SQL lives inside an indented host string. Formatted output must align to the string's column:

```python
# Before
def get_users():
    query = f"SELECT * FROM {table} WHERE id = {user_id} AND active = 1"

# After: lines aligned to string indentation
def get_users():
    query = f"""
        SELECT
            *
        FROM
            {table}
        WHERE
            id = {user_id}
            AND active = 1
    """
```

**String delimiter transformation.** A single-line string may need to become multi-line after formatting: `f"..."` → `f"""..."""` (Python), while JS template literals are already multi-line.

**Escape sequences.** The formatter output must be re-escaped for the host string type. Triple-quoted strings need fewer escapes than single-quoted ones.

### Editor integration: who calls the formatter?

There is no LSP primitive for embedded language formatting. Projects that format embedded languages use one of two approaches:

| Approach | How it works | Examples |
|----------|-------------|----------|
| **Formatter owns everything** | One formatter handles the whole file, calling sub-formatters for embedded regions internally | Prettier (HTML → CSS/JS), VS Code HTML extension |
| **Virtual document projection** | Language server creates virtual documents per embedded region, dispatches formatting to other servers | Volar (Vue), Astro language server |

syntaqlite owns the SQL formatting step, but it cannot own the host Python or JavaScript file. The remaining question is whether the host formatter can delegate SQL formatting to syntaqlite.

### Delegation from host formatters

| Formatter | Can delegate? | Mechanism |
|-----------|--------------|-----------|
| **Prettier** | **Yes** | Plugins define `embed()` function for embedded languages. First-class feature. |
| **dprint** | **Yes** | Plugin system supports embedded formatting |
| **Black** | No | Doesn't touch string contents, no plugin hook |
| **Ruff** | No | Same as Black |

For **JS/TS**, a Prettier plugin is the natural distribution path: ~50 lines of glue, users already have Prettier, and Prettier handles all re-embedding (indentation, template literal syntax).

For **Python**, no formatter will delegate. The only options are the syntaqlite **CLI** (`syntaqlite fmt --lang python`) or an **LSP Code Action** ("Format embedded SQL").

## Distribution Strategy: TypeScript vs. Everything Else

### Key insight

TypeScript has a rich plugin ecosystem (TS Language Service Plugins, Prettier plugins) that runs inside the user's existing toolchain. Other languages (Python, Go, Rust, etc.) don't have equivalents and therefore need a standalone LSP server or CLI.

This means two different distribution strategies:

### TypeScript/JavaScript: embed into existing toolchain

```
@syntaqlite/wasm                  ← the engine (parser, formatter, validator)
     │
     ├── @syntaqlite/ts-plugin     ← TS Language Service Plugin
     │     runs inside tsserver
     │     provides: validation, completions, diagnostics
     │     works in: VS Code, Neovim, JetBrains: anywhere tsserver runs
     │
     └── prettier-plugin-syntaqlite ← Prettier plugin
           runs inside prettier
           provides: formatting of SQL in tagged template literals
           coexists with user's existing Prettier config
```

Both packages call into the WASM build. The TS plugin handles validation + completions. The Prettier plugin handles formatting. Users install both and get the full experience with zero extra processes.

**TS Language Service Plugin advantages:**
- Runs inside tsserver: zero extra processes
- Sees tagged template literals with full type information
- Diagnostics appear as native TypeScript errors
- Works in any editor that uses tsserver (VS Code, Neovim, JetBrains, etc.)

### Python/Go/Rust/everything else: standalone LSP + CLI

```
syntaqlite LSP server             ← standalone server process
     provides: validation, completions, diagnostics, Code Actions
     server-side string extraction (Rust)

syntaqlite CLI
     provides: validation, formatting
     syntaqlite validate myapp.py
     syntaqlite fmt --lang python myapp.py
     usable in: pre-commit hooks, CI, editor save hooks
```

These languages have no equivalent of tsserver plugins. The syntaqlite LSP server handles everything: string extraction, parsing, validation, formatting (via Code Actions).

### Summary

| Concern | JS/TS path | Python/other path |
|---------|-----------|-------------------|
| **Validation** | `@syntaqlite/ts-plugin` (inside tsserver) | syntaqlite LSP server |
| **Formatting** | `prettier-plugin-syntaqlite` (inside Prettier) | CLI (`syntaqlite fmt --lang python`) or LSP Code Action |
| **Completions** | `@syntaqlite/ts-plugin` (inside tsserver) | syntaqlite LSP server |
| **CI/pre-commit** | CLI fallback | CLI |

The LSP server should focus on non-TS languages. The TS packages are the primary path for JS/TS users.
