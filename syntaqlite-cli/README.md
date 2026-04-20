# syntaqlite-cli

The `syntaqlite` command-line tool: format, validate, and parse SQLite SQL, with a bundled language server.

**[Docs](https://docs.syntaqlite.com)** · **[Playground](https://playground.syntaqlite.com)** · **[GitHub](https://github.com/LalitMaganti/syntaqlite)**

## Install

```bash
cargo install syntaqlite-cli
```

Other install methods (Homebrew, mise, pip, standalone download) are listed in the [top-level README](https://github.com/LalitMaganti/syntaqlite#readme).

## Usage

### Format

```bash
echo "select u.id,u.name, p.title from users u join posts p on u.id=p.user_id
where u.active=1 and p.published=true order by p.created_at desc limit 10" \
  | syntaqlite fmt
```
```sql
SELECT u.id, u.name, p.title
FROM users AS u
JOIN posts AS p ON u.id = p.user_id
WHERE
  u.active = 1
  AND p.published = true
ORDER BY
  p.created_at DESC
LIMIT 10;
```

Also works on files and globs:

```bash
syntaqlite fmt 'migrations/**/*.sql'
```

### Analyze

Check SQL against a schema without touching a database:

```bash
syntaqlite analyze --schema schema.sql -e "SELECT nme FROM users"
```
```text
error: unknown column 'nme'
 --> <expression>:1:8
  |
1 | SELECT nme FROM users
  |        ^~~
  = help: did you mean 'name'?
```

Pin to a specific SQLite version to catch syntax that won't run on your target:

```bash
syntaqlite --sqlite-version 3.32.0 validate -e "DELETE FROM users RETURNING *;"
```

### Parse

```bash
syntaqlite parse -e "SELECT 1 + 2"
```
```text
SelectStmt
  columns:
    ResultColumnList [1 items]
      ResultColumn
        expr:
          BinaryExpr
            op: PLUS
            left:  Literal { literal_type: INTEGER, source: "1" }
            right: Literal { literal_type: INTEGER, source: "2" }
```

### Language server

```bash
syntaqlite lsp
```

Point your editor's LSP client at this command for diagnostics, completions, and format-on-save. See [editor integration](https://docs.syntaqlite.com/latest/getting-started/other-editors/) for setup.

See the [CLI reference](https://docs.syntaqlite.com/latest/reference/cli/) for the full list of subcommands and flags, and [project configuration](https://docs.syntaqlite.com/latest/reference/cli/#configuration) for using `syntaqlite.toml` to set schemas and formatting options per project.

## Advanced: building a custom CLI wrapper

The crate also exposes a `CliApp` trait so downstream projects can ship their own `syntaqlite`-style binary with a pre-baked dialect, avoiding the runtime `--dialect` flag and shared-library plumbing. This is the right choice when your project has a custom SQLite dialect (extra tables, functions, or grammar) and you want end users to get the same UX as the stock `syntaqlite` binary without learning about dialect loading.

```toml
[dependencies]
syntaqlite-cli = { version = "0.4.2", default-features = false }
```

```rust
use syntaqlite_cli::{CliApp, run};
use syntaqlite::any::AnyDialect;

struct MyCli;

impl CliApp for MyCli {
    fn name(&self) -> &str { "my-sql-cli" }
    fn about(&self) -> &str { "SQL tools for MyProject's dialect" }
    fn default_dialect(&self) -> Option<AnyDialect> {
        Some(my_dialect().into())
    }
}

fn main() {
    run(&MyCli);
}
```

See [`examples/cli_wrapper.rs`](examples/cli_wrapper.rs) for a runnable example.

## Features

| Feature | Default | Description |
|---------|---------|-------------|
| `bundled-sqlite-dialect` | Yes | Bake in the SQLite dialect and ship the default `syntaqlite` binary |
| `dynload` | Yes | Expose `--dialect` / `--dialect-name` flags for loading dialects from shared libraries |
| `codegen` | Yes | Expose `dialect` / dialect-tool subcommands for generating dialect sources |
| `mcp` | Yes | Expose the `mcp` subcommand |

Wrappers that bake in a single dialect typically disable `bundled-sqlite-dialect`, `dynload`, and `codegen` to keep the surface area small.

## License

Apache 2.0. SQLite components are public domain under the [SQLite blessing](https://www.sqlite.org/copyright.html).
