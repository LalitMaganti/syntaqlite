+++
title = "Testing"
description = "Run tests, write diff tests, and rebaseline expected output."
weight = 2
+++

# Testing

syntaqlite uses a combination of Rust unit tests and Python-based integration
tests.

## Running tests

```bash
# Unit tests (cargo test across all crates)
tools/run-unit-tests

# Integration tests (all suites)
tools/run-integration-tests

# A specific integration test suite
tools/run-integration-tests --suite fmt

# Filter to specific test classes
tools/run-integration-tests --filter SelectFormat

# Run suites in parallel
tools/run-integration-tests --jobs 8

# List available suites
tools/run-integration-tests --list
```

## Integration test suites

| Suite | What it tests |
|-------|---------------|
| `ast` | Parser produces correct AST for SQLite SQL |
| `fmt` | Formatter output matches expected formatting |
| `perfetto-fmt` | Formatter for the Perfetto dialect |
| `perfetto-val` | Validation for the Perfetto dialect |
| `amalg` | Amalgamation (single-file C build) compiles and parses correctly |
| `grammar` | Grammar token-ID ordering invariants |
| `sql-idempotency` | Formatting preserves AST semantics |
| `upstream-sqlite` | Behavior matches upstream SQLite |

## Diff tests

The core test pattern is the **diff test**: provide SQL input and expected
output, then assert they match. Tests are Python classes that inherit from
`TestSuite`:

```python
# tests/fmt_diff_tests/select.py

class SelectFormat(TestSuite):
    def test_literal(self):
        return DiffTestBlueprint(
            sql="SELECT 1",
            out="SELECT 1;",
        )

    def test_columns(self):
        return DiffTestBlueprint(
            sql="select a,b,c from t",
            out="SELECT a, b, c\nFROM t;",
        )

```

Test discovery is automatic: the runner finds all `TestSuite` subclasses and
calls their `test_*` methods.

### Rebaselining

When you intentionally change formatting output, update the expected values:

```bash
tools/run-integration-tests --suite fmt --rebaseline
```

This overwrites the `out` fields in the test files to match actual output.
Review the diff before committing.

## Validation tests

Validation tests verify that the analyzer correctly catches schema
errors. These exist at two levels.

### Compile-time flag tests (Rust)

`syntaqlite/tests/cflag_validation.rs` tests that functions are correctly
gated behind SQLite compile-time flags:

- `SQLITE_ENABLE_MATH_FUNCTIONS` — `sin()`, `cos()`, `sqrt()`, `pi()`
  available only when enabled
- `SQLITE_OMIT_DATETIME_FUNCS` — `date()`, `strftime()` unavailable when
  omitted
- `SQLITE_SOUNDEX` — `soundex()` only available with flag
- `SQLITE_OMIT_JSON` — `json()` unavailable when omitted
- Parser-level flags — `OMIT_CTE` suppresses `WITH`, `OMIT_WINDOWFUNC`
  suppresses `OVER`, `OMIT_RETURNING` suppresses `RETURNING`
- Cross-cutting — enabling one flag doesn't affect unrelated functions

### Embedded SQL tests (Rust)

`syntaqlite/tests/embedded.rs` tests extraction and validation of SQL strings
from Python f-strings and TypeScript template literals. Covers interpolation
handling, offset mapping back to the host file, and ensuring placeholder holes
don't leak into validation.

### Dialect validation diff tests (Python)

`tests/perfetto_validation_diff_tests/` contains diff tests for the Perfetto
dialect's validation: `CREATE PERFETTO TABLE`, `CREATE PERFETTO VIEW`, and
function definitions with unknown/known table references.

### Upstream SQLite tests

The `upstream-sqlite` integration suite runs syntaqlite against approximately
1,400 upstream SQLite TCL test files, comparing parser and validator behavior
against real SQLite.

## Idempotency tests

The idempotency test suite collects all SQL inputs from the ast and fmt test
suites, formats each one, parses both the original and formatted versions, and
asserts the ASTs are identical.

## Unit tests

Rust unit tests live in each crate's `tests/` directory. These cover the
library API directly:

```bash
# Run all unit tests
tools/run-unit-tests

# Run tests for a specific crate
tools/cargo test -p syntaqlite
```

## Bootstrap tests

The bootstrap test (`tools/run-bootstrap-test`) verifies that code generation
is reproducible: it regenerates all generated files from scratch and asserts
they match what's committed. This catches cases where `.synq` changes weren't
followed by a codegen run.
