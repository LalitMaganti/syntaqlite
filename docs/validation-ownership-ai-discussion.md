# Validation Ownership & Efficiency Redesign

## Status

The codebase is mid-refactoring. The file reorganization (splitting `types.rs`, renaming types, updating imports) is mostly complete but **the build is broken** because `embedded/mod.rs` calls `self.catalog.clone()` on a `FunctionCatalog` that doesn't implement `Clone`. This exposed a fundamental ownership problem that needs a design decision before the refactoring can be completed.

### What's already done

- `validation/types.rs` deleted, split into:
  - `validation/diagnostics.rs`: `Diagnostic`, `DiagnosticMessage`, `Severity`, `Help`, `Diagnostic::from_parse_error`
  - `validation/schema.rs`: `DocumentSchema` (was `DocumentContext`), extraction helpers
  - `semantic/schema.rs`: `Schema` (was `SessionContext`), constructors
- `SourceContext` renamed to `DiagnosticRenderer` in `validation/render.rs`
- `AnalysisHost::validate_dialect` delegates to `Validator::validate_document`
- Free functions `validate_parse_results` / `validate_document` moved onto `Validator` as methods
- All imports updated across `lsp/`, `embedded/`, `validation/`, CLI, WASM

### What's broken

`embedded/mod.rs:396-404`:
```rust
let validator =
    crate::validation::Validator::with_catalog(dialect, self.catalog.clone(), None);
validator.validate_results(...)
```

`FunctionCatalog` doesn't implement `Clone`. Even if we added `Clone`, it would just paper over the real problem.

---

## The Problem

Three consumers need to validate SQL: `Validator`, `EmbeddedAnalyzer`, and `AnalysisHost`. All three need access to `(RawDialect, FunctionCatalog)`. The current ownership model creates two classes of problems.

### Problem 1: Ownership coupling prevents sharing

`Validator` **owns** a `FunctionCatalog`:

```rust
pub struct Validator<'d> {
    parser: RawParser<'d>,
    dialect: RawDialect<'d>,
    catalog: FunctionCatalog,  // owned
}
```

`EmbeddedAnalyzer` also **owns** a `FunctionCatalog`:

```rust
pub struct EmbeddedAnalyzer<'d> {
    dialect: RawDialect<'d>,
    catalog: FunctionCatalog,  // owned
    config: ValidationConfig,
}
```

When `EmbeddedAnalyzer` needs to call `Validator::validate_results`, it must construct a new `Validator`, which requires surrendering its catalog or cloning it. Neither option is available.

`AnalysisHost` doesn't store a `FunctionCatalog`; it rebuilds one from scratch every time `function_catalog()` is called (see Problem 2).

### Problem 2: Pervasive copying and redundant work

**`AnalysisHost::function_catalog()` rebuilds the entire catalog on every call:**

```rust
pub fn function_catalog(&self) -> FunctionCatalog {
    let config = self.dialect_config.as_ref().unwrap_or(&default);
    let mut catalog = FunctionCatalog::for_dialect(&self.dialect, config);
    if let Some(ctx) = self.context.as_ref() {
        catalog.add_session_functions(&ctx.functions);
    }
    catalog
}
```

This is called from `validate_dialect`, `completion_items`, `available_function_names`, etc. Each call:
1. Iterates all static builtin functions, filtering by config → new `Vec<&'static FunctionInfo>`
2. Iterates all dialect extensions, copying names/arities into `OwnedFunctionInfo` → new `Vec<OwnedFunctionInfo>`
3. Clones all session functions → new `Vec<SessionFunction>`

The catalog only changes when `set_session_context()` or `set_dialect_config()` is called, which is rare (typically once at startup or on config change).

**`AnalysisHost::all_diagnostics()` parses the same document twice:**

```rust
pub fn all_diagnostics(&mut self, uri: &str, config: &ValidationConfig) -> Vec<Diagnostic> {
    let mut result = self.diagnostics(uri).to_vec();   // parse #1 (via DocumentAnalysis::compute)
    result.extend(self.validate(uri, config));          // parse #2 (via validate_dialect)
    result
}
```

`DocumentAnalysis::compute` parses the document with `collect_tokens: true` for syntax highlighting and completions. `validate_dialect` then re-parses the same document from scratch for semantic validation. The parse results (AST nodes) from the first parse are thrown away: `DocumentAnalysis` only keeps diagnostics, semantic tokens, and raw token positions.

**`validate_dialect` also creates a throwaway `Validator`:**

```rust
pub fn validate_dialect<A>(&self, uri: &str, config: &ValidationConfig) -> Vec<Diagnostic> {
    let catalog = self.function_catalog();  // rebuild catalog
    let validator = Validator::with_catalog(self.dialect, catalog, self.dialect_config);  // throwaway
    // ... parse document, call validator.validate_document ...
}
```

### Problem 3: The catalog is dynamic

`FunctionCatalog` has three layers:
1. `builtins: Vec<&'static FunctionInfo>`: filtered from a static table at construction
2. `extensions: Vec<OwnedFunctionInfo>`: copied from C dialect data at construction
3. `session: Vec<SessionFunction>`: added via `add_session_functions(&mut self, ...)`

Layers 1 and 2 are immutable after construction. Layer 3 is mutable: session functions are added when the user provides a schema context. This mutation makes it non-trivial to have `Validator` borrow `&FunctionCatalog` instead of owning it, because someone needs to own the mutable catalog and its lifetime must outlive the borrower.

In `AnalysisHost`, session functions come from `self.context: Option<Schema>`, which holds `functions: Vec<SessionFunction>`. The mutation path is:
```
AnalysisHost::set_session_context(ctx) → stores Schema
AnalysisHost::function_catalog() → builds FunctionCatalog, calls add_session_functions(&ctx.functions)
```

In practice, the catalog is rebuilt from `(dialect, config, session_context)` whenever any input changes. This is full reconstruction rather than incremental mutation.

---

## Current data flow

```
                    ┌─────────────────────────────┐
                    │       AnalysisHost           │
                    │  dialect: RawDialect         │
                    │  context: Option<Schema>     │
                    │  dialect_config: Option<DC>  │
                    │  documents: HashMap<..>      │
                    └─────┬───────────┬────────────┘
                          │           │
              diagnostics()     validate_dialect()
                          │           │
                    ┌─────▼─────┐  ┌──▼──────────────┐
                    │ Document   │  │  (re-parse doc)  │
                    │ Analysis   │  │  function_catalog() ← rebuilds every time
                    │ .compute() │  │  Validator::new() ← throwaway
                    │  parse #1  │  │  validate_document()
                    └────────────┘  └──────────────────┘
```

```
                    ┌──────────────────────────────┐
                    │      EmbeddedAnalyzer         │
                    │  dialect: RawDialect          │
                    │  catalog: FunctionCatalog     │  ← owns, can't share
                    │  config: ValidationConfig     │
                    └─────────┬────────────────────┘
                              │
                   validate_fragment()
                              │
                    ┌─────────▼───────────────────┐
                    │  needs Validator to call     │
                    │  validate_results() but      │
                    │  can't give up its catalog   │
                    └─────────────────────────────┘
```

```
                    ┌──────────────────────────────┐
                    │         Validator             │
                    │  parser: RawParser            │  ← owns parser (used in validate())
                    │  dialect: RawDialect          │
                    │  catalog: FunctionCatalog     │  ← owns
                    └──────────────────────────────┘
```

---

## Design questions

### 1. Should `Validator` own or borrow `FunctionCatalog`?

**Own (current):** Simple lifetime story, but prevents sharing. Each consumer that wants to validate must either clone the catalog or construct their own `Validator`.

**Borrow `&'a FunctionCatalog`:** Validator becomes a lightweight view. But who owns the catalog? In `AnalysisHost`, the catalog is currently rebuilt each time: so it would need to be cached as a field. In `EmbeddedAnalyzer`, it's already owned.

The borrow approach would look like:

```rust
pub struct Validator<'d, 'c> {
    parser: RawParser<'d>,
    dialect: RawDialect<'d>,
    catalog: &'c FunctionCatalog,
}
```

Or alternatively, `validate_document` / `validate_results` could be free functions (or methods on a different type) that take `&FunctionCatalog` as a parameter, leaving `Validator` as a thin convenience wrapper for the "parse + validate" use case.

### 2. Should `AnalysisHost` cache the `FunctionCatalog`?

The catalog only changes when dialect config or session context changes. Caching it avoids rebuilding on every query. The invalidation points are clear:

```rust
impl AnalysisHost {
    fn invalidate_catalog(&mut self) {
        self.cached_catalog = None;
    }
    pub fn set_session_context(&mut self, ctx: Schema) {
        self.context = Some(ctx);
        self.invalidate_catalog();
    }
    pub fn set_dialect_config(&mut self, config: DialectConfig) {
        self.dialect_config = Some(config);
        self.invalidate_catalog();
    }
    fn catalog(&mut self) -> &FunctionCatalog {
        if self.cached_catalog.is_none() {
            self.cached_catalog = Some(/* build */);
        }
        self.cached_catalog.as_ref().unwrap()
    }
}
```

This makes `catalog()` take `&mut self` (for lazy init), which propagates through any methods that need the catalog. Alternatively, build eagerly at the two mutation points.

### 3. How to eliminate the double-parse in `all_diagnostics`?

Two approaches:

**A. Unified parse:** `DocumentAnalysis::compute` should store the parse results (statement IDs + reader) so `validate_dialect` can reuse them instead of re-parsing. This means `DocumentAnalysis` would need to hold onto the parser arena (which currently lives on the stack in `compute`).

**B. Unified entry point:** A single method that parses once and produces both parse diagnostics + semantic diagnostics + semantic tokens + completion tokens. This is what `Validator::validate` already does for the parse+semantic part, but `DocumentAnalysis` adds tokens and completion data.

Option A is more modular because it separates parsing from analysis. The challenge is that the parser arena has a complex lifetime (it borrows from the parser, which borrows from the source text). Storing it requires either:
- Making `DocumentAnalysis` own the parser and arena
- Storing just the data needed (stmt_ids, NodeId-based results) and having `validate_dialect` accept those as input

Option B is simpler but creates a monolithic analysis function.

A middle ground: `DocumentAnalysis::compute` already parses and collects `Result<NodeRef, ParseError>` for each statement. Store the `NodeId` results and provide them to `validate_document`:

```rust
pub struct DocumentAnalysis {
    diagnostics: Vec<Diagnostic>,
    semantic_tokens: Vec<SemanticToken>,
    tokens: Vec<CachedToken>,
    stmt_results: Vec<Result<NodeId, ParseError>>,  // NEW: reusable by validation
}
```

But `validate_document` needs a `RawParseResult` to walk the AST, and that borrows from the parser session which is dropped at the end of `compute`. So we'd need to also keep the parser session alive.

### 4. What should the core validation API look like?

Currently `Validator` bundles parser + dialect + catalog + "validate" method. The actual validation logic (`validate_document`, `validate_results`) only needs `(dialect, catalog)`: the parser is just there because `Validator::validate()` does "parse then validate" as a convenience.

Possible redesign:

```rust
/// Core validation: borrows everything, owns nothing.
pub fn validate_document<A: for<'a> AstTypes<'a>>(
    reader: RawParseResult<'_>,
    stmt_ids: &[NodeId],
    dialect: RawDialect<'_>,
    catalog: &FunctionCatalog,
    session: Option<&Schema>,
    config: &ValidationConfig,
) -> Vec<Diagnostic> { ... }

/// Convenience wrapper: parse + validate in one shot.
pub struct Validator<'d> {
    parser: RawParser<'d>,
    dialect: RawDialect<'d>,
}

impl Validator {
    pub fn validate(&mut self, source: &str, catalog: &FunctionCatalog, ...) -> Vec<Diagnostic> {
        let results = self.parser.parse(source);
        validate_document(reader, &results, self.dialect, catalog, ...)
    }
}
```

This makes `validate_document` a free function that takes `&FunctionCatalog` as a parameter. `Validator` becomes a thin wrapper around "parser + dialect" that calls it. `EmbeddedAnalyzer` calls `validate_document` directly without needing a `Validator`. `AnalysisHost` also calls `validate_document` directly with its cached catalog.

### 5. Should `EmbeddedAnalyzer` own or borrow the catalog?

If `validate_document` becomes a free function taking `&FunctionCatalog`, then `EmbeddedAnalyzer` can either own or borrow the catalog because it only passes a `&` reference to the free function.

But consider: `EmbeddedAnalyzer` is typically created once per CLI invocation or LSP request. In the CLI (`runtime.rs`), the catalog is built just before creating the analyzer:

```rust
let catalog = FunctionCatalog::for_default_dialect(&dialect);
let diags = EmbeddedAnalyzer::new(dialect)
    .with_catalog(catalog)
    .validate(&fragments);
```

If `EmbeddedAnalyzer` borrowed `&FunctionCatalog`, the caller would own the catalog:

```rust
let catalog = FunctionCatalog::for_default_dialect(&dialect);
let diags = EmbeddedAnalyzer::new(dialect, &catalog)
    .validate(&fragments);
```

Either way works. Borrowing is more flexible (the same catalog can be shared), owning is simpler for the simple case. If `validate_document` is a free function, ownership in `EmbeddedAnalyzer` becomes less consequential.

### 6. Separation of `RawParser` ownership

`Validator` currently owns a `RawParser` because `validate()` does "parse + validate". But:
- `AnalysisHost::validate_dialect` creates its own `RawParser` to parse the document
- `EmbeddedAnalyzer::validate_fragment` uses `RawIncrementalParser` (different parser type entirely)

So the parser in `Validator` is only used by the `validate(&mut self, source)` convenience method. For all other callers, the parser is wasted. This suggests the parser should be separated from the validation logic:

- `Validator::validate(source)` → "parse then validate" convenience (owns parser)
- `validate_document(reader, stmts, dialect, catalog, ...)` → core validation (no parser)
- `validate_results(reader, results, source, dialect, catalog, ...)` → results + diagnostics (no parser)

---

## Proposed direction

Make `validate_document` and `validate_results` free functions (or static methods) that borrow `&FunctionCatalog` instead of owning it. Keep `Validator` as a thin convenience for "parse + validate" with its own parser. Have `AnalysisHost` cache the `FunctionCatalog` and invalidate on config/context changes.

This resolves all three problems:
1. **Ownership coupling**: `EmbeddedAnalyzer` and `AnalysisHost` call `validate_document(&self.catalog, ...)` directly
2. **Copying**: `AnalysisHost` caches the catalog, rebuilds only on config/context change
3. **Dynamism**: The cached catalog is rebuilt (not mutated) when inputs change: `add_session_functions` is only called during cache construction

The double-parse in `all_diagnostics` is a separate optimization that can be done independently by having `DocumentAnalysis` store parse results for reuse.

---

## Files involved

| File | Current role | Change needed |
|------|-------------|---------------|
| `validation/mod.rs` | `Validator` struct, `validate_document`/`validate_results` methods | Make core validation free functions taking `&FunctionCatalog` |
| `semantic/functions/catalog.rs` | `FunctionCatalog` struct | No change needed (but consider adding `Clone` for convenience) |
| `embedded/mod.rs` | `EmbeddedAnalyzer`, broken `self.catalog.clone()` | Call free `validate_results` with `&self.catalog` |
| `lsp/host.rs` | `AnalysisHost`, rebuilds catalog every time | Cache `FunctionCatalog`, invalidate on config/context change |
| `lsp/analysis.rs` | `DocumentAnalysis::compute` | (Future) Store parse results for reuse |
