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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 1.7 ± 3.1 | 1.0 | 31.4 | 1.09 ± 2.14 |
| `lemon-rs` | 1.5 ± 1.0 | 0.8 | 5.0 | 1.00 |
| `sql-parser-cst` | 105.0 ± 2.8 | 100.2 | 111.8 | 67.97 ± 43.21 |
| `sqlglot[c]` | 83.3 ± 1.5 | 81.6 | 87.3 | 53.93 ± 34.27 |
| `sqlparser-rs` | 3.0 ± 1.7 | 1.3 | 7.5 | 1.95 ± 1.65 |
| `node-sql-parser` | 90.2 ± 1.9 | 86.7 | 94.0 | 58.41 ± 37.12 |
| `sqlfluff` | 505.9 ± 6.2 | 496.1 | 516.2 | 327.45 ± 208.02 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 3.4 ± 4.9 | 2.1 | 42.2 | 1.00 |
| `lemon-rs` | 5.0 ± 1.9 | 3.4 | 14.9 | 1.46 ± 2.20 |
| `sql-parser-cst` | 254.4 ± 14.5 | 238.5 | 275.3 | 74.95 ± 109.14 |
| `sqlglot[c]` | 232.9 ± 8.6 | 226.7 | 254.5 | 68.61 ± 99.86 |
| `sqlparser-rs` | 16.7 ± 0.4 | 15.7 | 17.7 | 4.91 ± 7.14 |
| `node-sql-parser` | 245.3 ± 14.6 | 234.0 | 280.6 | 72.24 ± 105.20 |
| `sqlfluff` | 8075.4 ± 29.9 | 8036.7 | 8125.4 | 2378.71 ± 3461.03 |

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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 2.1 ± 3.1 | 1.2 | 31.3 | 1.00 |
| `prettier-cst` | 168.8 ± 4.9 | 163.5 | 178.9 | 82.19 ± 124.81 |
| `sql-formatter` | 98.7 ± 3.0 | 94.8 | 104.0 | 48.07 ± 73.00 |
| `sqlglot[c]` | 89.6 ± 1.6 | 86.7 | 94.0 | 43.64 ± 66.26 |
| `sleek` | 2.9 ± 1.7 | 1.1 | 7.1 | 1.43 ± 2.33 |
| `sqruff` | 56.1 ± 0.9 | 54.5 | 58.6 | 27.31 ± 41.46 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 7.4 ± 3.5 | 6.2 | 36.7 | 1.00 |
| `prettier-cst` | 485.8 ± 24.7 | 462.8 | 529.4 | 65.41 ± 31.15 |
| `sql-formatter` | 307.2 ± 6.8 | 297.3 | 315.2 | 41.37 ± 19.61 |
| `sqlglot[c]` | 339.2 ± 9.5 | 333.3 | 365.5 | 45.68 ± 21.67 |
| `sleek` | 17.8 ± 0.5 | 16.9 | 21.0 | 2.39 ± 1.13 |
| `sqruff` | 2301.5 ± 13.5 | 2280.7 | 2324.2 | 309.91 ± 146.76 |

---

# Analysis

**What we test:** We check whether each tool can identify unknown tables, bad
column references, incorrect function arity, and CTE column mismatches without
running the query. The corpus contains 24 cases against a known schema, with 15
intentional errors and 9 valid queries. Each tool's verdict is checked against
sqlite3.

## Accuracy

Schema: `users`, `orders`, `products`, `order_items`. Ground truth: sqlite3.

| Tool              | Approach          | Correct                    | Missed | FP |
| ----------------- | ----------------- | -------------------------- | -----: | -: |
| sqlite3           | runtime execution | 24/24 ████████████████████ |      - |  - |
| sqlite-runner-lsp | runtime via LSP   | 24/24 ████████████████████ |      - |  - |
| syntaqlite        | static semantic   | 23/24 ███████████████████  |      1 |  - |
| sql-lint          | structural checks | 12/24 ██████████           |     11 |  1 |

## Diagnostic quality

The test query contains two errors: its CTE declares three columns while its
`SELECT` produces two, and `ROUND` is misspelled as `ROUDN`:

| Tool              | Approach          | Errors Found | Finds All | Did-you-mean |
| ----------------- | ----------------- | :----------: | :-------: | :----------: |
| syntaqlite        | static semantic   |     2/2      |    Yes    |     Yes      |
| sqlite3           | runtime execution |     1/2      |    No     |      No      |
| sqlite-runner-lsp | runtime via LSP   |     1/2      |    No     |      No      |
| sql-lint          | structural checks |     0/2      |    No     |      No      |

**syntaqlite** finds both errors in one pass, with source locations and a
did-you-mean suggestion:

```
error: table 'monthly_stats' has 2 values for 3 columns
  --> /tmp/tmpxh24_5pv.sql:30:3
   |
30 |   monthly_stats(month, revenue, order_count) AS (
   |   ^~~~~~~~~~~~~
warning: unknown function 'ROUDN'
  --> /tmp/tmpxh24_5pv.sql:42:3
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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 4.3 ± 3.5 | 1.8 | 31.9 | 1.69 ± 1.56 |
| `sqlite3` | 2.6 ± 1.1 | 1.9 | 8.1 | 1.00 |
| `sqlite-runner-lsp` | 254.5 ± 12.9 | 239.4 | 275.6 | 98.93 ± 42.84 |
| `sql-lint` | 83.7 ± 1.5 | 81.7 | 87.8 | 32.55 ± 14.01 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 9.7 ± 1.1 | 8.7 | 16.9 | 1.04 ± 0.16 |
| `sqlite3` | 9.3 ± 1.0 | 8.4 | 14.9 | 1.00 |
| `sqlite-runner-lsp` | 249.3 ± 9.8 | 239.5 | 273.9 | 26.82 ± 3.02 |
| `sql-lint` | 136.7 ± 8.3 | 130.3 | 165.8 | 14.72 ± 1.79 |

---

# LSP (Language Server)

**What we test:** We start each language server, open a test document, and probe
for completions, hover, diagnostics, and formatting. Results are from actual LSP
protocol responses, not self-reported feature lists.

**Key difference:** `sqls` requires a live database connection for its features.
syntaqlite and sql-language-server work offline.

## Features

| Feature                |    syntaqlite   |     sqls ¹    | sql-language-server |
| ---------------------- | :-------------: | :-----------: | :-----------------: |
| Completion             | Yes (133 items) | Yes (6 items) |   Yes (11 items)    |
| Hover                  |       Yes       |      Yes      |         No          |
| Go to definition       |       Yes       |      Yes      |         No          |
| Find references        |       Yes       |      No       |         No          |
| Diagnostics: syntax    |       Yes       |      No       |         Yes         |
| Diagnostics: semantic  |       Yes       |      No       |         Yes         |
| Formatting             |       Yes       |      Yes      |         No          |
| Rename                 |       Yes       |      Yes      |         Yes         |
| Signature help         |       Yes       |      Yes      |         No          |
| Requires DB connection |       No        |      Yes      |         No          |

¹ sqls requires a live database connection. Completion and hover results come
from the connected database schema, not static analysis. Without a database,
these features return no results.

### Completion depth

| Tool                | Items                    |
| ------------------- | ------------------------ |
| syntaqlite          | 133 ████████████████████ |
| sql-language-server | 11 █                     |
| sqls ¹              | 6 █                      |

## Startup + response speed

Time from server start → document open → diagnostics received → exit:

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 24.3 ± 3.3 | 21.7 | 47.0 | 1.00 |
| `sql-language-server` | 222.8 ± 2.9 | 216.6 | 227.1 | 9.18 ± 1.25 |
| `sqls` | publishes no diagnostics | | | |
