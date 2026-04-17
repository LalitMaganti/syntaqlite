+++
title = "Rust API reference"
description = "Feature flags, types, and methods."
weight = 4
+++

# Rust API reference

## Feature flags

| Feature | What it enables |
|---------|----------------|
| `fmt` | Formatter |
| `validation` | Semantic analysis (unknown tables, columns, functions) |
| `sqlite` | Built-in SQLite dialect (enabled by default) |
| `lsp` | Language server protocol implementation |
| `serde-json` | JSON serialization for AST and diagnostics |
| `dynload` | Load custom dialects from shared libraries at runtime |

## Formatter

| Type / Method | Description |
|---------------|-------------|
| `Formatter::new()` | Create with SQLite dialect and default settings |
| `Formatter::with_config(&FormatConfig)` | Create with custom config |
| `fmt.format(sql) -> Result<String>` | Format SQL string |
| `FormatConfig` | Builder: `with_line_width()`, `with_indent_width()`, `with_keyword_case()`, `with_semicolons()` |
| `KeywordCase` | `Upper` or `Lower` |

The formatter is reusable; call `format()` repeatedly. Internal allocations
are reused across calls. Defaults: 80-char lines, 2-space indent, uppercase
keywords, semicolons on.

## Parser

| Type / Method | Description |
|---------------|-------------|
| `Parser::new()` | Create a parser for the SQLite grammar |
| `parser.parse(sql) -> ParseSession` | Start a parse session |
| `session.next() -> ParseOutcome` | Yield next statement |
| `ParseOutcome::Ok(stmt)` | Successfully parsed statement |
| `ParseOutcome::Err(err)` | Parse error (parser recovers and continues) |
| `ParseOutcome::Done` | No more statements |

The parser yields statements one at a time, reusing internal allocations.
It recovers from errors and continues parsing subsequent statements.

## Tokenizer

| Type / Method | Description |
|---------------|-------------|
| `Tokenizer::new()` | Create a tokenizer for the SQLite grammar |
| `tokenizer.tokenize(sql) -> Result<Vec<Token>>` | Tokenize SQL string |
| `Token` | `token_type`, `text`, byte offsets into source |

Zero-copy: tokens reference byte offsets into the source string.

## Validator

| Type / Method | Description |
|---------------|-------------|
| `SemanticAnalyzer::new()` | Create an analyzer for the SQLite dialect |
| `analyzer.analyze(sql, &catalog, &config) -> SemanticModel` | Analyze SQL, returning diagnostics and lineage |
| `Catalog::new(dialect)` | Create an empty catalog |
| `catalog.layer_mut(CatalogLayer::Database).insert_table(name, cols, false)` | Register a table |
| `ValidationConfig::default()` | Default config (warnings for unknowns) |
| `ValidationConfig::default().with_strict_schema()` | Strict mode (errors for unknowns) |
| `model.diagnostics()` | Parse and semantic diagnostics |

The analyzer is reusable; call `analyze()` repeatedly. The catalog uses a
layered resolution order; populate the `Database` layer with your schema.

## Lineage

After `analyze()`, the returned `SemanticModel` provides column-level lineage
for SELECT statements. Lineage traces each result column back to its source
table and column.

`model.statements()` yields one `StatementModel` per statement; lineage
methods are per-statement.

| Type / Method | Description |
|---------------|-------------|
| `stmt.lineage()` | Per-column lineage: `Option<LineageResult<&[ColumnLineage]>>` |
| `stmt.relations_accessed()` | Relations in FROM: `Option<LineageResult<&[RelationAccess]>>` |
| `stmt.physical_tables_accessed()` | Physical tables after resolving CTEs/views: `Option<LineageResult<&[PhysicalTableAccess]>>` |
| `stmt.defined_relations()` | `&[DefinedRelation]` — targets created by DDL (`CREATE TABLE`/`CREATE VIEW`) |
| `stmt.unexpanded_views()` | `&[String]` — canonical names of views whose bodies weren't available for expansion |
| `LineageResult<T>` | `Complete(T)` — fully resolved, or `Partial(T)` — some view bodies unavailable |
| `ColumnLineage` | `name: String`, `index: u32`, `origin: Option<ColumnOrigin>` |
| `ColumnOrigin` | `table: String`, `column: String` |
| `RelationAccess` | `name: String`, `kind: RelationKind` |
| `RelationKind` | `Table` or `View` |
| `PhysicalTableAccess` | `name: String` |
| `DefinedRelation` | `name: String`, `is_view: bool` |

Lineage methods return `None` for non-query statements (CREATE, INSERT,
etc.). A column's `origin` is populated only when the column is an
untransformed passthrough — a direct column reference or a pure rename
alias. Expressions, casts, aggregates, and set-op (`UNION`, etc.)
columns have `origin: None`. Result is `Partial` when a referenced view's
body is unavailable for tracing through; `unexpanded_views()` lists the
view names.
