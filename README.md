# <img src="https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/web/docs/static/favicon.svg" width="32" height="32" alt="">&nbsp;&nbsp;syntaqlite

A parser, formatter, validator, and language server for SQLite SQL, built on SQLite's own grammar and tokenizer. If SQLite accepts it, syntaqlite parses it. If SQLite rejects it, so does syntaqlite.

**[Docs](https://docs.syntaqlite.com)** · **[Playground](https://playground.syntaqlite.com)** · **[VS Code Extension](https://marketplace.visualstudio.com/items?itemName=syntaqlite.syntaqlite)**

> **Note:** syntaqlite is at 0.x — APIs and CLI flags may change before 1.0.

## Why syntaqlite

Developer tooling for SQLite treats it as a second-class language. Most tools build a generic SQL parser and bolt SQLite on as a "flavor" — hand-written grammars, regex-based tokenizers, or subsets of SQL that approximate SQLite rather than matching it. That falls apart because SQLite has a deep surface area of syntax that generic parsers don't handle.

syntaqlite uses SQLite's own [Lemon-generated grammar](https://www.sqlite.org/lemon.html) and tokenizer, compiled from C. The parser doesn't approximate SQLite — it _is_ SQLite's grammar compiled into a reusable library.

SQLite SQL is also not one fixed language. It has [22 compile-time flags](https://www.sqlite.org/compile.html) that change what syntax the parser accepts, another 12 that gate built-in functions, and the language constantly evolves across versions. Because SQLite is embedded, you can't assume everyone is on the latest version — Android 15 ships SQLite 3.44.3, seven major versions behind latest. syntaqlite tracks all of this:

```bash
syntaqlite --sqlite-version 3.32.0 validate \
  -e "DELETE FROM users WHERE id = 1 RETURNING *;"
```
```text
error: syntax error near 'RETURNING'
 --> <stdin>:1:32
  |
1 | DELETE FROM users WHERE id = 1 RETURNING *;
  |                                ^~~~~~~~~
```

`RETURNING` was added in SQLite 3.35.0 — Android 13 still ships SQLite 3.32.2.

We've tested against ~396K statements from [SQLite's upstream test suite](https://sqlite.org/testing.html) with ~99.7% agreement on parse acceptance. See the [detailed comparison](https://docs.syntaqlite.com/main/reference/comparison/) for how syntaqlite stacks up against other tools in parser accuracy, formatter correctness, validator quality, and performance.

## What it does

### Validate — catch errors without a database ([docs](https://docs.syntaqlite.com/main/concepts/validation/))

Finds unknown tables, columns, and functions against your schema — the same errors `sqlite3_prepare` would catch, but without needing a database. Unlike `sqlite3`, syntaqlite finds **all** errors in one pass:

```sql
CREATE TABLE orders (id, status, total, created_at);

WITH
  monthly_stats(month, revenue, order_count) AS (
    SELECT strftime('%Y-%m', o.created_at), SUM(o.total)
    FROM orders o WHERE o.status = 'completed'
    GROUP BY strftime('%Y-%m', o.created_at)
  )
SELECT ms.month, ms.revenue, ms.order_count,
  ROUDN(ms.revenue / ms.order_count, 2) AS avg_order
FROM monthly_stats ms;
```

**sqlite3** stops at the first error and misses the function typo entirely:
```text
Error: in prepare, table monthly_stats has 2 values for 3 columns
```

**syntaqlite** finds both — CTE column count mismatch and the `ROUDN` typo — with source locations and suggestions:
```text
error: table 'monthly_stats' has 2 values for 3 columns
  |
2 | monthly_stats(month, revenue,
  | ^~~~~~~~~~~~~

warning: unknown function 'ROUDN'
   |
14 | ROUDN(ms.revenue / ms.order_count,
   | ^~~~~
   = help: did you mean 'round'?
```

### Format ([docs](https://docs.syntaqlite.com/main/reference/cli/#fmt))

Deterministic formatting with configurable line width, keyword casing, and indentation:

```bash
echo "select u.id,u.name, p.title from users u join posts p on u.id=p.user_id
where u.active=1 and p.published=true order by p.created_at desc limit 10" \
  | syntaqlite fmt
```
```sql
SELECT u.id, u.name, p.title
FROM users u
  JOIN posts p ON u.id = p.user_id
WHERE u.active = 1
  AND p.published = true
ORDER BY p.created_at DESC
LIMIT 10;
```

### Version and compile-flag aware ([docs](https://docs.syntaqlite.com/main/guides/version-pinning/))

Pin the parser to a specific SQLite version or enable [compile-time flags](https://www.sqlite.org/compile.html) to match your exact build:

```bash
# Reject syntax your target SQLite version doesn't support
syntaqlite --sqlite-version 3.32.0 validate query.sql

# Enable optional syntax from compile-time flags
syntaqlite --sqlite-cflag SQLITE_ENABLE_MATH_FUNCTIONS validate query.sql
```

### Validate SQL inside other languages *(experimental)*

SQL lives inside Python and TypeScript strings in most real codebases. syntaqlite extracts and validates it, handling interpolation holes:

```python
# app.py
def get_user_stats(user_id: int):
    return conn.execute(
        f"SELECT nme, ROUDN(score, 2) FROM users WHERE id = {user_id}"
    )
```
```bash
syntaqlite validate --experimental-lang python app.py
```
```text
warning: unknown function 'ROUDN'
 --> app.py:3:23
  |
3 |         f"SELECT nme, ROUDN(score, 2) FROM users WHERE id = {user_id}"
  |                       ^~~~~
  = help: did you mean 'round'?
```

### Project configuration

Create a `syntaqlite.toml` in your project root to configure schemas and formatting. The LSP, CLI, and all editor integrations read it automatically:

```toml
# Map SQL files to schema DDL files for validation and completions.
[schemas]
"src/**/*.sql" = ["schema/main.sql", "schema/views.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []  # no schema validation for migrations

# Default schema for SQL files that don't match any glob above.
# schema = ["schema.sql"]

# Formatting options (all optional, shown with defaults).
[format]
line-width = 80
indent-width = 2
keyword-case = "upper"    # "upper" | "lower"
semicolons = true
```

The config file is discovered by walking up from the file being processed — same as `rustfmt.toml` or `ruff.toml`. CLI flags override config file values.

### Editor integration ([docs](https://docs.syntaqlite.com/main/getting-started/vscode/))

Full language server — no database connection required. Diagnostics, format on save, completions, and semantic highlighting.

**VS Code** — install the [syntaqlite extension](https://marketplace.visualstudio.com/items?itemName=syntaqlite.syntaqlite) from the marketplace.

**[Other editors](https://docs.syntaqlite.com/main/getting-started/other-editors/)** — point your LSP client at:

```bash
syntaqlite lsp
```

**Claude Code** — `claude plugin install syntaqlite@lalitmaganti-plugins` ([docs](https://docs.syntaqlite.com/main/getting-started/claude-code/))

### Parse ([docs](https://docs.syntaqlite.com/main/guides/parsing/))

Full abstract syntax tree with side tables for tokens, comments, and whitespace — for code generation, migration tooling, or static analysis.

```bash
syntaqlite parse -e "SELECT 1 + 2"
```

## Install ([all methods](https://docs.syntaqlite.com/main/getting-started/cli/))

**Download and run (all platforms, no install)**

```bash
curl -sSf https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/tools/syntaqlite | python3 - fmt -e "select 1"
```

Downloads the binary on first run, caches it, auto-updates weekly.

**mise**

```bash
mise use github:LalitMaganti/syntaqlite
```

**pip (all platforms, bundled binary)**

```bash
pip install syntaqlite
```

**Homebrew (macOS)**

```bash
brew install LalitMaganti/tap/syntaqlite
```

**Cargo**

```bash
cargo install syntaqlite-cli
```

## Use as a library ([docs](https://docs.syntaqlite.com/main/integrating/))

**Rust** ([API docs](https://docs.syntaqlite.com/main/integrating/rust-api/))

```toml
[dependencies]
syntaqlite = { version = "0.2.3", features = ["fmt"] }
```

**JavaScript / WASM** ([API docs](https://docs.syntaqlite.com/main/reference/js-api/))

```bash
npm install syntaqlite
```

**C** — the parser, tokenizer, formatter, and validator all have C APIs. See the [C API docs](https://docs.syntaqlite.com/reference/c-api/) for details.

## Architecture ([docs](https://docs.syntaqlite.com/main/contributing/architecture/))

The parser and tokenizer are written in C, directly wrapping SQLite's own grammar. Everything else — formatter, validator, LSP — is written in Rust with C bindings available.

The split is intentional. The C parser is as portable as SQLite itself: it can run inside database engines, embedded systems, or anywhere SQLite runs. The Rust layer moves fast for developer tooling where the standard library and the crate ecosystem matter.

## Building from source

```bash
tools/install-build-deps
tools/cargo build
```

## Contributing

See the [contributing guide](https://docs.syntaqlite.com/contributing/) for architecture overview and testing instructions.

## License

Apache 2.0. SQLite components are public domain under the [SQLite blessing](https://www.sqlite.org/copyright.html). See [LICENSE](LICENSE) for details.
