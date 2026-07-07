# syntaqlite WASM Playground

Static browser playground for:
- AST dump (`ast` equivalent)
- SQL formatting (`fmt` equivalent)
- Loading a dialect `.wasm` module and binding it to the runtime module

## Build

Prerequisites:

```bash
rustup target add wasm32-unknown-emscripten
brew install emscripten
source "$(brew --prefix emscripten)/libexec/emsdk_env.sh"
```

1. Build runtime + built-in dialect wasm:

```bash
tools/build-web-playground
```

Optional: append extra target `rustc` flags.

```bash
tools/build-web-playground --rustflags "-C link-arg=..."
```

This builds:
- `syntaqlite-wasm` runtime -> `web-playground/syntaqlite-runtime.js` and `web-playground/syntaqlite-runtime.wasm`
- SQLite dialect module from `syntaqlite/csrc/*.c` via `emcc` -> `web-playground/syntaqlite-sqlite.wasm`

Target for both: `wasm32-unknown-emscripten`.

2. Serve this directory with any static file server:

```bash
tools/run-web-playground --port 8080
```

3. Open `http://localhost:8080`.

## Build Perfetto Dialect Module

To build a dialect wasm for Perfetto extension syntax (uploadable in the playground):

```bash
tools/build-perfetto-wasm
```

Default output:

- `web-playground/syntaqlite-perfetto.wasm`

Upload this file in the UI. Dialect symbol resolution can use either:

- `syntaqlite_perfetto_dialect_template`
- `syntaqlite_dialect_template`

## Runtime ABI (`syntaqlite-wasm`)

All stateful entry points operate on a session created with
`wasm_session_new() -> u32` and released with `wasm_session_free(session)`.
A session owns its dialect, SQLite version/cflag overrides, and LSP analysis
state; independent sessions can coexist in one runtime instance.

The runtime module exports (see `syntaqlite-wasm/src/main.rs` for the full
list and signatures):

- `memory`
- `wasm_session_new() -> u32`, `wasm_session_free(u32)`
- `wasm_set_dialect(session, u32) -> i32`, `wasm_clear_dialect(session)`
- `wasm_alloc(u32) -> u32`, `wasm_free(u32, u32)`
- `wasm_ast_json(session, u32, u32) -> i32`
- `wasm_fmt(session, u32, u32, u32, u32, u32, u32) -> i32`
- `wasm_diagnostics` / `wasm_semantic_tokens` / `wasm_completions` (session-scoped)
- `wasm_result_ptr() -> u32`, `wasm_result_len() -> u32`, `wasm_result_free()`

Setter calls return status code `0` on success, non-zero on error; query
calls return a non-negative count on success, negative on error. Result
text is read using `wasm_result_ptr/wasm_result_len`.

At runtime, the playground uses Emscripten's loader from `syntaqlite-runtime.js`
and loads `syntaqlite-sqlite.wasm` automatically as the built-in dialect.
Uploading a dialect wasm overrides it until unloaded.

## Dialect Module ABI

A dialect wasm module must export one of:

- `syntaqlite_dialect()`
- `syntaqlite_<name>_dialect()`

The function returns `const SyntaqliteDialect*` in shared linear memory.

Dialect symbols are resolved from the side-module export scope returned by
`loadDynamicLibrary(..., { global: false }, localScope)`.

Important: runtime wasm and dialect wasm must share memory/table (Emscripten main
module + side module) so dialect pointers and function pointers are valid for
runtime calls.
