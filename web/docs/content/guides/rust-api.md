+++
title = "Using libsyntaqlite from Rust"
description = "Formatter options, tokens, schemas built in code, and column lineage with the syntaqlite crate."
weight = 1
+++

# Using libsyntaqlite from Rust

This guide picks up where the [libsyntaqlite for Rust tutorial](@/getting-started/rust.md)
leaves off. It covers formatter options, reading tokens and comments,
building a schema in code, and column lineage.

## Customize formatting

Pass a `FormatConfig` to change the line width, indentation, keyword case, and
trailing semicolons:

```rust
use syntaqlite::fmt::KeywordCase;
use syntaqlite::{FormatConfig, Formatter};

fn main() {
    let config = FormatConfig::default()
        .with_line_width(40)
        .with_indent_width(4)
        .with_keyword_case(KeywordCase::Lower);
    let mut fmt = Formatter::with_config(&config);

    for sql in [
        "SELECT id, name, email FROM users WHERE active = 1 AND role = 'admin'",
        "SELECT FROM users",
    ] {
        match fmt.format(sql) {
            Ok(output) => print!("{output}"),
            Err(err) => println!("error: {err}"),
        }
    }
}
```

```text
select id, name, email
from users
where
    active = 1
    and role = 'admin';
error: syntax error near 'FROM'
```

SQL that does not parse is not formatted; `format` returns the parse error. See
[Formatting options](@/reference/formatting-options.md) for what each setting
does.

## Read tokens and comments

Turn on token collection to see every token the parser read, its byte range,
and how the parser used it. Comments are kept separately:

```rust
use syntaqlite::parse::ParserConfig;
use syntaqlite::{ParseOutcome, Parser};

fn main() {
    let config = ParserConfig::default().with_collect_tokens(true);
    let parser = Parser::with_config(&config);
    let mut session = parser.parse("SELECT max(x) FROM t -- largest x");

    if let ParseOutcome::Ok(stmt) = session.next() {
        for token in stmt.tokens() {
            println!(
                "{:>2}..{:<2} {:<12} {:?}{}",
                token.offset(),
                token.offset() + token.length(),
                format!("{:?}", token.token_type()),
                token.text(),
                if token.flags().used_as_function() { "  (function)" } else { "" },
            );
        }
        for comment in stmt.comments() {
            println!("comment at {}: {}", comment.offset(), comment.text());
        }
    }
}
```

```text
 0..6  Select       "SELECT"
 7..10 Id           "max"  (function)
10..11 Lp           "("
11..12 Id           "x"
12..13 Rp           ")"
14..18 From         "FROM"
19..20 Id           "t"
comment at 21: -- largest x
```

`max` is an identifier token that the parser used as a function name, which
`token.flags()` records.

## Build a schema in code

The tutorial loads the schema from `CREATE TABLE` statements. If your program
already knows its tables, for example from a database's catalog, add them to
the catalog directly. Mark generated columns with `Column::generated` and
hidden virtual-table columns with `Column::hidden`. A table registered without
columns accepts any column name:

```rust
use syntaqlite::analysis::{CatalogLayer, Column, TableDef};
use syntaqlite::{AnalysisConfig, AnalysisContext, Analyzer, Catalog, sqlite_dialect};

fn main() {
    let mut catalog = Catalog::new(sqlite_dialect());
    let db = catalog.layer_mut(CatalogLayer::Database);
    db.insert_table(TableDef::new("users").columns([
        Column::from("id"),
        Column::from("name"),
        Column::generated("name_lower"),
    ]));
    // A table whose columns aren't known: any column of it is accepted.
    db.insert_table(TableDef::new("events"));

    let config = AnalysisConfig::default().with_strict_schema();
    let mut ctx = AnalysisContext::new(&mut catalog).with_config(config);
    let mut analyzer = Analyzer::new();
    let model = analyzer.analyze(
        "SELECT name_lower FROM users; SELECT anything FROM events; SELECT nmae FROM users",
        &mut ctx,
    );

    for d in model.diagnostics() {
        println!("{:?}: {}", d.severity(), d.message());
        if let Some(help) = d.help() {
            println!("  help: {help}");
        }
    }
}
```

```text
Error: unknown column 'nmae'
  help: did you mean 'name'?
```

The catalog has several layers, which the analyzer searches in order. For most
programs, put the schema in the `Database` layer. See
[validation concepts](@/concepts/validation.md) for how the layers interact.

## Column lineage

After `analyze`, each statement reports which table and column every result
column comes from, and which tables it reads:

```rust
use syntaqlite::analysis::{CatalogLayer, TableDef};
use syntaqlite::{AnalysisContext, Analyzer, Catalog, sqlite_dialect};

fn main() {
    let mut catalog = Catalog::new(sqlite_dialect());
    let db = catalog.layer_mut(CatalogLayer::Database);
    db.insert_table(TableDef::new("users").columns(["id", "name"]));
    db.insert_table(TableDef::new("posts").columns(["id", "user_id", "body"]));

    let mut ctx = AnalysisContext::new(&mut catalog);
    let mut analyzer = Analyzer::new();
    let model = analyzer.analyze(
        "SELECT u.name, p.body FROM users u JOIN posts p ON u.id = p.user_id",
        &mut ctx,
    );

    for stmt in model.statements() {
        if let Some(lineage) = stmt.lineage() {
            println!("complete: {}", lineage.is_complete());
            for col in lineage.into_inner() {
                print!("  column {}: {}", col.index, col.name);
                if let Some(origin) = &col.origin {
                    print!(" <- {}.{}", origin.table, origin.column);
                }
                println!();
            }
        }
        if let Some(tables) = stmt.physical_tables_accessed() {
            for t in tables.into_inner() {
                println!("  table: {}", t.name);
            }
        }
    }
}
```

```text
complete: true
  column 0: name <- users.name
  column 1: body <- posts.body
  table: posts
  table: users
```

Lineage is complete when every source resolves to a table. It is partial when
the query reads a view whose definition the analyzer doesn't have;
`stmt.unexpanded_views()` lists those views. `lineage()` returns `None` for
statements other than queries.

## Next steps

- [libsyntaqlite Rust API reference](@/reference/rust-api.md): feature flags and
  every type
- [Validation concepts](@/concepts/validation.md): how the analyzer resolves
  names
