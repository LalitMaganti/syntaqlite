# syntaqlite

Parse, format, and validate SQLite SQL from Rust using SQLite's own grammar and tokenizer. No approximations: if SQLite accepts it, syntaqlite parses it.

**[Docs](https://docs.syntaqlite.com)** · **[Playground](https://playground.syntaqlite.com)** · **[GitHub](https://github.com/LalitMaganti/syntaqlite)**

```toml
[dependencies]
syntaqlite = "0.2.5"
```

The default feature set includes parsing, formatting, and validation for SQLite. See [Features](#features) for fine-grained control.

## Formatting

```rust
use syntaqlite::Formatter;

let mut fmt = Formatter::new();
let output = fmt.format("select id, name from users where active = 1").unwrap();
assert_eq!(output, "SELECT id, name\nFROM users\nWHERE active = 1;\n");
```

Configure line width, indentation, keyword casing, and semicolons:

```rust
use syntaqlite::{FormatConfig, Formatter, KeywordCase};

let config = FormatConfig::default()
    .with_line_width(120)
    .with_indent_width(4)
    .with_keyword_case(KeywordCase::Lower);

let mut fmt = Formatter::with_config(&config);
let output = fmt.format("SELECT 1").unwrap();
```

The `Formatter` is reusable across calls and recycles internal buffers.

## Parsing

```rust
use syntaqlite::{Parser, ParseOutcome};

let parser = Parser::new();
let mut session = parser.parse("SELECT 1 + 2; SELECT 3");

loop {
    match session.next() {
        ParseOutcome::Ok(stmt) => {
            // stmt.root() returns the typed AST node
        }
        ParseOutcome::Err(err) => {
            // `err.offset()` is relative to the failing statement; add
            // `err.statement_base_offset()` for a position in the full
            // source.
            eprintln!("parse error at offset {:?}: {}", err.offset(), err.message());
        }
        ParseOutcome::Done => break,
    }
}
```

The parser is incremental: it yields one statement at a time, so you can process multi-statement inputs without buffering everything upfront.

## Validation

Check SQL against a schema without touching a database. Catches unknown tables, columns, functions, CTE column mismatches, and more.

```rust
use syntaqlite::{AnalysisContext, Catalog, CatalogLayer, Analyzer, sqlite_dialect};

let mut analyzer = Analyzer::new();
let mut catalog = Catalog::new(sqlite_dialect());
catalog.layer_mut(CatalogLayer::Database)
    .insert_table("users", Some(vec!["id".into(), "name".into()]), false);

let mut ctx = AnalysisContext::new(&mut catalog);
let model = analyzer.analyze("SELECT id, email FROM users", &mut ctx);

for diag in model.diagnostics() {
    // severity: Error or Warning
    // message: structured enum (UnknownColumn, UnknownTable, etc.)
    // help: optional "did you mean?" suggestion
    println!("[{:?}] {}", diag.severity(), diag.message());
}
```

Output:

```text
[Warning] unknown column 'email'
```

### Catalog layers

The `Catalog` has five layers resolved in order: Query, Document, Connection, Database, Dialect. For most use cases, insert your schema into the `Database` layer:

```rust
let layer = catalog.layer_mut(CatalogLayer::Database);

// Known columns: validates column references
layer.insert_table("orders", Some(vec!["id".into(), "total".into()]), false);

// Unknown columns: table exists but accepts any column reference
layer.insert_table("legacy_data", None, false);

// Views
layer.insert_view("active_users", Some(vec!["id".into(), "name".into()]));

// Custom functions
use syntaqlite::{FunctionCategory, AritySpec};
layer.insert_function_overload("my_func", FunctionCategory::Scalar, AritySpec::Exact(2));
```

### Rendering diagnostics

Use `DiagnosticRenderer` for rustc-style error output:

```rust
use syntaqlite::DiagnosticRenderer;

let renderer = DiagnosticRenderer::new(source, "query.sql");
for diag in model.diagnostics() {
    renderer.render_diagnostic(diag).unwrap();
}
```

```text
error: unknown column 'email'
 --> query.sql:1:12
  |
1 | SELECT id, email FROM users
  |            ^~~~~
  = help: did you mean 'name'?
```

### Column lineage

For SELECT statements, validation results include column lineage tracing each output column back to its source:

```rust
if let Some(lineage) = model.lineage() {
    for col in lineage.columns() {
        println!("{} <- {}", col.name(), col.origin());
    }
}
```

## Version and compile-flag pinning

Pin the parser to a specific SQLite version or set of compile-time flags to match your target environment:

```rust
use syntaqlite::{SqliteVersion, SqliteFlags, SqliteFlag, sqlite_dialect};

// Version pinning
let dialect = sqlite_dialect()
    .with_version(SqliteVersion::V3_35);

// Compile-time flags
let flags = SqliteFlags::default()
    .with(SqliteFlag::EnableMathFunctions)
    .with(SqliteFlag::EnableFts5);

let dialect = sqlite_dialect()
    .with_cflags(flags);
```

Alternatively, use the `pin-version` and `pin-cflags` Cargo features to bake these in at compile time via environment variables, eliminating runtime branching:

```bash
SYNTAQLITE_SQLITE_VERSION=3035000 cargo build --features pin-version
```

## Features

| Feature | Default | Description |
|---------|---------|-------------|
| `sqlite` | Yes | SQLite dialect (grammar, tokens, built-in functions) |
| `fmt` | Yes | SQL formatter |
| `validation` | Yes | Analysis (schema checks, suggestions) |
| `serde` | No | `Serialize`/`Deserialize` for diagnostics and AST nodes |
| `serde-json` | No | JSON convenience helpers |
| `lsp` | No | Language server protocol implementation |
| `pin-version` | No | Pin SQLite version at compile time |
| `pin-cflags` | No | Pin compile-time flags at compile time |
| `experimental-embedded` | No | SQL extraction from Python/TypeScript strings |

To use only the parser without formatting or validation:

```toml
[dependencies]
syntaqlite = { version = "0.2.5", default-features = false, features = ["sqlite"] }
```

## License

Apache 2.0. SQLite components are public domain under the [SQLite blessing](https://www.sqlite.org/copyright.html).
