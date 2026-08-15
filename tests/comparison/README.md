# syntaqlite Competitive Comparison

Structured comparison of SQLite SQL tooling across 5 categories.

## Categories

| Category | Directory | What we test |
|---|---|---|
| **Tokenizer** | `tokenizer/` | Token stream accuracy on SQLite-specific tokens |
| **Parser** | `parser/` | AST/CST accuracy on obscure SQLite SQL |
| **Formatter** | `formatter/` | Formatting quality + speed benchmarks |
| **Validator** | `validator/` | Syntax + semantic error detection |
| **LSP** | `lsp/` | Editor features (completion, hover, diagnostics) |

## Competitors by Category

### Tokenizer
- sqlite3-parser / lemon-rs (Rust): SQLite Lemon port
- sqlparse (Python): generic SQL tokenizer
- sqlglot (Python): dialect-aware tokenizer
- tree-sitter-sqlite: tree-sitter grammar

### Parser
- sqlite3-parser / lemon-rs (Rust): full SQLite grammar
- liteparser (C): new, from sqliteai
- sqlparser-rs (Rust): multi-dialect, partial SQLite
- sqlglot (Python): transpiler with SQLite dialect
- sql-parser-cst (JS): full SQLite CST
- node-sql-parser (JS): multi-dialect
- sqlparser (Dart): SQLite-only, with static analysis
- ANTLR SQLite grammar: multi-language

### Formatter
- prettier-plugin-sql-cst (JS): CST-based, full SQLite
- sql-formatter (JS): token-based, SQLite dialect
- sqlfluff (Python): rule-based formatter + linter
- sqruff (Rust): sqlfluff port, faster
- sqlfmt (Python): opinionated, generic
- sleek (Rust): simple, generic
- sqlglot (Python): AST round-trip

### Validator
- sqlfluff (Python): style linter with SQLite dialect
- sqruff (Rust): sqlfluff port
- sqlcheck (C++): anti-pattern detection
- sqlx (Rust): compile-time validation against real DB
- eslint-plugin-sqlite (JS): validates against real DB
- sqlparser Dart: static analysis + type inference

### LSP
- sqls (Go): DB-connected, completion/hover/format
- sql-language-server (JS): DB-connected
- slqls (Go): DDL-based, no DB required
