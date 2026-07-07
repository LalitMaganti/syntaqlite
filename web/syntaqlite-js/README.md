# syntaqlite

SQLite SQL parser, formatter, and language server for the browser — powered
by WebAssembly.

Built from SQLite's own grammar for 100% syntax compatibility.

- **Format** SQL with configurable line width, keyword casing, and semicolons
- **Parse** SQL into a full syntax tree
- **Language server**: diagnostics, completions, hover, and semantic tokens
  over standard LSP JSON-RPC, schema-aware via your DDL
- **Analyze** SQL in one shot: schema-aware diagnostics without an
  editor session

## Install

```sh
npm install syntaqlite
```

## Usage

```ts
import { Engine, DialectManager } from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);

// Format SQL. Options mirror the Rust FormatConfig; omitted fields use
// the formatter defaults.
console.log(engine.format("select id,name from users where id=1", {
  lineWidth: 80,
  indentWidth: 2,
  keywordCase: "upper",
  semicolons: true,
}));
// SELECT id, name FROM users WHERE id = 1;

// Parse SQL to AST (JSON); parse errors are data, not exceptions
const {statements, errors} = engine.parse("SELECT 1");

// Tokenize SQL
const tokens = engine.tokenize("SELECT 1");

// Give the analyzer your schema
engine.setSessionContextDdl("CREATE TABLE users(id INTEGER, name TEXT);");
```

## Language server

Editor features are served by an in-process LSP server. Drive it with
JSON-RPC messages:

```ts
engine.lspMessage({ jsonrpc: "2.0", id: 1, method: "initialize", params: { capabilities: {} } });
engine.lspMessage({ jsonrpc: "2.0", method: "initialized", params: {} });

const out = engine.lspMessage({
  jsonrpc: "2.0",
  method: "textDocument/didOpen",
  params: {
    textDocument: { uri: "file:///q.sql", languageId: "sql", version: 1, text: "SELECT * FROM missing" },
  },
});
// out includes a textDocument/publishDiagnostics notification.
```

Or run it in a Web Worker and connect a standard LSP client
(CodeMirror, Monaco, ...):

```ts
// worker.ts
import { Engine, DialectManager, attachLspPort, type LspPortLike } from "syntaqlite";

const engine = new Engine();
await engine.load();
await new DialectManager().loadDefault(engine);
attachLspPort(engine, self as unknown as LspPortLike);
```

Beyond standard LSP, the server accepts a `syntaqlite/setSessionContext`
extension request to set or clear the schema catalog
(`Engine.setSessionContextDdl` wraps it).

## One-shot analysis

For programmatic validation without an editor session. The catalog can
come from DDL, from structured `tables` and `views`, or both:

```ts
const analysis = engine.analyze("SELECT c FROM users", {
  schemaDdl: "CREATE TABLE users(id INTEGER, name TEXT);",
});
console.log(analysis.diagnostics);
// [{ message: "unknown column 'c'", severity: "error", ... }]

engine.analyze("SELECT id FROM logs", {
  tables: [{name: "logs", columns: ["id", "ts"]}],
});
```

## Documentation

See [syntaqlite.com](https://syntaqlite.com) for full documentation.

## License

Apache-2.0
