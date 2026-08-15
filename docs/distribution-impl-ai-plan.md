# Distribution Implementation Plan

Parent: [distribution-ai-plan.md](distribution-ai-plan.md)

This document breaks the distribution plan into concrete implementation phases
with steps, verification gates, and a recommended execution order.

## Phase 1: Extract `syntaqlite-sys` crate

The foundational change: all subsequent work depends on it.

### 1.1 Create the crate skeleton

- Create `syntaqlite-sys/Cargo.toml` with `links = "syntaqlite_parser"`,
  `cc` build dependency
- Add it to the workspace `Cargo.toml`
- Create `syntaqlite-sys/src/lib.rs`: empty or minimal (this is a `-sys`
  crate; Rust consumers use `syntaqlite`)

### 1.2 Move C sources and headers

- Move `syntaqlite/csrc/` → `syntaqlite-sys/csrc/`
- Move `syntaqlite/include/` → `syntaqlite-sys/include/`
- Update any internal `#include` paths if needed

### 1.3 Move `build.rs` logic

- Create `syntaqlite-sys/build.rs` with the two `cc::Build` invocations
  (engine + SQLite dialect) currently in `syntaqlite/build.rs`
- Emit `cargo:include=...` so downstream crates can find headers
- Support `no-bundled-parser` feature: skip engine compilation when set
- Add `-fvisibility=hidden` to the engine build (needed for coexistence)

### 1.4 Update `syntaqlite/Cargo.toml`

- Add `syntaqlite-sys = { path = "../syntaqlite-sys" }` dependency
- Remove `cc` from build-dependencies
- Replace `syntaqlite/build.rs` with a minimal one (or remove it): C
  compilation now happens in the sys crate

### 1.5 Update codegen output paths

- `tools/run-codegen` must write C outputs to `syntaqlite-sys/csrc/` and
  headers to `syntaqlite-sys/include/`
- Verify the codegen pipeline round-trips cleanly

### 1.6 Verify

- `cargo check && cargo clippy`: zero warnings
- `tools/run-unit-tests`: all pass
- `tools/run-ast-diff-tests`, `tools/run-fmt-diff-tests`: all pass
- `tools/run-codegen` writes to the new paths

## Phase 2: Amalgamation generator

Validates that the sys crate split works for the embed use case. Good early
smoke test after Phase 1.

### 2.1 Update `syntaqlite-cli amalgamate` command

The command already exists: update it to produce distribution-ready output:

- **`syntaqlite_parser.h`**: concatenation of core + SQLite public headers
  (`parser.h`, `tokenizer.h`, `dialect.h`, `types.h`, `config.h`, `sqlite.h`,
  `sqlite_node.h`, `sqlite_tokens.h`) with include guards
- **`syntaqlite_parser.c`**: concatenation of all `.c` files + internal headers
  into one translation unit

### 2.2 Add dialect amalgamation variant

- Flag like `--no-sqlite` that excludes the SQLite dialect files
- Produces a core-only amalgamation + `syntaqlite_dialect.h`

### 2.3 Verify

- Generate amalgamation, compile with `gcc -c syntaqlite_parser.c`: no errors
- Write a minimal test program that parses SQL using only the amalgamation

## Phase 3: Sentinel symbol & coexistence infrastructure

Small, self-contained. Can be tested with amalgamation + library side-by-side.

### 3.1 Add sentinel symbol

- In `syntaqlite-sys/csrc/parser.c` (or a new `sentinel.c`), define
  `syntaqlite_parser_sentinel_`
- Guard it with `#ifndef SYNTAQLITE_ALLOW_DUPLICATE_PARSER`

### 3.2 Add version symbol

- Define `syntaqlite_version_X_Y_Z_` in the same place
- Add `syntaqlite_parser_version()` function returning encoded version
- Wire the version into `build.rs` from `Cargo.toml` version

### 3.3 Inject sentinel into amalgamation output

- The amalgamation generator (Phase 2) emits the
  `syntaqlite_parser_sentinel_` definition, guarded by
  `SYNTAQLITE_ALLOW_DUPLICATE_PARSER`
- Also emits `syntaqlite_parser_version()` and `syntaqlite_version_X_Y_Z_`

### 3.4 Add `no-bundled-parser` feature

- In `syntaqlite-sys/Cargo.toml`: `no-bundled-parser = []`
- In `build.rs`: skip `cc::Build` when feature is active
- Verify that `syntaqlite` still compiles (FFI externs resolve at link time, not
  compile time)

### 3.5 Verify

- Default: linking both library and amalgamation produces a duplicate symbol
  error on `syntaqlite_parser_sentinel_`
- `-DSYNTAQLITE_ALLOW_DUPLICATE_PARSER`: both link, no errors
- `no-bundled-parser`: library links against amalgamation's parser, one copy

## Phase 4: C API module (`capi`)

The core value proposition for language bindings. Must be solid before bindings.

### 4.1 Write `syntaqlite.h`

- Create `syntaqlite/include/syntaqlite.h`: hand-written header per the plan
- ~20 functions:
  - Lifecycle: `syntaqlite_engine_new`, `syntaqlite_engine_free`
  - Operations: `syntaqlite_format`, `syntaqlite_ast_json`,
    `syntaqlite_diagnostics`, `syntaqlite_completions`,
    `syntaqlite_semantic_tokens`
  - Memory: `syntaqlite_string_free`
  - Config: `syntaqlite_set_version`, `syntaqlite_set_schema_ddl`,
    `syntaqlite_set_schema_json`, `syntaqlite_clear_schema`,
    `syntaqlite_set_cflag`, `syntaqlite_clear_cflag`
- Include engine-mediated parser access (`syntaqlite_engine_parse`,
  `syntaqlite_engine_walk_nodes`, etc.)

### 4.2 Implement `src/capi/mod.rs`

- Create `syntaqlite/src/capi/mod.rs` with `extern "C"` functions matching the
  header
- `SyntaqliteEngine` is an opaque struct owning a `Formatter` (which owns a
  `Parser`) + schema state
- Each function: parse args → call Rust API → serialize result (JSON/string) →
  return `malloc`'d `CString`
- Gate module on `feature = "capi"`

### 4.3 Update `Cargo.toml` lib section

- Add `capi` feature: `capi = ["fmt", "lsp", "validation"]`
- Set `crate-type = ["rlib", "cdylib", "staticlib"]` (or conditionally add
  `cdylib`/`staticlib` only when `capi` is active)

### 4.4 Verify

- `cargo check --features capi && cargo clippy --features capi`: zero warnings
- Write a small C test program that links against the built library and calls
  `syntaqlite_format`

## Phase 5: Language bindings

Thin wrappers over a stable C API. Can be done incrementally per language.

### 5.1 C++ RAII wrapper (~50 lines)

- `syntaqlite/include/syntaqlite.hpp`: thin RAII wrapper around `syntaqlite.h`
- `SyntaqliteEngine` class with constructor/destructor, format/validate/complete
  methods
- Ship in the `libsyntaqlite` archive

### 5.2 Go binding (~100 lines)

- `bindings/go/syntaqlite/`: cgo package
- Vendor `syntaqlite.h` + `libsyntaqlite.a` (or use `pkg-config`)
- Go functions wrapping each C API call, handling `C.free` on returned strings

### 5.3 Python binding (~100 lines)

- `bindings/python/syntaqlite/`: cffi package
- Bundle `.so`/`.dylib` in the wheel
- Pythonic class: `engine = syntaqlite.Engine(); result = engine.format(sql)`
- `pyproject.toml` with `scikit-build` or `maturin` for wheel building

### 5.4 Update npm package

- Already exists as `syntaqlite` with WASM: ensure the API surface matches
  (format, validate, completions, semantic tokens)

## Phase 6: Release artifacts & CI

Automation over a working system. Last because it needs all artifacts to exist.

### 6.1 Build script for `libsyntaqlite` archives

- Script (shell or Python) that builds `syntaqlite` with `--features capi` and
  packages:
  - `syntaqlite.h` + `libsyntaqlite.a` + `libsyntaqlite.{so,dylib}` →
    `libsyntaqlite-{version}-{platform}.tar.gz`

### 6.2 GitHub Actions release workflow

- Matrix build: `{linux-x64, linux-arm64, macos-arm64, macos-x64, windows-x64}`
- Jobs:
  1. Build amalgamation → `syntaqlite-amalgamation-{version}.zip`
  2. Build amalgamation-dialect → `syntaqlite-amalgamation-dialect-{version}.zip`
  3. Build `libsyntaqlite` per platform →
     `libsyntaqlite-{version}-{platform}.tar.gz`
  4. Build CLI per platform → `syntaqlite-cli-{version}-{platform}.tar.gz`
  5. Build WASM → `syntaqlite-wasm-{version}.tar.gz`
- Upload all as GitHub release assets

### 6.3 Verify

- Dry-run the release workflow locally (or in a test branch)
- Verify artifact names match the naming convention from the parent plan

## Execution order

| Order | Phase | Risk | Rationale |
|-------|-------|------|-----------|
| 1 | Phase 1: extract sys crate | High | Touches everything. If this breaks, nothing else works. |
| 2 | Phase 2: amalgamation | Medium | Validates the sys crate split for the embed use case. |
| 3 | Phase 3: sentinel/coexistence | Low | Small, self-contained. Testable with amalgamation + library. |
| 4 | Phase 4: C API | Medium | Core value prop for bindings. Must be solid first. |
| 5 | Phase 5: language bindings | Low | Thin wrappers over a stable C API. Incremental per language. |
| 6 | Phase 6: CI/release | Low | Automation over a working system. Needs all artifacts. |
