# Upstream SQLite Test Integration Plan

## Overview

syntaqlite uses SQLite's own Lemon grammar for 100% parsing compatibility, but currently only has ~30 hand-written diff test files. SQLite's upstream test suite contains ~1,390 TCL test files with thousands of SQL statements. Integrating these dramatically improves confidence that syntaqlite correctly tokenizes, parses, and validates all valid SQLite SQL.

## Approach: Custom TCL Extension + Real TCL Interpreter

Run the actual SQLite test files using the **real system `tclsh`** with a **custom TCL extension** that implements the SQLite TCL API surface but routes all SQL through syntaqlite's parser and validator.

- **Full fidelity**: All TCL control flow, variable interpolation, conditionals work natively
- **No TCL parser needed**: `tclsh` handles all TCL complexity
- **Clean C API already exists** for parsing: `syntaqlite_parser_create()`, `_reset()`, `_next()`, `_destroy()`
- **New C API needed** for validation: `syntaqlite_validator_create_sqlite()`, `_analyze()`, `_destroy()`

## Architecture

### Dual-path SQL execution

The core insight is using `sqlite3_prepare_v2()` as ground truth. The TCL extension maintains **both** a real SQLite database handle and syntaqlite's parser/validator side by side. For each SQL statement:

1. `sqlite3_prepare_v2()` tells us exactly whether this is a prepare-time error
2. syntaqlite's parser + validator tells us what syntaqlite catches
3. Compare the two — no guessing about error categories

Runtime errors (constraints, arithmetic, etc.) never appear because we only call `prepare()`, not `step()`. DDL is stepped so schema accumulates in both the real database and syntaqlite's catalog.

### Comparison matrix

| `prepare()` result | syntaqlite result | Meaning |
|--------------------|--------------------|---------|
| OK | No diagnostics | **Agreement** — both accept |
| OK | Has diagnostics | **False positive** — syntaqlite flags valid SQL (regression if new) |
| ERROR | Has diagnostics | **Agreement** — both reject |
| ERROR | No diagnostics | **Gap** — syntaqlite misses a prepare-time error (baselined) |

## Components

### 1. Validator C API

Expose the existing Rust `SemanticAnalyzer` / `Catalog` to C, following the same FFI pattern as the parser.

**Files:**
- `syntaqlite-syntax/include/syntaqlite/validation.h` — C header
- `syntaqlite/src/semantic/ffi.rs` — Rust FFI layer

The validator works incrementally — `accumulate_ddl()` updates the catalog after each statement, so later statements can reference earlier DDL. The C API preserves this:

```c
SyntaqliteValidator* syntaqlite_validator_create_sqlite(void);
void syntaqlite_validator_destroy(SyntaqliteValidator*);

uint32_t syntaqlite_validator_analyze(
    SyntaqliteValidator* v,
    const char* source, uint32_t len
);

uint32_t syntaqlite_diagnostic_severity(const SyntaqliteValidator*, uint32_t idx);
const char* syntaqlite_diagnostic_message(const SyntaqliteValidator*, uint32_t idx);
uint32_t syntaqlite_diagnostic_start_offset(const SyntaqliteValidator*, uint32_t idx);
uint32_t syntaqlite_diagnostic_end_offset(const SyntaqliteValidator*, uint32_t idx);
```

### 2. TCL Extension (`tclsyntaqlite.c`)

C shared library loaded by `tclsh` that implements the SQLite TCL API surface.

**Key commands:**
- `sqlite3 DBNAME FILENAME` — creates real `sqlite3*` handle (in-memory) + syntaqlite parser + validator
- `DBNAME eval SQL` — runs SQL through **both** SQLite and syntaqlite, logs comparison as JSON lines
- `DBNAME close` — destroys everything
- Other subcommands (`exists`, `onecolumn`, `transaction`, `function`, `collate`) — stubs or delegate to real SQLite

**Output**: JSON lines to a file, one per SQL statement:
```json
{"sql":"SELECT * FROM t1","sqlite_ok":true,"parse_ok":true,"diagnostics":[]}
```

**Location:** `upstream-tests/csrc/tclsyntaqlite.c`

### 3. Tester Shim (`tester_shim.tcl`)

Minimal replacement for SQLite's `tester.tcl`. The C extension handles dual-path comparison — the shim just routes SQL and handles test framework conventions:

- `execsql`, `catchsql` — delegate to extension's `db eval`
- `do_test`, `do_execsql_test`, `do_catchsql_test` — run test body, don't check results
- `ifcapable` — capability check (all enabled by default)
- `finish_test`, `reset_db`, memory debug stubs, etc. — no-ops

**Location:** `upstream-tests/tcl/tester_shim.tcl`

### 4. Rust Test Runner (syntaqlite-buildtools)

New `run-upstream-tests` command in the buildtools binary.

```
syntaqlite-buildtools run-upstream-tests \
  --test-dir third_party/src/sqlite/test \
  --extension-lib target/tclsyntaqlite.so \
  --tester-shim upstream-tests/tcl/tester_shim.tcl \
  --baseline tests/upstream_baselines/parse_acceptance.json \
  --filter select \
  --validate \
  -j 4
```

**What the runner does:**
1. Discovers `*.test` files (filtered by `--filter`)
2. Spawns `tclsh` per test file with extension + shim loaded
3. Collects JSON log output from the extension
4. Aggregates per-statement results into a summary
5. Compares against baseline for regression detection
6. Reports summary with agreement rates

**Baseline regression detection:**
- `false_positive` increase = syntaqlite started rejecting valid SQL
- `parse_ok` decrease = syntaqlite lost parsing capability

**Location:** `syntaqlite-buildtools/src/upstream_tests/`

### 5. Convenience Script

`tools/run-upstream-tests` — builds the TCL extension, builds the runner, and invokes with standard paths.

## File Structure

```
syntaqlite-syntax/include/syntaqlite/
  validation.h                          # Validator C API header

syntaqlite/src/semantic/
  ffi.rs                                # Validator FFI layer

upstream-tests/
  csrc/tclsyntaqlite.c                  # TCL extension
  tcl/tester_shim.tcl                   # tester.tcl replacement

syntaqlite-buildtools/src/
  main.rs                               # +RunUpstreamTests command
  upstream_tests/
    mod.rs                              # Discovery, execution, baseline
    results.rs                          # Log parsing, aggregation

tools/
  run-upstream-tests                    # Convenience runner

tests/upstream_baselines/
  parse_acceptance.json                 # Generated on first run
```

## Prerequisites

- **System `tclsh`** + **TCL dev headers** (e.g., `apt install tcl-dev`)
- **SQLite sources** (downloaded by `tools/install-build-deps`, includes `test/`)
- **C compiler** (already required for building syntaqlite)

## Decisions

- **All orchestration in Rust** (buildtools crate), not Python
- **CI**: Separate CI job (requires tclsh + tcl-dev)
- **Cflags**: `ifcapable` works natively via real TCL interpreter
- **Formatting roundtrip testing**: Deferred to issue #8
- **Validation**: Included from the start — validator C API is independently valuable
- **Error matching**: `sqlite3_prepare_v2()` as ground truth, no regex classifier needed

## Expected Output

```
=== Upstream Test Summary ===
Files run:            1390
Files with errors:    42

Total SQL statements: 12483
  Parse OK:           11892
  Parse error:        591

  Both accept:        10847 (agreement)
  Both reject:        312   (agreement)
  False positives:    9     (syntaqlite rejects valid SQL)
  Gaps:               111   (syntaqlite misses prepare-time error)

No regressions from baseline.
```
