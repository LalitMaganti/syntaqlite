+++
title = "libsyntaqlite for Rust"
description = "Parse, format, and statically analyze SQLite SQL from Rust."
weight = 4
+++

# Using libsyntaqlite from Rust

The `syntaqlite` crate provides Rust APIs for formatting and analyzing SQLite
SQL. This tutorial adds it to a small project, formats a query, validates the
query against a schema, and prints any diagnostics.

## 1. Create a project

```bash
cargo new sql-check
cd sql-check
```

Add syntaqlite with the features you need:

```bash
cargo add syntaqlite --features fmt,analysis,sqlite
```

## 2. Format a query

Replace `src/main.rs` with:

```rust
use syntaqlite::Formatter;

fn main() {
    let mut fmt = Formatter::new();
    let output = fmt
        .format("select id,name,email from users where active=1 order by name")
        .expect("parse error");
    println!("{output}");
}
```

Run it:

```bash
cargo run
```

```sql
SELECT id, name, email FROM users WHERE active = 1 ORDER BY name;
```

The formatter handle is reusable; internal allocations are recycled across
calls.

## 3. Validate against a schema

To add schema validation, update `src/main.rs`:

```rust
use syntaqlite::{AnalysisContext, Catalog, Formatter, Analyzer, AnalysisConfig};

fn main() {
    // Format
    let mut fmt = Formatter::new();
    let output = fmt
        .format("select id,nme from users where active=1")
        .expect("parse error");
    println!("Formatted:\n{output}");

    // Validate
    let mut analyzer = Analyzer::new();

    // Register schema from CREATE TABLE statements
    let schema = "CREATE TABLE users (id INTEGER, name TEXT, email TEXT, active INTEGER);";
    let (mut catalog, errors) = Catalog::from_ddl(
        syntaqlite::sqlite_dialect(),
        &[(schema, None)],
    );
    assert!(errors.is_empty(), "Schema errors: {errors:?}");

    // Validate a query against the schema
    let config = AnalysisConfig::default().with_strict_schema();
    let mut ctx = AnalysisContext::new(&mut catalog).with_config(config);
    let query = "SELECT id, nme FROM users WHERE active = 1";
    let model = analyzer.analyze(query, &mut ctx);

    if model.diagnostics().is_empty() {
        println!("No errors found.");
    } else {
        for d in model.diagnostics() {
            println!("{:?}: {}", d.severity(), d.message());
        }
    }
}
```

Run it:

```bash
cargo run
```

```text
Formatted:
SELECT id, nme FROM users WHERE active = 1;

Error: unknown column 'nme'
```

The diagnostic identifies `nme` as an unknown column in the supplied schema.

## 4. Parse and inspect the AST

To work with the syntax tree directly, use `syntaqlite-syntax`:

```bash
cargo add syntaqlite-syntax --features sqlite
```

```rust
use syntaqlite_syntax::{Parser, ParseOutcome};

fn main() {
    let parser = Parser::new();
    let mut session = parser.parse("SELECT 1 + 2; SELECT 'hello';");

    let mut i = 0;
    loop {
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                i += 1;
                let dump = stmt.dump();
                println!("--- statement {i} ---\n{dump}");
            }
            ParseOutcome::Err(err) => {
                eprintln!("error: {}", err.message());
                break;
            }
            ParseOutcome::Done => break,
        }
    }
}
```

## Next steps

- [Using libsyntaqlite from Rust](@/guides/rust-api.md): formatting, parsing,
  analysis, and config options
- [libsyntaqlite Rust API reference](@/reference/rust-api.md): all types and
  methods
