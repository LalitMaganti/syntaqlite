+++
title = "Using from Rust"
description = "Add syntaqlite to a Rust project and format your first query."
weight = 1
+++

# Using syntaqlite from Rust

## Add the dependency

```toml
[dependencies]
syntaqlite = { version = "0.2.15", features = ["fmt"] }
```

## Format a query

```rust
use syntaqlite::Formatter;

let mut fmt = Formatter::new();
let output = fmt.format("select a,b from t where x=1")?;
println!("{output}");
// SELECT a, b
// FROM t
// WHERE x = 1;
```

That's it. `Formatter::new()` uses sensible defaults (80-char lines, 2-space
indent, uppercase keywords). The formatter is reusable: call `format()`
repeatedly and internal allocations are recycled.

## Customize formatting

```rust
use syntaqlite::{Formatter, FormatConfig, KeywordCase};

let config = FormatConfig::default()
    .with_line_width(120)
    .with_indent_width(4)
    .with_keyword_case(KeywordCase::Lower);
let mut fmt = Formatter::with_config(&config);
let output = fmt.format("SELECT 1")?;
```

## Parse SQL

The parser yields one statement at a time, so memory usage stays proportional
to the largest single statement:

```rust
use syntaqlite_syntax::{Parser, ParseOutcome};

let parser = Parser::new();
let mut session = parser.parse("SELECT 1; SELECT 2;");

loop {
    match session.next() {
        ParseOutcome::Ok(stmt) => {
            let mut buf = String::new();
            stmt.dump(&mut buf, 0);
            println!("{buf}");
        }
        ParseOutcome::Err(err) => {
            eprintln!("error: {}", err.message());
        }
        ParseOutcome::Done => break,
    }
}
```

### Accessing tokens

Enable token collection for token-level information:

```rust
use syntaqlite::parse::ParserConfig;

let config = ParserConfig::default().with_collect_tokens(true);
let parser = Parser::with_config(&config);
let mut session = parser.parse("SELECT max(x) FROM t");

if let ParseOutcome::Ok(stmt) = session.next() {
    for token in stmt.tokens() {
        println!(
            "{:4}..{:4}  {:?}  {:?}",
            token.offset(),
            token.offset() + token.length(),
            token.token_type(),
            token.text(),
        );
    }
    // Comments are separate from tokens:
    for comment in stmt.comments() {
        println!("comment at {}: {}", comment.offset(), comment.text());
    }
}
```

Token flags indicate how the parser used each token. For example,
`token.flags().used_as_function()` is `true` for `max` in `max(x)`.

### Generic traversal

For grammar-agnostic tree walking (works with any dialect), use the type-erased
API:

```rust
use syntaqlite::any::{AnyParsedStatement, FieldValue};

fn walk(stmt: &AnyParsedStatement, node_id: u32, depth: usize) {
    if let Some((tag, fields)) = stmt.extract_fields(node_id) {
        for (i, field) in fields.iter().enumerate() {
            match field {
                FieldValue::NodeId(child) => walk(stmt, *child, depth + 1),
                FieldValue::Span(text) => println!("{:indent$}{text}", "", indent = depth * 2),
                _ => {}
            }
        }
    }
}
```

## Validate SQL

Add the `validation` and `sqlite` features:

```toml
[dependencies]
syntaqlite = { version = "0.2.15", features = ["validation", "sqlite"] }
```

```rust
use syntaqlite::semantic::{
    SemanticAnalyzer, Catalog, CatalogLayer, ValidationConfig,
};
use syntaqlite::sqlite_dialect;

let mut analyzer = SemanticAnalyzer::new();

let mut catalog = Catalog::new(sqlite_dialect());
catalog.layer_mut(CatalogLayer::Database)
    .insert_table("users", Some(vec!["id".into(), "name".into(), "email".into()]), false);
catalog.layer_mut(CatalogLayer::Database)
    .insert_table("posts", Some(vec!["id".into(), "user_id".into(), "title".into()]), false);

let config = ValidationConfig::default();
let model = analyzer.analyze("SELECT nme FROM users", &catalog, &config);

for diag in model.diagnostics() {
    println!("[{}] {}", diag.severity(), diag.message());
    if let Some(help) = diag.help() {
        println!("  help: {help}");
    }
}
```

The catalog uses a layered resolution order; see
[validation concepts](@/concepts/validation.md) for details. For most use cases,
populate the `Database` layer with your schema and let the analyzer handle the
rest.

If you know a table exists but don't know its columns, pass `None` to
`insert_table`; this suppresses unknown-column warnings for that table.

When a schema is provided (via `--schema` or `syntaqlite.toml`), the CLI and
LSP automatically enable strict mode. When using the Rust API directly, set
this explicitly with `ValidationConfig::default().with_strict_schema(true)`.

## Column lineage

After validation, the `SemanticModel` also provides column-level lineage for
SELECT statements, tracing each result column back to its source table and
column:

```rust
use syntaqlite::semantic::{
    SemanticAnalyzer, Catalog, CatalogLayer, ValidationConfig,
};
use syntaqlite::sqlite_dialect;

let mut analyzer = SemanticAnalyzer::new();

let mut catalog = Catalog::new(sqlite_dialect());
catalog.layer_mut(CatalogLayer::Database)
    .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
catalog.layer_mut(CatalogLayer::Database)
    .insert_table("posts", Some(vec!["id".into(), "user_id".into(), "body".into()]), false);

let config = ValidationConfig::default();
let model = analyzer.analyze(
    "SELECT u.name, p.body FROM users u JOIN posts p ON u.id = p.user_id",
    &catalog,
    &config,
);

if let Some(lineage) = model.lineage() {
    println!("Complete: {}", lineage.is_complete());
    for col in lineage.into_inner() {
        print!("  column {}: {}", col.index, col.name);
        if let Some(ref origin) = col.origin {
            print!(" <- {}.{}", origin.table, origin.column);
        }
        println!();
    }
}

if let Some(tables) = model.tables_accessed() {
    for t in tables.into_inner() {
        println!("  table: {}", t.name);
    }
}
```

`lineage()` returns `None` for non-query statements. It returns
`LineageResult::Partial` when a view is referenced but its body is unavailable
for resolution.

## Next steps

- See the [Rust API reference](@/reference/rust-api.md) for all types and
  methods
