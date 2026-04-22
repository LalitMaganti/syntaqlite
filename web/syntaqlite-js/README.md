# syntaqlite

SQLite SQL parser, formatter, and validator for the browser — powered by WebAssembly.

Built from SQLite's own grammar for 100% syntax compatibility.

- **Format** SQL with configurable line width, keyword casing, and semicolons
- **Parse** SQL into a full concrete syntax tree
- **Validate** SQL with diagnostics for unknown tables, columns, and functions
- **Complete** keywords, table names, column names, and functions

## Install

```sh
npm install syntaqlite
```

## Usage

```ts
import { Engine } from "syntaqlite";

const engine = new Engine();
await engine.load();

// Format SQL
const result = engine.format("select id,name from users where id=1");
console.log(result.sql);
// SELECT id, name FROM users WHERE id = 1

// Parse SQL to AST (JSON)
const ast = engine.astJson("SELECT 1");
console.log(ast);

// Validate SQL
const diagnostics = engine.diagnostics("SELECT * FROM missing_table");
console.log(diagnostics.entries);
```

## Documentation

See [syntaqlite.com](https://syntaqlite.com) for full documentation.

## License

Apache-2.0
