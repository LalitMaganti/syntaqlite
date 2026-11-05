+++
title = "Using libsyntaqlite from JavaScript"
description = "Formatter options, schemas from objects, parse errors, tokens, the language server, and browser use with the syntaqlite npm package."
weight = 3
+++

# Using libsyntaqlite from JavaScript

This guide picks up where the
[libsyntaqlite for JavaScript tutorial](@/getting-started/javascript.md) leaves
off. It covers formatter options, building a schema from objects, rendered
reports, parse errors, tokens, the built-in language server, and loading the
package in the browser. Each example starts by loading the engine as the tutorial does.

## Customize formatting

`format` takes options for the line width, indentation, keyword case, and
trailing semicolons:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

const sql = "SELECT id, name, email FROM users WHERE active = 1 AND role = 'admin'";
process.stdout.write(engine.format(sql, {lineWidth: 40, indentWidth: 4, keywordCase: "lower"}));

try {
  engine.format("SELECT FROM users");
} catch (err) {
  console.log(`error: ${err.message}`);
}
```

```text
select id, name, email
from users
where
    active = 1
    and role = 'admin';
error: syntax error near 'FROM'
```

SQL that does not parse throws. See
[Formatting options](@/reference/formatting-options.md) for what each setting
does.

## Build a schema from objects

The tutorial loads the schema from `CREATE TABLE` statements. If your program
already knows its tables, pass them as `tables` and `views` instead. A column
is a name, or `{name, kind}` where `kind` is `"normal"`, `"generated"`, or
`"hidden"`. A table without `columns` accepts any column name:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

const result = engine.analyze(
  "SELECT name_lower FROM users; SELECT anything FROM events; SELECT nmae FROM user_names",
  {
    tables: [
      {name: "users", columns: ["id", "name", {name: "name_lower", kind: "generated"}]},
      // Columns unknown: any column of this table is accepted.
      {name: "events"},
    ],
    views: [{name: "user_names", columns: ["name"]}],
  },
);
for (const d of result.diagnostics) {
  console.log(`${d.severity} at bytes ${d.start_offset}-${d.end_offset}: ${d.message}`);
}
```

```text
warning at bytes 66-70: unknown column 'nmae'
```

`schemaDdl`, `tables`, and `views` can be combined in one call. Pass
`strictSchema: true` to report unknown names as errors.

## Show a report

`analyzeText` takes the same options as `analyze` and returns the diagnostics
as a terminal-style report, with the source shown under each one:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

const report = engine.analyzeText("SELECT id, nme FROM users WHERE active = 1", {
  schemaDdl: "CREATE TABLE users (id INTEGER, name TEXT, email TEXT, active INTEGER);",
  strictSchema: true,
  sourceName: "query.sql",
});
process.stdout.write(report);
```

```text
error: unknown column 'nme'
 --> query.sql:1:12
  |
1 | SELECT id, nme FROM users WHERE active = 1
  |            ^~~
  = help: did you mean 'name'?
```

## Parse errors

`parse` never throws for bad SQL. It returns the statements that parsed and,
separately, the errors for those that didn't:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

const {statements, errors} = engine.parse("SELECT FROM; SELECT 1");
console.log(statements.map((s) => s.type));
for (const e of errors) {
  console.log(`${e.severity} at bytes ${e.start_offset}-${e.end_offset}: ${e.message}`);
}
```

```text
[ 'SelectStmt' ]
error at bytes 7-11: syntax error near 'FROM'
```

## Tokens

`tokenize` returns every token, including whitespace, with its byte offset and
a category:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

for (const tok of engine.tokenize("SELECT max(x) FROM t")) {
  console.log(`${String(tok.offset).padStart(2)} ${JSON.stringify(tok.text).padEnd(8)} ${tok.category}`);
}
```

```text
 0 "SELECT" keyword
 6 " "      other
 7 "max"    identifier
10 "("      punctuation
11 "x"      identifier
12 ")"      punctuation
13 " "      other
14 "FROM"   keyword
18 " "      other
19 "t"      identifier
```

## Language server

The package includes a language server, the same one `syntaqlite lsp` runs,
inside the engine. Send it standard LSP messages with
`lspMessage`, which returns the messages the server sends back:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);
engine.setSessionContextDdl("CREATE TABLE users (id INTEGER, name TEXT);");

engine.lspMessage({jsonrpc: "2.0", id: 1, method: "initialize", params: {capabilities: {}}});
engine.lspMessage({jsonrpc: "2.0", method: "initialized", params: {}});

const out = engine.lspMessage({
  jsonrpc: "2.0",
  method: "textDocument/didOpen",
  params: {
    textDocument: {uri: "file:///query.sql", languageId: "sql", version: 1, text: "SELECT nme FROM users"},
  },
});
const published = out.find((m) => m.method === "textDocument/publishDiagnostics");
for (const d of published.params.diagnostics) {
  const {start, end} = d.range;
  console.log(`${start.line}:${start.character}-${end.character} ${d.message}`);
}
```

```text
0:7-10 unknown column 'nme' (did you mean 'name'?)
```

`setSessionContextDdl` gives the server the schema to check against. In an
editor, run the engine in a Web Worker and connect it to a standard LSP client
such as the ones for CodeMirror or Monaco. `attachLspPort` wires the server to
the worker's message port:

```js
// worker.js
import {attachLspPort, DialectManager, Engine} from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);
attachLspPort(engine, self);
```

## Use it in the browser

The package loads its WebAssembly files from its `wasm/` directory, which it
finds relative to its own JavaScript files. If your bundler copies the package
as is, there is nothing to configure. Otherwise, copy the whole `wasm/`
directory to where you serve static files and pass its URL as `wasmBaseUrl`.
The runtime and the SQLite dialect are both loaded from there:

```js
import {DialectManager, Engine} from "syntaqlite";

const engine = new Engine({wasmBaseUrl: "/assets/syntaqlite/"});
await engine.load();
await new DialectManager().loadDefault(engine);
```

`loadDefault` throws if the dialect cannot be loaded, for example because a
file is missing.

## Next steps

- [syntaqlite on npm](https://www.npmjs.com/package/syntaqlite): the package
  and its TypeScript types
- [Formatting options](@/reference/formatting-options.md): what each formatter
  setting does
