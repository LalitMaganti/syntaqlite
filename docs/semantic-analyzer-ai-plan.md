# Semantic Analyzer Redesign Plan

## Overview

The current validation/analysis architecture has fundamental ownership problems: pervasive cloning, redundant parsing, scattered responsibilities across `Validator`, `EmbeddedAnalyzer`, and `AnalysisHost`. This plan replaces all three with a unified `SemanticAnalyzer` and introduces `SemanticModel` as a cached parsed representation for efficient repeated queries.

## Design Principles

1. **One engine.** `SemanticAnalyzer` is the single entry point for all semantic analysis — diagnostics, semantic tokens, completions. No separate `Validator`, `EmbeddedAnalyzer`, or `AnalysisHost`.
2. **Callers own lifecycle.** The analyzer takes borrowed inputs (`&str`, `&DatabaseCatalog`, `&SemanticModel`). Callers decide what's long-lived vs. transient. The analyzer never forces ownership transfer or cloning.
3. **Parse once, query many.** `SemanticModel` is an opaque precomputed representation of SQL. Build it once with `prepare()`, pass it to any analysis method. Avoids re-parsing across diagnostics/tokens/completions.
4. **Simple by default, fast when needed.** The primary API takes `&str` — no setup required. The advanced API (`prepare` + `_prepared` methods) is opt-in for callers that need to avoid redundant parsing.
5. **Symmetric catalogs.** Functions and relations follow the same three-level resolution pattern: static (dialect) → database (user) → document (accumulated DDL). No asymmetric `FunctionCatalog` vs. bare relation slices.

## Public API

```rust
pub mod semantic {
    /// Analysis engine. Long-lived, reuses scratch buffers internally.
    pub struct SemanticAnalyzer<'d>;

    /// Opaque precomputed SQL representation. No public methods.
    /// Owns parser arena, source, token stream, future indexes.
    pub struct SemanticModel;

    /// What exists in the user's database. Symmetric: relations + functions.
    pub struct DatabaseCatalog;

    /// Output types.
    pub struct Diagnostic;
    pub struct SemanticToken;
    pub struct CompletionItem;

    /// Presentation utility for rendering diagnostics with source context.
    pub struct DiagnosticRenderer;
}
```

### SemanticAnalyzer

```rust
impl<'d> SemanticAnalyzer<'d> {
    pub fn new(dialect: RawDialect<'d>) -> Self;

    // ── Primary API — string in, results out ───────────────────────
    pub fn diagnostics(&mut self, source: &str, catalog: &DatabaseCatalog) -> Vec<Diagnostic>;
    pub fn semantic_tokens(&mut self, source: &str, catalog: &DatabaseCatalog) -> Vec<SemanticToken>;
    pub fn completions(&mut self, source: &str, offset: usize, catalog: &DatabaseCatalog) -> Vec<CompletionItem>;

    // ── Advanced API — prepare once, query many times ──────────────
    pub fn prepare(&mut self, source: impl Into<String>) -> SemanticModel;
    pub fn diagnostics_prepared(&mut self, model: &SemanticModel, catalog: &DatabaseCatalog) -> Vec<Diagnostic>;
    pub fn semantic_tokens_prepared(&mut self, model: &SemanticModel, catalog: &DatabaseCatalog) -> Vec<SemanticToken>;
    pub fn completions_prepared(&mut self, model: &SemanticModel, offset: usize, catalog: &DatabaseCatalog) -> Vec<CompletionItem>;

    // ── Embedded SQL — extract from host language + analyze ────────
    pub fn diagnostics_embedded<L: LanguageExtractor>(&mut self, source: &str, catalog: &DatabaseCatalog) -> Vec<Diagnostic>;
}
```

Primary API methods are thin wrappers: `prepare()` internally, then delegate to the `_prepared` variant.

### SemanticModel

Opaque struct with no public methods. Produced only by `SemanticAnalyzer::prepare()`.

```rust
pub struct SemanticModel {
    // All private. Callers never look inside.
    source: String,
    parser: RawParser<'d>,            // owns the arena — node IDs stay valid
    stmts: Vec<Result<NodeId, ParseError>>,
    tokens: Vec<CachedToken>,
    // Future: symbol indexes, scope caches, type maps, hashmaps
}
```

Freely storable, cacheable. No lifetime parameters in the public type. The LSP server caches one per open file, invalidates on edit.

### DatabaseCatalog

The only external input the caller provides. Symmetric — both relations and functions.

```rust
pub struct DatabaseCatalog {
    pub relations: Vec<RelationDef>,
    pub functions: Vec<FunctionDef>,
}
```

"Here's what exists in the user's database." The analyzer handles everything else internally.

## Internal Architecture

### SemanticAnalyzer internals

```rust
pub struct SemanticAnalyzer<'d> {
    dialect: RawDialect<'d>,

    // Built once from dialect at construction — dialect builtins
    static_catalog: StaticCatalog,

    // Reusable scratch buffers — cleared, not reallocated
    diag_buf: Vec<Diagnostic>,
    doc_catalog: DocumentCatalog,
    scope_buf: Vec<Scope>,
}
```

The analyzer does not own a parser. `SemanticModel` owns parsers. For the primary `&str` API, a temporary model is created internally.

### Three-level catalog resolution

Lookup priority: document → database → static. Identical pattern for both functions and relations.

```
                    Functions                Relations
                    ─────────                ─────────
Static (dialect):   substr, count, ...       sqlite_master, ...
Database (user):    user UDFs                user tables/views
Document (DDL):     CREATE FUNCTION in file  CREATE TABLE/VIEW in file
```

**StaticCatalog** — built from dialect data at `SemanticAnalyzer::new()`. Immutable. Contains dialect builtins and extensions. Internal type, not public.

**DatabaseCatalog** — provided by the caller. Contains relations and functions from the user's live database or configuration. Public type.

**DocumentCatalog** — accumulated from DDL statements during analysis. Rebuilt each analysis pass. Internal scratch buffer owned by the analyzer.

The C layer should expose static catalog data (both functions and relations) so the Rust side can read it through FFI at construction time, same pattern as existing function metadata.

### Composed catalog lookup

Flat composition, not a chain. One struct holds references to all three levels:

```rust
struct CatalogStack<'a> {
    static_: &'a StaticCatalog,
    database: &'a DatabaseCatalog,
    document: &'a DocumentCatalog,
}

impl CatalogStack<'_> {
    fn resolve_function(&self, name: &str) -> Option<&FunctionDef> {
        self.document.find_function(name)
            .or_else(|| self.database.find_function(name))
            .or_else(|| self.static_.find_function(name))
    }

    fn resolve_relation(&self, name: &str) -> Option<&RelationDef> {
        self.document.find_relation(name)
            .or_else(|| self.database.find_relation(name))
            .or_else(|| self.static_.find_relation(name))
    }
}
```

### Embedded SQL via trait

```rust
pub trait LanguageExtractor {
    type Fragment;
    fn extract(source: &str) -> Vec<Self::Fragment>;
    fn sql_text<'a>(fragment: &'a Self::Fragment) -> &'a str;
    fn map_span(fragment: &Self::Fragment, sql_span: TextSpan) -> TextSpan;
}
```

Current `EmbeddedAnalyzer` logic for Rust/C extraction becomes `impl LanguageExtractor for RustExtractor`. The analyzer calls `L::extract()`, parses each fragment, runs diagnostics, remaps spans back to host-file offsets.

## What this replaces

| Old type | Disposition |
|----------|-------------|
| `Validator` | Replaced by `SemanticAnalyzer` |
| `EmbeddedAnalyzer` | Replaced by `LanguageExtractor` trait + `diagnostics_embedded()` |
| `AnalysisHost` | Document management goes to `LspServer`. Analysis goes to `SemanticAnalyzer`. Type deleted. |
| `DocumentAnalysis` | Replaced by `SemanticModel` (cached per file in LSP server) |
| `FunctionCatalog` | Split into `StaticCatalog` (internal) + `DatabaseCatalog` (public) |
| `RelationCatalog` | Merged into symmetric `CatalogStack` |
| `Schema` / `SessionContext` | Replaced by `DatabaseCatalog` |
| `DocumentContext` / `DocumentSchema` | Replaced by `DocumentCatalog` (internal scratch buffer) |

## Call site examples

### CLI — one-shot

```rust
let catalog = DatabaseCatalog::default(); // no DB context
let mut analyzer = SemanticAnalyzer::new(dialect);
let diags = analyzer.diagnostics(source, &catalog);
for diag in &diags {
    eprintln!("{}", DiagnosticRenderer::render(diag, source));
}
```

### LSP server — persistent, multi-document

```rust
struct LspServer<'d> {
    analyzer: SemanticAnalyzer<'d>,
    catalog: DatabaseCatalog,
    documents: HashMap<String, CachedDocument>,
}

struct CachedDocument {
    source: String,
    model: SemanticModel,
    // cached results, invalidated on edit
}

impl<'d> LspServer<'d> {
    fn on_did_change(&mut self, uri: &str, source: String) {
        let model = self.analyzer.prepare(&source);
        self.documents.insert(uri.into(), CachedDocument { source, model, .. });
    }

    fn on_diagnostics(&mut self, uri: &str) -> Vec<Diagnostic> {
        let doc = &self.documents[uri];
        self.analyzer.diagnostics_prepared(&doc.model, &self.catalog)
    }

    fn on_completion(&mut self, uri: &str, offset: usize) -> Vec<CompletionItem> {
        let doc = &self.documents[uri];
        self.analyzer.completions_prepared(&doc.model, offset, &self.catalog)
    }
}
```

### Embedded SQL — host language extraction

```rust
let catalog = DatabaseCatalog::default();
let mut analyzer = SemanticAnalyzer::new(dialect);
let diags = analyzer.diagnostics_embedded::<RustExtractor>(rust_source, &catalog);
```

## Module structure

```
semantic/
    mod.rs              ← SemanticAnalyzer, public API, re-exports
    model.rs            ← SemanticModel (opaque)
    analyzer.rs         ← core analysis implementation
    walker.rs           ← AST walking (moved from validation/)
    checks.rs           ← individual validation checks (moved from validation/)
    scope.rs            ← scope stack (moved from validation/)
    catalog.rs          ← StaticCatalog, DocumentCatalog, CatalogStack
    diagnostics.rs      ← Diagnostic, Severity, Help (moved from validation/)
    render.rs           ← DiagnosticRenderer (moved from validation/)
    functions/          ← function definitions, lookup
    relations/          ← relation definitions, lookup
    embedded.rs         ← LanguageExtractor trait, extractors
```

The `validation/` module ceases to exist. Everything is under `semantic/`.

## Key design properties

- **No cloning.** `DatabaseCatalog` is borrowed. `SemanticModel` is borrowed. No ownership transfer anywhere.
- **No redundant parsing.** `SemanticModel` holds the parser arena. Multiple queries reuse the same parse.
- **No rebuilding.** `StaticCatalog` is built once at construction. `DatabaseCatalog` is owned by the caller with whatever lifecycle they choose.
- **Symmetric.** Functions and relations have identical resolution: document → database → static.
- **Opaque intermediate.** `SemanticModel` is a black box. Internal representation can evolve (add indexes, caches, type maps) without API changes.
- **Simple default.** The primary API is three methods that take `&str`. No setup, no intermediate types.
