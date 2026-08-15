# LSP Implementation Plan

Status: Phase 1 implemented
Last updated: 2026-02-21

## Goal

Add language server capabilities to syntaqlite, usable from:
1. **VSCode** (and any LSP-capable editor): via `syntaqlite lsp` subcommand over stdio
2. **Monaco in the browser**: via typed WASM exports in the existing web playground

Both targets share the same analysis engine. The native path speaks LSP/JSON-RPC; the browser path uses direct function calls through Monaco's provider API.

## Architecture

```
                     ┌──────────────────────┐
                     │   syntaqlite-lsp     │  pure Rust, no IO
                     │   AnalysisHost       │
                     │   AmbientContext     │
                     └───────┬─────────┬────┘
                             │         │
                ┌────────────┘         └────────────┐
                ▼                                    ▼
      ┌──────────────────┐              ┌──────────────────────┐
      │ syntaqlite-cli   │              │ syntaqlite-wasm      │
      │ `lsp` subcommand │              │ wasm_diagnostics     │
      │ lsp-server/stdio │              │ → Monaco providers   │
      └──────────────────┘              └──────────────────────┘
```

### `syntaqlite-lsp` crate

Workspace member (`syntaqlite-lsp/`). Depends on `syntaqlite-runtime` with `fmt` feature. No async, no IO, no transport concerns: pure computation.

Files:
- `src/lib.rs`: re-exports `AnalysisHost`, `FormatError`, `AmbientContext`, `Diagnostic`, `Severity`
- `src/host.rs`: `AnalysisHost`, `FormatError`, diagnostics computation
- `src/context.rs`: `AmbientContext` and schema types
- `src/types.rs`: `Diagnostic`, `Severity`

#### Core types

```rust
pub struct AnalysisHost<'d> {
    dialect: Dialect<'d>,
    documents: HashMap<String, Document>,
    context: Option<AmbientContext>,
}

struct Document {
    version: i32,
    source: String,
    state: Option<DocumentState>,
}

struct DocumentState {
    diagnostics: Vec<Diagnostic>,
}
```

#### Public API

```rust
impl<'d> AnalysisHost<'d> {
    pub fn new(dialect: Dialect<'d>) -> Self;

    // Ambient context
    pub fn set_ambient_context(&mut self, ctx: AmbientContext);
    pub fn ambient_context(&self) -> Option<&AmbientContext>;

    // Document lifecycle
    pub fn open_document(&mut self, uri: &str, version: i32, text: String);
    pub fn update_document(&mut self, uri: &str, version: i32, text: String);
    pub fn close_document(&mut self, uri: &str);

    // Queries
    pub fn diagnostics(&mut self, uri: &str) -> &[Diagnostic];
    pub fn format(&self, uri: &str, config: &FormatConfig) -> Result<String, FormatError>;
    pub fn document_source(&self, uri: &str) -> Option<&str>;
}
```

- `diagnostics()` lazily parses on first call after a change, caching the result in `DocumentState`
- `update_document()` clears cached state, so the next `diagnostics()` call re-parses
- `format()` creates a temporary `Formatter` per call (the host does not keep a long-lived parser for formatting)
- `document_source()` exposes the stored source text for offset→position conversion in the LSP layer

#### Result types

```rust
pub struct Diagnostic { /* private fields */ }

impl Diagnostic {
    pub fn start_offset(&self) -> usize;
    pub fn end_offset(&self) -> usize;
    pub fn message(&self) -> &DiagnosticMessage;
    pub fn severity(&self) -> Severity;
    pub fn help(&self) -> Option<&Help>;
}

pub enum Severity { Error, Warning, Info, Hint }

pub enum FormatError {
    UnknownDocument,
    Setup(&'static str),    // dialect has no fmt data
    Parse(ParseError),      // parse error during formatting
}
```

#### Error span → diagnostic range mapping

`compute_diagnostics()` maps `ParseError` spans to `Diagnostic` ranges with this fallback logic:

| `offset` | `length` | Diagnostic range |
|----------|----------|------------------|
| `Some(o)` | `Some(l)`, `l > 0` | `[o, o + l)` |
| `Some(o)` | `None` or `0` | `[o, o + 1)` (clamped to source length; if at end, highlights last char) |
| `None` | any | `[len - 1, len)` (last char of source, or `[0, 0)` if empty) |

### Ambient Context

The `AnalysisHost` accepts an optional `AmbientContext`, which represents the database schema against which queries are analyzed. Callers populate it however they want (introspecting a live SQLite DB, parsing CREATE statements, loading from a config file, etc.). The context is global to the host and shared by all open documents.

```rust
pub struct AmbientContext {
    pub tables: Vec<TableDef>,
    pub views: Vec<ViewDef>,
    pub functions: Vec<FunctionDef>,
}

pub struct TableDef {
    pub name: String,
    pub columns: Vec<ColumnDef>,
}

pub struct ColumnDef {
    pub name: String,
    pub type_name: Option<String>,
    pub is_primary_key: bool,
    pub is_nullable: bool,
}

pub struct ViewDef {
    pub name: String,
    pub columns: Vec<ColumnDef>,
}

pub struct FunctionDef {
    pub name: String,
    pub args: Option<usize>,  // None = variadic
    pub description: Option<String>,
}
```

**Phase usage:**
- Phase 1 (Diagnostics + Formatting): Not used: parse-level only
- Phase 3 (Completions): Suggest table/column/function names from context
- Phase 4 (Hover): Show column types, table schemas
- Phase 5 (Go-to-Definition): Resolve references against schema

### CLI integration (`syntaqlite lsp`)

New subcommand in `syntaqlite-cli`. File: `src/lsp.rs`.

Dependencies: `lsp-server` (sync transport, used by rust-analyzer), `lsp-types` 0.97, `serde_json`.

Advertised capabilities:
- `text_document_sync`: `Full` (entire document sent on every change)
- `document_formatting_provider`: `true`

Message handling:

| LSP message | Handler |
|-------------|---------|
| `textDocument/didOpen` | `host.open_document()` + push diagnostics |
| `textDocument/didChange` | `host.update_document()` (last change in array) + push diagnostics |
| `textDocument/didClose` | Push empty diagnostics to clear, then `host.close_document()` |
| `textDocument/formatting` | `host.format()` → single `TextEdit` replacing full document (range `0:0..MAX:0`) |
| `shutdown` | Handled by `lsp-server`'s `connection.handle_shutdown()` |

Offset → Position conversion: `offset_to_position()` walks the source `char_indices()`, counting `\n` for lines and `char.len_utf8()` for columns. Returns `Position { line, character }` with UTF-8 byte offsets for the character field (matching LSP's `UTF-8` position encoding when negotiated, which `lsp-server` 0.7 supports).

Diagnostics are published as `textDocument/publishDiagnostics` notifications with `source: "syntaqlite"`.

VSCode extension config (in a future `syntaqlite-vscode` package):
```json
{
  "serverOptions": { "command": "syntaqlite", "args": ["lsp"] },
  "documentSelector": [{ "language": "sql" }]
}
```

### Monaco/WASM integration

New export in `syntaqlite-wasm/src/main.rs`:

```rust
#[unsafe(no_mangle)]
pub extern "C" fn wasm_diagnostics(ptr: u32, len: u32) -> i32;
```

Returns the number of diagnostics (>= 0) on success, or -1 on error. Result is serialized into `RESULT_BUF` as a JSON array.

Implementation creates a temporary `AnalysisHost` per call (stateless: Monaco holds the document).

JSON output format:
```json
[
  {
    "startOffset": 0,
    "endOffset": 5,
    "message": "syntax error near 'SELEC'",
    "severity": "error"
  }
]
```

Fields:
- `startOffset` / `endOffset`: byte offsets into the input
- `message`: human-readable error string
- `severity`: one of `"error"`, `"warning"`, `"info"`, `"hint"`

TypeScript usage:
```ts
const count = engine.diagnostics(model.getValue());
if (count > 0) {
    const diags = JSON.parse(engine.getResult());
    monaco.editor.setModelMarkers(model, 'syntaqlite', diags.map(d => ({
        startLineNumber: ..., // convert from byte offset
        message: d.message,
        severity: monaco.MarkerSeverity.Error,
    })));
}
```

## Prerequisites / Parser Improvements

### Error spans in ParseError ✅

`ParseError` now carries `offset: Option<usize>` and `length: Option<usize>`.

**C side** (`syntaqlite-runtime/`):

| File | Change |
|------|--------|
| `include/syntaqlite_dialect/ast_builder.h` | Added `uint32_t error_length` to `SynqParseCtx` |
| `include/syntaqlite/parser.h` | Added `uint32_t error_offset` and `uint32_t error_length` to `SyntaqliteParseResult` |
| `csrc/parser.c`: `feed_one_token()` | Sets `ctx.error_offset = tok.z - source`, `ctx.error_length = tok.n` on error |
| `csrc/parser.c`: `finish_input()` | Sets `error_offset = p->offset` (end of input), `error_length = 0` for incomplete statements |
| `csrc/parser.c`: `syntaqlite_parser_next()` | Propagates `ctx.error_offset`/`error_length` into `ParseResult` on all error paths |
| `csrc/parser.c`: `syntaqlite_parser_result()` | Same propagation for low-level API |
| `csrc/parser.c`: `syntaqlite_parser_reset()` | Initializes `error_offset = 0xFFFFFFFF`, `error_length = 0` |

Sentinel: `error_offset == 0xFFFFFFFF` means "unknown offset". Zero-length means "unknown length" (point diagnostic).

**Rust side**:

| File | Change |
|------|--------|
| `src/parser/ffi.rs` | Added `error_offset: u32`, `error_length: u32` to `ParseResult` |
| `src/parser/parser.rs` | `ParseError` gains `offset: Option<usize>`, `length: Option<usize>` |
| `src/parser/parser.rs`: `StatementCursor::next_statement()` | Converts sentinel → `None`, populates fields |
| `src/parser/token_parser.rs`: `LowLevelCursor::finish()` | Same conversion for low-level API |

### Token collection

The parser's existing `collect_tokens: true` option records every token with its type and span, which provides the data needed for semantic tokens. `AnalysisHost` only needs to enable the option when necessary; Phase 1 uses `collect_tokens: false` for diagnostics-only parsing.

## Feature Phases

### Phase 1: Diagnostics + Formatting ✅

**Diagnostics:**
- Parse each document on change via `Parser::new()` (no token collection needed)
- Map `ParseError` spans to `Diagnostic` structs with byte offset ranges
- In LSP: push via `textDocument/publishDiagnostics` after every `didOpen`/`didChange`
- In WASM: `wasm_diagnostics()` → JSON array in result buffer

**Formatting:**
- Uses `Formatter::with_config()` from `syntaqlite-runtime` (creates a fresh formatter per call)
- In LSP: `textDocument/formatting` → single `TextEdit` replacing full document
- In WASM: existing `wasm_fmt()` export continues to work; `AnalysisHost::format()` available for future integration

### Phase 2: Semantic Tokens

- Use `collect_tokens` to get token stream with types
- Map token types to LSP semantic token types (keyword, string, number, comment, identifier, operator)
- Better highlighting than regex-based TextMate grammars: the real tokenizer knows about SQLite-specific tokens
- `textDocument/semanticTokens/full` in LSP
- `monaco.languages.registerDocumentSemanticTokensProvider` in Monaco

### Phase 3: Keyword Completions

- Build a static keyword list from the dialect's token table
- Context-aware: at minimum, filter by "beginning of statement" vs "mid-expression" using simple heuristics (look at preceding tokens)
- Include SQLite built-in function names
- Use ambient context to suggest table/column/function names
- Future: extract valid-next-token from Lemon parser state for grammar-accurate completions

### Phase 4: Hover

- Keyword hover: show brief documentation for SQL keywords (e.g., "SELECT: Retrieves rows from one or more tables")
- Function hover: show SQLite built-in function signatures and descriptions
- Table/column hover from ambient context: show column types, table schemas
- Static data, hand-curated or extracted from SQLite docs

### Phase 5: Go-to-Definition / References (future)

- Requires scope analysis: tracking CTE names, table aliases, column aliases
- Build a symbol table per statement during parse
- Use ambient context to resolve references against schema
- `textDocument/definition`, `textDocument/references`
- This is significantly more work and deferred past initial release

## Crate Dependency Graph

```
syntaqlite-lsp
  └── syntaqlite-runtime (features = ["fmt"])

syntaqlite-cli
  ├── syntaqlite-lsp
  ├── syntaqlite-runtime (features = ["fmt"])
  ├── syntaqlite (for default dialect)
  ├── lsp-server 0.7  (sync LSP transport)
  ├── lsp-types 0.97
  └── serde_json 1

syntaqlite-wasm
  ├── syntaqlite-lsp
  └── syntaqlite-runtime (features = ["fmt"])
```

## Open Questions

1. **Incremental re-parse?** Full re-parse on every keystroke for V0. SQL files are typically small. If perf becomes an issue with very large files, incremental can be added later (the parser would need to support partial re-parse, which is a larger change).

2. **VSCode extension packaging?** Out of scope for this plan. The extension is a thin `vscode-languageclient` wrapper. Can be a separate repo or a `vscode/` directory in this repo.

3. **Position encoding?** Current `offset_to_position()` counts UTF-8 bytes for the character field. LSP specifies UTF-16 by default but supports UTF-8 when negotiated. The `lsp-server` 0.7 / `lsp-types` 0.97 stack supports position encoding negotiation. For now, the implementation uses UTF-8 byte counts. This works correctly for ASCII SQL but may need adjustment for non-ASCII identifiers or string literals.

## Notes

- The `lsp` subcommand reuses the same dialect loading path as `ast`/`fmt`: `--dialect` flag works the same way.
- No new codegen needed. `syntaqlite-lsp` works purely at the runtime level.
- The WASM path doesn't need a document store because Monaco manages documents. Each WASM call gets full text: stateless from the WASM side (though we could add caching later).
- `lsp-server` chosen over `tower-lsp`: sync is fine for a single-file SQL tool, and it keeps deps minimal (no tokio needed).
- Multi-file schema awareness is handled via the ambient context API: callers populate it from whatever source they want, rather than the LSP scanning files.
- `ParseError` changes are fully backward-compatible: the new `offset`/`length` fields are `Option` types, so existing code constructing `ParseError` just needs to add `offset: None, length: None`.
