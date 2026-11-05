+++
title = "libsyntaqlite for JavaScript"
description = "Parse, format, and statically analyze SQLite SQL from JavaScript, in Node or the browser."
weight = 8
+++

# Using libsyntaqlite from JavaScript

The `syntaqlite` npm package is libsyntaqlite compiled to WebAssembly, with a
JavaScript API. It runs in Node and in the browser. In this tutorial you
install it, format a query, check the query against a schema, and look at its
syntax tree, using Node.

## 1. Install

```bash
mkdir sql-check && cd sql-check
npm init -y
npm pkg set type=module
npm install syntaqlite
```

The examples use ES modules and top-level `await`.

## 2. Format a query

Create `format.js`:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

process.stdout.write(engine.format("select id,name,email from users where active=1 order by name"));
```

```bash
node format.js
```

```sql
SELECT id, name, email FROM users WHERE active = 1 ORDER BY name;
```

`engine.load()` loads the WebAssembly runtime, and `loadDefault` adds the
SQLite dialect. Both are asynchronous, so do them once at startup and reuse
the engine.

## 3. Validate against a schema

Create `analyze.js`:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

const result = engine.analyze("SELECT id, nme FROM users WHERE active = 1", {
  schemaDdl: "CREATE TABLE users (id INTEGER, name TEXT, email TEXT, active INTEGER);",
});
for (const d of result.diagnostics) {
  console.log(`${d.severity}: ${d.message} (bytes ${d.start_offset}-${d.end_offset})`);
  if (d.help) console.log(`  help: ${d.help}`);
}
```

```bash
node analyze.js
```

```text
warning: unknown column 'nme' (bytes 11-14)
  help: name
```

Unknown tables and columns are reported as warnings. `help` holds the
suggested replacement.

## 4. Parse and inspect the AST

Create `parse.js`:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

const {statements} = engine.parse("SELECT id, name FROM users WHERE active = 1");
const stmt = statements[0];
console.log(stmt.type);
console.log(stmt.from_clause.type, stmt.from_clause.table_name);
for (const col of stmt.columns.children) {
  console.log(`  ${col.expr.type}: ${col.expr.column}`);
}
```

```bash
node parse.js
```

```text
SelectStmt
TableRef users
  ColumnRef: id
  ColumnRef: name
```

`parse` returns each statement as a plain object, with its node type in
`type` and its fields as properties.

## Next steps

- [Using libsyntaqlite from JavaScript](@/guides/javascript-api.md): formatter
  options, schemas from objects, parse errors, tokens, the language server, and
  using it in the browser
- [syntaqlite on npm](https://www.npmjs.com/package/syntaqlite): the package
  and its TypeScript types
