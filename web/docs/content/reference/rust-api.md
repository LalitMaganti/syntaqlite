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
| `analysis` | Analysis (unknown tables, columns, functions, lineage) |
| `sqlite` | Built-in SQLite dialect (enabled by default) |
| `lsp` | Language server protocol implementation |
| `rpc` | JSON-RPC dispatch (`parse`/`format`/`tokenize`/`analyze` over JSON) |
| `serde` | `serde::Serialize`/`Deserialize` for AST nodes and diagnostics |
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

## RPC

`syntaqlite::rpc` (feature `rpc`) — one JSON request in, one JSON envelope out.
Backs the CLI's `serve json` loop and the [C API](@/reference/c-api.md#rpc-json-dispatch).

| Type / Method | Description |
|---------------|-------------|
| `RpcSession::new(&dialect)` | Reusable session bundling parser, tokenizer, analyzer, and formatter cache |
| `rpc::call_json(&mut session, json) -> String` | Run one request, returning the `{"ok":...}` envelope string |
| `rpc::handle_request(&mut session, &Value) -> Result<Value, String>` | Lower-level dispatch returning the raw `result` value |

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

## Analyzer

| Type / Method | Description |
|---------------|-------------|
| `Analyzer::new()` | Create an analyzer for the SQLite dialect |
| `AnalysisContext::new(&mut catalog)` | Bundle the catalog, config, and optional module resolver for a single analysis call |
| `analyzer.analyze(sql, &mut ctx) -> Analysis` | Analyze SQL, returning diagnostics and lineage |
| `Catalog::new(dialect)` | Create an empty catalog |
| `catalog.layer_mut(CatalogLayer::Database).insert_table(name, cols, false)` | Register a table |
| `AnalysisConfig::default()` | Default config (warnings for unknowns) |
| `AnalysisConfig::default().with_strict_schema()` | Strict mode (errors for unknowns) |
| `model.diagnostics()` | Parse and semantic diagnostics |

The analyzer is reusable; call `analyze()` repeatedly. The catalog uses a
layered resolution order; populate the `Database` layer with your schema.

## Lineage

After `analyze()`, the returned `Analysis` provides column-level lineage
for SELECT statements. Lineage traces each result column back to its source
table and column.

`model.statements()` yields one `StatementAnalysis` per statement; lineage
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
