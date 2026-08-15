<!-- LOGO -->
<h1 align="center">
  <img src="https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/web/docs/static/favicon.svg" alt="syntaqlite logo" width="112">
  <br>syntaqlite
</h1>
<p align="center">
  A fast parser, formatter, static analyzer, and language server for SQLite SQL.
  <br>
  Available as a CLI or an embeddable library via <code>libsyntaqlite</code>.
  <br><br>
  <a href="#about">About</a>
  ·
  <a href="#performance">Performance</a>
  ·
  <a href="#quick-start">Install</a>
  ·
  <a href="https://docs.syntaqlite.com">Documentation</a>
  ·
  <a href="https://playground.syntaqlite.com">Playground</a>
  ·
  <a href="#building-and-contributing">Contributing</a>
</p>

## About

Most SQL tooling treats SQLite as a variation of generic SQL. That works for common queries,
but it misses SQLite-specific syntax and the differences introduced by SQLite versions and
compile-time flags. syntaqlite instead builds its parser and tokenizer from SQLite's own
source.

**The `syntaqlite` CLI and language server** format SQL, analyze it against your schema,
and provide completions and navigation in your editor. The analyzer works without opening a
database and can report multiple independent diagnostics in one pass.

**`libsyntaqlite`** is a fast, embeddable parser, formatter, and static analyzer for SQLite SQL.
It powers the CLI and language server. It is available through Rust and C APIs, with
packages exposing the same functionality to Python and JavaScript/WASM. The AST retains
comments and whitespace, making it suitable for migration tools, code generation, and other
source-to-source work.

The parser has been checked against roughly 396,000 statements from
[SQLite's upstream test suite](https://sqlite.org/testing.html), with about 99.7% agreement
on whether a statement should parse.

## Performance

Performance is a design constraint for syntaqlite. It is built for tasks that run on every
keystroke in an editor as well as large, generated SQL files. The tokenizer and parser are
written in C, parsing is incremental, and the library APIs reuse allocations across calls.
The CLI also keeps startup overhead low.

In reproducible head-to-head throughput benchmarks, syntaqlite was the fastest parser,
formatter, and analyzer tested. The measurements include process startup, not just time
spent inside the library. See the
[full comparison](https://docs.syntaqlite.com/latest/reference/comparison/) for the numbers,
methodology, and tool versions.

## Quick start

Install the latest release on macOS, Linux, or Windows:

```console
curl -sSf https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/tools/syntaqlite | python3 - install
```

You can also install it with [mise](https://mise.jdx.dev), pip, Homebrew, or Cargo:

```console
mise use github:LalitMaganti/syntaqlite
pip install syntaqlite
brew install LalitMaganti/tap/syntaqlite
cargo install syntaqlite-cli
```

See the [installation guide](https://docs.syntaqlite.com/latest/getting-started/cli/) for
platform-specific details.

### Format SQL

```console
$ syntaqlite fmt -e "select id,name,email from users where active=1 and role='admin' order by name"
SELECT id, name, email
FROM users
WHERE
  active = 1
  AND role = 'admin'
ORDER BY
  name;
```

`syntaqlite fmt -i query.sql` formats a file in place, while `--check` checks formatting
without changing anything.

### Analyze SQL

Give the analyzer your schema and it can find mistakes without connecting to a database:

```console
$ cat schema.sql
CREATE TABLE users (id, name, email);

$ syntaqlite analyze --schema schema.sql -e "SELECT nme, email FROM users"
error: unknown column 'nme'
 --> <expression>:1:8
  |
1 | SELECT nme, email FROM users
  |        ^~~
  = help: did you mean 'name'?
```

A `syntaqlite.toml` file can associate different groups of SQL files with different schema
files. The CLI and language server both use it; see the
[project setup guide](https://docs.syntaqlite.com/latest/guides/project-setup/) for an
example.

### Target a SQLite version

The version and compile flags are global options shared by the parser, formatter, analyzer,
and language server. For example, this checks a query as SQLite 3.32.0:

```console
$ syntaqlite --sqlite-version 3.32.0 analyze \
    -e "DELETE FROM users WHERE id = 1 RETURNING *;"
error: syntax error near 'RETURNING'
 --> <expression>:1:32
  |
1 | DELETE FROM users WHERE id = 1 RETURNING *;
  |                                ^~~~~~~~~
```

`RETURNING` was added in SQLite 3.35.0. Optional SQLite features can be enabled in the same
way:

```console
syntaqlite --sqlite-cflag SQLITE_ENABLE_MATH_FUNCTIONS analyze query.sql
```

### Parse SQL

Print the full abstract syntax tree for a query:

```console
syntaqlite parse -e "SELECT 1 + 2"
```

## Editor support

The language server provides diagnostics, completion, formatting, semantic highlighting,
rename, and navigation without requiring a live database connection.

- **VS Code:** install
  [syntaqlite: SQLite language server and formatter](https://marketplace.visualstudio.com/items?itemName=syntaqlite.syntaqlite).
- **Zed:** follow the [Zed setup guide](https://docs.syntaqlite.com/latest/getting-started/zed/).
- **Other editors:** configure your LSP client to start `syntaqlite lsp`; the
  [editor guide](https://docs.syntaqlite.com/latest/getting-started/other-editors/) has
  examples.
- **Claude Code:** install the plugin with
  `claude plugin install syntaqlite@lalitmaganti-plugins`.

## libsyntaqlite

`libsyntaqlite` is available in the following ecosystems:

- [Rust](https://docs.syntaqlite.com/latest/getting-started/rust/): `cargo add syntaqlite`
- [Python](https://docs.syntaqlite.com/latest/getting-started/python/):
  `pip install syntaqlite`
- [JavaScript/WASM](https://www.npmjs.com/package/syntaqlite): `npm install syntaqlite`
- [C](https://docs.syntaqlite.com/latest/guides/c-api/): parser, tokenizer, formatter, and
  analyzer APIs

## How it works

The parser generator consumes SQLite's `parse.y` grammar and combines the generated parser
with SQLite's tokenizer. A small, hand-maintained layer folds the concrete syntax tree into
an AST. Keeping those decisions separate from the grammar makes SQLite version updates less
fragile.

The parser and tokenizer are C, so they can be used in the same environments as SQLite. The
formatter, analyzer, and language server are written in Rust. See the
[architecture guide](https://docs.syntaqlite.com/latest/contributing/architecture/) for a
more detailed tour.

SQLite-based dialects can add grammar, AST nodes, functions, and formatting rules while
reusing the same runtime. The
[custom dialect guide](https://docs.syntaqlite.com/latest/guides/custom-dialects/) describes
the code-generation workflow.

## Project status

syntaqlite is usable today, but it is still a 0.x project. The parser, formatter, analyzer,
language server, and `libsyntaqlite` APIs are all available; their public APIs and
command-line interfaces may still change before 1.0.

## Building and contributing

```console
tools/install-build-deps
tools/cargo build
```

The [contributing guide](https://docs.syntaqlite.com/latest/contributing/) covers the
repository layout and test commands. Changes are welcome through pull requests.

## AI usage

For coding: AI was used extensively for mechanical implementation, but the design,
architecture, and overall shape of the project came from me. I understand all of the code
and take full responsibility for it.

For other tasks: AI was used for research, brainstorming, testing, documentation, and
integrations. I wrote a
[detailed account of how I built syntaqlite with AI](https://lalitm.com/post/building-syntaqlite-ai/),
including where it helped and where it was detrimental.

## License

Apache 2.0. The parts derived from SQLite are public domain under the
[SQLite blessing](https://www.sqlite.org/copyright.html). See [LICENSE](LICENSE) for details.
