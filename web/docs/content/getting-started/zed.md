+++
title = "Zed"
description = "Install the extension: diagnostics, formatting, and completions out of the box."
weight = 2
+++

# Zed

## 1. Install

Open the Extensions panel (`Cmd+Shift+X`) and search for `syntaqlite`. Click
**Install**. The extension downloads the binary automatically — no other setup
needed.

## 2. See diagnostics

Create a file called `demo.sql` and paste this in:

```sql
select id,name,email,created_at from users as u wehre active=1 and role='admin' order by created_at desc
```

`wehre` is immediately underlined. Fix it to `where` and the error clears.

## 3. Format

Run **Editor: Format Buffer** from the command palette (`Cmd+Shift+P`). The
query becomes:

```sql
SELECT id, name, email, created_at
FROM users AS u
WHERE
  active = 1
  AND role = 'admin'
ORDER BY
  created_at DESC;
```

## 4. Add a schema

So far syntaqlite only checks syntax. To catch unknown tables and columns,
create a file called `schema.sql` next to `demo.sql`:

```sql
CREATE TABLE users (
  id INTEGER, name TEXT, email TEXT,
  active INTEGER, role TEXT, created_at TEXT
);
```

Then create `syntaqlite.toml` in the same directory:

```toml
[schemas]
"**/*.sql" = ["schema.sql"]
```

Go back to `demo.sql` and change `name` to `nme`. You'll see `nme` underlined
with a warning: *unknown column 'nme', did you mean 'name'?* Fix it back to
`name` and the warning disappears.

## 5. Completions

Now that you have a schema, type a new query in `demo.sql`. After `FROM `,
you'll see `users` offered as a completion. After `SELECT `, you'll see `id`,
`name`, `email`, and `active`.

## Configuration

The extension works out of the box but supports two optional settings in your
Zed settings (`Cmd+,`):

**Custom binary path** — use your own build instead of the auto-downloaded one:

```json
{
  "lsp": {
    "syntaqlite": {
      "binary": {
        "path": "/path/to/syntaqlite"
      }
    }
  }
}
```

**Custom config path** — point to a specific `syntaqlite.toml` instead of
relying on auto-discovery:

```json
{
  "lsp": {
    "syntaqlite": {
      "initialization_options": {
        "config": "/path/to/syntaqlite.toml"
      }
    }
  }
}
```

## Next steps

- [Formatting options](@/reference/formatting-options.md) — line width,
  keyword casing, semicolons
- [Config file reference](@/reference/config-file.md) — glob-based schema
  routing and all `syntaqlite.toml` options
- [Project setup guide](@/guides/project-setup.md) — multi-schema
  setups, strict mode, CI integration
