# syntaqlite-cli

The `syntaqlite` command-line tool: format, validate, and parse SQLite SQL, plus an LSP server, MCP server, and dialect codegen.

**[Docs](https://docs.syntaqlite.com)** · **[Playground](https://playground.syntaqlite.com)** · **[GitHub](https://github.com/LalitMaganti/syntaqlite)**

Most users should install this crate for the binary, not depend on it as a library:

```bash
cargo install syntaqlite-cli
```

See the [top-level README](https://github.com/LalitMaganti/syntaqlite#readme) and [CLI docs](https://docs.syntaqlite.com/latest/reference/cli/) for usage.

## Advanced: building a custom CLI wrapper

The crate also exposes a `CliApp` trait so downstream projects can ship their own `syntaqlite`-style binary with a pre-baked dialect, avoiding the runtime `--dialect` flag and shared-library plumbing. This is the right choice when your project has a custom SQLite dialect (extra tables, functions, or grammar) and you want end users to get the same UX as the stock `syntaqlite` binary without learning about dialect loading.

```toml
[dependencies]
syntaqlite-cli = { version = "0.4.2", default-features = false, features = ["mcp"] }
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

All `CliApp` methods have defaults, so adding new ones is non-breaking. See [`examples/cli_wrapper.rs`](examples/cli_wrapper.rs) for a runnable example.

## Features

| Feature | Default | Description |
|---------|---------|-------------|
| `bundled-sqlite-dialect` | Yes | Bake in the SQLite dialect and ship the default `syntaqlite` binary |
| `dynload` | Yes | Expose `--dialect` / `--dialect-name` flags for loading dialects from shared libraries |
| `codegen` | Yes | Expose `dialect` / dialect-tool subcommands for generating dialect sources |
| `mcp` | Yes | Expose the `mcp` (Model Context Protocol) subcommand |

Wrappers that bake in a single dialect typically disable `bundled-sqlite-dialect`, `dynload`, and `codegen` to keep the surface area small.

## License

Apache 2.0. SQLite components are public domain under the [SQLite blessing](https://www.sqlite.org/copyright.html).
