+++
title = "Tutorials"
sort_by = "weight"
+++

# Getting started

syntaqlite provides a CLI and language server for parsing, formatting, and statically
analyzing SQLite SQL.
You can [try it in the browser](https://playground.syntaqlite.com) without
installing anything, or install the CLI. To embed the same parser, formatter, and
static analyzer in your own software, use libsyntaqlite from Rust, Python,
JavaScript/WASM, or C.

## Install the CLI

<div class="tabs" data-tab-group="install">
  <div class="tab-buttons">
    <button class="active" data-tab="binary" onclick="switchTab('install','binary')">Download &amp; run</button>
    <button data-tab="mise" onclick="switchTab('install','mise')">mise</button>
    <button data-tab="pip" onclick="switchTab('install','pip')">pip</button>
    <button data-tab="brew" onclick="switchTab('install','brew')">Homebrew</button>
    <button data-tab="cargo" onclick="switchTab('install','cargo')">Cargo</button>
  </div>
  <div class="tab-panel active" data-tab="binary">
    <pre><code class="language-bash">curl -sSf https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/tools/syntaqlite | python3 - fmt -e "select 1"</code></pre>
    <p>Downloads the binary on first run, caches it, auto-updates weekly. Works on macOS, Linux, and Windows.</p>
  </div>
  <div class="tab-panel" data-tab="mise">
    <pre><code class="language-bash">mise use github:LalitMaganti/syntaqlite</code></pre>
  </div>
  <div class="tab-panel" data-tab="pip">
    <pre><code class="language-bash">pip install syntaqlite</code></pre>
    <p>Installs the CLI binary and the <a href="@/getting-started/python.md">libsyntaqlite Python API</a>. No Rust toolchain needed.</p>
  </div>
  <div class="tab-panel" data-tab="brew">
    <pre><code class="language-bash">brew install LalitMaganti/tap/syntaqlite</code></pre>
  </div>
  <div class="tab-panel" data-tab="cargo">
    <pre><code class="language-bash">cargo install syntaqlite-cli</code></pre>
  </div>
</div>

## Format

```bash
syntaqlite fmt -e "select id,name from users where active=1 and role='admin'"
```
```sql
SELECT id, name FROM users WHERE active = 1 AND role = 'admin';
```

Format a file in place with `syntaqlite fmt -i query.sql`, or check formatting
in CI with `syntaqlite fmt --check "**/*.sql"`.

## Analyze

syntaqlite reads your `CREATE TABLE` statements to build a schema, then
validates queries against it without connecting to a database. A single run can
report multiple errors, including source locations and spelling suggestions:

```bash
syntaqlite analyze -e "CREATE TABLE users (id, name, email); SELECT nme FROM users;"
```
```text
warning: unknown column 'nme'
 --> <expression>:1:46
  |
1 | CREATE TABLE users (id, name, email); SELECT nme FROM users;
  |                                              ^~~
  = help: did you mean 'name'?
```

For real projects, separate your schema from your queries:

```bash
syntaqlite analyze --schema schema.sql queries.sql
```

## Parse

Inspect the full abstract syntax tree:

```bash
syntaqlite parse -e "SELECT 1 + 2"
```

The AST can be used for code generation, migration tooling, and static
analysis. The [libsyntaqlite Rust guide](@/guides/rust-api.md#parse-sql) and
[libsyntaqlite Python tutorial](@/getting-started/python.md) show how to work
with it programmatically.

