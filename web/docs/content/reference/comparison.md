+++
title = "Comparison to other tools"
weight = 9
+++
# How does syntaqlite compare?

There are many SQL tools out there. This page tests them head-to-head on
**SQLite-specific SQL**, the kind of syntax that trips up generic SQL parsers,
and shows raw, reproducible results.

> Generated on `x86_64-linux` with syntaqlite `0.14.0` on 2026-11-06.
> To reproduce: `tools/run-comparison --setup && tools/run-comparison --all`.
> Per-statement breakdowns and reproduction details are in the
> [detailed results](@/reference/comparison-details.md).

---

# Parsing

**What we test:** The corpus contains 40 statements covering advanced SQLite
syntax, including `UPSERT`, `RETURNING`, `STRICT` tables, window frames with
`EXCLUDE`, numeric underscores,
`IS NOT DISTINCT FROM`, recursive CTEs, and more. Each statement is first
validated against `sqlite3` itself (the ground truth), then run through every
parser. A tool scores "correct" only if it agrees with sqlite3.


## Accuracy

| Tool            | Correct                           | Rejects Valid | Accepts Invalid |
| --------------- | --------------------------------- | ------------: | --------------: |
| syntaqlite      | 40/40 (100%) ████████████████████ |             - |               - |
| lemon-rs        | 40/40 (100%) ████████████████████ |             - |               - |
| sql-parser-cst  | 39/40 (97%) ███████████████████   |             1 |               - |
| sqlglot[c]      | 35/40 (87%) █████████████████     |             5 |               - |
| sqlfluff        | 29/40 (72%) ██████████████        |            11 |               - |
| sqlparser-rs    | 26/40 (65%) █████████████         |            14 |               - |
| node-sql-parser | 15/40 (37%) ███████               |            25 |               - |

## Speed

We benchmark two file sizes: a 40-statement file (startup-dominated) and that
file repeated 30× (throughput-dominated).

### bench.sql (1×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 4.1 ± 2.0 |  |  |
| `lemon-rs` | 2.2 ± 1.2 | -47.1% | (1.9x faster) |
| `sql-parser-cst` | 104.2 ± 2.6 | +2459.0% | (25.6x slower) |
| `sqlglot[c]` | 84.0 ± 1.2 | +1964.2% | (20.6x slower) |
| `sqlparser-rs` | 3.0 ± 1.8 | -25.2% |  |
| `node-sql-parser` | 89.9 ± 2.4 | +2108.5% | (22.1x slower) |
| `sqlfluff` | 505.8 ± 13.1 | +12324.7% | (124.2x slower) |

### bench_30x.sql (30×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 4.1 ± 2.1 |  |  |
| `lemon-rs` | 5.9 ± 2.5 | +46.3% | (1.5x slower) |
| `sql-parser-cst` | 251.6 ± 19.2 | +6099.8% | (62.0x slower) |
| `sqlglot[c]` | 231.5 ± 9.1 | +5606.1% | (57.1x slower) |
| `sqlparser-rs` | 16.6 ± 0.3 | +309.3% | (4.1x slower) |
| `node-sql-parser` | 240.0 ± 6.7 | +5815.7% | (59.2x slower) |
| `sqlfluff` | 8069.8 ± 49.8 | +198785.9% | (1988.9x slower) |

---

# Formatting

**What we test:** Round-trip semantic preservation. Each of the same 40
statements is formatted, then we run `EXPLAIN` on both the original and
formatted SQL and compare the bytecode sqlite3 produces. Identical bytecode
means sqlite3 will execute the exact same operations, so the formatter preserved
semantics, not just validity. Tools that crash or refuse to format score
"refused". Tools whose output produces different bytecode score "corrupt".


## Accuracy

| Tool          | Correct                           | Corrupt | Refused |
| ------------- | --------------------------------- | ------: | ------: |
| syntaqlite    | 40/40 (100%) ████████████████████ |       - |       - |
| prettier-cst  | 39/40 (97%) ███████████████████   |       - |       1 |
| sql-formatter | 39/40 (97%) ███████████████████   |       - |       1 |
| sleek         | 37/40 (92%) ██████████████████    |       3 |       - |
| sqruff        | 32/40 (80%) ████████████████      |       - |       8 |
| sqlglot[c]    | 31/40 (77%) ███████████████       |       4 |       5 |

## Speed

### bench.sql (1×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 2.7 ± 4.1 |  |  |
| `prettier-cst` | 174.2 ± 8.0 | +6440.8% | (65.4x slower) |
| `sql-formatter` | 98.8 ± 2.6 | +3610.6% | (37.1x slower) |
| `sqlglot[c]` | 88.2 ± 2.1 | +3209.3% | (33.1x slower) |
| `sleek` | 16.5 ± 7.1 | +521.0% | (6.2x slower) |
| `sqruff` | 72.7 ± 2.5 | +2629.0% | (27.3x slower) |

### bench_30x.sql (30×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 8.0 ± 1.9 |  |  |
| `prettier-cst` | 485.8 ± 18.3 | +5965.4% | (60.7x slower) |
| `sql-formatter` | 305.7 ± 9.4 | +3716.4% | (38.2x slower) |
| `sqlglot[c]` | 340.5 ± 8.7 | +4150.9% | (42.5x slower) |
| `sleek` | 24.6 ± 7.0 | +207.3% | (3.1x slower) |
| `sqruff` | 2404.1 ± 23.0 | +29913.6% | (300.1x slower) |

---

# Analysis

**What we test:** We check whether each tool can identify unknown tables, bad
column references, incorrect function arity, and CTE column mismatches without
running the query. The corpus contains 24 cases against a known schema, with 15
intentional errors and 9 valid queries. Each tool's verdict is checked against
sqlite3.

## Accuracy

Schema: `users`, `orders`, `products`, `order_items`. Ground truth: sqlite3.

| Tool       | Approach          | Correct                    | Missed | FP |
| ---------- | ----------------- | -------------------------- | -----: | -: |
| sqlite3    | runtime execution | 24/24 ████████████████████ |      - |  - |
| syntaqlite | static semantic   | 23/24 ███████████████████  |      1 |  - |
| sql-lint   | structural checks | 12/24 ██████████           |     11 |  1 |

## Diagnostic quality

The test query contains two errors: its CTE declares three columns while its
`SELECT` produces two, and `ROUND` is misspelled as `ROUDN`:

| Tool       | Approach          | Errors Found | Finds All | Did-you-mean |
| ---------- | ----------------- | :----------: | :-------: | :----------: |
| syntaqlite | static semantic   |     2/2      |    Yes    |     Yes      |
| sqlite3    | runtime execution |     1/2      |    No     |      No      |
| sql-lint   | structural checks |     0/2      |    No     |      No      |

**syntaqlite** finds both errors in one pass, with source locations and a
did-you-mean suggestion:

```
error: table 'monthly_stats' has 2 values for 3 columns
  --> /tmp/tmp14kfwjx1.sql:30:3
   |
30 |   monthly_stats(month, revenue, order_count) AS (
   |   ^~~~~~~~~~~~~
warning: unknown function 'ROUDN'
  --> /tmp/tmp14kfwjx1.sql:42:3
   |
42 |   ROUDN(ms.revenue / ms.order_count, 2) AS avg_order
   |   ^~~~~
   = help: did you mean 'round'?
note: no schema provided; unresolved names are reported as warnings. Add a `syntaqlite.toml` with `schema = ["schema.sql"]` or pass `--schema` to treat them as errors.
```

**sqlite3** stops at the first error (runtime execution):

```
Error: in prepare, table monthly_stats has 2 values for 3 columns
```

## Speed

### bench.sql (1×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 3.4 ± 4.6 |  |  |
| `sqlite3` | 6.2 ± 3.5 | +82.9% | (1.8x slower) |
| `sql-lint` | 86.5 ± 3.1 | +2444.2% | (25.4x slower) |

### bench_30x.sql (30×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 9.9 ± 1.6 |  |  |
| `sqlite3` | 11.0 ± 1.5 | +11.8% |  |
| `sql-lint` | 135.0 ± 3.6 | +1267.8% | (13.7x slower) |

---

# LSP (Language Server)

**What we test:** We start each language server, open a test document, and probe
for completions, hover, diagnostics, and formatting. Results are from actual LSP
protocol responses, not self-reported feature lists.

**Key difference:** `sqls` requires a live database connection for its features.
syntaqlite and sql-language-server work offline.

## Features

| Feature                |    syntaqlite   |        sqls ¹        | sql-language-server |
| ---------------------- | :-------------: | :------------------: | :-----------------: |
| Completion             | Yes (133 items) | Advertised (0 items) |   Yes (11 items)    |
| Hover                  |       Yes       |          No          |         No          |
| Go to definition       |       Yes       |         Yes          |         No          |
| Find references        |       Yes       |          No          |         No          |
| Diagnostics: syntax    |       Yes       |          No          |         Yes         |
| Diagnostics: semantic  |       Yes       |          No          |         Yes         |
| Formatting             |       Yes       |         Yes          |         No          |
| Rename                 |       Yes       |         Yes          |         Yes         |
| Signature help         |       Yes       |         Yes          |         No          |
| Requires DB connection |       No        |         Yes          |         No          |

¹ sqls requires a live database connection. Completion and hover results come
from the connected database schema, not static analysis. Without a database,
these features return no results.

### Completion depth

| Tool                | Items                    |
| ------------------- | ------------------------ |
| syntaqlite          | 133 ████████████████████ |
| sql-language-server | 11 █                     |

## Startup + response speed

Time from server start → document open → diagnostics received → exit:

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 25.8 ± 3.6 |  |  |
| `sql-language-server` | 231.0 ± 13.5 | +796.1% | (9.0x slower) |
