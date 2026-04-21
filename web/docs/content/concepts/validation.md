+++
title = "Validation model"
description = "How semantic analysis works: catalog layers, scope resolution, and diagnostic generation."
weight = 3
+++

# Validation model

syntaqlite's validator is a single-pass semantic analyzer. It walks the AST
once, resolving names against a layered catalog and emitting diagnostics
inline. This page explains the design. For practical usage, see
[project setup](@/guides/project-setup.md).

## Why single-pass

The analyzer dispatches on **semantic roles** — annotations defined in the
[`.synq` grammar files](https://github.com/LalitMaganti/syntaqlite/tree/main/syntaqlite-syntax/parser-nodes)
that tell the analyzer what each AST node means:

| Role | Triggers |
|------|----------|
| `SourceRef` | Table/view reference in FROM, JOIN, INSERT INTO, etc. |
| `ColumnRef` | Column reference (qualified or unqualified) |
| `Call` | Function call (checks existence and arity) |
| `Query` | SELECT body (pushes/pops a scope frame) |
| `CteScope` | WITH clause (registers CTE bindings) |
| `DefineTable` / `DefineView` | DDL (accumulates to catalog) |

Because all roles are leaf-level operations (look up a name, push a scope,
register a definition), they can be handled inline as the AST walk encounters
them. There's no need for a separate resolution pass, which keeps the
implementation simple and means each node is visited exactly once.

## The catalog

The catalog is where all name information lives. It uses a **layered**
architecture where inner layers shadow outer ones: the analyzer searches from
the innermost layer outward and takes the first match:

| Layer | What it holds | Lifetime |
|-------|---------------|----------|
| Query | CTEs, subquery aliases, FROM aliases | Per-statement (pushed/popped during walk) |
| Document | `CREATE TABLE` / `CREATE VIEW` from the current file | Cleared between `analyze()` calls |
| Connection | DDL accumulated across calls | Persists (Execute mode only) |
| Database | User-provided schema | Set once by caller |
| Dialect | Built-in SQLite functions, version/cflag-gated | Set once by caller |

This layering is what makes the validator work without a database connection.
You provide the schema you care about in the Database layer, the analyzer
discovers DDL in the file automatically via the Document layer, and the Dialect
layer knows which functions are available for the target SQLite version.

The source is in
[`catalog.rs`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/analysis/catalog.rs).

### Known vs. unknown columns

When a table is registered with a column list, the validator checks that
referenced columns actually exist. When registered with `None` (columns
unknown), any column reference is accepted. This distinction matters because
schema information is often incomplete: you might know a table exists from
an ORM definition but not have the full DDL.

## Scope resolution

Each SELECT statement gets its own scope frame that tracks which tables are
visible. These frames are pushed and popped by the analyzer as it walks
([`query_scope.rs`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/analysis/engine/query_scope.rs)):

1. Entering a SELECT pushes a new frame
2. FROM/JOIN clauses register tables (with aliases) into that frame
3. Column references resolve against the frame's tables
4. Leaving the SELECT pops the frame

Qualified references (`t.col`) resolve in the named table only. Unqualified
references (`col`) search all tables in scope. SQLite resolves ambiguous
unqualified columns at runtime, so the validator accepts them, matching
SQLite's own behavior rather than over-reporting.

### CTE scoping

`WITH` clauses register CTE bindings before the main query. If the CTE
declares a column list (`WITH cte(a, b) AS (...)`), the declared columns are
used for validation and the count is checked against the SELECT's actual output
columns. Recursive CTEs work: the CTE name is visible within its own body.

## Fuzzy matching

When a name doesn't resolve, the analyzer computes case-insensitive
[Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance)
against all candidates in scope
([`fuzzy.rs`](https://github.com/LalitMaganti/syntaqlite/blob/main/syntaqlite/src/analysis/diagnostics/fuzzy.rs)).
If a candidate is within the threshold (default: 2 edits), a "did you mean?"
suggestion is attached to the diagnostic.

This applies uniformly to table names, column names, and function names.

## Diagnostics

Each diagnostic carries a severity, byte-accurate source span, a human-readable
message, and a machine-readable detail enum (`UnknownTable`, `UnknownColumn`,
`UnknownFunction`, `FunctionArity`) for programmatic consumers.

By default, unresolved names produce **warnings** because the schema might be
incomplete. Strict mode (`AnalysisConfig::with_strict_schema(true)`) promotes
them to errors. This lets you start with a permissive baseline and tighten
validation as your schema coverage improves.

## Version and compile-flag awareness

The Dialect layer knows which functions are available in each SQLite version
and which require compile-time flags. When you set a target version, functions
added after that version are removed from the catalog. This means the validator
catches version mismatches the same way it catches typos, as unresolved names
with suggestions.

This is the same mechanism described in
[Why SQLite's own grammar](@/concepts/sqlite-grammar.md#version-aware-parsing),
extended from syntax to semantics.
