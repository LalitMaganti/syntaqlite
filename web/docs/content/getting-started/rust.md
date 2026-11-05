+++
title = "libsyntaqlite for Rust"
description = "Parse, format, and statically analyze SQLite SQL from Rust."
weight = 4
+++

# Using libsyntaqlite from Rust

The `syntaqlite` crate is libsyntaqlite's Rust API. In this tutorial you add it
to a project, format a query, check the query against a schema, and print its
syntax tree.

## 1. Create a project

```bash
cargo new sql-check
cd sql-check
cargo add syntaqlite
```

The default features include the parser, formatter, and analyzer. The parser
is written in C, so building the crate needs a C compiler as well as Rust.

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

You can keep one `Formatter` and call `format` for every query you need to
format.

## 3. Validate against a schema

Replace `src/main.rs` with:

```rust
use syntaqlite::util::DiagnosticRenderer;
use syntaqlite::{AnalysisConfig, AnalysisContext, Analyzer, Catalog, sqlite_dialect};

fn main() {
    // Register the schema from CREATE TABLE statements.
    let schema = "CREATE TABLE users (id INTEGER, name TEXT, email TEXT, active INTEGER);";
    let (mut catalog, errors) = Catalog::from_ddl(sqlite_dialect(), &[schema]);
    assert!(errors.is_empty(), "schema errors: {errors:?}");

    // Analyze a query against the schema.
    let config = AnalysisConfig::default().with_strict_schema();
    let mut ctx = AnalysisContext::new(&mut catalog).with_config(config);
    let mut analyzer = Analyzer::new();
    let sql = "SELECT id, nme FROM users WHERE active = 1";
    let model = analyzer.analyze(sql, &mut ctx);

    let renderer = DiagnosticRenderer::new(sql, "query.sql");
    let mut out = std::io::stdout();
    for d in model.diagnostics() {
        renderer.render_diagnostic(d, &mut out).unwrap();
    }
}
```

Run it:

```bash
cargo run
```

```text
error: unknown column 'nme'
 --> query.sql:1:12
  |
1 | SELECT id, nme FROM users WHERE active = 1
  |            ^~~
  = help: did you mean 'name'?
```

`with_strict_schema` makes unknown tables and columns errors. Without it, they
are reported as warnings.

## 4. Parse and inspect the AST

Replace `src/main.rs` with:

```rust
use syntaqlite::{ParseOutcome, Parser};

fn main() {
    let parser = Parser::new();
    let mut session = parser.parse("SELECT 1 + 2; SELECT 'hello';");

    let mut i = 0;
    loop {
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                i += 1;
                let mut dump = String::new();
                stmt.dump(&mut dump, 0);
                println!("--- statement {i} ---\n{dump}");
            }
            ParseOutcome::Err(err) => eprintln!("error: {}", err.message()),
            ParseOutcome::Done => break,
        }
    }
}
```

Run it:

```bash
cargo run
```

```text
--- statement 1 ---
SelectStmt
  flags: (none)
  columns:
    ResultColumnList [1 items]
      ResultColumn
        flags: (none)
        alias: (none)
        alias_as: FALSE
        expr:
          BinaryExpr
            op: PLUS
            left:
              Literal
                literal_type: INTEGER
                source: "1"
            right:
              Literal
                literal_type: INTEGER
                source: "2"
  ...
```

The parser hands back one statement at a time, so memory use depends on the
largest statement rather than the whole input.

## Next steps

- [Using libsyntaqlite from Rust](@/guides/rust-api.md): formatter options,
  tokens, building a schema in code, and column lineage
- [libsyntaqlite Rust API reference](@/reference/rust-api.md): feature flags and
  every type
