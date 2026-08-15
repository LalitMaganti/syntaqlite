+++
title = "Neovim, Helix, and other editors"
description = "Set up syntaqlite's language server in Neovim, Helix, or any LSP-compatible editor."
weight = 3
+++

# Neovim, Helix, and other editors

syntaqlite's language server works with any editor that supports LSP. Configure
your editor to start `syntaqlite lsp` for SQL files to enable diagnostics and
formatting.

## 1. Install syntaqlite

If you haven't already:

<div class="tabs" data-tab-group="editor-install">
  <div class="tab-buttons">
    <button class="active" data-tab="binary" onclick="switchTab('editor-install','binary')">Download script</button>
    <button data-tab="mise" onclick="switchTab('editor-install','mise')">mise</button>
    <button data-tab="pip" onclick="switchTab('editor-install','pip')">pip</button>
    <button data-tab="brew" onclick="switchTab('editor-install','brew')">Homebrew</button>
    <button data-tab="cargo" onclick="switchTab('editor-install','cargo')">Cargo</button>
  </div>
  <div class="tab-panel active" data-tab="binary">
    <pre><code class="language-bash">curl -sSf https://raw.githubusercontent.com/LalitMaganti/syntaqlite/main/tools/syntaqlite | python3 - install</code></pre>
    <p>Downloads the latest release to <code>~/.local/bin</code>. Works on macOS, Linux, and Windows.</p>
  </div>
  <div class="tab-panel" data-tab="mise">
    <pre><code class="language-bash">mise use github:LalitMaganti/syntaqlite</code></pre>
  </div>
  <div class="tab-panel" data-tab="pip">
    <pre><code class="language-bash">pip install syntaqlite</code></pre>
  </div>
  <div class="tab-panel" data-tab="brew">
    <pre><code class="language-bash">brew install LalitMaganti/tap/syntaqlite</code></pre>
  </div>
  <div class="tab-panel" data-tab="cargo">
    <pre><code class="language-bash">cargo install syntaqlite-cli</code></pre>
  </div>
</div>

Verify it runs:

```bash
syntaqlite fmt -e "select 1"
```

## 2. Configure your editor

### Neovim

Add this to your Neovim config (requires
[nvim-lspconfig](https://github.com/neovim/nvim-lspconfig)):

```lua
vim.lsp.config('syntaqlite', {
  cmd = { 'syntaqlite', 'lsp' },
  filetypes = { 'sql' },
  root_markers = { 'syntaqlite.toml', '.git' },
})
vim.lsp.enable('syntaqlite')
```

After you open a `.sql` file, syntax errors should be underlined and your usual
LSP format keybinding should format the file.

### Helix

Add to `~/.config/helix/languages.toml`:

```toml
[[language]]
name = "sql"
language-servers = ["syntaqlite"]

[language-server.syntaqlite]
command = "syntaqlite"
args = ["lsp"]
```

After restarting Helix, open a `.sql` file to check that diagnostics appear
inline and that `:format` formats the buffer.

### Other LSP clients

The pattern is the same for any editor: set the server command to
`syntaqlite lsp` and associate it with SQL files. The server communicates over
stdin/stdout using JSON-RPC.

## 3. Try it out

Create a file called `test.sql`:

```sql
SELEC id, name FROM users;
```

Open the file in your editor and check that `SELEC` has a syntax error
diagnostic. Changing it to `SELECT` should remove the error.

## 4. Add schema validation

Without a schema, syntaqlite catches syntax errors and function misspellings.
To also catch unknown tables and columns, add a `syntaqlite.toml` to your
project root:

```toml
schema = ["schema.sql"]
```

Then create `schema.sql` with your table definitions:

```sql
CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT);
```

Restart the language server (or reopen the editor). Now a query like
`SELECT nme FROM users` will show a warning with a "did you mean 'name'?"
suggestion.

See [project setup](@/guides/project-setup.md) for the full configuration
reference.
