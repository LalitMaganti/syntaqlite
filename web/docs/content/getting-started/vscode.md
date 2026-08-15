+++
title = "VS Code"
description = "Install the SQLite SQL language server and formatter for VS Code."
weight = 1
+++

# VS Code

## 1. Install

Install **[syntaqlite: SQLite language server and formatter](https://marketplace.visualstudio.com/items?itemName=syntaqlite.syntaqlite)**
from the VS Code Marketplace, or search for `syntaqlite` in the Extensions
panel (`Cmd+Shift+X`). The extension bundles the binary. No other setup
needed.

## 2. Create a project folder

Create a new folder for this tutorial and open it in VS Code (`File → Open
Folder…`). All the files below go in this folder.

## 3. See diagnostics

Create a file called `demo.sql` and paste this in:

```sql
select id,name,email,created_at from users as u wehre active=1 and role='admin' order by created_at desc
```

`wehre` is immediately underlined in red. Fix it to `where` and the error
clears.

## 4. Format

With the cursor in `demo.sql`, run **Format Document** (`Shift+Alt+F`). The
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

To format automatically on every save, add this to your VS Code settings:

```json
{
  "editor.formatOnSave": true
}
```

## 5. Add a schema

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

Go back to `demo.sql` and change `name` to `nme`:

```sql
SELECT id, nme, email, created_at
FROM users AS u
WHERE
  active = 1
  AND role = 'admin'
ORDER BY
  created_at DESC;
```

You'll see `nme` underlined with a warning: *unknown column 'nme', did you
mean 'name'?* Fix it back to `name` and the warning disappears.

## 6. Completions

Now that you have a schema, type a new query in `demo.sql`. After `FROM `,
you'll see `users` offered as a completion. After `SELECT `, you'll see `id`,
`name`, `email`, and `active`.

## Next steps

- [Formatting options](@/reference/formatting-options.md): line width,
  keyword casing, semicolons
- [Config file reference](@/reference/config-file.md): glob-based schema
  routing and all `syntaqlite.toml` options
- [Project setup guide](@/guides/project-setup.md): multi-schema
  setups, strict mode, CI integration
