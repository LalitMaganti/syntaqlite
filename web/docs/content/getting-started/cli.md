+++
title = "Command line"
description = "Install the CLI for formatting, validation, CI, and scripting."
weight = 2
+++

# Command line

## Install

<div class="tabs" data-tab-group="cli-install">
  <div class="tab-buttons">
    <button class="active" data-tab="binary" onclick="switchTab('cli-install','binary')">Download script</button>
    <button data-tab="mise" onclick="switchTab('cli-install','mise')">mise</button>
    <button data-tab="pip" onclick="switchTab('cli-install','pip')">pip</button>
    <button data-tab="brew" onclick="switchTab('cli-install','brew')">Homebrew</button>
    <button data-tab="cargo" onclick="switchTab('cli-install','cargo')">Cargo</button>
  </div>
  <div class="tab-panel active" data-tab="binary">
    <pre><code class="language-bash">curl -sSf https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/tools/syntaqlite | python3 - install</code></pre>
    <p>Downloads the latest release to <code>~/.local/bin</code>. Works on macOS, Linux, and Windows. Optionally pass a custom directory: <code>python3 - install /usr/local/bin</code>.</p>
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

Check that it installed:

```bash
syntaqlite --version
```

## Format a query

Run this to see formatting in action:

```bash
syntaqlite fmt -e "select id,name,email from users where active=1 and role='admin' order by name"
```

Output:

```sql
SELECT id, name, email
FROM users
WHERE
  active = 1
  AND role = 'admin'
ORDER BY
  name;
```

Keywords are uppercased, clauses break onto separate lines, and a semicolon is
appended.

## Format a file

Create a file called `query.sql`:

```sql
select u.id,u.name,p.title from users u join posts p on u.id=p.user_id where u.active=1
```

Format it in place:

```bash
syntaqlite fmt -i query.sql
cat query.sql
```

```sql
SELECT u.id, u.name, p.title
FROM users AS u
JOIN posts AS p ON u.id = p.user_id
WHERE
  u.active = 1;
```

To format every SQL file in a project at once:

```bash
syntaqlite fmt -i "**/*.sql"
```

## Validate against a schema

syntaqlite can check table and column names against your schema. See the
[project setup guide](@/guides/project-setup.md) for setup instructions.

## Check formatting in CI

Use `--check` to verify files are formatted without modifying them. See the
[CI integration guide](@/guides/ci-integration.md) for full setup.

## Next steps

- [Project setup guide](@/guides/project-setup.md): configure
  `syntaqlite.toml` for a real project
- [libsyntaqlite for Python](@/getting-started/python.md): use `parse`,
  `format_sql`, `analyze` from Python
- [CLI reference](@/reference/cli.md): all flags for `fmt`, `analyze`,
  `parse`, and `lsp`
- [Config file reference](@/reference/config-file.md): `syntaqlite.toml`
  format
- [Formatting options](@/reference/formatting-options.md): line width,
  keyword casing, and more
